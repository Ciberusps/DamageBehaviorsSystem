#pragma once

#include "CapsuleHitRegistrator.h"
#include "CollisionTestObserver.generated.h"

UCLASS()
class UDBSCollisionTestObserver : public UObject
{
	GENERATED_BODY()
public:
	TArray<TWeakObjectPtr<AActor>> Actors;
	TArray<FDBSHitRegistratorHitResult> Hits;

	UFUNCTION()
	void OnHit(const FDBSHitRegistratorHitResult& Hit, UCapsuleHitRegistrator* Registrator)
	{
		Actors.Add(Hit.HitActor);
		Hits.Add(Hit);
	}

	void Reset() { Actors.Reset(); Hits.Reset(); }
	bool Contains(AActor* Actor) const { return Actors.ContainsByPredicate([Actor](const TWeakObjectPtr<AActor>& Item) { return Item.Get() == Actor; }); }
};
