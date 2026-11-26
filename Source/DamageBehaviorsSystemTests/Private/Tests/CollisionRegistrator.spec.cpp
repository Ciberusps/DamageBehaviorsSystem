#include "CollisionTestObserver.h"
#include "BehaviorTestObjects.h"
#include "Components/SphereComponent.h"
#include "DamageBehaviorsSystemSettings.h"
#include "DamageBehaviorsComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

namespace DBSCollisionTests
{
	class FFixture
	{
	public:
		FFixture()
		{
			if (!GEngine) return;
			static uint32 Counter = 0;
			const FString Name = FString::Printf(TEXT("DBSCollisionWorld_%u"), Counter++);
			World = UWorld::CreateWorld(EWorldType::Game, false, *Name, GetTransientPackage());
			if (!World) return;
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			if (IsValid(World->GetWorldSettings())) World->GetWorldSettings()->NotifyBeginPlay();
			World->BeginPlay();
			Owner = World->SpawnActor<AActor>();
			Registrator = NewObject<UCapsuleHitRegistrator>(Owner);
			Owner->SetRootComponent(Registrator);
			Registrator->SetCapsuleSize(10.0f, 20.0f);
			Registrator->RegisterComponent();
			Observer.Reset(NewObject<UDBSCollisionTestObserver>());
			Registrator->OnHitRegistered.AddDynamic(Observer.Get(), &UDBSCollisionTestObserver::OnHit);
		}

		~FFixture()
		{
			if (World && GEngine)
			{
				World->EndPlay(EEndPlayReason::Quit);
				for (TActorIterator<AActor> It(World); It; ++It) It->Destroy();
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
		}

		AActor* Target(const FVector& Location, ECollisionResponse Visibility = ECR_Block, ECollisionResponse Camera = ECR_Ignore)
		{
			AActor* Actor = World->SpawnActor<AActor>();
			USphereComponent* Sphere = NewObject<USphereComponent>(Actor);
			Actor->SetRootComponent(Sphere);
			Sphere->InitSphereRadius(15.0f);
			Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Sphere->SetCollisionObjectType(ECC_WorldDynamic);
			Sphere->SetCollisionResponseToAllChannels(ECR_Overlap);
			Sphere->SetCollisionResponseToChannel(ECC_Visibility, Visibility);
			Sphere->SetCollisionResponseToChannel(ECC_Camera, Camera);
			Sphere->SetGenerateOverlapEvents(true);
			Sphere->SetWorldLocation(Location);
			Sphere->RegisterComponent();
			return Actor;
		}

		void Move(float X) { Registrator->SetWorldLocation(FVector(X, 0, 0)); }
		void Trace(float X) { Move(X); Registrator->TickHitRegistration(0.016f); }

		UWorld* World = nullptr;
		AActor* Owner = nullptr;
		UCapsuleHitRegistrator* Registrator = nullptr;
		TStrongObjectPtr<UDBSCollisionTestObserver> Observer;
	};

	FDamageBehaviorHitDetectionSettings TraceSettings(ECollisionChannel Channel)
	{
		FDamageBehaviorHitDetectionSettings Settings;
		Settings.bUseCustomTraceChannel = true;
		Settings.CustomTraceChannel = Channel;
		return Settings;
	}

	FDamageBehaviorHitDetectionSettings EnterSettings()
	{
		FDamageBehaviorHitDetectionSettings Settings;
		Settings.HitDetectionType = EDamageBehaviorHitDetectionType::ByEntering;
		Settings.CollisionProfileName = FCollisionProfileName(FName(TEXT("OverlapAllDynamic")));
		return Settings;
	}
}

BEGIN_DEFINE_SPEC(FDBSCollisionRegistratorSpec, "DamageBehaviorsSystem.Collision.Registrator",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
	TUniquePtr<DBSCollisionTests::FFixture> Fixture;
END_DEFINE_SPEC(FDBSCollisionRegistratorSpec)

void FDBSCollisionRegistratorSpec::Define()
{
	BeforeEach([this]() { Fixture = MakeUnique<DBSCollisionTests::FFixture>(); });
	AfterEach([this]() { Fixture.Reset(); });

	It("sweeps the actual capsule, reports its hit and stops when disabled", [this]()
	{
		auto& F = *Fixture;
		if (!TestNotNull(TEXT("world"), F.World)) return;
		AActor* Target = F.Target(FVector(100, 0, 0));
		auto Settings = DBSCollisionTests::TraceSettings(ECC_Visibility);
		F.Registrator->SetIsHitRegistrationEnabled(true, Settings);
		F.Trace(200);
		TestTrue(TEXT("sweep hits target"), F.Observer->Contains(Target));
		if (F.Observer->Hits.Num())
		{
			TestTrue(TEXT("hit result actor matches"), F.Observer->Hits[0].HitResult.GetActor() == Target);
			TestTrue(TEXT("instigator is capsule owner"), F.Observer->Hits[0].Instigator.Get() == F.Owner);
			TestTrue(TEXT("direction follows sweep"), F.Observer->Hits[0].Direction.Equals(FVector::ForwardVector, 0.01f));
		}
		F.Observer->Reset();
		F.Registrator->SetIsHitRegistrationEnabled(false, Settings);
		F.Trace(0);
		TestFalse(TEXT("disabled flag"), F.Registrator->IsHitRegistrationEnabled());
		TestEqual(TEXT("disabled sweep emits no hit"), F.Observer->Hits.Num(), 0);
	});

	It("honors ignored actors and clears the list when re-enabled", [this]()
	{
		auto& F = *Fixture;
		if (!TestNotNull(TEXT("world"), F.World)) return;
		AActor* Ignored = F.Target(FVector(60, 0, 0));
		AActor* Target = F.Target(FVector(140, 0, 0));
		auto Settings = DBSCollisionTests::TraceSettings(ECC_Visibility);
		F.Registrator->SetIsHitRegistrationEnabled(true, Settings);
		F.Registrator->AddActorsToIgnoreList({Ignored});
		F.Trace(200);
		TestFalse(TEXT("ignored actor not reported"), F.Observer->Contains(Ignored));
		TestTrue(TEXT("unignored actor still hit"), F.Observer->Contains(Target));
		F.Observer->Reset();
		F.Registrator->SetIsHitRegistrationEnabled(true, Settings);
		F.Trace(0);
		// The first blocking hit on the return path is Target; move it away before testing Ignored.
		Target->SetActorLocation(FVector(500, 0, 0));
		F.Observer->Reset();
		F.Trace(200);
		TestTrue(TEXT("re-enable clears ignore list"), F.Observer->Contains(Ignored));
	});

	It("uses the configured default and custom trace channels", [this]()
	{
		auto& F = *Fixture;
		if (!TestNotNull(TEXT("world"), F.World)) return;
		AActor* Target = F.Target(FVector(100, 0, 0), ECR_Block, ECR_Ignore);
		FDamageBehaviorHitDetectionSettings Settings;
		F.Registrator->SetIsHitRegistrationEnabled(true, Settings);
		TestEqual(TEXT("default channel comes from settings"), F.Registrator->GetTraceChannel(), GetDefault<UDamageBehaviorsSystemSettings>()->HitRegistratorsTraceChannel.GetValue());
		Settings = DBSCollisionTests::TraceSettings(ECC_Camera);
		F.Registrator->SetIsHitRegistrationEnabled(true, Settings);
		F.Trace(200);
		TestEqual(TEXT("ignored camera channel emits no hit"), F.Observer->Hits.Num(), 0);
		Settings.CustomTraceChannel = ECC_Visibility;
		F.Registrator->SetIsHitRegistrationEnabled(true, Settings);
		F.Trace(0);
		TestTrue(TEXT("blocking visibility channel hits"), F.Observer->Contains(Target));
	});

	It("forwards a real trace through the component and resets between windows", [this]()
	{
		auto& F = *Fixture;
		if (!TestNotNull(TEXT("world"), F.World)) return;
		AActor* Target = F.Target(FVector(100, 0, 0));
		UDamageBehaviorsComponent* Component = NewObject<UDamageBehaviorsComponent>(F.Owner);
		F.Owner->AddInstanceComponent(Component);
		UDamageBehavior* Behavior = NewObject<UDamageBehavior>(Component);
		Behavior->Description = TEXT("Attack");
		Behavior->HitDetectionSettings = DBSCollisionTests::TraceSettings(ECC_Visibility);
		Behavior->HitRegistratorsToActivateBySource = { FDBSHitRegistratorsToActivateSource(DEFAULT_DAMAGE_BEHAVIOR_SOURCE, { F.Registrator->GetName() }) };
		Component->DamageBehaviorsList.Add(Behavior);
		Component->RegisterComponent();
		Component->Init();
		TStrongObjectPtr<UDBSBehaviorTestObserver> Listener(NewObject<UDBSBehaviorTestObserver>());
		Component->OnHitAnything.AddDynamic(Listener.Get(), &UDBSBehaviorTestObserver::OnComponentHit);
		Component->InvokeDamageBehavior(TEXT("Attack"), true, {}, FInstancedStruct());
		F.Move(200);
		Behavior->Tick(0.016f);
		TestEqual(TEXT("component receives one real hit"), Listener->HitCount, 1);
		TestTrue(TEXT("component event carries target"), Listener->LastActor.Get() == Target);
		F.Move(0);
		Behavior->Tick(0.016f);
		TestEqual(TEXT("same target is deduplicated"), Listener->HitCount, 1);
		Component->InvokeDamageBehavior(TEXT("Attack"), false, {}, FInstancedStruct());
		F.Move(200);
		Behavior->Tick(0.016f);
		TestEqual(TEXT("no hit after deactivate"), Listener->HitCount, 1);
		Component->InvokeDamageBehavior(TEXT("Attack"), true, {}, FInstancedStruct());
		F.Move(0);
		Behavior->Tick(0.016f);
		TestEqual(TEXT("next window hits target again"), Listener->HitCount, 2);
		Component->InvokeDamageBehavior(TEXT("Attack"), false, {}, FInstancedStruct());
	});

	It("checks an actor already overlapping when enabled", [this]()
	{
		auto& F = *Fixture;
		if (!TestNotNull(TEXT("world"), F.World)) return;
		AActor* Target = F.Target(FVector::ZeroVector);
		auto Settings = DBSCollisionTests::EnterSettings();
		Settings.bCheckOverlappingActorsOnStart = true;
		F.Registrator->SetGenerateOverlapEvents(true);
		F.Registrator->SetIsHitRegistrationEnabled(true, Settings);
		TestTrue(TEXT("existing overlap reports target"), F.Observer->Contains(Target));
	});

	It("reports a real entry overlap and stops reporting after disable", [this]()
	{
		auto& F = *Fixture;
		if (!TestNotNull(TEXT("world"), F.World)) return;
		AActor* Target = F.Target(FVector(100, 0, 0));
		auto Settings = DBSCollisionTests::EnterSettings();
		F.Registrator->SetGenerateOverlapEvents(true);
		F.Registrator->SetIsHitRegistrationEnabled(true, Settings);
		F.Move(100);
		TestTrue(TEXT("entry reports target"), F.Observer->Contains(Target));
		if (F.Observer->Hits.Num()) TestTrue(TEXT("entry instigator is owner"), F.Observer->Hits[0].Instigator.Get() == F.Owner);
		F.Move(0);
		F.Observer->Reset();
		F.Registrator->SetIsHitRegistrationEnabled(false, Settings);
		F.Move(100);
		TestFalse(TEXT("disabled flag"), F.Registrator->IsHitRegistrationEnabled());
		TestEqual(TEXT("disabled overlap emits no hit"), F.Observer->Hits.Num(), 0);
	});
}

#endif
