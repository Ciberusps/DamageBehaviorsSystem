// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "DamageBehavior.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

namespace DamageBehaviorsSystemTestFixtures
{
	// Throwaway world for specs, pattern from NinjaInventoryTests FWorldFixture.
	class FWorldFixture
	{
	public:
		FWorldFixture()
		{
			if (GEngine != nullptr)
			{
				static uint32 WorldCounter = 0;
				const FString WorldName = FString::Printf(TEXT("DBSTestWorld_%d"), WorldCounter++);

				UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, *WorldName, GetTransientPackage());
				if (World != nullptr)
				{
					FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
					WorldContext.SetCurrentWorld(World);

					World->InitializeActorsForPlay(FURL());
					if (IsValid(World->GetWorldSettings()))
					{
						World->GetWorldSettings()->NotifyBeginPlay();
					}
					World->BeginPlay();

					WeakWorld = MakeWeakObjectPtr(World);
				}
			}
		}

		UWorld* GetWorld() const { return WeakWorld.Get(); }

		~FWorldFixture()
		{
			UWorld* World = WeakWorld.Get();
			if (World != nullptr && GEngine != nullptr)
			{
				World->EndPlay(EEndPlayReason::Quit);
				for (auto It = TActorIterator<AActor>(World); It; ++It)
				{
					It->Destroy();
				}
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
				WeakWorld.Reset();
			}
		}

	private:
		TWeakObjectPtr<UWorld> WeakWorld;
	};
}

namespace
{
	// Spawned AActor has no root component, attachment needs one.
	AActor* SpawnActorWithRoot(UWorld* World)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("TestRoot"));
		Actor->SetRootComponent(Root);
		Root->RegisterComponent();
		return Actor;
	}
}

// Guards the BugSplat crash group "AActor::ForEachAttachedActors" (v0.7.29, v0.8.3):
// the crash PC sat in the recursive GetAttachedActors walk called from
// UCapsuleHitRegistrator::ProcessHitRegistration while UDamageBehavior::Tick was still
// ticking during world teardown, after GC had started freeing the attach hierarchy.
BEGIN_DEFINE_SPEC(FDamageBehaviorLifecycleSpec, "DamageBehaviorsSystem.DamageBehavior.Lifecycle",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
	TUniquePtr<DamageBehaviorsSystemTestFixtures::FWorldFixture> WorldFixture;
END_DEFINE_SPEC(FDamageBehaviorLifecycleSpec)

void FDamageBehaviorLifecycleSpec::Define()
{
	BeforeEach([this]()
	{
		WorldFixture = MakeUnique<DamageBehaviorsSystemTestFixtures::FWorldFixture>();
		this->TestNotNull(TEXT("Test world"), WorldFixture->GetWorld());
	});

	AfterEach([this]()
	{
		WorldFixture.Reset();
	});

	Describe("Owner lifecycle", [this]()
	{
		It("stops ticking when the owner actor is destroyed", [this]()
		{
			UWorld* World = WorldFixture->GetWorld();

			AActor* Owner = SpawnActorWithRoot(World);
			// In production the DamageBehaviorsComponent roots the behavior in a UPROPERTY array; here nothing does.
			// A raw pointer roots nothing (outer is not a GC reference), so the CollectGarbage below would purge
			// the behavior together with its dead owner and IsTickable() would read freed memory.
			TStrongObjectPtr<UDamageBehavior> Behavior(NewObject<UDamageBehavior>(World));
			Behavior->Init(Owner);
			Behavior->MakeActive(true, FInstancedStruct());

			// FDamageBehaviorHitDetectionSettings defaults to ByTrace
			this->TestTrue(TEXT("ticks while owner is alive"), Behavior->IsTickable());

			World->DestroyActor(Owner);
			this->TestFalse(TEXT("pending-kill owner is not tickable"), Behavior->IsTickable());

			CollectGarbage(RF_NoFlags);
			this->TestFalse(TEXT("garbage owner is not tickable"), Behavior->IsTickable());
		});

		It("BeginDestroy disables the tickable before destruction", [this]()
		{
			UWorld* World = WorldFixture->GetWorld();

			AActor* Owner = SpawnActorWithRoot(World);
			UDamageBehavior* Behavior = NewObject<UDamageBehavior>(World);
			Behavior->Init(Owner);
			Behavior->MakeActive(true, FInstancedStruct());

			this->TestTrue(TEXT("ticks before destroy"), Behavior->IsTickable());

			// UObject::BeginDestroy fatals when called outside ConditionalBeginDestroy (Obj.cpp sanity check)
			Behavior->ConditionalBeginDestroy();
			this->TestTrue(TEXT("BeginDestroy routed to UObject"), Behavior->HasAnyFlags(RF_BeginDestroyed));
		});
	});

	Describe("Attach hierarchy", [this]()
	{
		It("mutual AddHittedActor does not create a cycle and the recursive walk terminates", [this]()
		{
			UWorld* World = WorldFixture->GetWorld();

			AActor* ActorA = SpawnActorWithRoot(World);
			AActor* ActorB = SpawnActorWithRoot(World);

			UDamageBehavior* BehaviorA = NewObject<UDamageBehavior>(World);
			BehaviorA->bAttachEnemiesToCapsuleWhileActive = true;
			BehaviorA->Init(ActorA);
			BehaviorA->MakeActive(true, FInstancedStruct());

			UDamageBehavior* BehaviorB = NewObject<UDamageBehavior>(World);
			BehaviorB->bAttachEnemiesToCapsuleWhileActive = true;
			BehaviorB->Init(ActorB);
			BehaviorB->MakeActive(true, FInstancedStruct());

			// both characters hit each other in the same frame: A attaches B, then B attaches A
			BehaviorA->AddHittedActor(ActorB, true, false);
			// AttachToComponent refuses the reverse attach with an FMessageLog("PIE") warning mirrored
			// to the log — consume it so the automation controller elevating warnings cannot fail the spec
			this->AddExpectedMessagePlain(TEXT("would form cycle"), ELogVerbosity::Warning);
			BehaviorB->AddHittedActor(ActorA, true, false);

			this->TestTrue(TEXT("B attached to A"), ActorB->GetAttachParentActor() == ActorA);
			this->TestNull(TEXT("engine refuses the A<->B cycle"), ActorA->GetAttachParentActor());

			TArray<AActor*> AttachedActors;
			ActorA->GetAttachedActors(AttachedActors, true, true);
			this->TestTrue(TEXT("walk contains B"), AttachedActors.Contains(ActorB));
			this->TestFalse(TEXT("walk does not contain A itself"), AttachedActors.Contains(ActorA));
		});

		It("recursive GetAttachedActors survives a destroyed attached actor", [this]()
		{
			UWorld* World = WorldFixture->GetWorld();

			AActor* ActorA = SpawnActorWithRoot(World);
			AActor* ActorB = SpawnActorWithRoot(World);
			ActorB->AttachToActor(ActorA, FAttachmentTransformRules::KeepWorldTransform);
			this->TestTrue(TEXT("B attached to A"), ActorB->GetAttachParentActor() == ActorA);

			// enemy dies while attached: UWorld::DestroyActor detaches it synchronously
			World->DestroyActor(ActorB);
			this->TestTrue(TEXT("destroyed actor detached from its parent"), ActorB->GetAttachParentActor() == nullptr);

			TArray<AActor*> AttachedActors;
			ActorA->GetAttachedActors(AttachedActors, true, true);
			this->TestFalse(TEXT("walk skips the destroyed actor"), AttachedActors.Contains(ActorB));

			CollectGarbage(RF_NoFlags);
			AttachedActors.Reset();
			ActorA->GetAttachedActors(AttachedActors, true, true);
			this->TestTrue(TEXT("walk completes after GC"), AttachedActors.Num() == 0);
		});
	});
}

#endif
