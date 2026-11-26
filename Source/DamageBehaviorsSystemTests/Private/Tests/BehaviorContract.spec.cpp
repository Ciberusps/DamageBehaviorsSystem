// Characterization tests for damage behavior hit processing.
#include "BehaviorTestObjects.h"
#include "ANS_InvokeDamageBehavior.h"
#include "Components/SkeletalMeshComponent.h"
#include "DBSGameplayTags.h"
#include "Components/SceneComponent.h"
#include "DamageBehaviorsSystemBlueprintLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
namespace DBSBehaviorContractTests
{
	class FFixture
	{
	public:
		UWorld* World = nullptr;
		AActor* Owner = nullptr;
		UCapsuleHitRegistrator* Capsule = nullptr;
		TStrongObjectPtr<UDBSBehaviorTestDouble> Behavior;
		TStrongObjectPtr<UDBSBehaviorTestObserver> Observer;

		FFixture()
		{
			if (!GEngine) return;
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (!World) return;
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			Owner = SpawnActor();
			Capsule = NewObject<UCapsuleHitRegistrator>(Owner, TEXT("TestCapsule"));
			Capsule->RegisterComponent();
			Behavior.Reset(NewObject<UDBSBehaviorTestDouble>(World));
			Observer.Reset(NewObject<UDBSBehaviorTestObserver>(World));
			FDBSHitRegistratorsSource Source;
			Source.SourceName = DEFAULT_DAMAGE_BEHAVIOR_SOURCE;
			Source.Actor = Owner;
			Source.CapsuleHitRegistrators.Add(Capsule);
			Behavior->HitRegistratorsToActivateBySource = { FDBSHitRegistratorsToActivateSource(DEFAULT_DAMAGE_BEHAVIOR_SOURCE, { Capsule->GetName() }) };
			Behavior->Init(Owner, { Source });
			Behavior->OnHitRegistered.AddDynamic(Observer.Get(), &UDBSBehaviorTestObserver::OnHit);
			Behavior->OnInvokeEnd.AddDynamic(Observer.Get(), &UDBSBehaviorTestObserver::OnEnd);
		}

		AActor* SpawnActor()
		{
			AActor* Actor = World->SpawnActor<AActor>();
			USceneComponent* Root = NewObject<USceneComponent>(Actor);
			Actor->SetRootComponent(Root);
			Root->RegisterComponent();
			return Actor;
		}

		void Hit(AActor* Target)
		{
			FDBSHitRegistratorHitResult Result;
			Result.HitActor = Target;
			Result.Instigator = Owner;
			Capsule->OnHitRegistered.Broadcast(Result, Capsule);
		}

		~FFixture()
		{
			if (Behavior.IsValid()) Behavior->MakeActive(false, FInstancedStruct());
			Behavior.Reset();
			Observer.Reset();
			if (World && GEngine)
			{
				World->EndPlay(EEndPlayReason::Quit);
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
		}
	};
}

BEGIN_DEFINE_SPEC(FDBSBehaviorContractSpec, "DamageBehaviorsSystem.Behavior.Contract",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
	TUniquePtr<DBSBehaviorContractTests::FFixture> Fixture;
END_DEFINE_SPEC(FDBSBehaviorContractSpec)

void FDBSBehaviorContractSpec::Define()
{
	BeforeEach([this]() { Fixture = MakeUnique<DBSBehaviorContractTests::FFixture>(); });
	AfterEach([this]() { Fixture.Reset(); });

	It("filters inactive and duplicate hits and resets on deactivation", [this]()
	{
		if (!TestNotNull(TEXT("World"), Fixture->World)) return;
		AActor* Target = Fixture->SpawnActor();
		Fixture->Hit(Target);
		TestEqual(TEXT("inactive ignores hit"), Fixture->Observer->HitCount, 0);
		Fixture->Behavior->MakeActive(true, FInstancedStruct());
		TestTrue(TEXT("selected capsule enabled"), Fixture->Capsule->IsHitRegistrationEnabled());
		Fixture->Hit(Target);
		Fixture->Hit(Target);
		TestEqual(TEXT("duplicate processed once"), Fixture->Behavior->ProcessCount, 1);
		TestEqual(TEXT("one hit event"), Fixture->Observer->HitCount, 1);
		TestTrue(TEXT("event carries hit actor"), Fixture->Observer->LastActor.Get() == Target);
		Fixture->Behavior->MakeActive(false, FInstancedStruct());
		TestTrue(TEXT("end reports hit"), Fixture->Observer->bLastEndHit);
		TestFalse(TEXT("capsule disabled"), Fixture->Capsule->IsHitRegistrationEnabled());
		Fixture->Behavior->MakeActive(true, FInstancedStruct());
		Fixture->Hit(Target);
		TestEqual(TEXT("new window processes actor again"), Fixture->Observer->HitCount, 2);
	});

	It("keeps invocation payload separate from hit output", [this]()
	{
		if (!TestNotNull(TEXT("World"), Fixture->World)) return;
		FDBSNoiseEventOnDamageSettings Input;
		Input.Loudness = 7.0f;
		FInstancedStruct Payload;
		Payload.InitializeAs<FDBSNoiseEventOnDamageSettings>(Input);
		Fixture->Behavior->MakeActive(true, Payload);
		TestEqual(TEXT("invocation payload preserved"), Fixture->Behavior->GetCurrentInvokePayload().Get<FDBSNoiseEventOnDamageSettings>().Loudness, 7.0f);
		Fixture->Hit(Fixture->SpawnActor());
		TestFalse(TEXT("default output starts empty"), Fixture->Observer->LastPayload.IsValid());
		Fixture->Behavior->bSetOutput = true;
		Fixture->Hit(Fixture->SpawnActor());
		TestTrue(TEXT("override supplies output"), Fixture->Observer->LastPayload.IsValid());
		if (Fixture->Observer->LastPayload.IsValid())
		{
			TestEqual(TEXT("output reaches delegate"), Fixture->Observer->LastPayload.Get<FDBSNoiseEventOnDamageSettings>().Loudness, 42.0f);
		}
	});

	It("records the target before a rejected ProcessHit", [this]()
	{
		if (!TestNotNull(TEXT("World"), Fixture->World)) return;
		Fixture->Behavior->bProcessResult = false;
		Fixture->Behavior->MakeActive(true, FInstancedStruct());
		AActor* Target = Fixture->SpawnActor();
		Fixture->Hit(Target);
		Fixture->Behavior->bProcessResult = true;
		Fixture->Hit(Target);
		TestEqual(TEXT("rejected hit still deduplicates"), Fixture->Behavior->ProcessCount, 1);
		TestEqual(TEXT("no hit delegate for rejected result"), Fixture->Observer->HitCount, 0);
		Fixture->Behavior->MakeActive(false, FInstancedStruct());
		TestTrue(TEXT("end reports recorded target"), Fixture->Observer->bLastEndHit);
	});

	It("processes hits even when adding targets is disallowed", [this]()
	{
		if (!TestNotNull(TEXT("World"), Fixture->World)) return;
		Fixture->Behavior->bAllowAdd = false;
		Fixture->Behavior->MakeActive(true, FInstancedStruct());
		AActor* Target = Fixture->SpawnActor();
		Fixture->Hit(Target);
		Fixture->Hit(Target);
		TestEqual(TEXT("both hits processed"), Fixture->Observer->HitCount, 2);
		Fixture->Behavior->MakeActive(false, FInstancedStruct());
		TestFalse(TEXT("end reports no recorded target"), Fixture->Observer->bLastEndHit);
	});

	It("does not deduplicate ByEntering hits in the default implementation", [this]()
	{
		if (!TestNotNull(TEXT("World"), Fixture->World)) return;
		Fixture->Behavior->HitDetectionSettings.HitDetectionType = EDamageBehaviorHitDetectionType::ByEntering;
		Fixture->Behavior->HitDetectionSettings.CollisionProfileName.Name = TEXT("NoCollision");
		Fixture->Behavior->MakeActive(true, FInstancedStruct());
		TestFalse(TEXT("entering mode does not tick"), Fixture->Behavior->IsTickable());
		AActor* Target = Fixture->SpawnActor();
		Fixture->Hit(Target);
		Fixture->Hit(Target);
		TestEqual(TEXT("both events reach behavior"), Fixture->Observer->HitCount, 2);
		Fixture->Behavior->MakeActive(false, FInstancedStruct());
		TestFalse(TEXT("end reports no recorded target"), Fixture->Observer->bLastEndHit);
	});

	It("filters invalid actors and resolves attached actors as one target", [this]()
	{
		if (!TestNotNull(TEXT("World"), Fixture->World)) return;
		Fixture->Behavior->MakeActive(true, FInstancedStruct());
		Fixture->Hit(nullptr);
		TestEqual(TEXT("invalid target ignored"), Fixture->Behavior->ProcessCount, 0);
		AActor* Parent = Fixture->SpawnActor();
		AActor* Child = Fixture->SpawnActor();
		Child->AttachToActor(Parent, FAttachmentTransformRules::KeepWorldTransform);
		Fixture->Hit(Child);
		Fixture->Hit(Parent);
		TestEqual(TEXT("parent and child share hit state"), Fixture->Observer->HitCount, 1);
		Fixture->Behavior->MakeActive(false, FInstancedStruct());
		Fixture->Behavior->MakeActive(true, FInstancedStruct());
		Fixture->Hit(Parent);
		TestEqual(TEXT("new window clears attachment hit state"), Fixture->Observer->HitCount, 2);
	});

	It("uses the configured registrator and exposes shared activation state", [this]()
	{
		if (!TestNotNull(TEXT("World"), Fixture->World)) return;
		UCapsuleHitRegistrator* Other = NewObject<UCapsuleHitRegistrator>(Fixture->Owner, TEXT("OtherCapsule"));
		Other->RegisterComponent();
		Fixture->Behavior->HitRegistratorsSources[0].CapsuleHitRegistrators.Add(Other);
		Fixture->Behavior->MakeActive(true, FInstancedStruct());
		TestTrue(TEXT("selected capsule enabled"), Fixture->Capsule->IsHitRegistrationEnabled());
		TestFalse(TEXT("unselected capsule stays disabled"), Other->IsHitRegistrationEnabled());
		TStrongObjectPtr<UDBSBehaviorTestDouble> OtherBehavior(NewObject<UDBSBehaviorTestDouble>(Fixture->World));
		OtherBehavior->HitRegistratorsToActivateBySource = Fixture->Behavior->HitRegistratorsToActivateBySource;
		OtherBehavior->Init(Fixture->Owner, Fixture->Behavior->HitRegistratorsSources);
		OtherBehavior->MakeActive(true, FInstancedStruct());
		Fixture->Behavior->MakeActive(false, FInstancedStruct());
		TestFalse(TEXT("disabling one behavior disables shared capsule"), Fixture->Capsule->IsHitRegistrationEnabled());
		OtherBehavior->MakeActive(false, FInstancedStruct());
	});

	It("routes runtime notify begin and end by tag before legacy name", [this]()
	{
		if (!TestNotNull(TEXT("World"), Fixture->World)) return;
		USkeletalMeshComponent* Mesh = NewObject<USkeletalMeshComponent>(Fixture->Owner);
		Fixture->Owner->AddInstanceComponent(Mesh);
		Mesh->RegisterComponent();
		UDamageBehaviorsComponent* Component = NewObject<UDamageBehaviorsComponent>(Fixture->Owner);
		Fixture->Owner->AddInstanceComponent(Component);
		UDamageBehavior* Tagged = NewObject<UDamageBehavior>(Component);
		Tagged->Tags.AddTag(DBSGameplayTags::TAG_DamageBehaviors_Default);
		UDamageBehavior* Named = NewObject<UDamageBehavior>(Component);
		Named->Description = TEXT("LegacyAttack");
		Component->DamageBehaviorsList = { Tagged, Named };
		Component->RegisterComponent();
		Component->Init();
		UANS_InvokeDamageBehavior* Notify = NewObject<UANS_InvokeDamageBehavior>(Fixture->World);
		Notify->Name = TEXT("LegacyAttack");
		Notify->Tag = DBSGameplayTags::TAG_DamageBehaviors_Default;
		Notify->TargetSources.Add(DEFAULT_DAMAGE_BEHAVIOR_SOURCE, false);
		const FAnimNotifyEventReference EventReference;
		Notify->NotifyBegin(Mesh, nullptr, 1.0f, EventReference);
		TestTrue(TEXT("tag selected and empty enabled sources fall back to ThisActor"), Tagged->IsTickable());
		TestFalse(TEXT("legacy name not selected when tag valid"), Named->IsTickable());
		Notify->NotifyEnd(Mesh, nullptr, EventReference);
		TestFalse(TEXT("end deactivates tagged behavior"), Tagged->IsTickable());
		Notify->Tag = FGameplayTag();
		Notify->NotifyBegin(Mesh, nullptr, 1.0f, EventReference);
		TestTrue(TEXT("invalid tag selects legacy name"), Named->IsTickable());
		Notify->NotifyEnd(Mesh, nullptr, EventReference);
		TestFalse(TEXT("end deactivates named behavior"), Named->IsTickable());
	});

	It("resolves the uppermost attached actor", [this]()
	{
		if (!TestNotNull(TEXT("World"), Fixture->World)) return;
		AActor* Parent = Fixture->SpawnActor();
		AActor* Child = Fixture->SpawnActor();
		AActor* Leaf = Fixture->SpawnActor();
		Child->AttachToActor(Parent, FAttachmentTransformRules::KeepWorldTransform);
		Leaf->AttachToActor(Child, FAttachmentTransformRules::KeepWorldTransform);
		TestTrue(TEXT("top of chain"), UDamageBehaviorsSystemBlueprintLibrary::GetTopmostAttachedActor(Leaf) == Parent);
		Leaf->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		TestTrue(TEXT("detached actor resolves to itself"), UDamageBehaviorsSystemBlueprintLibrary::GetTopmostAttachedActor(Leaf) == Leaf);
	});
}
#endif
