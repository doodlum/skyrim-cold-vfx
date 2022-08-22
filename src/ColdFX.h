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
	void GetSunHelmGameForms();

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

		//struct ProcessLists_Player
		//{
		//	static INT64 thunk(RE::PlayerCharacter* a_player, float unk1, char unk2)
		//	{
		//		INT64 res = func(a_player, unk1, unk2);
		//		GetSingleton()->UpdatePlayer(a_player, g_deltaTime);
		//		return res;
		//	}
		//	static inline REL::Relocation<decltype(thunk)> func;
		//};

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
		//	stl::write_vfunc<RE::PlayerCharacter, 0xAD, PlayerCharacter_Update>();
		//	stl::write_vfunc<RE::Character, 0xAD, Character_Update>();
			//stl::write_thunk_jump<ProcessLists_Player>(RELOCATION_ID(38107, 39063).address() + REL::Relocate(0xD, 0xD));
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

	// SunHelm Survival

	RE::BGSListForm* _SHHeatSourceSmall;
	RE::BGSListForm* _SHHeatSourcesNormal;
	RE::BGSListForm* _SHHeatSourcesLarge;

	RE::BGSListForm* _SHColdCloudyWeather;
	RE::BGSListForm* _SHBlizzardWeathers;

	RE::BGSListForm* _SHInteriorWorldSpaces;
	RE::BGSListForm* _SHColdInteriors;

	RE::TESCondition* volcanicRegion;
	RE::TESCondition* marshRegion;
	RE::TESCondition* throatRegion;
	RE::TESCondition* pineRegion;
	RE::TESCondition* comfRegion;
	RE::TESCondition* freezingRegion;
	RE::TESCondition* highHrothgarRegion;
	RE::TESCondition* reachRegion;
	RE::TESCondition* coolRegion;

	float _SHFreezingTemp = 250;
	float _SHCoolTemp = 125;
	float _SHReachTemp = 100;
	float _SHComfTemp = 35;
	float _SHMarshTemp = 150;
	float _SHVolcanicTemp = 25;
	float _SHThroatFreezeTemp = 450;
	float _SHWarmTemp = 0;

	float SeasonMult[12];

	int _SHInInteriorType = -1;

	float SnowWeatherPen = 250;
	float BlizzardWeatherPen = 500;
	float CloudySnowPen = 100;
	float RainWeatherPen = 50;
	float ClearWeatherPen = 0;

	float _SHFreezingNightPen = 100;
	float _SHCoolNightPen = 50;
	float _SHWarmNightPen = 25;

	float _SHColdLevelCap = 900;



	enum class REGION_TYPE
	{
		VOLCANIC,
		MARSH,
		THROAT,
		PINE,
		COMF,
		FREEZING,
		REACH,
		COOL,
		WARMINTERIOR,
		COLDINTERIOR
	};

	static RE::TESWeather* Sky_FindWeatherImpl(RE::Sky* a_sky, uint32_t auiType)
	{
		using func_t = decltype(&Sky_FindWeatherImpl);
		REL::Relocation<func_t> func{ REL::RelocationID(25709, 26256) };
		return func(a_sky, auiType);
	}
	static RE::TESWeather* Sky_FindWeather(uint32_t auiType)
	{
		return Sky_FindWeatherImpl(RE::Sky::GetSingleton(), auiType);
	}

	REGION_TYPE SunHelmMakeUnknownRegionGuess();
	REGION_TYPE SunHelmGetCurrentRegion();
	float       SunHelmCalculateRegionTemp();
	float       SunHelmCalculateNightPenalty(float a_regionTemp);
	float       SunHelmCalculateWeatherTemp(RE::TESWeather* a_weather);
	float       SunHelmCalculateColdLevel();

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

	float    intervalDelay = 0;
	bool  SpellHasFireEffect(RE::SpellItem* a_spell);
	void  ScheduleHeatSourceUpdate(float a_delta);

	void DebugCurrentHeatGetValueBetweenTwoFixedColors(float value, uint8_t& red, uint8_t& green, uint8_t& blue);
	uint32_t DebugCreateRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	void     DebugDrawHeatSource(RE::NiPoint3 a_position, float a_innerRadius, float a_outerRadius, float a_heat);

};
