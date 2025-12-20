// Fill out your copyright notice in the Description page of Project Settings.


#include "AssassinsGameplayTags.h"

namespace AssassinsGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack, "Ability.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Ability1, "Ability.Ability1");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Ability2, "Ability.Ability2");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Ability3, "Ability.Ability3");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Ability4, "Ability.Ability4");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Type_UnitTargeted, "Ability.Type.UnitTargeted");

    UE_DEFINE_GAMEPLAY_TAG(Ability_Interruptible_Click, "Ability.Interruptible.Click");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Cancelable_Ability1, "Ability.Cancelable.Ability1");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Cancelable_Ability2, "Ability.Cancelable.Ability2");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Cancelable_Ability3, "Ability.Cancelable.Ability3");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Cancelable_Ability4, "Ability.Cancelable.Ability4");

    UE_DEFINE_GAMEPLAY_TAG(Ability_Event_AttackHit, "Ability.Event.AttackHit");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Event_SpawnProjectile, "Ability.Event.SpawnProjectile");

	UE_DEFINE_GAMEPLAY_TAG(InputTag_SetDestination_Click, "InputTag.SetDestination.Click");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Attack, "InputTag.Attack");
    UE_DEFINE_GAMEPLAY_TAG(InputTag_Ability1, "InputTag.Ability1");
    UE_DEFINE_GAMEPLAY_TAG(InputTag_Ability2, "InputTag.Ability2");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Ability3, "InputTag.Ability3");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Ability4, "InputTag.Ability4");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Suicide, "InputTag.Suicide");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Attack, "GameplayCue.Attack");
    UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ability1, "GameplayCue.Ability1");
    UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ability2, "GameplayCue.Ability2");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ability3, "GameplayCue.Ability3");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ability4, "GameplayCue.Ability4");

    UE_DEFINE_GAMEPLAY_TAG(Status_Channeling, "Status.Channeling");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_Spawned, "InitState.Spawned", "1: Actor/component has initially spawned and can be extended");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataAvailable, "InitState.DataAvailable", "2: All required data has been loaded/replicated and is ready for initialization");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataInitialized, "InitState.DataInitialized", "3: The available data has been initialized for this actor/component, but it is not ready for full gameplay");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_GameplayReady, "InitState.GameplayReady", "4: The actor/component is fully ready for active gameplay");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Game, "UI.Layer.Game", "Things like the HUD");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_GameMenu, "UI.Layer.GameMenu", "Menus specifically related to gameplay, like maybe an in game inventory UI");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Menu, "UI.Layer.Menu", "Thing like the settings screen");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Modal, "UI.Layer.Modal", "Confirmation dialogs. error dialogs");

};