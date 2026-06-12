// Fill out your copyright notice in the Description page of Project Settings.
#include "EnemyCharacter.h"
#include "Combat/HealthComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyCharacter::AEnemyCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

    PawnSensingComponent = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensingComponent"));
    PawnSensingComponent->bSeePawns = true;
    PawnSensingComponent->bOnlySensePlayers = true;
    PawnSensingComponent->SightRadius = 10000.0f;
    PawnSensingComponent->SetPeripheralVisionAngle(180.0f);
    PawnSensingComponent->SensingInterval = 0.25f;
    PawnSensingComponent->SetSensingUpdatesEnabled(true);

    GetCharacterMovement()->MaxWalkSpeed = 300.0f;
}

void AEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (HealthComponent)
    {
        HealthComponent->OnDeath.AddDynamic(this, &AEnemyCharacter::HandleDeath);
    }
}

void AEnemyCharacter::HandleDeath()
{
    Destroy();
}