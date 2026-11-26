#include "CoreMinimal.h"
#include "DamageBehaviorsComponent.h"
#include "DBSGameplayTags.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

namespace ComponentLookupInvokeTests
{
	class FFixture
	{
	public:
		FFixture()
		{
			if (GEngine)
			{
				World = UWorld::CreateWorld(EWorldType::Game, false);
				if (World)
				{
					GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
				}
			}
		}

		~FFixture()
		{
			if (World && GEngine)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
		}

		UDamageBehaviorsComponent* AddComponent() const
		{
			AActor* Actor = World ? World->SpawnActor<AActor>() : nullptr;
			if (!Actor) return nullptr;
			UDamageBehaviorsComponent* Component = NewObject<UDamageBehaviorsComponent>(Actor);
			Actor->AddInstanceComponent(Component);
			return Component;
		}

		static UDamageBehavior* AddBehavior(UDamageBehaviorsComponent* Component, const FString& Description, bool bDefaultTag = false)
		{
			UDamageBehavior* Behavior = NewObject<UDamageBehavior>(Component);
			Behavior->Description = Description;
			if (bDefaultTag) Behavior->Tags.AddTag(DBSGameplayTags::TAG_DamageBehaviors_Default);
			Component->DamageBehaviorsList.Add(Behavior);
			return Behavior;
		}

		static bool AddSource(UDamageBehaviorsComponent* Component, const FString& Name, AActor* Actor, UDamageBehaviorsSourceEvaluator* Evaluator = nullptr)
		{
			FArrayProperty* Property = FindFProperty<FArrayProperty>(UDamageBehaviorsComponent::StaticClass(), TEXT("DamageBehaviorsSources"));
			const FStructProperty* Inner = Property ? CastField<FStructProperty>(Property->Inner) : nullptr;
			const FWeakObjectProperty* OwnerProperty = Inner ? FindFProperty<FWeakObjectProperty>(Inner->Struct, TEXT("OwnerActor")) : nullptr;
			if (!OwnerProperty) return false;
			auto* Sources = Property->ContainerPtrToValuePtr<TArray<FDamageBehaviorsSource>>(Component);
			FDamageBehaviorsSource& Source = Sources->Emplace_GetRef();
			Source.SourceName = Name;
			Source.Evaluator = Evaluator;
			OwnerProperty->SetObjectPropertyValue_InContainer(&Source, Actor);
			return true;
		}

		static bool IsActive(const UDamageBehavior* Behavior)
		{
			const FBoolProperty* Property = FindFProperty<FBoolProperty>(UDamageBehavior::StaticClass(), TEXT("bIsActive"));
			return Property && Property->GetPropertyValue_InContainer(Behavior);
		}

	private:
		UWorld* World = nullptr;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FComponentLookupTest, "DamageBehaviorsSystem.Component.Lookup",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)

bool FComponentLookupTest::RunTest(const FString& Parameters)
{
	using namespace ComponentLookupInvokeTests;
	FFixture Fixture;
	UDamageBehaviorsComponent* Component = Fixture.AddComponent();
	if (!TestNotNull(TEXT("component"), Component)) return false;

	Component->DamageBehaviorsList.Add(nullptr);
	UDamageBehavior* Tagged = Fixture.AddBehavior(Component, TEXT("Tagged"), true);
	UDamageBehavior* Legacy = Fixture.AddBehavior(Component, TEXT("DmgBehDefault"));
	UDamageBehavior* Duplicate = Fixture.AddBehavior(Component, TEXT("Tagged"), true);
	const FGameplayTag DefaultTag = DBSGameplayTags::TAG_DamageBehaviors_Default;
	const FGameplayTag ParentTag = FGameplayTag::RequestGameplayTag(TEXT("DamageBehaviors"), false);

	TestTrue(TEXT("legacy alias selects first matching tagged behavior"), Component->GetDamageBehavior(TEXT("DmgBehDefault")) == Tagged);
	TestTrue(TEXT("tag selects first matching behavior"), Component->GetDamageBehaviorByTag(DefaultTag) == Tagged);
	TestTrue(TEXT("description selects first duplicate"), Component->GetDamageBehavior(TEXT("Tagged")) == Tagged);
	TestNull(TEXT("unknown name"), Component->GetDamageBehavior(TEXT("Unknown")));
	TestNull(TEXT("invalid tag"), Component->GetDamageBehaviorByTag(FGameplayTag()));
	if (TestTrue(TEXT("parent tag is registered"), ParentTag.IsValid()))
	{
		TestNull(TEXT("parent tag does not match child"), Component->GetDamageBehaviorByTag(ParentTag));
	}

	Component->DamageBehaviorsList.Remove(Tagged);
	TestTrue(TEXT("legacy description matches default tag"), Component->GetDamageBehaviorByTag(DefaultTag) == Legacy);
	TestTrue(TEXT("legacy description wins before later tagged behavior"), Component->GetDamageBehavior(TEXT("DmgBehDefault")) == Legacy);
	Component->DamageBehaviorsList.Remove(Legacy);
	TestTrue(TEXT("tagged fallback remains after removal"), Component->GetDamageBehavior(TEXT("DmgBehDefault")) == Duplicate);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FComponentInvokeTest, "DamageBehaviorsSystem.Component.InvokeAndSources",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)

bool FComponentInvokeTest::RunTest(const FString& Parameters)
{
	using namespace ComponentLookupInvokeTests;
	FFixture Fixture;
	UDamageBehaviorsComponent* Local = Fixture.AddComponent();
	UDamageBehaviorsComponent* Remote = Fixture.AddComponent();
	UDamageBehaviorsComponent* Third = Fixture.AddComponent();
	if (!TestNotNull(TEXT("local component"), Local) || !TestNotNull(TEXT("remote component"), Remote) || !TestNotNull(TEXT("third component"), Third)) return false;
	UDamageBehavior* LocalBehavior = Fixture.AddBehavior(Local, TEXT("Attack"), true);
	UDamageBehavior* RemoteBehavior = Fixture.AddBehavior(Remote, TEXT("Attack"), true);
	UDamageBehavior* ThirdBehavior = Fixture.AddBehavior(Third, TEXT("Attack"), true);
	if (!TestTrue(TEXT("active property exists"), FindFProperty<FBoolProperty>(UDamageBehavior::StaticClass(), TEXT("bIsActive")) != nullptr)) return false;
	if (!TestTrue(TEXT("install remote source"), Fixture.AddSource(Local, TEXT("Weapon"), Remote->GetOwner())) ||
		!TestTrue(TEXT("install self source"), Fixture.AddSource(Local, TEXT("Self"), Local->GetOwner())) ||
		!TestTrue(TEXT("install missing actor source"), Fixture.AddSource(Local, TEXT("NoActor"), nullptr)) ||
		!TestTrue(TEXT("install null evaluator result"), Fixture.AddSource(Local, TEXT("NoResult"), Local->GetOwner(), NewObject<UDamageBehaviorsSourceEvaluator>(Local))) ||
		!TestTrue(TEXT("install remote onward source"), Fixture.AddSource(Remote, TEXT("Weapon"), Third->GetOwner()))) return false;

	const FGameplayTag DefaultTag = DBSGameplayTags::TAG_DamageBehaviors_Default;
	FInstancedStruct Payload;
	Payload.InitializeAs<FGameplayTagContainer>();
	Payload.GetMutable<FGameplayTagContainer>().AddTag(DefaultTag);

	Local->InvokeDamageBehaviorByTag(DefaultTag, true, {}, Payload);
	TestTrue(TEXT("empty sources invoke local default"), Fixture.IsActive(LocalBehavior));
	TestFalse(TEXT("empty sources do not invoke remote"), Fixture.IsActive(RemoteBehavior));
	TestTrue(TEXT("payload forwarded locally"), LocalBehavior->GetCurrentInvokePayload().GetScriptStruct() == FGameplayTagContainer::StaticStruct());
	if (LocalBehavior->GetCurrentInvokePayload().GetScriptStruct() == FGameplayTagContainer::StaticStruct())
	{
		TestTrue(TEXT("payload value forwarded locally"), LocalBehavior->GetCurrentInvokePayload().Get<FGameplayTagContainer>().HasTagExact(DefaultTag));
	}
	Local->InvokeDamageBehavior(TEXT("Attack"), false, {}, FInstancedStruct());
	TestFalse(TEXT("legacy invoke deactivates local"), Fixture.IsActive(LocalBehavior));

	Local->InvokeDamageBehaviorByTag(DefaultTag, true, {TEXT("Weapon")}, Payload);
	TestFalse(TEXT("explicit remote source does not invoke local"), Fixture.IsActive(LocalBehavior));
	TestTrue(TEXT("tag invoke reaches remote"), Fixture.IsActive(RemoteBehavior));
	TestFalse(TEXT("forwarding only invokes remote default, not onward source"), Fixture.IsActive(ThirdBehavior));
	TestTrue(TEXT("payload forwarded remotely"), RemoteBehavior->GetCurrentInvokePayload().GetScriptStruct() == FGameplayTagContainer::StaticStruct());
	if (RemoteBehavior->GetCurrentInvokePayload().GetScriptStruct() == FGameplayTagContainer::StaticStruct())
	{
		TestTrue(TEXT("remote payload value"), RemoteBehavior->GetCurrentInvokePayload().Get<FGameplayTagContainer>().HasTagExact(DefaultTag));
	}
	Local->InvokeDamageBehavior(TEXT("Attack"), false, {TEXT("Weapon")}, FInstancedStruct());
	TestFalse(TEXT("legacy invoke reaches remote"), Fixture.IsActive(RemoteBehavior));
	Local->InvokeDamageBehaviorByTag(DefaultTag, true, {DEFAULT_DAMAGE_BEHAVIOR_SOURCE, TEXT("Weapon")}, Payload);
	TestTrue(TEXT("combined sources invoke local"), Fixture.IsActive(LocalBehavior));
	TestTrue(TEXT("combined sources invoke remote"), Fixture.IsActive(RemoteBehavior));
	Local->InvokeDamageBehaviorByTag(DefaultTag, false, {DEFAULT_DAMAGE_BEHAVIOR_SOURCE, TEXT("Weapon")}, FInstancedStruct());
	TestFalse(TEXT("combined sources deactivate local"), Fixture.IsActive(LocalBehavior));
	TestFalse(TEXT("combined sources deactivate remote"), Fixture.IsActive(RemoteBehavior));

	Local->InvokeDamageBehavior(TEXT("Attack"), true, {TEXT("Missing"), TEXT("Self"), TEXT("NoActor"), TEXT("NoResult")}, Payload);
	Local->InvokeDamageBehaviorByTag(DefaultTag, true, {TEXT("Missing"), TEXT("Self"), TEXT("NoActor"), TEXT("NoResult")}, Payload);
	TestFalse(TEXT("invalid or self sources do not invoke local"), Fixture.IsActive(LocalBehavior));
	TestFalse(TEXT("invalid or self sources do not invoke remote"), Fixture.IsActive(RemoteBehavior));
	Local->InvokeDamageBehavior(TEXT("Unknown"), true, {}, Payload);
	Local->InvokeDamageBehaviorByTag(FGameplayTag::RequestGameplayTag(TEXT("DamageBehaviors"), false), true, {}, Payload);
	TestFalse(TEXT("unmatched names and parent tags do not invoke"), Fixture.IsActive(LocalBehavior));

	AddExpectedMessagePlain(TEXT("called with empty DamageBehaviorName"), ELogVerbosity::Warning);
	Local->InvokeDamageBehavior(TEXT(""), true, {}, Payload);
	AddExpectedMessagePlain(TEXT("called with invalid DamageBehaviorTag"), ELogVerbosity::Warning);
	Local->InvokeDamageBehaviorByTag(FGameplayTag(), true, {}, Payload);
	TestFalse(TEXT("empty name and invalid tag do not invoke"), Fixture.IsActive(LocalBehavior));
	return true;
}

#endif
