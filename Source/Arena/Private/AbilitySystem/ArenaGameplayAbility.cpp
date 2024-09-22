// Copyright Ludens Studio. All Rights Reserved.


#include "AbilitySystem/ArenaGameplayAbility.h"

#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "Character/ArenaCharacterBase.h"


AController* UArenaGameplayAbility::GetControllerFromActorInfo() const
{
	if (CurrentActorInfo)
	{
		if (AController* PC = CurrentActorInfo->PlayerController.Get())
		{
			return PC;
		}

		// Look for a player controller or pawn in the owner chain.
		AActor* TestActor = CurrentActorInfo->OwnerActor.Get();
		while (TestActor)
		{
			if (AController* C = Cast<AController>(TestActor))
			{
				return C;
			}

			if (const APawn* Pawn = Cast<APawn>(TestActor))
			{
				return Pawn->GetController();
			}

			TestActor = TestActor->GetOwner();
		}
	}

	return nullptr;	
}

AArenaCharacterBase* UArenaGameplayAbility::GetArenaCharacterFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AArenaCharacterBase>(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

UArenaAbilitySystemComponent* UArenaGameplayAbility::GetArenaAbilitySystemComponentFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<UArenaAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get()) : nullptr);
}
