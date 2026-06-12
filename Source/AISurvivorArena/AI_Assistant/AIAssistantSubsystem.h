#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "AIAssistantSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIResponseReceived, const FString&, Response);

UENUM(BlueprintType)
enum class EEchoInputType : uint8
{
    PlayerMessage UMETA(DisplayName = "Player Message"),
    WorldEvent UMETA(DisplayName = "World Event")
};

USTRUCT(BlueprintType)
struct FChatMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString Role;

    UPROPERTY(BlueprintReadOnly)
    FString Content;
};

USTRUCT(BlueprintType)
struct FEchoMemory
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString Key;

    UPROPERTY(BlueprintReadWrite)
    FString Value;
};

UCLASS()
class AISURVIVORARENA_API UEchoMemorySaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TArray<FEchoMemory> SavedMemories;

    UPROPERTY()
    TArray<FChatMessage> SavedChatHistory;

    UPROPERTY()
    int32 SavedRelationshipLevel = 0;

    UPROPERTY()
    int32 SavedMessagesExchanged = 0;
};

UCLASS()
class AISURVIVORARENA_API UAIAssistantSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "AI Assistant")
    void SendMessage(const FString& UserMessage);

    UFUNCTION(BlueprintCallable, Category = "AI Assistant")
    void InjectWorldEvent(const FString& EventDescription);

    UFUNCTION(BlueprintCallable, Category = "AI Assistant")
    void AddOrUpdateMemory(const FString& Key, const FString& Value);

    UFUNCTION(BlueprintCallable, Category = "AI Assistant")
    FString GetMemoryValue(const FString& Key) const;

    UFUNCTION(BlueprintCallable, Category = "AI Assistant")
    void ClearHistory();

    UFUNCTION(BlueprintCallable, Category = "AI Assistant")
    void ClearMemory();

    UFUNCTION(BlueprintCallable, Category = "AI Assistant")
    void SaveEchoMemory();

    UFUNCTION(BlueprintCallable, Category = "AI Assistant")
    void LoadEchoMemory();

    UPROPERTY(BlueprintAssignable, Category = "AI Assistant")
    FOnAIResponseReceived OnAIResponseReceived;

    UPROPERTY(BlueprintReadOnly, Category = "AI Assistant")
    TArray<FChatMessage> ChatHistory;

    UPROPERTY(BlueprintReadOnly, Category = "AI Assistant")
    TArray<FEchoMemory> Memories;

    UPROPERTY(BlueprintReadOnly, Category = "AI Assistant")
    bool bIsWaitingForResponse = false;

    UPROPERTY(BlueprintReadOnly, Category = "AI Assistant")
    int32 RelationshipLevel = 0;

    UPROPERTY(BlueprintReadOnly, Category = "AI Assistant")
    int32 MessagesExchanged = 0;

private:
    FString APIEndpoint;
    FString APIKey;
    FString ModelName;
    FString SystemPrompt;

    FString SaveSlotName = TEXT("EchoMemorySlot");
    int32 SaveUserIndex = 0;

    FString EchoSessionId;

    double LastWorldEventResponseTime = -9999.0;
    double WorldEventCooldown = 8.0;

private:
    void SendToAI(const FString& InputText, EEchoInputType InputType);

    FString BuildRequestBody(const FString& InputText, EEchoInputType InputType);
    FString BuildMemoryText() const;
    FString BuildRelationshipText() const;
    FString BuildModeText(const FString& InputText, EEchoInputType InputType) const;

    void HandleHTTPResponse(const FString& ResponseContent, EEchoInputType InputType);
    void LearnFromPlayerMessage(const FString& UserMessage);
    void ImproveRelationship(int32 Amount);
    void AddChatHistoryMessage(const FString& Role, const FString& Content);
    void TrimChatHistory();

    bool IsGameplayQuestion(const FString& Message) const;
    bool IsDetailRequest(const FString& Message) const;
    bool ShouldEchoRespondToWorldEvent(const FString& EventDescription) const;
};