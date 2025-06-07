// Pavel Penkov 2025 All Rights Reserved.

#include "DBSEditorDamageBehaviorDetails.h"

#include "DamageBehavior.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Widgets/Text/STextBlock.h"
#include "PropertyHandle.h"
#include "Widgets/Text/STextBlock.h"

void FDBSEditorDamageBehaviorDetails::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& /*StructCustomizationUtils*/
)
{
	// // Show the array-element name (e.g. “Sources[0]”, “Sources[1]”, …)
	// HeaderRow
	// .NameContent()
	// [
	// 	// this will draw exactly the parent array’s element label
	// 	StructPropertyHandle->CreatePropertyNameWidget()
	// ]
	// .ValueContent()
	// [
	// 	// no inline value widget for the struct itself
	// 	SNew(SBox)
	// 	.MinDesiredWidth(0)
	// ];
	// Skip the default struct header entirely
}

void FDBSEditorDamageBehaviorDetails::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& /*StructCustomizationUtils*/
)
{
	// 1) Read the SourceName so we can label the row
	FString SourceName;
	if (TSharedPtr<IPropertyHandle> NameHandle =
		StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(
			FDBSHitRegistratorsToActivateSource, SourceName)))
	{
		NameHandle->GetValue(SourceName);
	}
	if (SourceName.IsEmpty())
	{
		SourceName = TEXT("UnnamedSource");
	}

	// 2) Show the HitRegistratorsNames array using its built-in editor
	if (TSharedPtr<IPropertyHandle> ArrHandle =
		StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(
			FDBSHitRegistratorsToActivateSource, HitRegistratorsNames)))
	{
		StructBuilder.AddProperty(ArrHandle.ToSharedRef())
			.DisplayName(FText::FromString(SourceName))
			.ToolTip(FText::FromString(
				FString::Printf(TEXT("Registrator names for \"%s\""), *SourceName)))
			.ShowPropertyButtons(true);
	}
}