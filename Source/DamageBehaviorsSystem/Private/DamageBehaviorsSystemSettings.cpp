// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageBehaviorsSystemSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DamageBehaviorsSystemSettings)

FName UDamageBehaviorsSystemSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}

#if WITH_EDITOR
void UDamageBehaviorsSystemSettings::PostInitProperties()
{
	Super::PostInitProperties();
}
#endif