// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "ArenaEffectActor.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UArenaEffectActorDefinition;
class UArenaGrenadeDefinitionData;

UENUM(BlueprintType)
enum class EEffectApplicationPolicy : uint8
{
	ApplyOnOverlap,
	ApplyOnEndOverlap,
	DoNotApply
};

UENUM(BlueprintType)
enum class EEffectRemovalPolicy : uint8
{
	RemoveOnEndOverlap,
	DoNotRemove
};

UCLASS()
class ARENA_API AArenaEffectActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AArenaEffectActor();

	/** Called before FinishSpawning; specialized effects interpret their own data. */
	UFUNCTION(BlueprintNativeEvent, Category = "Effect")
	void InitializeEffect(const UArenaEffectActorDefinition* Definition);

	/**
	 * Captures who produced this effect; call before FinishSpawning.
	 * A thrower's Pawn can be destroyed and collected before a delayed effect spawns, so the source ability system
	 * (PlayerState-owned for players) is retained independently of GetInstigator().
	 */
	void SetSource(APawn* InInstigator, UAbilitySystemComponent* InSourceAbilitySystem);

	/** The ability system owner (PlayerState for players), falling back to the Instigator. */
	AActor* GetSourceActor() const;

	UAbilitySystemComponent* GetSourceAbilitySystem() const { return SourceAbilitySystem.Get(); }

	/** Server-side attribution: the grenade the player threw to produce this effect, if any. Call before FinishSpawning. */
	void SetSourceGrenadeDefinition(const UArenaGrenadeDefinitionData* InSourceGrenadeDefinition);

	const UArenaGrenadeDefinitionData* GetSourceGrenadeDefinition() const { return SourceGrenadeDefinition; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void ApplyEffectToTarget(AActor* Target, TSubclassOf<UGameplayEffect> GameplayEffectClass);

	UFUNCTION(BlueprintCallable)
	void OnOverlap(AActor* TargetActor);

	UFUNCTION(BlueprintCallable)
	void OnEndOverlap(AActor* TargetActor);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Applied Effects")
	bool bDestroyOnEffectApplication = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Applied Effects")
	TSubclassOf<UGameplayEffect> InstantGameplayEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Applied Effects")
	EEffectApplicationPolicy InstantEffectApplicationPolicy = EEffectApplicationPolicy::DoNotApply;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Applied Effects")
	TSubclassOf<UGameplayEffect> DurationGameplayEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Applied Effects")
	EEffectApplicationPolicy DurationEffectApplicationPolicy = EEffectApplicationPolicy::DoNotApply;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Applied Effects")
	TSubclassOf<UGameplayEffect> InfiniteGameplayEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Applied Effects")
	EEffectApplicationPolicy InfiniteEffectApplicationPolicy = EEffectApplicationPolicy::DoNotApply;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Applied Effects")
	EEffectRemovalPolicy InfiniteEffectRemovalPolicy = EEffectRemovalPolicy::RemoveOnEndOverlap;

	/** The Actor that produced this effect; may outlive the source Pawn. */
	TWeakObjectPtr<AActor> SourceActor;

	/**
	 * Retained so a posthumous (or post-detonation) effect can still build a spec.
	 * For player characters this is the PlayerState-owned component, which outlives the Pawn.
	 * Weak on purpose: the effect must not extend the thrower's lifetime.
	 */
	TWeakObjectPtr<UAbilitySystemComponent> SourceAbilitySystem;

	UPROPERTY(Transient)
	TObjectPtr<const UArenaGrenadeDefinitionData> SourceGrenadeDefinition;

	TMap<FActiveGameplayEffectHandle, UAbilitySystemComponent*> ActiveEffectHandles;
};
