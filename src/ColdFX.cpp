#include "ColdFX.h"
#include "DataStorage.h"


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
		actorData->breathDelay = max(0.0f, actorData->breathDelay - g_deltaTime);
		UpdateLocalTemperature(a_actor, actorData);

		if (actorData->breathDelay == 0.0f) {
			auto container = dataStorage->GetContainer(a_actor);
			a_actor->InstantiateHitArt(container->Breath, (container->TempFrequency / 2), nullptr, false, false);
			actorData->breathDelay = container->TempFrequency / actorData->activityLevel;
		}
	}
}

void UpdateEffectMaterialAlpha(RE::NiAVObject* a_object, float a_alpha)
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
						UpdateEffectMaterialAlpha(a_modelEffect.Get3D(), std::clamp((coldLevel - actorData->localTemp) / 7.5f, 0.0f, 1.0f));
						if (actor->IsPlayerRef())
							logger::debug("alpha {}", std::clamp((coldLevel - actorData->localTemp) / 7.5f, 0.0f, 1.0f));
					}
				}
			}
		return true;
	});
}

bool IsInRegion(std::string a_region)
{
	RE::TESRegionList* regionList = ((RE::ExtraRegionList*)RE::PlayerCharacter::GetSingleton()->GetParentCell()->extraList.GetByType(RE::ExtraDataType::kRegionList))->list;
	for (const auto& region : *regionList) {
		std::string name = region->GetFormEditorID(); 
		if (name == a_region)
			return true;
	}
	return false;
}

ColdFX::AREA_TYPE ColdFX::GetSurvivalModeAreaType()
{
	auto player = RE::PlayerCharacter::GetSingleton();

	if (player->GetParentCell()->IsInteriorCell() || Survival_InteriorAreas->HasForm(player->GetWorldspace())) {
		if (Survival_ColdInteriorLocations->HasForm(player->GetCurrentLocation() || Survival_ColdInteriorCells->HasForm(player->GetParentCell()))) {
			return ColdFX::AREA_TYPE::kAreaTypeChillyInterior;
		}
		else {
			return ColdFX::AREA_TYPE::kAreaTypeInterior;
		}
	}

	if (inFreezingArea->IsTrue(player, nullptr) || inSouthForestMountainsFreezingArea->IsTrue(player, nullptr) || inFallForestMountainsFreezingArea->IsTrue(player, nullptr))
		return ColdFX::AREA_TYPE::kAreaTypeFreezing;
	else if (inWarmArea->IsTrue(player, nullptr))
		return ColdFX::AREA_TYPE::kAreaTypeWarm;

	return ColdFX::AREA_TYPE::kAreaTypeCool;
}

void ColdFX::GetGameForms() {
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

float ConvertClimateTimeToGameTime(std::uint8_t a_time)
{
	float hours = (float) ((a_time * 10) / 60);
	float minutes = (float) ((a_time * 10) % 60);
	return hours + (minutes * (100 / 60) / 100);
}

float ColdFX::GetSurvivalModeColdLevel() {
	auto areaType = GetSurvivalModeAreaType();
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

//typedef void (*tNiBound_Combine)(RE::NiBound& a_this, const RE::NiBound& a_other);
//static REL::Relocation<tNiBound_Combine> NiBound_Combine{ RELOCATION_ID(69588, 70973) };  // C73920, C9C220
//
//void TraverseMeshes(RE::NiAVObject* a_object, bool a_bStrict, std::function<void(RE::BSGeometry*)> a_func)
//{
//	if (!a_object) {
//		return;
//	}
//
//	if (a_object->GetFlags().any(RE::NiAVObject::Flag::kHidden)) {
//		return;
//	}
//
//	// Skip billboards and their children
//	if (a_object->GetRTTI() == (RE::NiRTTI*)RE::NiRTTI_NiBillboardNode.address()) {
//		return;
//	}
//
//	auto geom = a_object->AsGeometry();
//	if (geom) {
//		if (a_bStrict) {
//			if (geom->GetFlags().none(RE::NiAVObject::Flag::kRenderUse)) {
//				return;
//			}
//		}
//
//		// Skip particles
//		auto& type = geom->GetType();
//		if (type == RE::BSGeometry::Type::kParticles || type == RE::BSGeometry::Type::kStripParticles) {
//			return;
//		}
//
//		// Skip anything that does not write into zbuffer
//		const auto effect = geom->GetGeometryRuntimeData().properties[RE::BSGeometry::States::kEffect];
//		if (a_bStrict) {
//			const auto effectShader = netimmerse_cast<RE::BSEffectShaderProperty*>(effect.get());
//			if (effectShader && effectShader->flags.none(RE::BSShaderProperty::EShaderPropertyFlag::kZBufferWrite)) {
//				return;
//			}
//		}
//		const auto lightingShader = netimmerse_cast<RE::BSLightingShaderProperty*>(effect.get());
//		if (lightingShader && lightingShader->flags.none(RE::BSShaderProperty::EShaderPropertyFlag::kZBufferWrite)) {
//			return;
//		}
//
//		return a_func(geom);
//	}
//
//	auto node = a_object->AsNode();
//	if (node) {
//		for (auto& child : node->GetChildren()) {
//			TraverseMeshes(child.get(), a_bStrict, a_func);
//		}
//	}
//}
//
//RE::NiBound GetModelBounds(RE::NiAVObject* a_obj)
//{
//	RE::NiBound ret{};
//	TraverseMeshes(a_obj, true, [&](auto&& a_geometry) {
//		RE::NiBound modelBound = a_geometry->GetModelData().modelBound;
//
//		modelBound.center *= a_geometry->local.scale;
//		modelBound.radius *= a_geometry->local.scale;
//
//		modelBound.center += a_geometry->local.translate;
//
//		NiBound_Combine(ret, modelBound);
//	});
//
//	if (ret.radius == 0.f) {
//		// try less strict
//		TraverseMeshes(a_obj, false, [&](auto&& a_geometry) {
//			RE::NiBound modelBound = a_geometry->GetModelData().modelBound;
//
//			modelBound.center *= a_geometry->local.scale;
//			modelBound.radius *= a_geometry->local.scale;
//
//			modelBound.center += a_geometry->local.translate;
//
//			NiBound_Combine(ret, modelBound);
//		});
//	}
//
//	return ret;
//}

void ColdFX::UpdateLocalTemperature(RE::Actor* a_actor, std::shared_ptr<ActorData> a_actorData)
{
	if (a_actor->IsPlayerRef()) {
		float localTemperature = 0.0f;
		a_actor->GetParentCell()->ForEachReferenceInRange(a_actor->GetPosition(), 512, [&](RE::TESObjectREFR& a_ref) {
			if (Survival_WarmUpObjectsList->HasForm(a_ref.GetBaseObject())) {
				auto distance = a_ref.GetPosition().GetDistance(a_actor->GetPosition());
				auto radius = a_ref.GetBoundMin().GetDistance(a_ref.GetBoundMax()) / 2;
				localTemperature = max(localTemperature,  (25.0f / distance) * (radius / 10.0f));
			}
			return true;
		});
		a_actorData->localTemp = localTemperature;
	}
}
