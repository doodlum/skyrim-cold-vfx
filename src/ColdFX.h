#pragma once

#include "DataStorage.h"

static float& g_deltaTime = (*(float*)RELOCATION_ID(523660, 410199).address());

class ColdFX
{
public:
	static void InstallHooks()
	{
		Hooks::Install();
	}

	static ColdFX* GetSingleton()
	{
		static ColdFX avInterface;
		return &avInterface;
	}

	void GetSurvivalModeGameForms();

protected:
	struct Hooks
	{
		struct PlayerCharacter_Update
		{
			static void thunk(RE::PlayerCharacter* a_player, float a_delta)
			{
				func(a_player, a_delta);
				GetSingleton()->UpdateActor(a_player, g_deltaTime);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct Character_Update
		{
			static void thunk(RE::Actor* a_actor, float a_delta)
			{
				func(a_actor, a_delta);
				GetSingleton()->UpdateActor(a_actor, g_deltaTime);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct MainUpdate_Nullsub
		{
			static void thunk()
			{
				func();
				GetSingleton()->Update(g_deltaTime);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		static void Install()
		{
			stl::write_thunk_jump<MainUpdate_Nullsub>(REL::RelocationID(35565, 36564).address() + REL::Relocate(0x748, 0xC26));
		}
	};

private:
	// Shared

	float ConvertClimateTimeToGameTime(std::uint8_t a_time);
	int   TemperatureMode = 0;

	// Survival Mode

	RE::BGSListForm* Survival_AshWeather;
	RE::BGSListForm* Survival_BlizzardWeather;
	RE::BGSListForm* Survival_WarmUpObjectsList;
	RE::BGSListForm* Survival_OblivionCells;
	RE::BGSListForm* Survival_OblivionLocations;
	RE::BGSListForm* Survival_OblivionAreas;
	RE::BGSListForm* Survival_ColdInteriorCells;
	RE::BGSListForm* Survival_ColdInteriorLocations;
	RE::BGSListForm* Survival_InteriorAreas;

	RE::TESCondition* inWarmArea;
	RE::TESCondition* inCoolArea;
	RE::TESCondition* inFreezingArea;
	RE::TESCondition* inSouthForestMountainsFreezingArea;
	RE::TESCondition* inFallForestMountainsFreezingArea;

	float coldLevelCoolArea = 3;
	float coldLevelFreezingAreaNightMod = 4;
	float coldLevelWarmAreaNightMod = 1;
	float coldLevelBlizzardMod = 10;
	float coldLevelSnowMod = 6;
	float coldLevelRainMod = 3;
	float coldLevelCoolAreaNightMod = 2;
	float coldLevelWarmArea = 0;
	float coldLevelFreezingArea = 6;

	float const heatSourceMax = 5;
	float const coldSourceMax = 5;

	enum class AREA_TYPE
	{
		kAreaTypeChillyInterior = -1,
		kAreaTypeCool = 2,
		kAreaTypeInterior = 0,
		kAreaTypeFreezing = 3,
		kAreaTypeWarm = 1
	};

	AREA_TYPE GetSurvivalModeAreaType();
	float     GetSurvivalModeWeatherColdLevel(RE::TESWeather* a_weather);
	float     GetSurvivalModeColdLevel();

	// System

	float coldLevel = 0.0f;

	void UpdateLocalTemperature(RE::Actor* a_actor, std::shared_ptr<ActorData> a_actorData);
	void UpdateActivity(RE::Actor* a_actor, std::shared_ptr<ActorData> a_actorData, float a_delta);
	void UpdateEffects();
	void UpdateEffectMaterialAlpha(RE::NiAVObject* a_object, float a_alpha);
	void UpdateActorEffect(RE::ModelReferenceEffect& a_modelEffect);
	void UpdateFirstPersonEffect(RE::ModelReferenceEffect& a_modelEffect);
	void UpdateActor(RE::Actor* a_actor, float a_delta);
	void Update(float a_delta);

	float intervalDelay = 0;
	void  ScheduleHeatSourceUpdate(float a_delta);

	void     DebugCurrentHeatGetValueBetweenTwoFixedColors(float value, uint8_t& red, uint8_t& green, uint8_t& blue);
	uint32_t DebugCreateRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	void     DebugDrawHeatSource(RE::NiPoint3 a_position, float a_innerRadius, float a_outerRadius, float a_heat, float a_heatPct);
	void     DebugDrawDamageNodes(RE::Actor* a_actor);
};
