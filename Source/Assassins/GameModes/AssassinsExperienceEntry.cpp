// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/AssassinsExperienceEntry.h"
#include "CommonSessionSubsystem.h"

UCommonSession_HostSessionRequest* UAssassinsExperienceEntry::CreateHostingRequest(const UObject* WorldContextObject) const
{
    const FString ExperienceName = ExperienceID.PrimaryAssetName.ToString();
    const FString ExperienceEntryName = GetPrimaryAssetId().PrimaryAssetName.ToString();

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    UCommonSession_HostSessionRequest* Result = NewObject<UCommonSession_HostSessionRequest>();

    Result->OnlineMode = ECommonSessionOnlineMode::Online;
    Result->bUseLobbies = true;
    Result->bUseLobbiesVoiceChat = false;
    // We always enable presence on this session because it is the primary session used for matchmaking. 
    // For online systems that care about presence, only the primary session should have presence enabled
    Result->bUsePresence = !IsRunningDedicatedServer();

    Result->MapID = MapID;
    Result->ModeNameForAdvertisement = ExperienceEntryName;
    Result->ExtraArgs = ExtraArgs;
    // Me: It is applied to OptionString of the game mode
    Result->ExtraArgs.Add(TEXT("Experience"), ExperienceName);
    Result->MaxPlayerCount = MaxPlayerCount;

    return Result;
}
