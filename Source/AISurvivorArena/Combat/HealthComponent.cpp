// Fill out your copyright notice in the Description page of Project Settings.


// Sets default values for this component's properties
#include "HealthComponent.h"

UHealthComponent::UHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    CurrentHealth = MaxHealth;
}

void UHealthComponent::ApplyDamage(float DamageAmount)
{
    if (DamageAmount <= 0.0f)
    {
        return;
    }

    if (IsDead())
    {
        return;
    }

    CurrentHealth -= DamageAmount;

    UE_LOG(LogTemp, Warning, TEXT("Health: %.1f / %.1f"), CurrentHealth, MaxHealth);

    if (CurrentHealth <= 0.0f)
    {
        CurrentHealth = 0.0f;
        OnDeath.Broadcast();
    }
}

float UHealthComponent::GetHealth() const
{
    return CurrentHealth;
}

float UHealthComponent::GetMaxHealth() const
{
    return MaxHealth;
}

bool UHealthComponent::IsDead() const
{
    return CurrentHealth <= 0.0f;
}

void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}