// Copyright Epic Games, Inc. All Rights Reserved.

#include "AISurvivorArenaCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "AISurvivorArena.h"
#include "Combat/HealthComponent.h"
#include "Blueprint/UserWidget.h"
#include "UI/AIChatWidget.h"
#include "DrawDebugHelpers.h"
#include "UI/AIChatWidget.h"

void AAISurvivorArenaCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &AAISurvivorArenaCharacter::HandleDeath);
	}
	CreateAIChatWidgetIfNeeded();
}

AAISurvivorArenaCharacter::AAISurvivorArenaCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 570.f;
	GetCharacterMovement()->AirControl = 3.0f;
	GetCharacterMovement()->MaxWalkSpeed = 700.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	//Create health
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AAISurvivorArenaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAISurvivorArenaCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AAISurvivorArenaCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAISurvivorArenaCharacter::Look);
		//Attack
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AAISurvivorArenaCharacter::Attack);

		EnhancedInputComponent->BindAction(ToggleAIChatAction, ETriggerEvent::Started, this, &AAISurvivorArenaCharacter::ToggleAIChat);

		PlayerInputComponent->BindAction(
			TEXT("ToggleAIChat"),
			IE_Pressed,
			this,
			&AAISurvivorArenaCharacter::ToggleAIChat
		);

	}
	else
	{
		UE_LOG(LogAISurvivorArena, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AAISurvivorArenaCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AAISurvivorArenaCharacter::Attack()
{
	FVector Start = GetActorLocation();
	FVector Forward = GetActorForwardVector();
	FVector End = Start + Forward * AttackRange;

	FHitResult HitResult;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
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

	if (bHit)
	{
		AActor* HitActor = HitResult.GetActor();

		if (HitActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("Hit actor: %s"), *HitActor->GetName());

			UHealthComponent* TargetHealth = HitActor->FindComponentByClass<UHealthComponent>();

			if (TargetHealth)
			{
				TargetHealth->ApplyDamage(AttackDamage);
			}
		}
	}
}

void AAISurvivorArenaCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AAISurvivorArenaCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AAISurvivorArenaCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AAISurvivorArenaCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AAISurvivorArenaCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void AAISurvivorArenaCharacter::HandleDeath()
{
	UE_LOG(LogTemp, Error, TEXT("Player died - Game Over"));

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->DisableInput(PC);
		PC->SetPause(true);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("GAME OVER"));
		}
	}

	SetActorHiddenInGame(true);
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
		MoveComp->StopMovementImmediately();
	}
}

//void AAISurvivorArenaCharacter::ToggleAIChat()
//{
//	APlayerController* PlayerController = Cast<APlayerController>(GetController());
//
//	if (!PlayerController)
//	{
//		return;
//	}
//
//	if (bIsChatOpen)
//	{
//		if (AIChatWidget)
//		{
//			AIChatWidget->RemoveFromParent();
//			AIChatWidget = nullptr;
//		}
//
//		FInputModeGameOnly InputMode;
//		PlayerController->SetInputMode(InputMode);
//		PlayerController->bShowMouseCursor = false;
//
//		bIsChatOpen = false;
//
//		UE_LOG(LogTemp, Warning, TEXT("AI chat closed."));
//	}
//	else
//	{
//		if (!AIChatWidgetClass)
//		{
//			UE_LOG(LogTemp, Error, TEXT("AIChatWidgetClass is not set in player blueprint."));
//			return;
//		}
//
//		AIChatWidget = CreateWidget<UUserWidget>(PlayerController, AIChatWidgetClass);
//
//		if (!AIChatWidget)
//		{
//			UE_LOG(LogTemp, Error, TEXT("Failed to create AIChatWidget."));
//			return;
//		}
//
//		AIChatWidget->AddToViewport();
//
//		FInputModeGameAndUI InputMode;
//		InputMode.SetWidgetToFocus(AIChatWidget->TakeWidget());
//		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
//
//		PlayerController->SetInputMode(InputMode);
//		PlayerController->bShowMouseCursor = true;
//
//		bIsChatOpen = true;
//
//		UE_LOG(LogTemp, Warning, TEXT("AI chat opened."));
//	}
//}

void AAISurvivorArenaCharacter::CreateAIChatWidgetIfNeeded()
{
	if (AIChatWidget)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());

	if (!PlayerController)
	{
		return;
	}

	if (!AIChatWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("AIChatWidgetClass is not set in player blueprint."));
		return;
	}

	AIChatWidget = CreateWidget<UAIChatWidget>(PlayerController, AIChatWidgetClass);

	if (!AIChatWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create AIChatWidget."));
		return;
	}

	AIChatWidget->AddToViewport(10);
	AIChatWidget->SetExpanded(false);

	UE_LOG(LogTemp, Warning, TEXT("Echo mini window created."));
}

void AAISurvivorArenaCharacter::ToggleAIChat()
{
	CreateAIChatWidgetIfNeeded();

	if (AIChatWidget)
	{
		AIChatWidget->ToggleExpanded();
	}
}