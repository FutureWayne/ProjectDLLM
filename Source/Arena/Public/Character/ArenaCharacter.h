// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Teams/ArenaTeamAgentInterface.h"
#include "ArenaCharacter.generated.h"

class UArenaEquipmentManagerComponent;
class USpringArmComponent;
class UArenaCombatSet;
class UCameraComponent;
class UArenaAbilitySet;
class UGameplayAbility;
class UInputMappingContext;
class UArenaAbilitySystemComponent;
class UArenaHealthComponent;
class UArenaHealthSet;
class UAttributeSet;
class UAbilitySystemComponent;

UCLASS()
class ARENA_API AArenaCharacter : public ACharacter, public IAbilitySystemInterface, public IArenaTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AArenaCharacter();

	UFUNCTION(BlueprintCallable, Category = "Arena|Character")
	UArenaAbilitySystemComponent* GetArenaAbilitySystemComponent() const;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

private:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UArenaHealthSet> ArenaHealthSet;

	UPROPERTY()
	TObjectPtr<UArenaCombatSet> ArenaCombatSet;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArenaHealthComponent> HealthComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArenaAbilitySet> AbilitySet;
	
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
	
	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArenaEquipmentManagerComponent> EquipmentManagerComponent;
	
	UPROPERTY(ReplicatedUsing = OnRep_MyTeamId)
	FGenericTeamId MyTeamId;

	UPROPERTY()
	FOnArenaTeamIndexChangedDelegate OnTeamChangedDelegate;

public:	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	USkeletalMeshComponent* GetDisplayMesh();

protected:
	// ~Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// ~End AActor Interface

	//~APawn interface
	virtual void NotifyControllerChanged() override;
	//~End of APawn interface

	//~IArenaTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnArenaTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of IArenaTeamAgentInterface interface
	
	void DisableMovementAndCollision() const;
	void DestroyDueToDeath();
	
	// Begins the death sequence for the character (disables collision, disables movement, etc...)
	UFUNCTION()
	virtual void OnDeathStarted(AActor* OwningActor);

	// Ends the death sequence for the character (detaches controller, destroys pawn, etc...)
	UFUNCTION()
	virtual void OnDeathFinished(AActor* OwningActor);

	// Called when the death sequence for the character has completed
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnDeathFinished"))
	void K2_OnDeathFinished();

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnSpeedBuffChanged"))
	void K2_OnSpeedBuffChanged(float OldValue, float NewValue, AActor* EffectInstigator);

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnAbilityActorInfoInitialized"))
	void K2_OnAbilityActorInfoInitialized();

	// Called to determine what happens to the team ID when possession ends
	virtual FGenericTeamId DetermineNewTeamAfterPossessionEnds(FGenericTeamId OldTeamID) const
	{
		// This could be changed to return, e.g., OldTeamID if you want to keep it assigned afterwards, or return an ID for some neutral faction, or etc...
		return FGenericTeamId::NoTeam;
	}
	
private:
	void InitAbilityActorInfo();
	
	virtual void OnRep_PlayerState() override;

	UFUNCTION()
	void OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);
	
	UFUNCTION()
	void OnRep_MyTeamId(FGenericTeamId OldTeamId);

public:
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetCameraComponent() const { return CameraComponent; }

	UFUNCTION(BlueprintCallable, Category = "Arena|Character")
	FORCEINLINE UArenaHealthSet* GetArenaHealthSet() const { return ArenaHealthSet; }

	UFUNCTION(BlueprintCallable, Category = "Arena|Character")
	FORCEINLINE UArenaCombatSet* GetArenaCombatSet() const { return ArenaCombatSet; }
};
