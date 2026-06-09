// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

UCLASS()
class AISURVIVORARENA_API AEnemyAIController : public AAIController
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnSeePawn(APawn* SeenPawn);

    void TryAttack();

    UPROPERTY()
    APawn* CurrentTarget = nullptr;

    FTimerHandle AttackTimerHandle;

    float LastAttackTime = -9999.0f;
    bool bIsChasing = false;
};