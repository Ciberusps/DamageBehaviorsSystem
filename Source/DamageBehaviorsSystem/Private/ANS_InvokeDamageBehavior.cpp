// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_InvokeDamageBehavior.h"

#include "DamageBehaviorsComponent.h"

void UANS_InvokeDamageBehavior::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	UActorComponent* Component = MeshComp->GetOwner()->GetComponentByClass(UDamageBehaviorsComponent::StaticClass());
	if (!Component) return;
	
	UDamageBehaviorsComponent* DmgBehaviorComponent = StaticCast<UDamageBehaviorsComponent*>(Component);
    if (!DmgBehaviorComponent) return;
	
    DmgBehaviorComponent->InvokeDamageBehavior(DamageBehaviorName, true, DamageBehaviorSource, Payload);
}

void UANS_InvokeDamageBehavior::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	UActorComponent* Component = MeshComp->GetOwner()->GetComponentByClass(UDamageBehaviorsComponent::StaticClass());
	if (!Component) return;
	
	UDamageBehaviorsComponent* DmgBehaviorComponent = StaticCast<UDamageBehaviorsComponent*>(Component);
	if (!DmgBehaviorComponent) return;
	
	DmgBehaviorComponent->InvokeDamageBehavior(DamageBehaviorName, false, DamageBehaviorSource, Payload);
}
