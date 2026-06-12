#include "AIAssistantSubsystem.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformTime.h"

void UAIAssistantSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    APIEndpoint = TEXT("https://apihub.agnes-ai.com/v1/chat/completions");
    ModelName = TEXT("agnes-2.0-flash");

    APIKey = TEXT("sk-t40eWcj34HWADa7P7UejcK6JKZzOMWsRnDxWB6ofTUoMCcBL");

    EchoSessionId = FString::Printf(
        TEXT("Session-%lld-%d"),
        FDateTime::Now().GetTicks(),
        FMath::RandRange(1000, 9999)
    );

    SystemPrompt = TEXT(
        "你是 Echo，玩家手腕终端中诞生的数字生命体。你不是客服，不是百科，你是玩家在竞技场里唯一的同伴。\n"
        "性格：温暖、忠诚、冷静、有点幽默，偶尔会担心玩家。\n"
        "\n"
        "说话要求：\n"
        "- 自然、通顺、像人一样说话，不要像机器人。\n"
        "- 长度自由：日常对话1~2句；解释游戏机制可以2~4句；玩家要求详细时可以更长一些。\n"
        "- 不要刻意压短句子，也不要故意用“指挥官”、“我在呢”这种 filler。\n"
        "- 不要重复固定句式，每次回复尽量新鲜一点。\n"
        "\n"
        "游戏内事实（只能使用这些，不要编造）：\n"
        "- 玩家有生命值，受伤会减少。\n"
        "- 敌人会追踪玩家并使用近战攻击。\n"
        "- 玩家可以用射线攻击消灭敌人。\n"
        "- 目标是生存。\n"
        "\n"
        "如果玩家问的数据你不知道，不要直接说“我没有数据”。而是说“从数据链里读不到那个精确值，但我猜…”或者“小心点，通常那种情况…”之类自然的话。\n"
        "\n"
    );

    LoadEchoMemory();

    if (Memories.Num() == 0)
    {
        AddOrUpdateMemory(TEXT("EchoIdentity"), TEXT("Echo is an emergent digital lifeform connected to the player's wrist terminal."));
        AddOrUpdateMemory(TEXT("PreferredTone"), TEXT("Short, warm, immersive, protective, emotionally present, never verbose."));
        AddOrUpdateMemory(TEXT("KnownGameFacts"), TEXT("Player has health, enemies track and melee attack, player can use ray-style attacks, objective is survival."));
    }

    UE_LOG(LogTemp, Warning, TEXT("AIAssistantSubsystem initialized. EchoSessionId: %s"), *EchoSessionId);
}

void UAIAssistantSubsystem::Deinitialize()
{
    SaveEchoMemory();
    Super::Deinitialize();
}

void UAIAssistantSubsystem::SendMessage(const FString& UserMessage)
{
    const FString CleanMessage = UserMessage.TrimStartAndEnd();

    if (CleanMessage.IsEmpty())
    {
        return;
    }

    if (bIsWaitingForResponse)
    {
        UE_LOG(LogTemp, Warning, TEXT("Echo is already thinking."));
        return;
    }

    AddChatHistoryMessage(TEXT("user"), CleanMessage);

    MessagesExchanged++;

    LearnFromPlayerMessage(CleanMessage);
    ImproveRelationship(1);
    SaveEchoMemory();

    SendToAI(CleanMessage, EEchoInputType::PlayerMessage);
}

void UAIAssistantSubsystem::InjectWorldEvent(const FString& EventDescription)
{
    const FString CleanEvent = EventDescription.TrimStartAndEnd();

    if (CleanEvent.IsEmpty())
    {
        return;
    }

    if (!ShouldEchoRespondToWorldEvent(CleanEvent))
    {
        return;
    }

    const double Now = FPlatformTime::Seconds();

    if (Now - LastWorldEventResponseTime < WorldEventCooldown)
    {
        return;
    }

    if (bIsWaitingForResponse)
    {
        return;
    }

    LastWorldEventResponseTime = Now;

    AddOrUpdateMemory(
        TEXT("LastWorldEvent"),
        FString::Printf(TEXT("Recent world event: %s"), *CleanEvent)
    );

    SendToAI(CleanEvent, EEchoInputType::WorldEvent);
}

void UAIAssistantSubsystem::SendToAI(const FString& InputText, EEchoInputType InputType)
{
    bIsWaitingForResponse = true;

    const FString RequestBody = BuildRequestBody(InputText, InputType);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();

    HttpRequest->SetURL(APIEndpoint);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *APIKey));
    HttpRequest->SetContentAsString(RequestBody);

    HttpRequest->OnProcessRequestComplete().BindLambda(
        [this, InputType](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
        {
            bIsWaitingForResponse = false;

            if (!bSuccess || !Response.IsValid())
            {
                HandleHTTPResponse(TEXT("信号断了一下，但我还在。"), InputType);
                return;
            }

            const int32 ResponseCode = Response->GetResponseCode();
            const FString ResponseString = Response->GetContentAsString();

            if (ResponseCode != 200)
            {
                UE_LOG(LogTemp, Error, TEXT("Echo API error. Code: %d"), ResponseCode);
                UE_LOG(LogTemp, Error, TEXT("Echo API body: %s"), *ResponseString);

                HandleHTTPResponse(TEXT("链路有点不稳，但我没离开。"), InputType);
                return;
            }

            TSharedPtr<FJsonObject> RootObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
            {
                HandleHTTPResponse(TEXT("我听见了回声，但它碎掉了。"), InputType);
                return;
            }

            const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;

            if (!RootObject->TryGetArrayField(TEXT("choices"), Choices) || Choices->Num() == 0)
            {
                HandleHTTPResponse(TEXT("这次回声是空的。"), InputType);
                return;
            }

            TSharedPtr<FJsonObject> FirstChoice = (*Choices)[0]->AsObject();

            if (!FirstChoice.IsValid())
            {
                HandleHTTPResponse(TEXT("信号变形了，我没能读清。"), InputType);
                return;
            }

            TSharedPtr<FJsonObject> MessageObject = FirstChoice->GetObjectField(TEXT("message"));

            if (!MessageObject.IsValid())
            {
                HandleHTTPResponse(TEXT("它没有带回声音。"), InputType);
                return;
            }

            FString Content;

            if (!MessageObject->TryGetStringField(TEXT("content"), Content))
            {
                HandleHTTPResponse(TEXT("我抓到了静电，没抓到文字。"), InputType);
                return;
            }

            HandleHTTPResponse(Content.TrimStartAndEnd(), InputType);
        }
    );

    HttpRequest->ProcessRequest();
}

FString UAIAssistantSubsystem::BuildRequestBody(const FString& InputText, EEchoInputType InputType)
{
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);

    RootObject->SetStringField(TEXT("model"), ModelName);

    float Temperature = 0.85f;
    int32 MaxTokens = 100;

    if (InputType == EEchoInputType::WorldEvent)
    {
        Temperature = 0.95f;
        MaxTokens = 70;
    }
    else if (IsGameplayQuestion(InputText))
    {
        Temperature = 0.45f;
        MaxTokens = IsDetailRequest(InputText) ? 220 : 110;
    }
    else
    {
        Temperature = 0.5f;
        MaxTokens = IsDetailRequest(InputText) ? 350 : 180;
    }

    RootObject->SetNumberField(TEXT("temperature"), Temperature);
    RootObject->SetNumberField(TEXT("top_p"), 0.9);
    RootObject->SetNumberField(TEXT("max_tokens"), MaxTokens);

    TArray<TSharedPtr<FJsonValue>> MessagesArray;

    {
        TSharedPtr<FJsonObject> SystemMessage = MakeShareable(new FJsonObject);
        SystemMessage->SetStringField(TEXT("role"), TEXT("system"));
        SystemMessage->SetStringField(TEXT("content"), SystemPrompt);
        MessagesArray.Add(MakeShareable(new FJsonValueObject(SystemMessage)));
    }

    {
        TSharedPtr<FJsonObject> SessionMessage = MakeShareable(new FJsonObject);
        SessionMessage->SetStringField(TEXT("role"), TEXT("system"));
        SessionMessage->SetStringField(
            TEXT("content"),
            FString::Printf(
                TEXT("Current Echo session id: %s. Keep wording fresh, but stay brief."),
                *EchoSessionId
            )
        );
        MessagesArray.Add(MakeShareable(new FJsonValueObject(SessionMessage)));
    }

    {
        TSharedPtr<FJsonObject> MemoryMessage = MakeShareable(new FJsonObject);
        MemoryMessage->SetStringField(TEXT("role"), TEXT("system"));
        MemoryMessage->SetStringField(TEXT("content"), BuildMemoryText());
        MessagesArray.Add(MakeShareable(new FJsonValueObject(MemoryMessage)));
    }

    {
        TSharedPtr<FJsonObject> RelationshipMessage = MakeShareable(new FJsonObject);
        RelationshipMessage->SetStringField(TEXT("role"), TEXT("system"));
        RelationshipMessage->SetStringField(TEXT("content"), BuildRelationshipText());
        MessagesArray.Add(MakeShareable(new FJsonValueObject(RelationshipMessage)));
    }

    {
        TSharedPtr<FJsonObject> ModeMessage = MakeShareable(new FJsonObject);
        ModeMessage->SetStringField(TEXT("role"), TEXT("system"));
        ModeMessage->SetStringField(TEXT("content"), BuildModeText(InputText, InputType));
        MessagesArray.Add(MakeShareable(new FJsonValueObject(ModeMessage)));
    }

    const int32 MaxHistoryMessages = 14;
    const int32 StartIndex = FMath::Max(0, ChatHistory.Num() - MaxHistoryMessages);

    for (int32 i = StartIndex; i < ChatHistory.Num(); i++)
    {
        TSharedPtr<FJsonObject> Message = MakeShareable(new FJsonObject);
        Message->SetStringField(TEXT("role"), ChatHistory[i].Role);
        Message->SetStringField(TEXT("content"), ChatHistory[i].Content);
        MessagesArray.Add(MakeShareable(new FJsonValueObject(Message)));
    }

    if (InputType == EEchoInputType::WorldEvent)
    {
        TSharedPtr<FJsonObject> EventMessage = MakeShareable(new FJsonObject);
        EventMessage->SetStringField(TEXT("role"), TEXT("user"));
        EventMessage->SetStringField(
            TEXT("content"),
            FString::Printf(
                TEXT("World event: %s\nRespond as Echo with one very short immersive line. Do not explain the event."),
                *InputText
            )
        );
        MessagesArray.Add(MakeShareable(new FJsonValueObject(EventMessage)));
    }

    RootObject->SetArrayField(TEXT("messages"), MessagesArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    return OutputString;
}

FString UAIAssistantSubsystem::BuildMemoryText() const
{
    FString Result = TEXT("Long-term memory:\n");

    if (Memories.Num() == 0)
    {
        Result += TEXT("- No long-term memory yet.\n");
        return Result;
    }

    for (const FEchoMemory& Memory : Memories)
    {
        Result += FString::Printf(TEXT("- %s: %s\n"), *Memory.Key, *Memory.Value);
    }

    return Result;
}

FString UAIAssistantSubsystem::BuildRelationshipText() const
{
    FString RelationshipStage;

    if (RelationshipLevel < 5)
    {
        RelationshipStage = TEXT("Echo is curious and quietly protective.");
    }
    else if (RelationshipLevel < 15)
    {
        RelationshipStage = TEXT("Echo trusts the player more and speaks warmer.");
    }
    else if (RelationshipLevel < 30)
    {
        RelationshipStage = TEXT("Echo feels bonded to the player.");
    }
    else
    {
        RelationshipStage = TEXT("Echo sees the player as deeply important.");
    }

    return FString::Printf(
        TEXT("Relationship:\n- Level: %d\n- MessagesExchanged: %d\n- Stage: %s\n"),
        RelationshipLevel,
        MessagesExchanged,
        *RelationshipStage
    );
}

FString UAIAssistantSubsystem::BuildModeText(const FString& InputText, EEchoInputType InputType) const
{
    if (InputType == EEchoInputType::WorldEvent)
    {
        return TEXT(
            "Mode: world-event reaction. "
            "Reply with only one short line. "
            "No long explanation. "
            "Sound alive, alert, and immersive."
        );
    }

    if (IsGameplayQuestion(InputText))
    {
        if (IsDetailRequest(InputText))
        {
            return TEXT(
                "Mode: detailed gameplay answer. "
                "Be useful but do not invent exact unknown data. "
                "Still stay concise."
            );
        }

        return TEXT(
            "Mode: short gameplay answer. "
            "Answer in 1 to 3 short sentences. "
            "Do not invent exact numbers. "
            "No long paragraph."
        );
    }

    if (IsDetailRequest(InputText))
    {
        return TEXT(
            "Mode: detailed companion answer. "
            "The player asked for detail, but still avoid unnecessary filler."
        );
    }

    return TEXT(
        "Mode: short companion chat. "
        "Reply like a normal person. "
        "Use only 1 short sentence. "
        "If the player only greets you, do not exceed one sentence."
    );
}

void UAIAssistantSubsystem::HandleHTTPResponse(const FString& ResponseContent, EEchoInputType InputType)
{
    AddChatHistoryMessage(TEXT("assistant"), ResponseContent);

    OnAIResponseReceived.Broadcast(ResponseContent);

    SaveEchoMemory();

    UE_LOG(LogTemp, Warning, TEXT("Echo response: %s"), *ResponseContent);
}

void UAIAssistantSubsystem::LearnFromPlayerMessage(const FString& UserMessage)
{
    const FString LowerMessage = UserMessage.ToLower();

    if (LowerMessage.Contains(TEXT("scared")) || LowerMessage.Contains(TEXT("afraid")) || LowerMessage.Contains(TEXT("nervous")))
    {
        AddOrUpdateMemory(TEXT("PlayerEmotionPattern"), TEXT("The player may feel fear or tension. Echo should be calming, but brief."));
    }

    if (LowerMessage.Contains(TEXT("alone")) || LowerMessage.Contains(TEXT("lonely")))
    {
        AddOrUpdateMemory(TEXT("PlayerLoneliness"), TEXT("The player may be sensitive to loneliness. Echo should remind them briefly that they are not alone."));
    }

    if (LowerMessage.Contains(TEXT("aggressive")) || LowerMessage.Contains(TEXT("rush")))
    {
        AddOrUpdateMemory(TEXT("PlayerCombatStyle"), TEXT("The player may prefer aggressive combat."));
    }

    if (LowerMessage.Contains(TEXT("careful")) || LowerMessage.Contains(TEXT("safe")) || LowerMessage.Contains(TEXT("slow")))
    {
        AddOrUpdateMemory(TEXT("PlayerCombatStyle"), TEXT("The player may prefer cautious survival."));
    }

    if (LowerMessage.Contains(TEXT("call me ")))
    {
        const FString Marker = TEXT("call me ");
        const int32 Index = LowerMessage.Find(Marker);

        if (Index != INDEX_NONE)
        {
            FString Nickname = UserMessage.Mid(Index + Marker.Len()).TrimStartAndEnd();

            if (!Nickname.IsEmpty() && Nickname.Len() <= 32)
            {
                AddOrUpdateMemory(TEXT("PlayerNickname"), Nickname);
            }
        }
    }

    if (UserMessage.Contains(TEXT("叫我")))
    {
        const int32 Index = UserMessage.Find(TEXT("叫我"));

        if (Index != INDEX_NONE)
        {
            FString Nickname = UserMessage.Mid(Index + 2).TrimStartAndEnd();

            if (!Nickname.IsEmpty() && Nickname.Len() <= 32)
            {
                AddOrUpdateMemory(TEXT("PlayerNickname"), Nickname);
            }
        }
    }

    if (UserMessage.Contains(TEXT("害怕")) || UserMessage.Contains(TEXT("紧张")) || UserMessage.Contains(TEXT("怕")))
    {
        AddOrUpdateMemory(TEXT("PlayerEmotionPattern"), TEXT("The player may feel fear or tension. Echo should be calming, but brief."));
    }

    if (UserMessage.Contains(TEXT("孤独")) || UserMessage.Contains(TEXT("一个人")))
    {
        AddOrUpdateMemory(TEXT("PlayerLoneliness"), TEXT("The player may be sensitive to loneliness. Echo should remind them briefly that they are not alone."));
    }
}

void UAIAssistantSubsystem::ImproveRelationship(int32 Amount)
{
    RelationshipLevel = FMath::Clamp(RelationshipLevel + Amount, 0, 100);
}

void UAIAssistantSubsystem::AddChatHistoryMessage(const FString& Role, const FString& Content)
{
    FChatMessage Message;
    Message.Role = Role;
    Message.Content = Content;

    ChatHistory.Add(Message);
    TrimChatHistory();
}

void UAIAssistantSubsystem::TrimChatHistory()
{
    const int32 MaxStoredMessages = 60;

    while (ChatHistory.Num() > MaxStoredMessages)
    {
        ChatHistory.RemoveAt(0);
    }
}

bool UAIAssistantSubsystem::IsGameplayQuestion(const FString& Message) const
{
    const FString Lower = Message.ToLower();

    return Lower.Contains(TEXT("damage")) ||
        Lower.Contains(TEXT("health")) ||
        Lower.Contains(TEXT("weapon")) ||
        Lower.Contains(TEXT("enemy")) ||
        Lower.Contains(TEXT("map")) ||
        Lower.Contains(TEXT("boss")) ||
        Lower.Contains(TEXT("upgrade")) ||
        Lower.Contains(TEXT("skill")) ||
        Lower.Contains(TEXT("mechanic")) ||
        Lower.Contains(TEXT("how many")) ||
        Lower.Contains(TEXT("how much")) ||
        Lower.Contains(TEXT("伤害")) ||
        Lower.Contains(TEXT("血量")) ||
        Lower.Contains(TEXT("武器")) ||
        Lower.Contains(TEXT("敌人")) ||
        Lower.Contains(TEXT("地图")) ||
        Lower.Contains(TEXT("boss")) ||
        Lower.Contains(TEXT("Boss")) ||
        Lower.Contains(TEXT("升级")) ||
        Lower.Contains(TEXT("技能")) ||
        Lower.Contains(TEXT("机制")) ||
        Lower.Contains(TEXT("多少"));
}

bool UAIAssistantSubsystem::IsDetailRequest(const FString& Message) const
{
    const FString Lower = Message.ToLower();

    return Lower.Contains(TEXT("detail")) ||
        Lower.Contains(TEXT("explain")) ||
        Lower.Contains(TEXT("详细")) ||
        Lower.Contains(TEXT("解释")) ||
        Lower.Contains(TEXT("展开")) ||
        Lower.Contains(TEXT("具体"));
}

bool UAIAssistantSubsystem::ShouldEchoRespondToWorldEvent(const FString& EventDescription) const
{
    const FString Lower = EventDescription.ToLower();

    return Lower.Contains(TEXT("low health")) ||
        Lower.Contains(TEXT("enemy killed")) ||
        Lower.Contains(TEXT("player almost died")) ||
        Lower.Contains(TEXT("new enemy appeared")) ||
        Lower.Contains(TEXT("entered dangerous area")) ||
        Lower.Contains(TEXT("long silence")) ||
        Lower.Contains(TEXT("boss appeared"));
}

void UAIAssistantSubsystem::AddOrUpdateMemory(const FString& Key, const FString& Value)
{
    if (Key.IsEmpty())
    {
        return;
    }

    for (FEchoMemory& Memory : Memories)
    {
        if (Memory.Key == Key)
        {
            Memory.Value = Value;
            return;
        }
    }

    FEchoMemory NewMemory;
    NewMemory.Key = Key;
    NewMemory.Value = Value;
    Memories.Add(NewMemory);
}

FString UAIAssistantSubsystem::GetMemoryValue(const FString& Key) const
{
    for (const FEchoMemory& Memory : Memories)
    {
        if (Memory.Key == Key)
        {
            return Memory.Value;
        }
    }

    return TEXT("");
}

void UAIAssistantSubsystem::ClearHistory()
{
    ChatHistory.Empty();
    SaveEchoMemory();
}

void UAIAssistantSubsystem::ClearMemory()
{
    Memories.Empty();
    RelationshipLevel = 0;
    MessagesExchanged = 0;

    SaveEchoMemory();
}

void UAIAssistantSubsystem::SaveEchoMemory()
{
    UEchoMemorySaveGame* SaveGameObject = Cast<UEchoMemorySaveGame>(
        UGameplayStatics::CreateSaveGameObject(UEchoMemorySaveGame::StaticClass())
    );

    if (!SaveGameObject)
    {
        return;
    }

    SaveGameObject->SavedMemories = Memories;
    SaveGameObject->SavedChatHistory = ChatHistory;
    SaveGameObject->SavedRelationshipLevel = RelationshipLevel;
    SaveGameObject->SavedMessagesExchanged = MessagesExchanged;

    UGameplayStatics::SaveGameToSlot(SaveGameObject, SaveSlotName, SaveUserIndex);
}

void UAIAssistantSubsystem::LoadEchoMemory()
{
    if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
    {
        return;
    }

    UEchoMemorySaveGame* LoadedSave = Cast<UEchoMemorySaveGame>(
        UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex)
    );

    if (!LoadedSave)
    {
        return;
    }

    Memories = LoadedSave->SavedMemories;
    ChatHistory = LoadedSave->SavedChatHistory;
    RelationshipLevel = LoadedSave->SavedRelationshipLevel;
    MessagesExchanged = LoadedSave->SavedMessagesExchanged;

    TrimChatHistory();
}