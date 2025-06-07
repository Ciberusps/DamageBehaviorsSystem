// Pavel Penkov 2025 All Rights Reserved.

#include "DBSEditorDamageBehaviorDetails.h"

#include "DamageBehavior.h"
#include "DamageBehaviorsSystemSettings.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "PropertyHandle.h"
#include "Widgets/Text/STextBlock.h"
#include "PropertyHandle.h"
#include "Widgets/Text/STextBlock.h"

void FDBSEditorDamageBehaviorDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// 1) Locate the property handle for your instanced-array:
	auto BehaviorsHandle = DetailBuilder
		.GetProperty(GET_MEMBER_NAME_CHECKED(UDamageBehaviorsComponent, DamageBehaviorsInstancedTest));

	// 2) Hide the engine’s default widget for that one property:
	// DetailBuilder.HideProperty(BehaviorsHandle);
	
	// 3) Grab the category so we can re-add a custom version under it:
	IDetailCategoryBuilder& Cat = DetailBuilder.EditCategory("DamageBehaviorsComponent");

    // 3) Read evaluator classes from settings
	const UDamageBehaviorsSystemSettings* Settings =
		GetDefault<UDamageBehaviorsSystemSettings>();
	if (!Settings)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load DamageBehaviorsSystemSettings!"));
		return;
	}

    // Build desired source names
	// 2) Read the class‐list property from settings
	//    (make sure the UPROPERTY is really spelled this way in your .h)
	const auto& EvalClasses = Settings->AdditionalDamageBehaviorsSourcesEvaluators;

	// 3) Build your names array
	TArray<FString> DesiredNames = { DEFAULT_DAMAGE_BEHAVIOR_SOURCE };
	DesiredNames.Reserve(EvalClasses.Num());

	for (auto& EvalClass : EvalClasses)
	{
		// EvalClass is a TSubclassOf<UAdditionalDamageBehaviorsSourceEvaluator>
		if (!*EvalClass)   // checks EvalClass.IsValid()
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Found null subclass in SourceEvaluators array!"));
			continue;
		}

		// Correctly get its CDO:
		UAdditionalDamageBehaviorsSourceEvaluator* CDO =
			EvalClass->GetDefaultObject<UAdditionalDamageBehaviorsSourceEvaluator>();

		if (!CDO)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("GetDefaultObject() failed for %s"),
				*EvalClass->GetName());
			continue;
		}

		if (!CDO->SourceName.IsEmpty())
		{
			DesiredNames.AddUnique(CDO->SourceName);
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Evaluator %s has empty SourceName"),
				*EvalClass->GetName());
		}
	}

	// 5) Determine how many behaviors are in the array:
    uint32 Num = 0;
    BehaviorsHandle->GetNumChildren(Num);

	bool bDidAdd = false;
    // 6) For each array element, draw your custom sub‐UI:
    for (uint32 Index = 0; Index < Num; ++Index)
    {
        // a) the array‐element handle:
        TSharedRef<IPropertyHandle> ElemHandle = BehaviorsHandle->GetChildHandle(Index).ToSharedRef();

        // b) get the UObject* value it holds:
        UObject* Obj = nullptr;
        ElemHandle->GetValue(Obj);
        UDamageBehavior* Behavior = Cast<UDamageBehavior>(Obj);
        if (!Behavior)
        {
            // if null, just show the default slot row so user can assign a behavior asset
            Cat.AddProperty(ElemHandle);
            continue;
        }

        // c) show a sub‐heading named after the behavior (or index):
        FText Header = FText::Format(
            NSLOCTEXT("DBS", "BehaviorSlotFmt", "Behavior {0}"),
            FText::AsNumber(Index)
        );
        IDetailGroup& Group = Cat.AddGroup(
            FName(*FString::Printf(TEXT("BehaviorGroup_%d"), Index)),
            Header
        );

        // d) Sync that behavior’s Sources array to match DesiredNames:
        for (auto& Name : DesiredNames)
        {
            bool bFound = false;
            for (auto& Entry : Behavior->SourceHitRegistratorsToActivate)
            {
                if (Entry.SourceName == Name)
                {
                    bFound = true;
                    break;
                }
            }
            if (!bFound)
            {
                FSourceHitRegistratorsToActivate NewE;
                NewE.SourceName = Name;
                Behavior->Modify();
                Behavior->SourceHitRegistratorsToActivate.Add(NewE);
            	bDidAdd = true;
            }
        }

        // e) Now for each desired source, draw its HitRegistratorsNames array:
        //    get handle to Behavior->Sources
        TSharedRef<IPropertyHandle> SourcesArrayHandle = 
            ElemHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(UDamageBehavior, SourceHitRegistratorsToActivate)).ToSharedRef();

        for (auto& Name : DesiredNames)
        {
            int32 SIndex = Behavior->SourceHitRegistratorsToActivate.IndexOfByPredicate(
                [&](auto& E){ return E.SourceName == Name; }
            );
            if (SIndex == INDEX_NONE) continue;

            auto SourceElemHandle = SourcesArrayHandle->GetChildHandle(SIndex);
            if (!SourceElemHandle.IsValid()) continue;

            auto HitRegsHandle = SourceElemHandle
                ->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSourceHitRegistratorsToActivate, HitRegistratorsNames));
            if (!HitRegsHandle.IsValid()) continue;

            // Render inline array editor under this group:
            Group.AddPropertyRow(HitRegsHandle.ToSharedRef())
                .DisplayName(FText::FromString(FString::Printf(TEXT("Activate HitRegistrators on %s"), *Name)))
                .ToolTip(FText::FromString("HitRegistrators To Activate on “" + Name + "”"))
                .ShowPropertyButtons(true);
        }
    }

	// 3) If we added anything, ask for a single detail‐refresh and bail out
	if (bDidAdd)
	{
		DetailBuilder.ForceRefreshDetails();
		return; // next pass will display the new rows
	}
}