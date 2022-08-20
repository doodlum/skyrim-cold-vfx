#include "DataStorage.h"
#include "ColdFX.h"


bool TESSwitchRaceCompleteEventEventHandler::Register()
{
	static TESSwitchRaceCompleteEventEventHandler singleton;
	auto                                          ScriptEventSource = RE::ScriptEventSourceHolder::GetSingleton();

	if (!ScriptEventSource) {
		logger::error("Script event source not found");
		return false;
	}

	ScriptEventSource->AddEventSink(&singleton);

	logger::info("Registered {}", typeid(singleton).name());

	return true;
}

RE::BSEventNotifyControl TESSwitchRaceCompleteEventEventHandler::ProcessEvent(const RE::TESSwitchRaceCompleteEvent* a_event, RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*)
{
	DataStorage::GetSingleton()->RaceChange(a_event);
	return RE::BSEventNotifyControl::kContinue;
}

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
	DataStorage::GetSingleton()->FormDelete(a_event);
	return RE::BSEventNotifyControl::kContinue;
}

void DataStorage::FormDelete(const RE::TESFormDeleteEvent* a_event)
{
	mtx.lock();
	formCache.erase(a_event->formID);
	mtx.unlock();
}

void DataStorage::RaceChange(const RE::TESSwitchRaceCompleteEvent* a_event)
{
	mtx.lock();
	formCache.erase(a_event->subject.get()->formID);
	mtx.unlock();
}

void DataStorage::RegisterEvents()
{
	TESSwitchRaceCompleteEventEventHandler::Register();
	TESFormDeleteEventHandler::Register();
	//TESCellAttachDetachEventHandler::Register();
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
	if (humanoidRaceMap.contains(a_actor->GetRace())) {
		return humanoidRaceMap[a_actor->GetRace()];
	} else if (creatureRaceMap.contains(a_actor->GetRace())) {
		return creatureRaceMap[a_actor->GetRace()];
	}
	return humanoidDefault;
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

	actordata.get()->breathDelay = (float) (rand() / (RAND_MAX + 1.) * (container->TempFrequency / 2));
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

	std::shared_ptr<Container> playerContainer(new Container);
	playerContainer->Breath = jsonData["Player"]["1stPerson"] != nullptr ? GetArtObject(jsonData["Player"]["1stPerson"]) : nullptr;
	playerContainer->TempFrequency = jsonData["Player"]["TempFrequency"];

	player1stPerson = playerContainer;

	for (auto& humanoid : jsonData["Humanoids"]) {
		std::shared_ptr<HumanoidContainer> container(new HumanoidContainer);
		container->Breath = humanoid["Breathe"] != nullptr ? GetArtObject(humanoid["Breathe"]) : nullptr;
		container->BreathFast = humanoid["BreathFast"] != nullptr ? GetArtObject(humanoid["BreathFast"]) : nullptr;
		container->BreathFrigid = humanoid["BreathFrigid"] != nullptr ? GetArtObject(humanoid["BreathFrigid"]) : nullptr;
		container->TempFrequency = humanoid["TempFrequency"];
		mtx.lock();
		for (auto& race : humanoid["Races"]) {
			if (race == "Default")
				humanoidDefault = container;
			humanoidRaceMap.insert({ GetRace(race), container });
		}
		mtx.unlock();
	}

	for (auto& creature : jsonData["Creatures"]) {
		std::shared_ptr<CreatureContainer> container(new CreatureContainer);
		container->Breath = creature["Breathe"] != nullptr ? GetArtObject(creature["Breathe"]) : nullptr;
		container->BreathTwo = creature["BreathTwo"] != nullptr ? GetArtObject(creature["BreathTwo"]) : nullptr;
		container->TempFrequency = creature["TempFrequency"];
		mtx.lock();
		for (auto& race : creature["Races"])
			creatureRaceMap.insert({ GetRace(race), container });
		mtx.unlock();
	}
}

void DataStorage::ResetData()
{
	mtx.lock();
	humanoidRaceMap.clear();
	creatureRaceMap.clear();
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
