// Fill out your copyright notice in the Description page of Project Settings.


#include "AssassinsGameplayTags.h"

namespace AssassinsGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Ability_Cancelable_Click, "Ability.Cancelable.Click");

	UE_DEFINE_GAMEPLAY_TAG(InputTag_SetDestination_Click, "InputTag.SetDestination.Click");

	UE_DEFINE_GAMEPLAY_TAG(Event_Death, "Event.Death");

    UE_DEFINE_GAMEPLAY_TAG(Status_Channeling, "Status.Channeling");
	UE_DEFINE_GAMEPLAY_TAG(Status_Dashing, "Status.Dashing");
	UE_DEFINE_GAMEPLAY_TAG(Status_Death_Dying, "Status.Death.Dying");
	UE_DEFINE_GAMEPLAY_TAG(Status_Death_Dead, "Status.Death.Dead");
	UE_DEFINE_GAMEPLAY_TAG(Status_Untargetable_Invisible, "Status.Untargetable.Invisible");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_Spawned, "InitState.Spawned", "1: Actor/component has initially spawned and can be extended");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataAvailable, "InitState.DataAvailable", "2: All required data has been loaded/replicated and is ready for initialization");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataInitialized, "InitState.DataInitialized", "3: The available data has been initialized for this actor/component, but it is not ready for full gameplay");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_GameplayReady, "InitState.GameplayReady", "4: The actor/component is fully ready for active gameplay");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Game, "UI.Layer.Game", "Things like the HUD");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_GameMenu, "UI.Layer.GameMenu", "Menus specifically related to gameplay, like maybe an in game inventory UI");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Menu, "UI.Layer.Menu", "Thing like the settings screen");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Modal, "UI.Layer.Modal", "Confirmation dialogs. error dialogs");

};