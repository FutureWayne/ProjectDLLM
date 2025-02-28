// Copyright Ludens Studio. All Rights Reserved.


#include "Feedback/NumberPops/ArenaNumberPopComponent_NiagaraText.h"
#include "Feedback/NumberPops/ArenaDamagePopStyleNiagara.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "ArenaGameplayTags.h"
#include "ArenaLogChannel.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaNumberPopComponent_NiagaraText)

UArenaNumberPopComponent_NiagaraText::UArenaNumberPopComponent_NiagaraText(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UArenaNumberPopComponent_NiagaraText::AddNumberPop(const FArenaNumberPopRequest& NewRequest)
{
	int32 LocalDamage = NewRequest.NumberToDisplay;

	if (LocalDamage <= 0)
	{
		return;
	}

	if (NewRequest.TargetTags.HasTagExact(ArenaGameplayTags::DamageType_Grenade_DirectHit))
	{
		LocalDamage *= -1;
	}

	//Add a NiagaraComponent if we don't already have one
	if (!NiagaraComp)
	{
		ACharacter* OwnerCharacter = nullptr;
		if (APlayerController* OwnerController = Cast<APlayerController>(GetOwner()))
		{
			OwnerCharacter = Cast<ACharacter>(OwnerController->GetPawn());
		}

		if (OwnerCharacter)
		{
			NiagaraComp = NewObject<UNiagaraComponent>(OwnerCharacter);
			if (Style != nullptr)
			{
				NiagaraComp->SetAsset(Style->TextNiagara);
				NiagaraComp->bAutoActivate = false;
			}
			NiagaraComp->SetupAttachment(OwnerCharacter->GetRootComponent());
			check(NiagaraComp);
			NiagaraComp->RegisterComponent();
		}
	}

	NiagaraComp->Activate(true);
	NiagaraComp->SetWorldLocation(NewRequest.WorldLocation);

	UE_LOG(LogArena, Log, TEXT("DamageHit location : %s"), *(NewRequest.WorldLocation.ToString()));
	
	//Add Damage information to the current Niagara list - Damage information are packed inside a FVector4 where XYZ = Position, W = Damage
	TArray<FVector4> DamageList = UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayVector4(NiagaraComp, Style->NiagaraArrayName);
	DamageList.Add(FVector4(NewRequest.WorldLocation.X, NewRequest.WorldLocation.Y, NewRequest.WorldLocation.Z, LocalDamage));
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(NiagaraComp, Style->NiagaraArrayName, DamageList);
}
