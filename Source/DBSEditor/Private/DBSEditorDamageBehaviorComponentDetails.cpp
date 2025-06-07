// Pavel Penkov 2025 All Rights Reserved.

#include "DBSEditorDamageBehaviorComponentDetails.h"

#include "DamageBehavior.h"
#include "PropertyHandle.h"
#include "Widgets/Text/STextBlock.h"
#include "PropertyHandle.h"
#include "Widgets/Text/STextBlock.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "PropertyHandle.h"
#include "DamageBehaviorsComponent.h"
#include "IDetailGroup.h"

void FDBSEditorDamageBehaviorComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Locate the array property
	TSharedRef<IPropertyHandle> ArrayHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UDamageBehaviorsComponent, DamageBehaviorsInstancedTest));

	// Hide default layout
	DetailBuilder.HideProperty(ArrayHandle);

	// Add to desired category
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("DamageBehaviorsComponent");

	// Get all selected objects (component instances)
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() == 0) return;

	// Get pointer to your component
	UDamageBehaviorsComponent* Component = Cast<UDamageBehaviorsComponent>(Objects[0].Get());
	if (!Component) return;

	// Iterate over array
	for (int32 i = 0; i < Component->DamageBehaviorsInstancedTest.Num(); ++i)
	{
		UDamageBehavior* Behavior = Component->DamageBehaviorsInstancedTest[i];
		if (!Behavior) continue;

		// Create group for each element, label it with TitleProperty if available
		FText HeaderText = FText::FromString(Behavior->GetName());
		FName GroupName = *FString::Printf(TEXT("DamageBehavior_%d"), i);

		IDetailGroup& Group = Category.AddGroup(GroupName, HeaderText, false, true);

		UClass* BaseClass = UDamageBehavior::StaticClass();
		UClass* ActualClass = Behavior->GetClass();

		// Base class properties
		for (TFieldIterator<FProperty> PropIt(BaseClass); PropIt; ++PropIt)
		{
			FProperty* Property = *PropIt;
			if (!Property->HasAllPropertyFlags(CPF_Edit)) continue;

			TSharedPtr<IPropertyHandle> PropHandle = DetailBuilder.GetProperty(Property->GetFName(), ActualClass);
			if (PropHandle->IsValidHandle())
			{
				Group.AddPropertyRow(PropHandle.ToSharedRef());
			}
		}

		// Derived class properties
		for (TFieldIterator<FProperty> PropIt(ActualClass); PropIt; ++PropIt)
		{
			FProperty* Property = *PropIt;
			if (!Property->HasAllPropertyFlags(CPF_Edit)) continue;

			// Skip inherited properties
			if (Property->GetOwnerClass() == BaseClass) continue;

			TSharedPtr<IPropertyHandle> PropHandle = DetailBuilder.GetProperty(Property->GetFName(), ActualClass);
			if (PropHandle->IsValidHandle())
			{
				Group.AddPropertyRow(PropHandle.ToSharedRef());
			}
		}
	}





	
	
    // // Grab the array
    // TSharedRef<IPropertyHandle> BehaviorsHandle =
    //     DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(
    //         UDamageBehaviorsComponent,
    //         DamageBehaviorsInstancedTest
    //     ));
    //
    // // Hide the default widget (so nothing auto‐draws)
    // DetailBuilder.HideProperty(BehaviorsHandle);
    //
    // // Our category
    // IDetailCategoryBuilder& Cat =
    //     DetailBuilder.EditCategory("DamageBehaviorsComponent");
    //
    // // How many elements?
    // uint32 Num = 0;
    // BehaviorsHandle->GetNumChildren(Num);
    //
    // for (uint32 Index = 0; Index < Num; ++Index)
    // {
    //     // Handle to the UObject* slot
    //     TSharedRef<IPropertyHandle> ElemHandle =
    //         BehaviorsHandle->GetChildHandle(Index).ToSharedRef();
    //
    //     // Get the actual object
    //     UObject* Obj = nullptr;
    //     ElemHandle->GetValue(Obj);
    //     UDamageBehavior* Behavior = Cast<UDamageBehavior>(Obj);
    //
    //     // If it's null, let them assign one
    //     if (!Behavior)
    //     {
    //         Cat.AddProperty(ElemHandle);
    //         continue;
    //     }
    //
    //     //–– Now *only* draw its inner properties ––
    //
    //     // For example: draw its “Sources” array (assuming that’s what you want)
    //     TSharedRef<IPropertyHandle> SourcesHandle =
    //         ElemHandle
    //           ->GetChildHandle(GET_MEMBER_NAME_CHECKED(UDamageBehavior, Sources))
    //           .ToSharedRef();
    //
    //     // // You can give it a nice label:
    //     // Cat.AddProperty(SourcesHandle)
    //     //     .DisplayName(FText::FromString(TEXT("HitRegistratorsBySource")))
    //     //     .ToolTip(FText::FromString(TEXT("...")));
    //
    //     // Or if you want each element of Sources inlined too,
    //     // you can iterate SourcesHandle->GetNumChildren, GetChildHandle(i), then AddProperty
    //     uint32 NumSrc = 0;
    //     SourcesHandle->GetNumChildren(NumSrc);
    //     for (uint32 Si = 0; Si < NumSrc; ++Si)
    //     {
    //         TSharedRef<IPropertyHandle> SourceElem = 
    //             SourcesHandle->GetChildHandle(Si).ToSharedRef();
    //
    //         // Draw the struct’s children rather than the struct wrapper
    //         uint32 NumFields = 0;
    //         SourceElem->GetNumChildren(NumFields);
    //         for (uint32 Fi = 0; Fi < NumFields; ++Fi)
    //         {
    //             TSharedRef<IPropertyHandle> Field = 
    //                 SourceElem->GetChildHandle(Fi).ToSharedRef();
    //
    //             Cat.AddProperty(Field)
    //               .DisplayName(Field->GetPropertyDisplayName())
    //               .ShowPropertyButtons(true);
    //         }
    //     }
    //
    //     // …and so on for any other inner UObject properties you want…
    // }
}
