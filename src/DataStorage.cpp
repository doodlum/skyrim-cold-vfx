#include "DataStorage.h"
#include "ColdFX.h"


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
	DataStorage::GetSingleton()->FormDelete(a_event->formID);
	return RE::BSEventNotifyControl::kContinue;
}

bool TESCellAttachDetachEventHandler::Register()
{
	static TESCellAttachDetachEventHandler singleton;
	auto                             ScriptEventSource = RE::ScriptEventSourceHolder::GetSingleton();

	if (!ScriptEventSource) {
		logger::error("Script event source not found");
		return false;
	}

	ScriptEventSource->AddEventSink(&singleton);

	logger::info("Registered {}", typeid(singleton).name());

	return true;
}

RE::BSEventNotifyControl TESCellAttachDetachEventHandler::ProcessEvent(const RE::TESCellAttachDetachEvent*, RE::BSTEventSource<RE::TESCellAttachDetachEvent>*)
{
	DataStorage::GetSingleton()->GarbageCollection();
	return RE::BSEventNotifyControl::kContinue;
}

void DataStorage::FormDelete(RE::FormID a_formID)
{
	mtx.lock();
	formCache.erase(a_formID);
	mtx.unlock();
}

void DataStorage::GarbageCollection()
{
	mtx.lock();
	std::unordered_map<RE::FormID, std::shared_ptr<ActorData>> newCache;
	for (auto& cachedForm : formCache) {
		if (auto form = RE::TESForm::LookupByID(cachedForm.first))
			if (auto actor = form->As<RE::Actor>()) {
				if (actor->currentProcess && actor->currentProcess->InHighProcess() && !actor->IsGhost() && !actor->currentProcess->cachedValues->booleanValues.any(RE::CachedValues::BooleanValue::kOwnerIsUndead))
					newCache.insert(cachedForm);
			}
	}
	formCache.swap(newCache);
	mtx.unlock();
}


void DataStorage::RegisterEvents()
{
//	TESFormDeleteEventHandler::Register();
//	TESCellAttachDetachEventHandler::Register();
}

void DataStorage::LoadJSON()
{
	mtx.lock();
	std::ifstream i(L"Data\\SKSE\\Plugins\\ColdFX.json");
	i >> jsonData;
	mtx.unlock();
}

std::shared_ptr<Container> DataStorage::GetContainer(RE::Actor* a_actor)
{
	if (containerMap.contains(a_actor->GetRace())) {
		return containerMap[a_actor->GetRace()];
	}
	return containerDefault;
}

std::shared_ptr<ActorData> DataStorage::GetOrCreateFromCache(RE::Actor* a_actor)
{
	mtx.lock();
	if (formCache.contains(a_actor->formID)) {
		auto actorData = formCache.at(a_actor->formID);
		mtx.unlock();
		return actorData;
	}

	std::shared_ptr<ActorData> actordata(new ActorData);
	auto                       container = GetContainer(a_actor).get();

	actordata.get()->breathDelay = (float) (rand() / (RAND_MAX + 1.) * container->TempFrequency);
	formCache.insert({ a_actor->formID, actordata });
	mtx.unlock();
	return actordata;
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
	mtx.lock();
	for (auto& actors : jsonData["Actors"]) {
		std::shared_ptr<Container> container(new Container);
		container->Breath = actors["Breathe"] != nullptr ? GetArtObject(actors["Breathe"]) : nullptr;
		container->BreathFast = actors["BreathFast"] != nullptr ? GetArtObject(actors["BreathFast"]) : nullptr;
		container->BreathFrigid = actors["BreathFrigid"] != nullptr ? GetArtObject(actors["BreathFrigid"]) : nullptr;
		container->BreathTwo = actors["BreathTwo"] != nullptr ? GetArtObject(actors["BreathTwo"]) : nullptr;
		container->TempFrequency = actors["TempFrequency"] != nullptr ? (float) actors["TempFrequency"] : 1.0f;
		for (auto& race : actors["Races"]) {
			if (race == "Default")
				containerDefault = container;
			else
				containerMap.insert({ GetRace(race), container });
		}
	}
	breathFirstPerson = jsonData["Player"]["FirstPerson"]["Breath"] != nullptr ? GetArtObject(jsonData["Player"]["FirstPerson"]["Breath"]) : nullptr;
	mtx.unlock();
}

void DataStorage::ResetData()
{
	mtx.lock();
	containerMap.clear();
	mtx.unlock();
}

//RE::BSEventNotifyControl TESCellAttachDetachEventHandler::ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>*)
//{
//	DataStorage::GetSingleton()->ProcessCellChange(a_event->reference->As<RE::TESObjectCELL>(), a_event->attached == 1);
//	return RE::BSEventNotifyControl::kContinue;
//}
//
//void DataStorage::ProcessCellChange(RE::TESObjectCELL* a_cell, bool attached) {
//	heatSourceListLock.lock();
//	if (!attached) {
//		heatSourceList.erase(a_cell);
//	} else {
//		std::list<RE::TESObjectREFR*> list;
//		a_cell->ForEachReference([](RE::TESObjectREFR& a_ref) { 
//
//		})
//	}
//	heatSourceListLock.unlock();
//}

