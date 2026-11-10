// Copyright Ludens Studio. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS
#include "AbilitySystem/Ability/ArenaGameplayAbility_GrenadeFire.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Equipment/ArenaEquipmentInstance.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "Inventory/ArenaInventoryItemDefinition.h"
#include "Inventory/ArenaInventoryItemInstance.h"
#include "Inventory/InventoryFragment_GrenadeDef.h"
#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "Misc/ScopeExit.h"
#include "UObject/UnrealType.h"
#include "Weapon/ArenaGrenadeDefinitionData.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaGrenadeCooldownAssetsTest, "Arena.Grenade.Cooldown.Assets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaGrenadeCooldownAssetsTest::RunTest(const FString& Parameters)
{
	const FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Grenade"));
	for (const TCHAR* Path :
		{
			TEXT("/Game/Blueprints/AbilitySystem/Abilities/Grenade_New/GA_GrenadeTossingWithADS.GA_GrenadeTossingWithADS_C"),
			TEXT("/Game/Blueprints/AbilitySystem/Abilities/Grenade_New/GA_GrenadeTossingInstant.GA_GrenadeTossingInstant_C")
		})
	{
		UClass* AbilityClass = LoadClass<UArenaGameplayAbility_GrenadeFire>(nullptr, Path);
		if (!TestNotNull(TEXT("Production grenade ability loads"), AbilityClass))
		{
			return false;
		}
		const UArenaGameplayAbility_GrenadeFire* Ability = AbilityClass->GetDefaultObject<UArenaGameplayAbility_GrenadeFire>();
		FDataValidationContext Context;
		TestTrue(TEXT("Production grenade ability passes default cooldown validation"), Ability->IsDataValid(Context) == EDataValidationResult::Valid);
		const UGameplayEffect* Effect = Ability->GetCooldownGameplayEffect();
		if (!TestNotNull(TEXT("Production cooldown effect is configured"), Effect))
		{
			return false;
		}
		TestTrue(TEXT("Cooldown effect grants the shared marker after load"), Effect->GetGrantedTags().HasTagExact(CooldownTag));
		TestEqual(TEXT("Cooldown effect has a finite duration"), Effect->DurationPolicy, EGameplayEffectDurationType::HasDuration);
		TestEqual(TEXT("Duration is supplied by the grenade definition"), Effect->DurationMagnitude.GetMagnitudeCalculationType(), EGameplayEffectMagnitudeCalculation::SetByCaller);
		TestEqual(TEXT("Duration consumes the same key written by ApplyCooldown"), Effect->DurationMagnitude.GetSetByCallerFloat().DataTag, CooldownTag);
		TestFalse(TEXT("Shared effect does not statically lock slot one"), Effect->GetGrantedTags().HasTagExact(TAG_GrenadeSlot_1));
		TestFalse(TEXT("Shared effect does not statically lock slot two"), Effect->GetGrantedTags().HasTagExact(TAG_GrenadeSlot_2));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaGrenadeCooldownSlotsTest, "Arena.Grenade.Cooldown.IndependentSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaGrenadeCooldownSlotsTest::RunTest(const FString& Parameters)
{
	UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
	Instance->InitializeStandalone();
	UWorld* World = Instance->GetWorld();
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	ON_SCOPE_EXIT
	{
		Instance->Shutdown();
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	};

	APawn* Pawn = World->SpawnActor<APawn>();
	UArenaAbilitySystemComponent* ASC = NewObject<UArenaAbilitySystemComponent>(Pawn);
	Pawn->AddInstanceComponent(ASC);
	ASC->RegisterComponent();
	ASC->InitAbilityActorInfo(Pawn, Pawn);
	UClass* EffectClass = LoadClass<UGameplayEffect>(nullptr,
		TEXT("/Game/Blueprints/AbilitySystem/GameplayEffects/Cooldown/GE_Grenade_Cooldown.GE_Grenade_Cooldown_C"));
	const FClassProperty* ItemDefProperty = FindFProperty<FClassProperty>(UArenaInventoryItemInstance::StaticClass(), TEXT("ItemDef"));
	const FClassProperty* EffectProperty = FindFProperty<FClassProperty>(UGameplayAbility::StaticClass(), TEXT("CooldownGameplayEffectClass"));
	if (!TestNotNull(TEXT("Shared cooldown effect loads"), EffectClass) ||
		!TestNotNull(TEXT("Inventory item definition property exists"), ItemDefProperty) ||
		!TestNotNull(TEXT("Ability cooldown effect property exists"), EffectProperty))
	{
		return false;
	}

	UArenaGameplayAbility_GrenadeFire* Abilities[2] = {};
	FGameplayAbilitySpecHandle Handles[2];
	const UArenaGrenadeDefinitionData* Definitions[2] = {};
	const TCHAR* ItemPaths[] =
	{
		TEXT("/Game/Blueprints/Weapon/Grenade/PrimaryGrenade/RapidGrenade/ID_RapidGrenade.ID_RapidGrenade_C"),
		TEXT("/Game/Blueprints/Weapon/Grenade/SecondaryGrenade/NormalGrenade/ID_NormalGrenade.ID_NormalGrenade_C"),
	};
	for (int32 Slot = 0; Slot < 2; ++Slot)
	{
		UClass* ItemClass = LoadClass<UArenaInventoryItemDefinition>(nullptr, ItemPaths[Slot]);
		if (!TestNotNull(TEXT("Production inventory item loads"), ItemClass))
		{
			return false;
		}
		UArenaInventoryItemInstance* Item = NewObject<UArenaInventoryItemInstance>(Pawn);
		ItemDefProperty->SetObjectPropertyValue_InContainer(Item, ItemClass);
		const UInventoryFragment_GrenadeDef* Fragment = Item->FindFragmentByClass<UInventoryFragment_GrenadeDef>();
		Definitions[Slot] = Fragment ? Fragment->GetGrenadeDefinitionData() : nullptr;
		if (!TestNotNull(TEXT("Item resolves its production grenade definition"), Definitions[Slot]))
		{
			return false;
		}
		UArenaEquipmentInstance* Equipment = NewObject<UArenaEquipmentInstance>(Pawn);
		Equipment->SetInstigator(Item);
		Handles[Slot] = ASC->GiveAbility(FGameplayAbilitySpec(UArenaGameplayAbility_GrenadeFire::StaticClass(), 1, INDEX_NONE, Equipment));
		Abilities[Slot] = Cast<UArenaGameplayAbility_GrenadeFire>(ASC->FindAbilitySpecFromHandle(Handles[Slot])->GetPrimaryInstance());
		if (!TestNotNull(TEXT("Granted grenade ability has an instance"), Abilities[Slot]))
		{
			return false;
		}
		EffectProperty->SetObjectPropertyValue_InContainer(Abilities[Slot], EffectClass);
	}
	TestNotEqual(TEXT("Grenades use different slot tags"), Definitions[0]->GrenadeSlotCooldownTag, Definitions[1]->GrenadeSlotCooldownTag);
	const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();
	const FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Grenade"));
	FActiveGameplayEffectHandle Effects[2];
	for (int32 Slot = 0; Slot < 2; ++Slot)
	{
		TestTrue(TEXT("Other slot remains usable while a shared cooldown marker exists"), Abilities[Slot]->CheckCooldown(Handles[Slot], ActorInfo, nullptr));
		Abilities[Slot]->ApplyCooldown(Handles[Slot], ActorInfo, FGameplayAbilityActivationInfo());
		TestFalse(TEXT("Applying cooldown blocks its own slot"), Abilities[Slot]->CheckCooldown(Handles[Slot], ActorInfo, nullptr));
		const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Definitions[Slot]->GrenadeSlotCooldownTag));
		const TArray<FActiveGameplayEffectHandle> Active = ASC->GetActiveEffects(Query);
		if (!TestEqual(TEXT("One real cooldown effect owns the slot tag"), Active.Num(), 1))
		{
			return false;
		}
		Effects[Slot] = Active[0];
		const FActiveGameplayEffect* Effect = ASC->GetActiveGameplayEffect(Effects[Slot]);
		TestEqual(TEXT("Applied duration matches the selected grenade definition"), Effect->Spec.GetDuration(), Definitions[Slot]->GrenadeAbilityCooldownTime);
		TestTrue(TEXT("Applying cooldown grants the shared marker"), ASC->HasMatchingGameplayTag(CooldownTag));
	}
	ASC->RemoveActiveGameplayEffect(Effects[0]);
	TestTrue(TEXT("Removing slot one restores only slot one"), Abilities[0]->CheckCooldown(Handles[0], ActorInfo, nullptr));
	TestFalse(TEXT("Slot two remains blocked by its own effect"), Abilities[1]->CheckCooldown(Handles[1], ActorInfo, nullptr));
	TestTrue(TEXT("Shared marker remains while slot two is cooling"), ASC->HasMatchingGameplayTag(CooldownTag));
	const float ExpirySeconds = Definitions[1]->GrenadeAbilityCooldownTime + 0.1f;
	const int32 Steps = FMath::CeilToInt(ExpirySeconds / 0.02f);
	for (int32 Step = 0; Step < Steps; ++Step)
	{
		// Standalone test timers and tick functions execute once per engine frame.
		++GFrameCounter;
		World->Tick(LEVELTICK_All, ExpirySeconds / Steps);
	}
	TestTrue(TEXT("Slot two becomes available after its duration expires"), Abilities[1]->CheckCooldown(Handles[1], ActorInfo, nullptr));
	TestFalse(TEXT("Shared marker clears when all cooldown effects end"), ASC->HasMatchingGameplayTag(CooldownTag));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaGrenadeCooldownMissingEffectTest, "Arena.Grenade.Cooldown.MissingEffect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaGrenadeCooldownMissingEffectTest::RunTest(const FString& Parameters)
{
	AddExpectedError(TEXT("cooldown effect is not configured"), EAutomationExpectedErrorFlags::Contains, 1);
	GetDefault<UArenaGameplayAbility_GrenadeFire>()->ApplyCooldown(FGameplayAbilitySpecHandle(), nullptr, FGameplayAbilityActivationInfo());
	return true;
}
#endif
