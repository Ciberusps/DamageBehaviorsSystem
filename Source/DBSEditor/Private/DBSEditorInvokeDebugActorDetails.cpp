// Pavel Penkov 2025 All Rights Reserved.

#include "DBSEditorInvokeDebugActorDetails.h"

#include "DamageBehaviorsSystemSettings.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "PropertyHandle.h"

TSharedRef<IPropertyTypeCustomization> FDBSEditorInvokeDebugActorDetails::MakeInstance()
{
	return MakeShareable(new FDBSEditorInvokeDebugActorDetails());
}

void FDBSEditorInvokeDebugActorDetails::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	HeaderRow.NameContent()[StructPropertyHandle->CreatePropertyNameWidget()];
}

void FDBSEditorInvokeDebugActorDetails::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	TSharedPtr<IPropertyHandle> SourceNameHandle = StructPropertyHandle->GetChildHandle(TEXT("SourceName"));
	TSharedPtr<IPropertyHandle> ActorHandle = StructPropertyHandle->GetChildHandle(TEXT("Actor"));
	TSharedPtr<IPropertyHandle> bCustomSocketHandle = StructPropertyHandle->GetChildHandle(TEXT("bCustomSocketName"));
	TSharedPtr<IPropertyHandle> SocketNameHandle = StructPropertyHandle->GetChildHandle(TEXT("SocketName"));
	TSharedPtr<IPropertyHandle> SpawnInPreviewHandle = StructPropertyHandle->GetChildHandle(TEXT("bSpawnInPreview"));

	if (SourceNameHandle.IsValid())
	{
		ChildBuilder.AddProperty(SourceNameHandle.ToSharedRef());
	}

	if (ActorHandle.IsValid())
	{
		ChildBuilder.AddProperty(ActorHandle.ToSharedRef());
	}

	if (bCustomSocketHandle.IsValid())
	{
		ChildBuilder.AddProperty(bCustomSocketHandle.ToSharedRef());
	}

	if (SocketNameHandle.IsValid())
	{
		ChildBuilder.AddProperty(SocketNameHandle.ToSharedRef());
	}

	if (SpawnInPreviewHandle.IsValid())
	{
		ChildBuilder.AddProperty(SpawnInPreviewHandle.ToSharedRef());
	}
}


