#include "SoccerGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "SoccerSim/SoccerSim.h"

void USoccerGameInstance::Init()
{
    Super::Init();

    if (GConfig && GGameIni.Len() > 0)
    {
        FString S;
        if (GConfig->GetString(TEXT("/Script/SoccerSim.SoccerGameInstance"), TEXT("MatchMapName"), S, GGameIni) && !S.IsEmpty())
        {
            MatchMapName = FName(*S);
        }
        if (GConfig->GetString(TEXT("/Script/SoccerSim.SoccerGameInstance"), TEXT("MainMenuMapName"), S, GGameIni) && !S.IsEmpty())
        {
            MainMenuMapName = FName(*S);
        }
    }
}

void USoccerGameInstance::OpenMatchMap()
{
    UWorld* World = GetWorld();
    if (World)
    {
        UGameplayStatics::OpenLevel(World, MatchMapName);
    }
}

void USoccerGameInstance::OpenMainMenuMap()
{
    UWorld* World = GetWorld();
    if (World)
    {
        UGameplayStatics::OpenLevel(World, MainMenuMapName);
    }
}

void USoccerGameInstance::Rematch()
{
    UWorld* World = GetWorld();
    if (World)
    {
        UGameplayStatics::OpenLevel(World, FName(*World->GetName()));
    }
}
