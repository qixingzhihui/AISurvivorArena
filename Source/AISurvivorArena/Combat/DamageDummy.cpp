// Fill out your copyright notice in the Description page of Project Settings.
#include "DamageDummy.h"
#include "Components/StaticMeshComponent.h"
#include "HealthComponent.h"

ADamageDummy::ADamageDummy()
{
    PrimaryActorTick.bCanEverTick = false;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
}

void ADamageDummy::BeginPlay()
{
    Super::BeginPlay();

    if (HealthComponent)
    {
        HealthComponent->OnDeath.AddDynamic(this, &ADamageDummy::HandleDeath);
    }
}

void ADamageDummy::HandleDeath()
{
    UE_LOG(LogTemp, Error, TEXT("DamageDummy died"));
    Destroy();
}