#include "Character/Champions/Akali/AssassinsChampionSkillState_Akali.h"

#include "AbilitySystem/AssassinsProjectile.h"

namespace AkaliUltimate
{
	static constexpr float CapsuleRadius = 88.0f;
	static constexpr float CapsuleHalfHeight = 96.0f;
};

FAssassinsInterframeHitShape UAssassinsChampionSkillState_Akali::GetUltimateHitShape()
{
	FAssassinsInterframeHitShape Shape;
	Shape.Radius = AkaliUltimate::CapsuleRadius;
	Shape.HalfHeight = AkaliUltimate::CapsuleHalfHeight;
	return Shape;
}
