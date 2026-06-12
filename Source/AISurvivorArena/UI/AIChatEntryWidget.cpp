#include "AIChatEntryWidget.h"
#include "Components/TextBlock.h"

void UAIChatEntryWidget::SetMessage(const FString& Role, const FString& Content)
{
    if (RoleText)
    {
        if (Role == TEXT("user"))
        {
            RoleText->SetText(FText::FromString(TEXT("You: ")));
            RoleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        }
        else
        {
            RoleText->SetText(FText::FromString(TEXT("Echo: ")));
            RoleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.f, 1.0f, 1.0f)));
        }
    }

    if (ContentText)
    {
        ContentText->SetText(FText::FromString(Content));
    }
}