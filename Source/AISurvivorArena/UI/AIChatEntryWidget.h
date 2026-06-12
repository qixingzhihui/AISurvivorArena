#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AIChatEntryWidget.generated.h"

class UTextBlock;

UCLASS()
class AISURVIVORARENA_API UAIChatEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void SetMessage(const FString& Role, const FString& Content);

    UPROPERTY(meta = (BindWidget))
    UTextBlock* RoleText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* ContentText;
};