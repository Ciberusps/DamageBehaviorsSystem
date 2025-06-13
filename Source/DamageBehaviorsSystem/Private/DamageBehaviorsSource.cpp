// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageBehaviorsSource.h"

#include "DamageBehaviorsComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DamageBehaviorsSource)

FDamageBehaviorsSource::FDamageBehaviorsSource(FString SourceName_In, AActor* OwnerActor_In, UDamageBehaviorsSourceEvaluator* Evaluator_In)
{
	OwnerActor = OwnerActor_In;
	SourceName = SourceName_In;
	Evaluator = Evaluator_In;
	
	if (OwnerActor.IsValid())
	{
		// AActor* Source = GetSourceActor();
		// if (Source)
		// {
		// 	DamageBehaviorsComponent = GetDamageBehaviorsComponent();
		// }
		// else
		// {
		// 	// TODO: Source actor can't be reached possibly this actor already source actor
		// }
	}
	else
	{
		// TODO: validation error
	}
}

AActor* UDamageBehaviorsSourceEvaluator::GetActorWithDamageBehaviors_Implementation(AActor* OwnerActor) const
{
	return nullptr;
}

AActor* FDamageBehaviorsSource::GetSourceActor() const
{
	// if no Evaluator for example for "ThisActor" just return OwnerActor
	if (!Evaluator) return OwnerActor.Get();
	
	if (!OwnerActor.IsValid()) return nullptr;

	return Evaluator->GetActorWithDamageBehaviors(OwnerActor.Get());
}

UDamageBehaviorsComponent* FDamageBehaviorsSource::GetDamageBehaviorsComponent() const
{
	UDamageBehaviorsComponent* Result = nullptr;

	AActor* Source = GetSourceActor();
	if (!Source) return Result;

	Result = Source->FindComponentByClass<UDamageBehaviorsComponent>();
	if (!Result)
	{
		// TODO: validation error
	}
	return Result;
		
}
