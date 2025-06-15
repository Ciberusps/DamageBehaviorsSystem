// Pavel Penkov 2025 All Rights Reserved.

#include "DBSEditor.h"
#include "GraphPanelNodeFactory_InvokeDamageBehavior.h"
#include "EdGraphUtilities.h"
#include "PropertyEditorModule.h"
#include "DamageBehaviorsComponent.h"
#include "DBSEditorDamageBehaviorDetails.h"

#define LOCTEXT_NAMESPACE "FDBSEditorModule"

void FDBSEditorModule::RegisterAnimGraphNodeFactory()
{
	AnimGraphNodeFactory = MakeShared<FGraphPanelNodeFactory_InvokeDamageBehavior>();
	FEdGraphUtilities::RegisterVisualNodeFactory(AnimGraphNodeFactory);
}

void FDBSEditorModule::UnregisterAnimGraphNodeFactory()
{
	if (AnimGraphNodeFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(AnimGraphNodeFactory);
		AnimGraphNodeFactory.Reset();
	}
}

void FDBSEditorModule::StartupModule()
{
	// Register detail customization
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(
		UDamageBehaviorsComponent::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateLambda([]() { return MakeShared<FDBSEditorDamageBehaviorDetails>(); })
	);

	RegisterAnimGraphNodeFactory();

	// Check UDamageBehaviorsComponent::DamageBehaviorsList description why this used
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FDBSEditorModule::HandleObjectPropertyChanged);
}

void FDBSEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(UDamageBehaviorsComponent::StaticClass()->GetFName());
	}

	UnregisterAnimGraphNodeFactory();

	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
}

void FDBSEditorModule::HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event)
{
	// TODO: Implement if needed
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDBSEditorModule, DBSEditor)
