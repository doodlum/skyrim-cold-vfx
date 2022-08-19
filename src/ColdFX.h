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

	enum class AREA_TYPE
	{
		kAreaTypeChillyInterior = -1,
		kAreaTypeCool = 2,
		kAreaTypeInterior = 0,
		kAreaTypeFreezing = 3,
		kAreaTypeWarm = 1
	};

	AREA_TYPE GetSurvivalModeAreaType();

	float GetWeatherColdLevel(RE::TESWeather* a_weather);

	float GetSurvivalModeColdLevel();

	void UpdateLocalTemperature(RE::Actor* a_actor, std::shared_ptr<ActorData> a_actorData);
	void UpdateActivity(RE::Actor* a_actor, std::shared_ptr<ActorData> a_actorData);
	void Update(RE::Actor* a_actor, float a_delta);
	void UpdateEffects();

	void GetGameForms();

	float coldLevelCoolArea = 3;
	float coldLevelFreezingAreaNightMod = 4;
	float coldLevelWarmAreaNightMod = 1;
	float coldLevelBlizzardMod = 10;
	float coldLevelSnowMod = 6;
	float coldLevelRainMod = 3;
	float coldLevelCoolAreaNightMod = 2;
	float coldLevelWarmArea = 0;
	float coldLevelFreezingArea = 6;

	float coldLevel = 0.0f;

protected:
	struct Hooks
	{
		struct PlayerCharacter_Update
		{
			static void thunk(RE::PlayerCharacter* a_player, float a_delta)
			{
				func(a_player, a_delta);
				GetSingleton()->coldLevel = GetSingleton()->GetSurvivalModeColdLevel();
				GetSingleton()->Update(a_player, g_deltaTime);
				GetSingleton()->UpdateEffects();
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct Character_Update
		{
			static void thunk(RE::Actor* a_actor, float a_delta)
			{
				func(a_actor, a_delta);
				GetSingleton()->Update(a_actor, g_deltaTime);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		
		//struct Sky_UpdateWeather
		//{
		//	static void thunk(RE::Sky* a_sky)
		//	{
		//		func(a_sky);
		//	//	GetSingleton()->UpdateAmbientTemperature(a_sky);
		//	}
		//	static inline REL::Relocation<decltype(thunk)> func;
		//};

		static void Install()
		{
			stl::write_vfunc<RE::PlayerCharacter, 0xAD, PlayerCharacter_Update>();
			stl::write_vfunc<RE::Character, 0xAD, Character_Update>();
		//	stl::write_thunk_call<Sky_UpdateWeather>(RELOCATION_ID(25682, 26229).address() + REL::Relocate(0x29C, 0x3E6));
		}
	};
};
