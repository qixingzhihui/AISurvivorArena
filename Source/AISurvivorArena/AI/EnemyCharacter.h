// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class UHealthComponent;
class UPawnSensingComponent;

UCLASS()
class AISURVIVORARENA_API AEnemyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AEnemyCharacter();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UPawnSensingComponent* PawnSensingComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    float PreferredAttackDistance = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    float AttackDamage = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    float AttackRange = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    float AttackCooldown = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    float LoseAttackRange = 320.0f;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UHealthComponent* HealthComponent;

    UFUNCTION()
    void HandleDeath();
};