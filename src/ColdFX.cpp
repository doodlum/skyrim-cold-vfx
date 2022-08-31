#include "ColdFX.h"

#include "DataStorage.h"
#include "Util.h"


void ColdFX::UpdateActivity(RE::Actor* a_actor, std::shared_ptr<DataStorage::ActorData> a_actorData, float a_delta)
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
	a_actorData->dispersalPercent = std::lerp(a_actorData->dispersalPercent, 1.0f / (maxActivityLevel + (IsTurning(a_actor) * 2.0f)), a_delta) * !IsSubmerged(a_actor);
}

void ColdFX::UpdateLocalTemperature(RE::Actor* a_actor, std::shared_ptr<DataStorage::ActorData> a_actorData)
{
	a_actorData->localTemp = 0.0f;
	auto storage = DataStorage::GetSingleton();
	for (auto heatSourceData : storage->heatSourceCache) {
		auto  distance = heatSourceData.position.GetDistance(a_actor->GetPosition());
		auto  outerRadius = heatSourceData.radius;
		auto  innerRadius = outerRadius / storage->Sources.iInnerRadiusDivisor;
		float percentage = std::clamp((distance - innerRadius) / (outerRadius - innerRadius), 0.0f, 1.0f);
		a_actorData->localTemp += heatSourceData.heat * (1 - percentage);
		if (storage->Debug.bEnabled && a_actor->IsPlayerRef()) {
			DebugDrawHeatSource(heatSourceData.position, innerRadius, outerRadius, heatSourceData.heat, percentage);
		}
	}

	for (auto& form : storage->formCache) {
		auto actorData = form.second;
		if (actorData->heat != 0) {
			auto  distance = actorData->heatSourcePosition.GetDistance(a_actor->GetPosition());
			auto  outerRadius = actorData->heatRadius;
			auto  innerRadius = outerRadius / storage->Sources.iInnerRadiusDivisor;
			float percentage = std::clamp((distance - innerRadius) / (outerRadius - innerRadius), 0.0f, 1.0f);
			a_actorData->localTemp += actorData->heat * (1 - percentage);
			if (storage->Debug.bEnabled && a_actor->IsPlayerRef()) {
				DebugDrawHeatSource(actorData->heatSourcePosition, innerRadius, outerRadius, actorData->heat, percentage);
			}
		}
	}
	a_actorData->localTemp = min(a_actorData->localTemp, storage->Sources.iHeatSourceMax);
}

void ColdFX::UpdateActor(RE::Actor* a_actor, float a_delta)
{
	auto storage = DataStorage::GetSingleton();

	if (storage->Debug.bEnabled)
		DebugDrawDamageNodes(a_actor);

	if (a_actor->currentProcess->InHighProcess() && !a_actor->IsGhost() && !a_actor->currentProcess->cachedValues->booleanValues.any(RE::CachedValues::BooleanValue::kOwnerIsUndead)) {
		auto actorData = storage->GetOrCreateFromCache(a_actor);

		if (actorData->heat != 0) {
			if (auto node = GetDamageNode(a_actor, 0)) {
				actorData->heatSourcePosition = node->world.translate;
			} else {
				actorData->heatSourcePosition = a_actor->GetPosition();
			}
		}

		UpdateActivity(a_actor, actorData, a_delta);
		UpdateLocalTemperature(a_actor, actorData);

		actorData->breathDelay -= a_delta;
		if (actorData->breathDelay <= 0.0f) {
			auto container = storage->GetContainer(a_actor);
			if (a_actor->IsPlayerRef() && RE::PlayerCamera::GetSingleton()->IsInFirstPerson()) {
				a_actor->InstantiateHitArt(storage->breathFirstPerson, container->TempFrequency, nullptr, true, true);
			} else {
				a_actor->InstantiateHitArt(container->Breath, container->TempFrequency, nullptr, false, false);
				if (container->BreathTwo)
					a_actor->InstantiateHitArt(container->BreathTwo, container->TempFrequency, nullptr, false, false);
			}
			actorData->breathDelay = container->TempFrequency / actorData->activityLevel;
		}
	}
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
	UpdateEffectMaterialAlpha(a_modelEffect.Get3D(), std::clamp(actorData->dispersalPercent * (coldLevel - actorData->localTemp) * storage->Breath.fAlphaMultiplier, 0.0f, storage->Breath.fAlphaMax));
}

void ColdFX::UpdateFirstPersonEffect(RE::ModelReferenceEffect& a_modelEffect)
{
	if (!a_modelEffect.Get3D())
		return;
	auto storage = DataStorage::GetSingleton();
	auto actorData = storage->GetOrCreateFromCache(a_modelEffect.controller->GetTargetReference()->As<RE::Actor>());
	UpdateEffectMaterialAlpha(a_modelEffect.Get3D(), std::clamp(actorData->dispersalPercent * (coldLevel - actorData->localTemp) * storage->FirstPersonBreath.fAlphaMultiplier, 0.0f, storage->FirstPersonBreath.fAlphaMax));
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

void ColdFX::ScheduleHeatSourceUpdate(float a_delta)
{
	intervalDelay -= a_delta;
	if (intervalDelay <= 0) {
		auto storage = DataStorage::GetSingleton();
		storage->heatSourceCache.clear();
		switch (TemperatureMode) {
		case 0:
			// undefined behaviour
			break;
		case 1:
			RE::TES::GetSingleton()->ForEachReference([&](RE::TESObjectREFR& a_ref) {
				if (!a_ref.IsDisabled()) {
					if (auto baseObject = a_ref.GetBaseObject()) {
						if (Survival_WarmUpObjectsList->HasForm(baseObject)) {
							DataStorage::HeatSourceData heatSourceData;
							heatSourceData.position = a_ref.GetPosition();
							heatSourceData.radius = (float)storage->Sources.iDefaultOuterRadius;
							heatSourceData.heat = (float)storage->Sources.iHeatSourceMax;
							storage->heatSourceCache.insert(storage->heatSourceCache.end(), heatSourceData);
							return true;
						} else if (auto hazard = baseObject->As<RE::BGSHazard>()) {
							if (hazard->data.spell) {
								for (const auto& effect : hazard->data.spell->effects) {
									if (effect->baseEffect->data.resistVariable == RE::ActorValue::kResistFire) {
										DataStorage::HeatSourceData heatSourceData;
										heatSourceData.position = a_ref.GetPosition();
										heatSourceData.radius = 96 * hazard->data.radius;
										heatSourceData.heat = (float)storage->Sources.iHeatSourceMax;
										storage->heatSourceCache.insert(storage->heatSourceCache.end(), heatSourceData);
										break;
									}

									if (effect->baseEffect->data.resistVariable == RE::ActorValue::kResistFrost) {
										DataStorage::HeatSourceData heatSourceData;
										heatSourceData.position = a_ref.GetPosition();
										heatSourceData.radius = 96 * hazard->data.radius;
										heatSourceData.heat = (float)-storage->Sources.iColdSourceMax;
										storage->heatSourceCache.insert(storage->heatSourceCache.end(), heatSourceData);
										break;
									}
								}
							}
						}
						if (auto actor = a_ref.As<RE::Actor>()) {
							auto actorData = storage->GetOrCreateFromCache(actor);
							actorData->heat = 0;
							if (auto leftEquipped = actor->GetEquippedObject(true)) {
								if (leftEquipped->As<RE::TESObjectLIGH>()) {
									actorData->heat += storage->Sources.iHeatSourceMax;
								}
							}
							if (auto effects = actor->GetActiveEffectList()) {
								RE::EffectSetting* setting = nullptr;
								for (auto& effect : *effects) {
									setting = effect ? effect->GetBaseObject() : nullptr;
									if (setting) {
										if (setting->data.resistVariable == RE::ActorValue::kResistFire) {
											actorData->heat += storage->Sources.iHeatSourceMax;
											break;
										} else if (setting->data.resistVariable == RE::ActorValue::kResistFrost) {
											actorData->heat -= storage->Sources.iColdSourceMax;
											break;
										}
									}
								}
							}
							actorData->heat = std::clamp(actorData->heat, (float)-storage->Sources.iColdSourceMax, (float)storage->Sources.iHeatSourceMax);
							if (actorData->heat != 0) {
								actorData->heatRadius = ((float)actor->refScale / 100.0f) * actor->GetBoundMin().GetDistance(actor->GetBoundMax()) * 2;
							}
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
