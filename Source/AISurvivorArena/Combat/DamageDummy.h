// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DamageDummy.generated.h"

class UStaticMeshComponent;
class UHealthComponent;

UCLASS()
class AISURVIVORARENA_API ADamageDummy : public AActor
{
    GENERATED_BODY()

public:
    ADamageDummy();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UHealthComponent* HealthComponent;

    UFUNCTION()
    void HandleDeath();
};