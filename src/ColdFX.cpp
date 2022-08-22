#include "ColdFX.h"

#include "DataStorage.h"

#include <TrueHUDAPI.h>

extern TRUEHUD_API::IVTrueHUD3* g_TrueHUDInterface;

void ColdFX::UpdateActivity(RE::Actor* a_actor, std::shared_ptr<ActorData> a_actorData, float a_delta)
{
	float maxActivityLevel;

	if (a_actor->IsSprinting())
		maxActivityLevel = 2.5;
	else if (a_actor->IsRunning() || a_actor->IsSwimming() || a_actor->GetAttackingWeapon() || a_actor->IsFlying())
		maxActivityLevel = 1.5;
	else if (a_actor->IsWalking() || a_actor->IsSneaking())
		maxActivityLevel = 1.25;
	else
		maxActivityLevel = 0.75;

	auto changeSpeed = maxActivityLevel > a_actorData->activityLevel ? 0.25f : 0.05f;

	a_actorData->activityLevel = std::lerp(a_actorData->activityLevel, maxActivityLevel, a_delta * changeSpeed);
}

void ColdFX::DebugCurrentHeatGetValueBetweenTwoFixedColors(float value, uint8_t& red, uint8_t& green, uint8_t& blue)
{
	uint8_t aR = 55;
	uint8_t aG = 145;
	uint8_t aB = 245;
	uint8_t bR = 245;
	uint8_t bG = 155;
	uint8_t bB = 55;
	red = (uint8_t)((float)(bR - aR) * value + aR);    // Evaluated as -255*value + 255.
	green = (uint8_t)((float)(bG - aG) * value + aG);  // Evaluates as 0.
	blue = (uint8_t)((float)(bB - aB) * value + aB);   // Evaluates as 255*value + 0.
}

uint32_t ColdFX::DebugCreateRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return ((r & 0xff) << 24) + ((g & 0xff) << 16) + ((b & 0xff) << 8) + (a & 0xff);
}

void ColdFX::DebugDrawHeatSource(RE::NiPoint3 a_position, float a_innerRadius, float a_outerRadius, float a_heat)
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;

	DebugCurrentHeatGetValueBetweenTwoFixedColors(1.0f, red, green, blue);
	auto innerColor = DebugCreateRGBA(red, green, blue, 20);

	DebugCurrentHeatGetValueBetweenTwoFixedColors(1.0f - a_heat, red, green, blue);
	auto outerColor = DebugCreateRGBA(red, green, blue, 20);

	g_TrueHUDInterface->DrawSphere(a_position, a_innerRadius, 1U, 0, innerColor);
	g_TrueHUDInterface->DrawSphere(a_position, a_outerRadius, 1U, 0, outerColor);
}

void ColdFX::UpdateLocalTemperature(RE::Actor* a_actor, std::shared_ptr<ActorData> a_actorData)
{
	a_actorData->localTemp = 1.0f;
	auto storage = DataStorage::GetSingleton();
	for (auto position : storage->smallHeatSourcePositionCache) {
		auto distance = position.GetDistance(a_actor->GetPosition());
		auto outerRadius = 384;
		auto innerRadius = outerRadius / 3;
		a_actorData->localTemp = min(a_actorData->localTemp, std::clamp((distance - innerRadius) / (outerRadius - innerRadius), 0.0f, 1.0f));
		if (storage->debugDrawHeatSources && a_actor->IsPlayerRef()) {
			DebugDrawHeatSource(position, (float)innerRadius, (float)outerRadius, std::clamp((distance - innerRadius) / (outerRadius - innerRadius), 0.0f, 1.0f));
		}
	}
	for (auto position : storage->normalHeatSourcePositionCache) {
		auto distance = position.GetDistance(a_actor->GetPosition());
		auto outerRadius = 512;
		auto innerRadius = outerRadius / 3;
		a_actorData->localTemp = min(a_actorData->localTemp, std::clamp((distance - innerRadius) / (outerRadius - innerRadius), 0.0f, 1.0f));
		if (storage->debugDrawHeatSources && a_actor->IsPlayerRef()) {
			DebugDrawHeatSource(position, (float)innerRadius, (float)outerRadius, std::clamp((distance - innerRadius) / (outerRadius - innerRadius), 0.0f, 1.0f));
		}
	}
	for (auto position : storage->largeHeatSourcePositionCache) {
		auto distance = position.GetDistance(a_actor->GetPosition());
		auto outerRadius = 768;
		auto innerRadius = outerRadius / 3;
		a_actorData->localTemp = min(a_actorData->localTemp, std::clamp((distance - innerRadius) / (outerRadius - innerRadius), 0.0f, 1.0f));
		if (storage->debugDrawHeatSources && a_actor->IsPlayerRef()) {
			DebugDrawHeatSource(position, (float)innerRadius, (float)outerRadius, std::clamp((distance - innerRadius) / (outerRadius - innerRadius), 0.0f, 1.0f));
		}
	}

	for (auto& form : storage->formCache) {
		auto actorData = form.second;
		if (actorData->hasHeatSource) {
			auto distance = actorData->heatSourcePosition.GetDistance(a_actor->GetPosition());
			auto outerRadius = 256;
			auto innerRadius = outerRadius / 3;
			a_actorData->localTemp = min(a_actorData->localTemp, std::clamp((distance - innerRadius) / (outerRadius - innerRadius), 0.0f, 1.0f));
			if (storage->debugDrawHeatSources && a_actor->IsPlayerRef()) {
				DebugDrawHeatSource(actorData->heatSourcePosition, (float)innerRadius, (float)outerRadius, std::clamp((distance - innerRadius) / (outerRadius - innerRadius), 0.0f, 1.0f));
			}
		}
	}
}

 std::chrono::time_point<std::chrono::system_clock> m_StartTime = std::chrono::system_clock::now();

void ColdFX::UpdateActor(RE::Actor* a_actor, float a_delta)
{
	auto storage = DataStorage::GetSingleton();

	if (a_actor->currentProcess->InHighProcess() && !a_actor->IsGhost() && !a_actor->currentProcess->cachedValues->booleanValues.any(RE::CachedValues::BooleanValue::kOwnerIsUndead)) {
		auto actorData = storage->GetOrCreateFromCache(a_actor);
		
		actorData->hasHeatSource = false;
		if (auto leftEquipped = a_actor->GetEquippedObject(true)) {
			if (leftEquipped->As<RE::TESObjectLIGH>()) {
				actorData->heatSourcePosition = a_actor->GetPosition();
				actorData->hasHeatSource = true;
			}
		}
	
		UpdateActivity(a_actor, actorData, a_delta);
		UpdateLocalTemperature(a_actor, actorData);

		actorData->breathDelay -= a_delta;
		if (actorData->breathDelay <= 0.0f) {
			auto container = storage->GetContainer(a_actor);
			if (a_actor->IsPlayerRef() && RE::PlayerCamera::GetSingleton()->IsInFirstPerson()) {
				auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - m_StartTime).count();
				logger::debug("Time between breaths (ms: {}", milliseconds);
				a_actor->InstantiateHitArt(storage->breathFirstPerson, container->TempFrequency, nullptr, true, true);
				m_StartTime = std::chrono::system_clock::now();
			} else {
				if (a_actor->IsPlayerRef()) {
					auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - m_StartTime).count();
					logger::debug("Time between breaths (ms: {}", milliseconds);
					m_StartTime = std::chrono::system_clock::now();
				}
				a_actor->InstantiateHitArt(container->Breath, container->TempFrequency, nullptr, false, false);
				if (container->BreathTwo)
					a_actor->InstantiateHitArt(container->BreathTwo, container->TempFrequency, nullptr, false, false);
			}
			actorData->breathDelay = container->TempFrequency / actorData->activityLevel;
		}
	}
}


void ColdFX::Update(float a_delta)
 {
	if (RE::UI::GetSingleton()->GameIsPaused()) {
		return;
	}
	auto storage = DataStorage::GetSingleton();
	ScheduleHeatSourceUpdate(a_delta);
	switch (TemperatureMode) {
	case 0:
		// undefined behaviour
		break;
	case 1:
		coldLevel = GetSurvivalModeColdLevel();
		break;
	case 2:
		coldLevel = SunHelmCalculateColdLevel();
		break;
	}

	storage->GarbageCollectCache();

	auto player = RE::PlayerCharacter::GetSingleton();
	UpdateActor(player, a_delta);

	auto processLists = RE::ProcessLists::GetSingleton();
	for (auto& handle : processLists->highActorHandles) {
		if (auto actor = handle.get()) {
			UpdateActor(actor.get(), a_delta);
		}
	}
	UpdateEffects();
}

void ColdFX::UpdateEffectMaterialAlpha(RE::NiAVObject* a_object, float a_alpha)
{
	RE::BSVisit::TraverseScenegraphGeometries(a_object, [&](RE::BSGeometry* a_geometry) -> RE::BSVisit::BSVisitControl {
		if (auto effect = a_geometry->GetGeometryRuntimeData().properties[RE::BSGeometry::States::kEffect].get())
			if (auto lightingShader = netimmerse_cast<RE::BSEffectShaderProperty*>(effect))
				lightingShader->SetMaterialAlpha(a_alpha);
		return RE::BSVisit::BSVisitControl::kContinue;
	});
}

void ColdFX::UpdateActorEffect(RE::ModelReferenceEffect& a_modelEffect)
{
	if (!a_modelEffect.Get3D())
		return;
	auto storage = DataStorage::GetSingleton();
	auto actorData = storage->GetOrCreateFromCache(a_modelEffect.controller->GetTargetReference()->As<RE::Actor>());
	UpdateEffectMaterialAlpha(a_modelEffect.Get3D(), std::clamp((coldLevel * actorData->localTemp) / 10.0f, 0.0f, 1.0f));
}

void ColdFX::UpdateFirstPersonEffect(RE::ModelReferenceEffect& a_modelEffect)
{
	if (!a_modelEffect.Get3D())
		return;
	auto storage = DataStorage::GetSingleton();
	auto actorData = storage->GetOrCreateFromCache(a_modelEffect.controller->GetTargetReference()->As<RE::Actor>());
	UpdateEffectMaterialAlpha(a_modelEffect.Get3D(), std::clamp((coldLevel * actorData->localTemp) / 10.0f, 0.0f, 0.5f));
}

void ColdFX::UpdateEffects()
{
	auto processList = RE::ProcessLists::GetSingleton();
	processList->GetModelEffects([&](RE::ModelReferenceEffect& a_modelEffect) {
		if (a_modelEffect.controller) {
			if (auto owner = a_modelEffect.controller->GetTargetReference())
				if (auto actor = owner->As<RE::Actor>()) {
					if (auto storage = DataStorage::GetSingleton()) {
						auto container = storage->GetContainer(actor);
						if (auto art = a_modelEffect.artObject) {
							if (actor->IsPlayerRef()) {
								if (art == container->Breath) {
									if (RE::PlayerCamera::GetSingleton()->IsInFirstPerson()) {
										UpdateEffectMaterialAlpha(a_modelEffect.Get3D(), 0.0f);
									} else {
										UpdateActorEffect(a_modelEffect);
									}
								} else if (art == storage->breathFirstPerson) {
									if (RE::PlayerCamera::GetSingleton()->IsInFirstPerson()) {
										UpdateFirstPersonEffect(a_modelEffect);
									} else {
										UpdateEffectMaterialAlpha(a_modelEffect.Get3D(), 0.0f);
									}
								}
							} else if (art == container->Breath) {
								UpdateActorEffect(a_modelEffect);
							} else if (art == container->BreathTwo) {
								UpdateActorEffect(a_modelEffect);
							}
						}
					}
				}
		}
		return true;
	});
}

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

void ColdFX::ScheduleHeatSourceUpdate(float a_delta)
{
	intervalDelay -= a_delta;
	if (intervalDelay <= 0) {
		auto storage = DataStorage::GetSingleton();
		storage->smallHeatSourcePositionCache.clear();
		storage->normalHeatSourcePositionCache.clear();
		storage->largeHeatSourcePositionCache.clear();
		switch (TemperatureMode) {
		case 0:
			// undefined behaviour
			break;
		case 1:
			RE::TES::GetSingleton()->ForEachReference([&](RE::TESObjectREFR& a_ref) {
				if (!a_ref.IsDisabled()) {
					if (auto baseObject = a_ref.GetBaseObject()) {
						if (Survival_WarmUpObjectsList->HasForm(baseObject)) {
							storage->normalHeatSourcePositionCache.insert(storage->normalHeatSourcePositionCache.end(), a_ref.GetPosition());
						}
					}
				}
				return true;
			});
			break;
		case 2:
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
			break;
		}
		intervalDelay = 1.0f;
	}
}

void ColdFX::GetSunHelmGameForms()
{
	if (!RE::TESDataHandler::GetSingleton()->LookupLoadedModByName("SunHelmSurvival.esp"))
		return;

	TemperatureMode = 2;

	_SHColdCloudyWeather = RE::TESForm::LookupByEditorID("_SHColdCloudyWeather")->As<RE::BGSListForm>();
	_SHBlizzardWeathers = RE::TESForm::LookupByEditorID("_SHBlizzardWeathers")->As<RE::BGSListForm>();

	_SHInteriorWorldSpaces = RE::TESForm::LookupByEditorID("_SHInteriorWorldSpaces")->As<RE::BGSListForm>();
	_SHColdInteriors = RE::TESForm::LookupByEditorID("_SHColdInteriors")->As<RE::BGSListForm>();

	_SHHeatSourceSmall = RE::TESForm::LookupByEditorID("_SHHeatSourceSmall")->As<RE::BGSListForm>();
	_SHHeatSourcesNormal = RE::TESForm::LookupByEditorID("_SHHeatSourcesNormal")->As<RE::BGSListForm>();
	_SHHeatSourcesLarge = RE::TESForm::LookupByEditorID("_SHHeatSourcesLarge")->As<RE::BGSListForm>();

	RE::SpellItem* _SHRegionSpell = RE::TESForm::LookupByEditorID("_SHRegionSpell")->As<RE::SpellItem>();
	comfRegion = &_SHRegionSpell->effects[0]->conditions;
	freezingRegion = &_SHRegionSpell->effects[1]->conditions;
	coolRegion = &_SHRegionSpell->effects[2]->conditions;
	pineRegion = &_SHRegionSpell->effects[3]->conditions;
	highHrothgarRegion = &_SHRegionSpell->effects[4]->conditions;
	marshRegion = &_SHRegionSpell->effects[5]->conditions;
	volcanicRegion = &_SHRegionSpell->effects[6]->conditions;
	throatRegion = &_SHRegionSpell->effects[7]->conditions;
	reachRegion = &_SHRegionSpell->effects[8]->conditions;
}

// SunHelm will try to infer as to the current regions cold level. If there can be a blizzard here, it infers it as a freezing region. Otherwise, its cool if it can snow, and comfortable if it cannot. (Assuming weathers are tagged correctly)
ColdFX::REGION_TYPE ColdFX::SunHelmMakeUnknownRegionGuess()
{
	RE::TESWeather* snow = Sky_FindWeather(3);
	if (snow) {
		if (!strstr(snow->GetFormEditorID(), "Ash")) {
			if (_SHBlizzardWeathers->HasForm(snow)) {
				return REGION_TYPE::FREEZING;
			} else if (strcmp(RE::PlayerCharacter::GetSingleton()->GetWorldspace()->GetFormEditorID(), "BSHeartland") == 0) {
				return REGION_TYPE::FREEZING;
			} else {
				return REGION_TYPE::REACH;
			}
		}
	}
	return REGION_TYPE::COMF;
}

ColdFX::REGION_TYPE ColdFX::SunHelmGetCurrentRegion()
{
	_SHInInteriorType = -1;
	auto player = RE::PlayerCharacter::GetSingleton();
	if (player->GetParentCell()->IsInteriorCell() || _SHInteriorWorldSpaces->HasForm(player->GetWorldspace())) {
		if (_SHColdInteriors->HasForm(player->GetCurrentLocation()) || _SHColdInteriors->HasForm(player->GetParentCell())) {
			_SHInInteriorType = 1;
			return REGION_TYPE::COLDINTERIOR;
		} else {
			_SHInInteriorType = 0;
			return REGION_TYPE::WARMINTERIOR;
		}
	} else if (volcanicRegion->IsTrue(player, nullptr)) {
		return REGION_TYPE::VOLCANIC;
	} else if (marshRegion->IsTrue(player, nullptr)) {
		return REGION_TYPE::MARSH;
	} else if (throatRegion->IsTrue(player, nullptr)) {
		return REGION_TYPE::THROAT;
	} else if (pineRegion->IsTrue(player, nullptr)) {
		return REGION_TYPE::PINE;
	} else if (comfRegion->IsTrue(player, nullptr)) {
		return REGION_TYPE::COMF;
	} else if (freezingRegion->IsTrue(player, nullptr) || highHrothgarRegion->IsTrue(player, nullptr)) {
		return REGION_TYPE::FREEZING;
	} else if (reachRegion->IsTrue(player, nullptr)) {
		return REGION_TYPE::REACH;
	} else if (coolRegion->IsTrue(player, nullptr)) {
		return REGION_TYPE::COOL;
	}
	return SunHelmMakeUnknownRegionGuess();
}

float ColdFX::SunHelmCalculateRegionTemp()
{
	switch (SunHelmGetCurrentRegion()) {
	case REGION_TYPE::VOLCANIC:
		return _SHVolcanicTemp;
		break;
	case REGION_TYPE::MARSH:
		return _SHMarshTemp;
		break;
	case REGION_TYPE::THROAT:
		return _SHComfTemp;
		break;
	case REGION_TYPE::PINE:
		return _SHFreezingTemp;
		break;
	case REGION_TYPE::COMF:
		return _SHComfTemp;
		break;
	case REGION_TYPE::FREEZING:
		return _SHFreezingTemp;
		break;
	case REGION_TYPE::REACH:
		return _SHReachTemp;
		break;
	case REGION_TYPE::COOL:
		return _SHCoolTemp;
		break;
	case REGION_TYPE::WARMINTERIOR:
		return _SHWarmTemp;
		break;
	case REGION_TYPE::COLDINTERIOR:
		return _SHFreezingTemp;
		break;
	}
	return _SHCoolTemp;
}

float ColdFX::SunHelmCalculateNightPenalty(float a_regionTemp)
{
	auto  player = RE::PlayerCharacter::GetSingleton();
	float pen = 0;
	if (!player->GetParentCell()->IsInteriorCell()) {
		auto sky = RE::Sky::GetSingleton();
		auto climate = sky->currentClimate;
		auto midSunrise = std::lerp(ConvertClimateTimeToGameTime(climate->timing.sunrise.begin), ConvertClimateTimeToGameTime(climate->timing.sunrise.end), 0.5f);
		auto midSunset = std::lerp(ConvertClimateTimeToGameTime(climate->timing.sunset.begin), ConvertClimateTimeToGameTime(climate->timing.sunset.end), 0.5f);
		auto currentHour = RE::Calendar::GetSingleton()->GetHour();
		if (currentHour < midSunrise || currentHour > midSunset) {
			if (a_regionTemp >= 250) {
				return _SHFreezingNightPen;
			} else if (a_regionTemp >= 100) {
				return _SHCoolNightPen;
			} else {
				return _SHWarmNightPen;
			}
		}
	}
	return pen;
}

float ColdFX::SunHelmCalculateWeatherTemp(RE::TESWeather* a_weather)
{
	float weatherTemp = ClearWeatherPen;
	if (a_weather && !RE::PlayerCharacter::GetSingleton()->GetParentCell()->IsInteriorCell()) {
		if (_SHColdCloudyWeather->HasForm(a_weather)) {
			weatherTemp = CloudySnowPen;
		} else {
			if (a_weather->data.flags.any(RE::TESWeather::WeatherDataFlag::kRainy)) {
				weatherTemp = RainWeatherPen;
			} else if (a_weather->data.flags.any(RE::TESWeather::WeatherDataFlag::kSnow) && !strstr(a_weather->GetFormEditorID(), "Ash")) {
				if (_SHBlizzardWeathers->HasForm(a_weather)) {
					weatherTemp = BlizzardWeatherPen;
				} else {
					weatherTemp = SnowWeatherPen;
				}
			}
		}
	}
	return weatherTemp;
}

float ColdFX::SunHelmCalculateColdLevel()
{
	float tempTotal = 0;
	tempTotal += SunHelmCalculateRegionTemp();
	tempTotal += SunHelmCalculateNightPenalty(tempTotal);
	auto sky = RE::Sky::GetSingleton();
	tempTotal += std::lerp(SunHelmCalculateWeatherTemp(sky->lastWeather), SunHelmCalculateWeatherTemp(sky->currentWeather), sky->currentWeatherPct);
	return min(tempTotal, _SHColdLevelCap) * (coldLevelBlizzardMod / _SHFreezingTemp);
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
