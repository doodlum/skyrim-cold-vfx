#include "ColdFX.h"
#include "DataStorage.h"
#include <TrueHUDAPI.h>

void ColdFX::UpdateActivity(RE::Actor* a_actor, std::shared_ptr<ActorData> a_actorData)
{
	float maxActivityLevel;

	if (a_actor->IsSprinting())
		maxActivityLevel = 2.5;
	else if (a_actor->IsRunning() || a_actor->IsSwimming() || a_actor->GetAttackingWeapon())
		maxActivityLevel = 1.5;
	else if (a_actor->IsWalking() || a_actor->IsSneaking())
		maxActivityLevel = 1.25;
	else
		maxActivityLevel = 0.75;

	auto changeSpeed = maxActivityLevel > a_actorData->activityLevel ? 0.5f : 0.05f;

	a_actorData->activityLevel = std::lerp(a_actorData->activityLevel, maxActivityLevel, g_deltaTime * changeSpeed);
}

void ColdFX::Update(RE::Actor* a_actor, float)
{
	if (a_actor->currentProcess->InHighProcess()) {
		auto dataStorage = DataStorage::GetSingleton();
		auto actorData = dataStorage->GetOrCreateFromCache(a_actor);

		UpdateActivity(a_actor, actorData);
		UpdateLocalTemperature(a_actor, actorData);

		actorData->breathDelay = max(0.0f, actorData->breathDelay - g_deltaTime);
		if (actorData->breathDelay == 0.0f) {
			auto container = dataStorage->GetContainer(a_actor);
			a_actor->InstantiateHitArt(container->Breath, (container->TempFrequency / 2), nullptr, false, false);
			actorData->breathDelay = container->TempFrequency / actorData->activityLevel;
		}
	}
}

void ColdFX::UpdateEffectMaterialAlpha(RE::NiAVObject* a_object, float a_alpha)
{
	using namespace RE;
	BSVisit::TraverseScenegraphGeometries(a_object, [&](BSGeometry* a_geometry) -> BSVisit::BSVisitControl {
		using State = BSGeometry::States;
		using Feature = BSShaderMaterial::Feature;

		auto effect = a_geometry->GetGeometryRuntimeData().properties[State::kEffect].get();
		if (effect) {
			auto lightingShader = netimmerse_cast<BSEffectShaderProperty*>(effect);
			if (lightingShader) {
				lightingShader->SetMaterialAlpha(a_alpha);
			}
		}

		return BSVisit::BSVisitControl::kContinue;
	});
}

//RE::NiBound FindObjectBounds(RE::NiAVObject* a_object)
//{
//	using namespace RE;
//	NiBound worldBound;
//	BSVisit::TraverseScenegraphCollision(a_object, [&](bhkNiCollisionObject* a_collisionobject) -> BSVisit::BSVisitControl {
//		auto a_geometry = (NiAVObject*)a_collisionobject;
//		if (worldBound.radius < a_geometry->worldBound.radius)
//			worldBound = a_geometry->worldBound;
//		return BSVisit::BSVisitControl::kContinue;
//	});
//	BSVisit::TraverseScenegraphObjects(a_object, [&](NiAVObject* a_geometry) -> BSVisit::BSVisitControl {
//		if (worldBound.radius < a_geometry->worldBound.radius)
//			worldBound = a_geometry->worldBound;
//		return BSVisit::BSVisitControl::kContinue;
//	});
//	return worldBound;
//}

void ColdFX::UpdateEffects()
{
	auto processList = RE::ProcessLists::GetSingleton();
	processList->GetModelEffects([&](RE::ModelReferenceEffect& a_modelEffect) {
		if (auto owner = a_modelEffect.controller->GetTargetReference())
			if (auto actor = owner->As<RE::Actor>()) {
				auto container = DataStorage::GetSingleton()->GetContainer(actor);
				if (a_modelEffect.artObject == container->Breath) {
					auto actorData = DataStorage::GetSingleton()->GetOrCreateFromCache(actor);
					if (a_modelEffect.Get3D()) {
						UpdateEffectMaterialAlpha(a_modelEffect.Get3D(), std::clamp((coldLevel * actorData->localTemp) / 5.0f, 0.0f, 1.0f));
					}
				}
			}
		return true;
	});
}

//bool IsInRegion(std::string a_region)
//{
//	RE::TESRegionList* regionList = ((RE::ExtraRegionList*)RE::PlayerCharacter::GetSingleton()->GetParentCell()->extraList.GetByType(RE::ExtraDataType::kRegionList))->list;
//	for (const auto& region : *regionList) {
//		std::string name = region->GetFormEditorID();
//		if (name == a_region)
//			return true;
//	}
//	return false;
//}

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

void ColdFX::GetGameForms()
{
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

	_SHHeatSourceSmall = RE::TESForm::LookupByEditorID("_SHHeatSourceSmall")->As<RE::BGSListForm>();
	_SHHeatSourcesNormal = RE::TESForm::LookupByEditorID("_SHHeatSourcesNormal")->As<RE::BGSListForm>();
	_SHHeatSourcesLarge = RE::TESForm::LookupByEditorID("_SHHeatSourcesLarge")->As<RE::BGSListForm>();
}

float ColdFX::GetWeatherColdLevel(RE::TESWeather* a_weather)
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

		newColdlevel += std::lerp(GetWeatherColdLevel(sky->lastWeather), GetWeatherColdLevel(sky->currentWeather), sky->currentWeatherPct);
	}
	return newColdlevel;
}

extern TRUEHUD_API::IVTrueHUD3* gTrueHUDInterface;

void DebugCurrentHeatGetValueBetweenTwoFixedColors(float value, uint8_t& red, uint8_t& green, uint8_t& blue)
{
	uint8_t aR = 55;
	uint8_t aG = 145;
	uint8_t aB = 245;  // RGB for our 1st color (blue in this case).
	uint8_t bR = 245;
	uint8_t bG = 155;
	uint8_t bB = 55;  // RGB for our 2nd color (red in this case).

	red = (uint8_t)((float)(bR - aR) * value + aR);    // Evaluated as -255*value + 255.
	green = (uint8_t)((float)(bG - aG) * value + aG);  // Evaluates as 0.
	blue = (uint8_t)((float)(bB - aB) * value + aB);   // Evaluates as 255*value + 0.
}

uint32_t DebugCreateRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return ((r & 0xff) << 24) + ((g & 0xff) << 16) + ((b & 0xff) << 8) + (a & 0xff);
}

void DebugDrawHeatSource(RE::NiPoint3 a_position, float a_innerRadius, float a_outerRadius, float a_heat)
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;

	DebugCurrentHeatGetValueBetweenTwoFixedColors(1.0f, red, green, blue);
	auto innerColor = DebugCreateRGBA(red, green, blue, 10);

	DebugCurrentHeatGetValueBetweenTwoFixedColors(1.0f - a_heat, red, green, blue);
	auto outerColor = DebugCreateRGBA(red, green, blue, 10);

	gTrueHUDInterface->DrawSphere(a_position, a_innerRadius, 4U, 0, innerColor);
	gTrueHUDInterface->DrawSphere(a_position, a_outerRadius, 4U, 0, outerColor);
}

void ColdFX::UpdateLocalTemperature(RE::Actor* a_actor, std::shared_ptr<ActorData> a_actorData)
{
	a_actorData->localTemp = 1.0f;
	auto storage = DataStorage::GetSingleton();
	storage->heatSourceListLock.lock();
	for (auto position : storage->smallHeatSourcePositionCache) {
		auto distance = position.GetDistance(a_actor->GetPosition());
		auto outerRadius = 384;
		auto innerRadius = outerRadius / 2;
		a_actorData->localTemp = min(a_actorData->localTemp, std::clamp((distance - innerRadius) / (outerRadius - innerRadius), 0.0f, 1.0f));
		if (a_actor->IsPlayerRef()) {
			DebugDrawHeatSource(position, (float)innerRadius, (float)outerRadius, std::clamp((distance - innerRadius) / (outerRadius - innerRadius), 0.0f, 1.0f));
		}
	}
	for (auto position : storage->normalHeatSourcePositionCache) {
		auto distance = position.GetDistance(a_actor->GetPosition());
		auto outerRadius = 512;
		auto innerRadius = outerRadius / 2;
		a_actorData->localTemp = min(a_actorData->localTemp, std::clamp((distance - innerRadius) / (outerRadius - innerRadius), 0.0f, 1.0f));
		if (a_actor->IsPlayerRef()) {
			DebugDrawHeatSource(position, (float)innerRadius, (float)outerRadius, std::clamp((distance - innerRadius) / (outerRadius - innerRadius), 0.0f, 1.0f));
		}
	}
	for (auto position : storage->largeHeatSourcePositionCache) {
		auto distance = position.GetDistance(a_actor->GetPosition());
		auto outerRadius = 768;
		auto innerRadius = outerRadius / 2;
		a_actorData->localTemp = min(a_actorData->localTemp, std::clamp((distance - innerRadius) / (outerRadius - innerRadius), 0.0f, 1.0f));
		if (a_actor->IsPlayerRef()) {
			DebugDrawHeatSource(position, (float)innerRadius, (float)outerRadius, std::clamp((distance - innerRadius) / (outerRadius - innerRadius), 0.0f, 1.0f));
		}
	}
	storage->heatSourceListLock.unlock();
}

void ColdFX::ScheduleHeatSourceUpdate()
{
	intervalDelay -= g_deltaTime;
	if (intervalDelay <= 0) {
		SKSE::GetTaskInterface()->AddTask([&]() {
			auto storage = DataStorage::GetSingleton();
			storage->heatSourceListLock.lock();
			storage->smallHeatSourcePositionCache.clear();
			storage->normalHeatSourcePositionCache.clear();
			storage->largeHeatSourcePositionCache.clear();
			RE::TES::GetSingleton()->ForEachReference([&](RE::TESObjectREFR& a_ref) {
				if (!a_ref.IsDisabled()) {
					if (auto baseObject = a_ref.GetBaseObject()) {
						if (_SHHeatSourceSmall->HasForm(baseObject)) {
							storage->smallHeatSourcePositionCache.insert(storage->smallHeatSourcePositionCache.end(), a_ref.GetPosition());
						} else if (_SHHeatSourcesNormal->HasForm(baseObject)) {
							storage->normalHeatSourcePositionCache.insert(storage->normalHeatSourcePositionCache.end(), a_ref.GetPosition());
						} else if (_SHHeatSourcesLarge->HasForm(baseObject)) {
							storage->largeHeatSourcePositionCache.insert(storage->largeHeatSourcePositionCache.end(), a_ref.GetPosition());
						}
					}
				}
				return true;
			});
			storage->heatSourceListLock.unlock();
		});
		intervalDelay = 1.0f;
	}
}
