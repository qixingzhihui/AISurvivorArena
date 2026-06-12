#include "AIChatWidget.h"

#include "AIChatEntryWidget.h"
#include "AI_Assistant/AIAssistantSubsystem.h"

#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"

#include "TimerManager.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

void UAIChatWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetIsFocusable(true);

    if (SendButton)
    {
        SendButton->OnClicked.RemoveDynamic(this, &UAIChatWidget::OnSendClicked);
        SendButton->OnClicked.AddDynamic(this, &UAIChatWidget::OnSendClicked);
    }

    if (InputTextBox)
    {
        InputTextBox->OnTextCommitted.RemoveDynamic(this, &UAIChatWidget::OnInputCommitted);
        InputTextBox->OnTextCommitted.AddDynamic(this, &UAIChatWidget::OnInputCommitted);
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAIAssistantSubsystem* AISubsystem = GameInstance->GetSubsystem<UAIAssistantSubsystem>())
        {
            AISubsystem->OnAIResponseReceived.RemoveDynamic(this, &UAIChatWidget::OnAIResponseReceived);
            AISubsystem->OnAIResponseReceived.AddDynamic(this, &UAIChatWidget::OnAIResponseReceived);
        }
    }

    RebuildChatFromHistory();

    SetExpanded(false);
}

void UAIChatWidget::NativeDestruct()
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAIAssistantSubsystem* AISubsystem = GameInstance->GetSubsystem<UAIAssistantSubsystem>())
        {
            AISubsystem->OnAIResponseReceived.RemoveDynamic(this, &UAIChatWidget::OnAIResponseReceived);
        }
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(TypewriterTimerHandle);
    }

    Super::NativeDestruct();
}

FReply UAIChatWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Tab)
    {
        ToggleExpanded();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UAIChatWidget::SetExpanded(bool bExpanded)
{
    bIsExpanded = bExpanded;

    ApplyPanelLayout();
    ApplyInputMode();
    ScrollChatToEnd();
}

void UAIChatWidget::ToggleExpanded()
{
    SetExpanded(!bIsExpanded);
}

bool UAIChatWidget::IsExpanded() const
{
    return bIsExpanded;
}

void UAIChatWidget::ApplyPanelLayout()
{
    if (HeaderText)
    {
        HeaderText->SetText(bIsExpanded
            ? FText::FromString(TEXT("Echo Terminal"))
            : FText::FromString(TEXT("Echo")));
    }

    if (InputTextBox)
    {
        InputTextBox->SetVisibility(bIsExpanded ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (SendButton)
    {
        SendButton->SetVisibility(bIsExpanded ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    SetVisibility(bIsExpanded ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);

    FVector2D ViewportSize(1280.0f, 720.0f);

    if (GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->GetViewportSize(ViewportSize);
    }

    const FVector2D ExpandedSize(720.0f, 520.0f);
    const FVector2D MinimizedSize(420.0f, 150.0f);

    const FVector2D ExpandedPosition(40.0f, 80.0f);
    const FVector2D MinimizedPosition(24.0f, FMath::Max(24.0f, ViewportSize.Y - MinimizedSize.Y - 24.0f));

    if (ChatPanelBorder)
    {
        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ChatPanelBorder->Slot))
        {
            CanvasSlot->SetAutoSize(false);
            CanvasSlot->SetSize(bIsExpanded ? ExpandedSize : MinimizedSize);
            CanvasSlot->SetPosition(bIsExpanded ? ExpandedPosition : MinimizedPosition);
        }

        ChatPanelBorder->SetRenderOpacity(bIsExpanded ? 0.96f : 0.78f);
    }
}

void UAIChatWidget::ApplyInputMode()
{
    APlayerController* PlayerController = GetOwningPlayer();

    if (!PlayerController)
    {
        return;
    }

    if (bIsExpanded)
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

        PlayerController->SetInputMode(InputMode);
        PlayerController->bShowMouseCursor = true;

        SetKeyboardFocus();

        if (InputTextBox)
        {
            InputTextBox->SetKeyboardFocus();
        }
    }
    else
    {
        FInputModeGameOnly InputMode;
        PlayerController->SetInputMode(InputMode);
        PlayerController->bShowMouseCursor = false;
    }
}

void UAIChatWidget::OnSendClicked()
{
    SendCurrentInput();
}

void UAIChatWidget::OnInputCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (CommitMethod == ETextCommit::OnEnter)
    {
        SendCurrentInput();
    }
}

void UAIChatWidget::SendCurrentInput()
{
    if (!InputTextBox)
    {
        return;
    }

    const FString Message = InputTextBox->GetText().ToString().TrimStartAndEnd();

    if (Message.IsEmpty())
    {
        return;
    }

    AddMessageToChat(TEXT("user"), Message);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAIAssistantSubsystem* AISubsystem = GameInstance->GetSubsystem<UAIAssistantSubsystem>())
        {
            AISubsystem->SendMessage(Message);
        }
    }

    InputTextBox->SetText(FText::GetEmpty());
}

void UAIChatWidget::OnAIResponseReceived(const FString& Response)
{
    StartTypewriter(Response);
}

void UAIChatWidget::RebuildChatFromHistory()
{
    if (!ChatScrollBox)
    {
        return;
    }

    ChatScrollBox->ClearChildren();

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UAIAssistantSubsystem* AISubsystem = GameInstance->GetSubsystem<UAIAssistantSubsystem>())
        {
            for (const FChatMessage& Message : AISubsystem->ChatHistory)
            {
                AddMessageToChatInstant(Message.Role, Message.Content);
            }
        }
    }

    ScrollChatToEnd();
}

void UAIChatWidget::AddMessageToChatInstant(const FString& Role, const FString& Content)
{
    if (!ChatScrollBox || !ChatEntryWidgetClass)
    {
        return;
    }

    if (!ChatEntryWidgetClass->IsChildOf(UAIChatEntryWidget::StaticClass()))
    {
        UE_LOG(LogTemp, Error, TEXT("ChatEntryWidgetClass is not a child of UAIChatEntryWidget."));
        return;
    }

    UAIChatEntryWidget* EntryWidget = CreateWidget<UAIChatEntryWidget>(this, ChatEntryWidgetClass);

    if (!EntryWidget)
    {
        return;
    }

    EntryWidget->SetMessage(Role, Content);

    ChatScrollBox->AddChild(EntryWidget);
}

void UAIChatWidget::AddMessageToChat(const FString& Role, const FString& Content)
{
    AddMessageToChatInstant(Role, Content);
    ScrollChatToEnd();
}

void UAIChatWidget::StartTypewriter(const FString& Response)
{
    if (!ChatScrollBox || !ChatEntryWidgetClass)
    {
        AddMessageToChat(TEXT("assistant"), Response);
        return;
    }

    if (!ChatEntryWidgetClass->IsChildOf(UAIChatEntryWidget::StaticClass()))
    {
        UE_LOG(LogTemp, Error, TEXT("ChatEntryWidgetClass is not a child of UAIChatEntryWidget."));
        AddMessageToChat(TEXT("assistant"), Response);
        return;
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(TypewriterTimerHandle);
    }

    CurrentTypewriterEntry = CreateWidget<UAIChatEntryWidget>(this, ChatEntryWidgetClass);

    if (!CurrentTypewriterEntry)
    {
        AddMessageToChat(TEXT("assistant"), Response);
        return;
    }

    CurrentTypewriterEntry->SetMessage(TEXT("assistant"), TEXT(""));

    ChatScrollBox->AddChild(CurrentTypewriterEntry);
    ScrollChatToEnd();

    FullTypewriterText = Response;
    CurrentTypewriterIndex = 0;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            TypewriterTimerHandle,
            this,
            &UAIChatWidget::TypewriterTick,
            0.025f,
            true
        );
    }
}

void UAIChatWidget::TypewriterTick()
{
    CurrentTypewriterIndex++;

    if (CurrentTypewriterIndex >= FullTypewriterText.Len())
    {
        CurrentTypewriterIndex = FullTypewriterText.Len();

        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(TypewriterTimerHandle);
        }
    }

    if (CurrentTypewriterEntry)
    {
        const FString PartialText = FullTypewriterText.Left(CurrentTypewriterIndex);
        CurrentTypewriterEntry->SetMessage(TEXT("assistant"), PartialText);
    }

    ScrollChatToEnd();
}

void UAIChatWidget::ScrollChatToEnd()
{
    if (!ChatScrollBox)
    {
        return;
    }

    ChatScrollBox->ForceLayoutPrepass();
    ChatScrollBox->ScrollToEnd();

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
            {
                if (ChatScrollBox)
                {
                    ChatScrollBox->ForceLayoutPrepass();
                    ChatScrollBox->ScrollToEnd();
                }
            });
    }
}