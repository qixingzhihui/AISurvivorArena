// Fill out your copyright notice in the Description page of Project Settings.
#include "EnemyAIController.h"
#include "EnemyCharacter.h"
#include "Combat/HealthComponent.h"
#include "Perception/PawnSensingComponent.h"

void AEnemyAIController::BeginPlay()
{
    Super::BeginPlay();

    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn)
    {
        UE_LOG(LogTemp, Error, TEXT("EnemyAIController has no pawn"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("EnemyAIController controls: %s"), *ControlledPawn->GetName());

    AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(ControlledPawn);

    if (!EnemyCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("Controlled pawn is not EnemyCharacter"));
        return;
    }

    if (!EnemyCharacter->PawnSensingComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("EnemyCharacter has no PawnSensingComponent"));
        return;
    }

    EnemyCharacter->PawnSensingComponent->OnSeePawn.AddDynamic(
        this,
        &AEnemyAIController::OnSeePawn
    );

}

void AEnemyAIController::OnSeePawn(APawn* SeenPawn)
{
    if (!SeenPawn)
    {
        return;
    }
    CurrentTarget = SeenPawn;

    MoveToActor(CurrentTarget, 50.0f);
    bIsChasing = true;

    GetWorldTimerManager().SetTimer(
        AttackTimerHandle,
        this,
        &AEnemyAIController::TryAttack,
        0.2f,
        true
    );
}

void AEnemyAIController::TryAttack()
{
    if (!CurrentTarget)
    {
        return;
    }

    AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(GetPawn());

    if (!EnemyCharacter)
    {
        return;
    }

    float Distance = FVector::Dist2D(
        EnemyCharacter->GetActorLocation(),
        CurrentTarget->GetActorLocation()
    );

    if (bIsChasing&&Distance > EnemyCharacter->AttackRange*3/4)
    {
        MoveToActor(CurrentTarget, 50.0f);
        return;
    }
    if (!bIsChasing && Distance > EnemyCharacter->AttackRange)
    {
        MoveToActor(CurrentTarget, 50.0f);
        bIsChasing = true;
        return;
    }
    if (bIsChasing && Distance <= EnemyCharacter->AttackRange*3/4)
    {
        StopMovement();
        bIsChasing = false;
    }
    float CurrentTime = GetWorld()->GetTimeSeconds();

    if (CurrentTime - LastAttackTime < EnemyCharacter->AttackCooldown)
    {
        return;
    }

    FVector EnemyLocation = EnemyCharacter->GetActorLocation();
    FVector TargetLocation = CurrentTarget->GetActorLocation();

    FVector DirectionToTarget = TargetLocation - EnemyLocation;
    DirectionToTarget.Z = 0.0f;

    if (!DirectionToTarget.IsNearlyZero())
    {
        EnemyCharacter->SetActorRotation(DirectionToTarget.Rotation());
    }

    FVector Start = EnemyCharacter->GetActorLocation();
    Start.Z += 30.0f;

    FVector Forward = EnemyCharacter->GetActorForwardVector();
    FVector End = Start + Forward * EnemyCharacter->AttackRange;

    FHitResult HitResult;

    FCollisionQueryParams Params;

    Params.AddIgnoredActor(EnemyCharacter);
    Params.bTraceComplex = false;

    bool bHit = GetWorld()->SweepSingleByChannel(
        HitResult,
        Start,
        End,
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(40.0f),
        Params
    );

    DrawDebugLine(
        GetWorld(),
        Start,
        End,
        FColor::Red,
        false,
        1.0f,
        0,
        2.0f
    );

    LastAttackTime = CurrentTime;

    if (!bHit)
    {
        UE_LOG(LogTemp, Warning, TEXT("Enemy attack missed"));
        return;
    }

    AActor* HitActor = HitResult.GetActor();

    if (!HitActor)
    {
        return;
    }

    UHealthComponent* TargetHealth = HitActor->FindComponentByClass<UHealthComponent>();

    if (!TargetHealth)
    {
        UE_LOG(LogTemp, Warning, TEXT("Hit target has no HealthComponent"));
        return;
    }

    if (TargetHealth->IsDead())
    {
        return;
    }

    TargetHealth->ApplyDamage(EnemyCharacter->AttackDamage);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("hit %s Enemy attacked target for %.1f damage"),
        *HitActor->GetName(),
        EnemyCharacter->AttackDamage
    );
}