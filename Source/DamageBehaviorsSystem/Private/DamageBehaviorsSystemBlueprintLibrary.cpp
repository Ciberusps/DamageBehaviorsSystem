// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageBehaviorsSystemBlueprintLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DamageBehaviorsSystemBlueprintLibrary)

AActor* UDamageBehaviorsSystemBlueprintLibrary::GetTopmostAttachedActor(AActor* Actor_In)
{
	AActor* Current = Actor_In;
	// Loop until there is no longer an AttachParent
	while (AActor* Parent = Current->GetAttachParentActor())
	{
		Current = Parent;
	}
	return Current;
}
