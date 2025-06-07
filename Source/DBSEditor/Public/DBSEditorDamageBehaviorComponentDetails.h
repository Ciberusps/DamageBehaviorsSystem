// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class IDetailLayoutBuilder;

class FDBSEditorDamageBehaviorComponentDetails : public IDetailCustomization
{
public:
	// Factory method
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FDBSEditorDamageBehaviorComponentDetails);
	}

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
