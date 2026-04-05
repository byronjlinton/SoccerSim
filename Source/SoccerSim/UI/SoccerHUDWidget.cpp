#include "SoccerHUDWidget.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "SoccerSim/Core/SoccerGameState.h"
#include "Kismet/GameplayStatics.h"
#include "SoccerSim/SoccerSim.h"

void USoccerHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogSoccerSim, Log, TEXT("SoccerHUDWidget::NativeConstruct - WidgetTree=%p"), WidgetTree.Get());

    UCanvasPanel* RootCanvas = nullptr;

    if (WidgetTree)
    {
        RootCanvas = WidgetTree->FindWidget<UCanvasPanel>(TEXT("RootCanvas"));
        if (!RootCanvas && WidgetTree->RootWidget)
        {
            RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
            if (RootCanvas)
            {
                UE_LOG(LogSoccerSim, Log, TEXT("  Using existing RootWidget as canvas"));
            }
        }
    }

    if (!RootCanvas && WidgetTree)
    {
        RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
        WidgetTree->RootWidget = RootCanvas;
        UE_LOG(LogSoccerSim, Log, TEXT("  Created new RootCanvas"));
    }

    if (!RootCanvas)
    {
        UE_LOG(LogSoccerSim, Warning, TEXT("  No RootCanvas available!"));
        return;
    }

    UE_LOG(LogSoccerSim, Log, TEXT("  RootCanvas found, creating text blocks..."));

    auto SetupTextBlock = [&](UTextBlock* TB, const FText& InitialText, const FLinearColor& Color, int32 FontSize, const FVector2D& Position) -> UTextBlock*
    {
        if (!TB)
        {
            TB = NewObject<UTextBlock>(this);
            UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(TB);
            Slot->SetAnchors(FAnchors(0.5f, 0.0f));
            Slot->SetAlignment(FVector2D(0.5f, 0.0f));
            Slot->SetAutoSize(true);
            Slot->SetPosition(Position);
        }
        else
        {
            // Widget came from designer — fix its slot positioning
            if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(TB->Slot))
            {
                Slot->SetAnchors(FAnchors(0.5f, 0.0f));
                Slot->SetAlignment(FVector2D(0.5f, 0.0f));
                Slot->SetAutoSize(true);
                Slot->SetPosition(Position);
            }
        }

        TB->SetText(InitialText);
        TB->SetColorAndOpacity(Color);
        TB->SetJustification(ETextJustify::Center);

        FSlateFontInfo Font = TB->GetFont();
        Font.Size = FontSize;
        Font.TypefaceFontName = FName("Bold");
        TB->SetFont(Font);

        return TB;
    };

    if (WidgetTree)
    {
        HomeScoreText = WidgetTree->FindWidget<UTextBlock>(TEXT("HomeScoreText"));
        AwayScoreText = WidgetTree->FindWidget<UTextBlock>(TEXT("AwayScoreText"));
        ClockText = WidgetTree->FindWidget<UTextBlock>(TEXT("ClockText"));
        PhaseText = WidgetTree->FindWidget<UTextBlock>(TEXT("PhaseText"));
    }

    UE_LOG(LogSoccerSim, Log, TEXT("  HomeScoreText found=%s, AwayScoreText found=%s, ClockText found=%s, PhaseText found=%s"),
        HomeScoreText ? TEXT("yes") : TEXT("no"), AwayScoreText ? TEXT("yes") : TEXT("no"),
        ClockText ? TEXT("yes") : TEXT("no"), PhaseText ? TEXT("yes") : TEXT("no"));

    HomeScoreText = SetupTextBlock(HomeScoreText, FText::FromString(TEXT("0")),
        FLinearColor(1.0f, 0.3f, 0.3f, 1.0f), 48, FVector2D(-80.0f, 30.0f));
    AwayScoreText = SetupTextBlock(AwayScoreText, FText::FromString(TEXT("0")),
        FLinearColor(0.3f, 0.5f, 1.0f, 1.0f), 48, FVector2D(80.0f, 30.0f));
    ClockText = SetupTextBlock(ClockText, FText::FromString(TEXT("00:00")),
        FLinearColor::White, 36, FVector2D(0.0f, 30.0f));
    PhaseText = SetupTextBlock(PhaseText, FText::FromString(TEXT("")),
        FLinearColor(1.0f, 0.9f, 0.3f, 1.0f), 24, FVector2D(0.0f, 80.0f));

    UE_LOG(LogSoccerSim, Log, TEXT("  RootCanvas children: %d"), RootCanvas->GetChildrenCount());

    bWidgetsCreated = true;
    UE_LOG(LogSoccerSim, Log, TEXT("  HUD widgets created successfully"));
}

void USoccerHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bWidgetsCreated) return;

    int32 Home = GetHomeScore();
    int32 Away = GetAwayScore();
    FString Clock = GetMatchClockText();
    FString Phase = GetCurrentPhaseText();

    if (Home != LastHomeScore && HomeScoreText)
    {
        HomeScoreText->SetText(FText::AsNumber(Home));
        LastHomeScore = Home;
    }

    if (Away != LastAwayScore && AwayScoreText)
    {
        AwayScoreText->SetText(FText::AsNumber(Away));
        LastAwayScore = Away;
    }

    if (Clock != LastClockText && ClockText)
    {
        ClockText->SetText(FText::FromString(Clock));
        LastClockText = Clock;
    }

    if (Phase != LastPhaseText && PhaseText)
    {
        PhaseText->SetText(FText::FromString(Phase));
        LastPhaseText = Phase;
    }
}

int32 USoccerHUDWidget::GetHomeScore() const
{
    ASoccerGameState* GS = GetSoccerGameState();
    return GS ? GS->HomeScore : 0;
}

int32 USoccerHUDWidget::GetAwayScore() const
{
    ASoccerGameState* GS = GetSoccerGameState();
    return GS ? GS->AwayScore : 0;
}

FString USoccerHUDWidget::GetMatchClockText() const
{
    ASoccerGameState* GS = GetSoccerGameState();
    return GS ? GS->GetMatchClockDisplay() : TEXT("00:00");
}

FString USoccerHUDWidget::GetCurrentPhaseText() const
{
    ASoccerGameState* GS = GetSoccerGameState();
    if (!GS) return TEXT("");
    return UEnum::GetValueAsString(GS->GetMatchPhase());
}

ASoccerGameState* USoccerHUDWidget::GetSoccerGameState() const
{
    UWorld* World = GetWorld();
    return World ? World->GetGameState<ASoccerGameState>() : nullptr;
}
