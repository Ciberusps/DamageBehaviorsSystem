// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_InvokeDamageBehavior.h"

#include "DamageBehaviorsComponent.h"
#include "DamageBehaviorsSystemSettings.h"

UANS_InvokeDamageBehavior::UANS_InvokeDamageBehavior()
{
	TargetSources = {
		{DEFAULT_DAMAGE_BEHAVIOR_SOURCE, true }
	};

	const UDamageBehaviorsSystemSettings* DamageBehaviorsSystemSettings = GetDefault<UDamageBehaviorsSystemSettings>();
	for (TSubclassOf<UDamageBehaviorsSourceEvaluator> AdditionalDamageBehaviorsSourcesEvaluator : DamageBehaviorsSystemSettings->AdditionalDamageBehaviorsSourcesEvaluators)
	{
		TargetSources.Add(AdditionalDamageBehaviorsSourcesEvaluator.GetDefaultObject()->SourceName, false);
	}
}

void UANS_InvokeDamageBehavior::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	UActorComponent* Component = MeshComp->GetOwner()->GetComponentByClass(UDamageBehaviorsComponent::StaticClass());
	if (!Component) return;
	
	UDamageBehaviorsComponent* DmgBehaviorComponent = StaticCast<UDamageBehaviorsComponent*>(Component);
    if (!DmgBehaviorComponent) return;

    DmgBehaviorComponent->InvokeDamageBehavior(Name, true, GetDamageBehaviorSourcesList(), Payload);
}

void UANS_InvokeDamageBehavior::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	UActorComponent* Component = MeshComp->GetOwner()->GetComponentByClass(UDamageBehaviorsComponent::StaticClass());
	if (!Component) return;
	
	UDamageBehaviorsComponent* DmgBehaviorComponent = StaticCast<UDamageBehaviorsComponent*>(Component);
	if (!DmgBehaviorComponent) return;
	
	DmgBehaviorComponent->InvokeDamageBehavior(Name, false, GetDamageBehaviorSourcesList(), Payload);
}

TArray<FString> UANS_InvokeDamageBehavior::GetDamageBehaviorSourcesList() const
{
	TArray<FString> Result = {};
	for (TPair<FString, bool> DamageBehaviorSource : TargetSources)
	{
		if (DamageBehaviorSource.Value)
		{
			Result.Add(DamageBehaviorSource.Key);
		}
	}
	return Result;
}
