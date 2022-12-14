#include "DataStorage.h"
#include "ColdFX.h"

#include <SimpleIni.h>

bool TESFormDeleteEventHandler::Register()
{
	static TESFormDeleteEventHandler singleton;
	auto                             ScriptEventSource = RE::ScriptEventSourceHolder::GetSingleton();

	if (!ScriptEventSource) {
		logger::error("Script event source not found");
		return false;
	}

	ScriptEventSource->AddEventSink(&singleton);

	logger::info("Registered {}", typeid(singleton).name());

	return true;
}

RE::BSEventNotifyControl TESFormDeleteEventHandler::ProcessEvent(const RE::TESFormDeleteEvent* a_event, RE::BSTEventSource<RE::TESFormDeleteEvent>*)
{
	DataStorage::GetSingleton()->DeleteFromCache(a_event->formID);
	return RE::BSEventNotifyControl::kContinue;
}

void DataStorage::Register()
{
	TESFormDeleteEventHandler::Register();
}

void DataStorage::LoadSettings()
{
#define GetSettingBool(a_section, a_setting) ini.GetBoolValue(a_section, a_setting, defaults.GetBoolValue(a_section, a_setting));
#define GetSettingFloat(a_section, a_setting) (float)ini.GetDoubleValue(a_section, a_setting, (float)defaults.GetDoubleValue(a_section, a_setting));

	CSimpleIniA defaults;
	defaults.SetUnicode();
	defaults.LoadFile(L"Data\\MCM\\Config\\Settings.ini");

	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(L"Data\\MCM\\Settings\\ColdFX.ini");
	Sources.iDefaultOuterRadius = (int)GetSettingFloat("Sources", "iDefaultOuterRadius");
	Sources.iInnerRadiusDivisor = (int)GetSettingFloat("Sources", "iDefaultOuterRadius");
	Sources.iHeatSourceMax = (int)GetSettingFloat("Sources", "iDefaultOuterRadius");
	Sources.iColdSourceMax = (int)GetSettingFloat("Sources", "iDefaultOuterRadius");

	Breath.fAlphaMultiplier = GetSettingFloat("Breath", "fAlphaMultiplier");
	Breath.fAlphaMax = GetSettingFloat("Breath", "fAlphaMax");

	FirstPersonBreath.fAlphaMultiplier = GetSettingFloat("FirstPersonBreath", "fAlphaMultiplier");
	FirstPersonBreath.fAlphaMax = GetSettingFloat("FirstPersonBreath", "fAlphaMax");

	Debug.bEnabled = GetSettingBool("Debug", "bEnabled");
}

void DataStorage::EraseCache()
{
	std::lock_guard<std::shared_mutex> lk(mtx);
	formCache.clear();
}

void DataStorage::DeleteFromCache(RE::FormID a_formID)
{
	std::lock_guard<std::shared_mutex> lk(mtx);
	formCache.erase(a_formID);
}

void DataStorage::GarbageCollectCache()
{
	std::lock_guard<std::shared_mutex> lk(mtx);
	auto                               processLists = RE::ProcessLists::GetSingleton();
	auto                               player = RE::PlayerCharacter::GetSingleton();
	for (auto it = formCache.cbegin(); it != formCache.cend() /* not hoisted */; /* no increment */) {
		bool       must_delete = true;
		auto       formID = it->first;
		RE::Actor* foundActor = nullptr;
		if (formID == player->formID) {
			foundActor = player;
		} else {
			for (auto& handle : processLists->highActorHandles) {
				auto actor = handle.get();
				if (actor && formID == actor->formID) {
					foundActor = actor.get();
					break;
				}
			}
		}
		if (foundActor) {
			if (!foundActor->IsGhost() && !foundActor->IsDead() && !foundActor->currentProcess->cachedValues->booleanValues.any(RE::CachedValues::BooleanValue::kOwnerIsUndead)) {
				must_delete = false;
			}
		}
		if (must_delete) {
			formCache.erase(it++);
		} else {
			++it;
		}
	}
}

std::shared_ptr<DataStorage::ActorData> DataStorage::GetOrCreateFromCache(RE::Actor* a_actor)
{
	std::lock_guard<std::shared_mutex> lk(mtx);
	if (formCache.contains(a_actor->formID)) {
		auto actorData = formCache.at(a_actor->formID);
		return actorData;
	}

	std::shared_ptr<ActorData> actordata(new ActorData);
	auto                       container = GetContainer(a_actor).get();

	actordata.get()->breathDelay = (float)(rand() / (RAND_MAX + 1.) * container->TempFrequency);
	formCache.insert({ a_actor->formID, actordata });
	return actordata;
}

void DataStorage::LoadJSON()
{
	std::ifstream i(L"Data\\SKSE\\Plugins\\ColdFX.json");
	i >> jsonData;
}

std::shared_ptr<DataStorage::Container> DataStorage::GetContainer(RE::Actor* a_actor)
{
	if (containerMap.contains(a_actor->GetRace())) {
		return containerMap[a_actor->GetRace()];
	}
	return containerDefault;
}


RE::TESRace* DataStorage::GetRace(std::string a_editorID)
{
	auto race = RE::TESForm::LookupByEditorID(a_editorID);
	if (race)
		return race->As<RE::TESRace>();
	return nullptr;
}

RE::BGSArtObject* DataStorage::GetArtObject(std::string a_editorID)
{
	auto race = RE::TESForm::LookupByEditorID(a_editorID);
	if (race)
		return race->As<RE::BGSArtObject>();
	return nullptr;
}

void DataStorage::LoadData()
{
	LoadJSON();
	for (auto& actors : jsonData["Actors"]) {
		std::shared_ptr<Container> container(new Container);
		container->Breath = actors["Breathe"] != nullptr ? GetArtObject(actors["Breathe"]) : nullptr;
		container->BreathTwo = actors["BreathTwo"] != nullptr ? GetArtObject(actors["BreathTwo"]) : nullptr;
		container->TempFrequency = actors["TempFrequency"] != nullptr ? (float)actors["TempFrequency"] : 1.0f;
		for (auto& race : actors["Races"]) {
			if (race == "Default")
				containerDefault = container;
			else
				containerMap.insert({ GetRace(race), container });
		}
	}
	breathFirstPerson = jsonData["Player"]["FirstPerson"]["Breath"] != nullptr ? GetArtObject(jsonData["Player"]["FirstPerson"]["Breath"]) : nullptr;
}

void DataStorage::ResetData()
{
	containerMap.clear();
}
