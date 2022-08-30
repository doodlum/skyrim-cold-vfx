#include "ColdFX.h"

ColdFX::AREA_TYPE ColdFX::GetSurvivalModeAreaType()
{
	auto player = RE::PlayerCharacter::GetSingleton();

	if (player->GetParentCell()->IsInteriorCell() || Survival_InteriorAreas->HasForm(player->GetWorldspace())) {
		if (Survival_ColdInteriorLocations->HasForm(player->GetCurrentLocation() || Survival_ColdInteriorCells->HasForm(player->GetParentCell()))) {
			return ColdFX::AREA_TYPE::kAreaTypeChillyInterior;
		} else {
			return ColdFX::AREA_TYPE::kAreaTypeInterior;
		}
	}

	if (inFreezingArea->IsTrue(player, nullptr) || inSouthForestMountainsFreezingArea->IsTrue(player, nullptr) || inFallForestMountainsFreezingArea->IsTrue(player, nullptr))
		return ColdFX::AREA_TYPE::kAreaTypeFreezing;
	else if (inWarmArea->IsTrue(player, nullptr))
		return ColdFX::AREA_TYPE::kAreaTypeWarm;

	return ColdFX::AREA_TYPE::kAreaTypeCool;
}

void ColdFX::GetSurvivalModeGameForms()
{
	if (!RE::TESDataHandler::GetSingleton()->LookupLoadedLightModByName("ccQDRSSE001-SurvivalMode.esl"))
		return;

	TemperatureMode = 1;

	Survival_AshWeather = RE::TESForm::LookupByEditorID("Survival_AshWeather")->As<RE::BGSListForm>();
	Survival_BlizzardWeather = RE::TESForm::LookupByEditorID("Survival_BlizzardWeather")->As<RE::BGSListForm>();
	Survival_WarmUpObjectsList = RE::TESForm::LookupByEditorID("Survival_WarmUpObjectsList")->As<RE::BGSListForm>();
	Survival_OblivionCells = RE::TESForm::LookupByEditorID("Survival_OblivionCells")->As<RE::BGSListForm>();
	Survival_OblivionLocations = RE::TESForm::LookupByEditorID("Survival_OblivionLocations")->As<RE::BGSListForm>();
	Survival_OblivionAreas = RE::TESForm::LookupByEditorID("Survival_OblivionAreas")->As<RE::BGSListForm>();
	Survival_ColdInteriorCells = RE::TESForm::LookupByEditorID("Survival_ColdInteriorCells")->As<RE::BGSListForm>();
	Survival_ColdInteriorLocations = RE::TESForm::LookupByEditorID("Survival_ColdInteriorLocations")->As<RE::BGSListForm>();
	Survival_InteriorAreas = RE::TESForm::LookupByEditorID("Survival_InteriorAreas")->As<RE::BGSListForm>();

	RE::SpellItem* Survival_RegionInfoSpell = RE::TESForm::LookupByEditorID("Survival_RegionInfoSpell")->As<RE::SpellItem>();
	inWarmArea = &Survival_RegionInfoSpell->effects[0]->conditions;
	inCoolArea = &Survival_RegionInfoSpell->effects[1]->conditions;
	inFreezingArea = &Survival_RegionInfoSpell->effects[2]->conditions;
	inFallForestMountainsFreezingArea = &Survival_RegionInfoSpell->effects[3]->conditions;
	inSouthForestMountainsFreezingArea = &Survival_RegionInfoSpell->effects[4]->conditions;
}

float ColdFX::GetSurvivalModeWeatherColdLevel(RE::TESWeather* a_weather)
{
	if (a_weather) {
		if (a_weather->data.flags.any(RE::TESWeather::WeatherDataFlag::kSnow)) {
			if (Survival_AshWeather->HasForm(a_weather)) {
				return 0;
			} else if (Survival_BlizzardWeather->HasForm(a_weather)) {
				return coldLevelBlizzardMod;
			} else {
				return coldLevelSnowMod;
			}
		} else if (a_weather->data.flags.any(RE::TESWeather::WeatherDataFlag::kRainy)) {
			return coldLevelRainMod;
		}
	}
	return 0;
}

float ColdFX::ConvertClimateTimeToGameTime(std::uint8_t a_time)
{
	float hours = (float)((a_time * 10) / 60);
	float minutes = (float)((a_time * 10) % 60);
	return hours + (minutes * (100 / 60) / 100);
}

float ColdFX::GetSurvivalModeColdLevel()
{
	auto  areaType = GetSurvivalModeAreaType();
	float newColdlevel = 0.0f;
	switch (areaType) {
	case ColdFX::AREA_TYPE::kAreaTypeChillyInterior:
		newColdlevel += coldLevelFreezingArea;
		break;
	case ColdFX::AREA_TYPE::kAreaTypeInterior:
		newColdlevel += coldLevelWarmArea;
		break;
	case ColdFX::AREA_TYPE::kAreaTypeFreezing:
		newColdlevel += coldLevelFreezingArea;
		break;
	case ColdFX::AREA_TYPE::kAreaTypeWarm:
		newColdlevel += coldLevelWarmArea;
		break;
	case ColdFX::AREA_TYPE::kAreaTypeCool:
		newColdlevel += coldLevelCoolArea;
		break;
	}

	if (areaType != ColdFX::AREA_TYPE::kAreaTypeInterior) {
		auto sky = RE::Sky::GetSingleton();
		auto climate = sky->currentClimate;
		auto midSunrise = std::lerp(ConvertClimateTimeToGameTime(climate->timing.sunrise.begin), ConvertClimateTimeToGameTime(climate->timing.sunrise.end), 0.5f);
		auto midSunset = std::lerp(ConvertClimateTimeToGameTime(climate->timing.sunset.begin), ConvertClimateTimeToGameTime(climate->timing.sunset.end), 0.5f);
		auto currentHour = RE::Calendar::GetSingleton()->GetHour();
		if (currentHour < midSunrise || currentHour > midSunset) {
			switch (areaType) {
			case ColdFX::AREA_TYPE::kAreaTypeFreezing:
				newColdlevel += coldLevelFreezingAreaNightMod;
				break;
			case ColdFX::AREA_TYPE::kAreaTypeWarm:
				newColdlevel += coldLevelWarmAreaNightMod;
				break;
			case ColdFX::AREA_TYPE::kAreaTypeCool:
				newColdlevel += coldLevelCoolAreaNightMod;
				break;
			}
		}

		newColdlevel += std::lerp(GetSurvivalModeWeatherColdLevel(sky->lastWeather), GetSurvivalModeWeatherColdLevel(sky->currentWeather), sky->currentWeatherPct);
	}
	return newColdlevel;
}
