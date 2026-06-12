#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AIChatWidget.generated.h"

class UScrollBox;
class UEditableTextBox;
class UButton;
class UAIChatEntryWidget;
class UBorder;
class UTextBlock;

UCLASS()
class AISURVIVORARENA_API UAIChatWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    UFUNCTION(BlueprintCallable, Category = "Echo")
    void SetExpanded(bool bExpanded);

    UFUNCTION(BlueprintCallable, Category = "Echo")
    void ToggleExpanded();

    UFUNCTION(BlueprintCallable, Category = "Echo")
    bool IsExpanded() const;

    UFUNCTION(BlueprintCallable, Category = "Echo")
    void RebuildChatFromHistory();

private:
    UPROPERTY(meta = (BindWidgetOptional))
    UBorder* ChatPanelBorder;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* HeaderText;

    UPROPERTY(meta = (BindWidget))
    UScrollBox* ChatScrollBox;

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* InputTextBox;

    UPROPERTY(meta = (BindWidget))
    UButton* SendButton;

    UPROPERTY(EditDefaultsOnly, Category = "Chat")
    TSubclassOf<UAIChatEntryWidget> ChatEntryWidgetClass;

private:
    UFUNCTION()
    void OnSendClicked();

    UFUNCTION()
    void OnInputCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void OnAIResponseReceived(const FString& Response);

    void SendCurrentInput();
    void AddMessageToChat(const FString& Role, const FString& Content);
    void AddMessageToChatInstant(const FString& Role, const FString& Content);

private:
    FTimerHandle TypewriterTimerHandle;

    FString FullTypewriterText;
    int32 CurrentTypewriterIndex = 0;

    UPROPERTY()
    UAIChatEntryWidget* CurrentTypewriterEntry = nullptr;

    bool bIsExpanded = false;

    void StartTypewriter(const FString& Response);
    void TypewriterTick();

    void ScrollChatToEnd();
    void ApplyPanelLayout();
    void ApplyInputMode();
};