// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageBehaviorsSource.h"

#include "DamageBehaviorsComponent.h"
#include "Utils/UnrealHelperLibraryBPL.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DamageBehaviorsSource)

FDamageBehaviorsSource::FDamageBehaviorsSource(FString SourceName_In, AActor* Actor_In)
{
	Actor = Actor_In;
	SourceName = SourceName_In;
	
	if (Actor)
	{
		DamageBehaviorsComponent = Actor->FindComponentByClass<UDamageBehaviorsComponent>();
		if (!DamageBehaviorsComponent)
		{
			// TODO: validation error	
		}
	}
	else
	{
		// TODO: validation error
	}
}

AActor* UDamageBehaviorsSourceEvaluator::GetActorWithCapsules_Implementation(AActor* OwnerActor) const
{
	return nullptr;
}
