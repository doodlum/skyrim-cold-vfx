#pragma once

#include <iostream>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>
#include <shared_mutex>
using json = nlohmann::json;


namespace ColdFXData
{
	class Container
	{
	public:
		RE::BGSArtObject* Breath;
		float             TempFrequency;
	};

	class HumanoidContainer : public Container
	{
	public:
		RE::BGSArtObject* BreathFast;
		RE::BGSArtObject* BreathFrigid;
	};

	class CreatureContainer : public Container
	{
	public:
		RE::BGSArtObject* BreathTwo;
	};

	class ActorData
	{
	public:
		float breathDelay = 0;
		float activityLevel = 1.0f;
		float localTemp = 0;
		RE::ModelReferenceEffect* lastKnownEffect = nullptr;
	};

}

using namespace ColdFXData;


class TESSwitchRaceCompleteEventEventHandler : public RE::BSTEventSink<RE::TESSwitchRaceCompleteEvent>
{
public:
	virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESSwitchRaceCompleteEvent* a_event, RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>* a_eventSource);
	static bool                      Register();
};

class TESFormDeleteEventHandler : public RE::BSTEventSink<RE::TESFormDeleteEvent>
{
public:
	virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESFormDeleteEvent* a_event, RE::BSTEventSource<RE::TESFormDeleteEvent>* a_eventSource);
	static bool                      Register();
};


//class TESCellAttachDetachEventHandler : public RE::BSTEventSink<RE::TESCellAttachDetachEvent>
//{
//public:
//	virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>* a_eventSource);
//	static bool                      Register();
//};


class DataStorage
{
public:

	static DataStorage* GetSingleton()
	{
		static DataStorage avInterface;
		return &avInterface;
	}

	std::shared_mutex mtx;

	json jsonData;
	void LoadJSON();

	std::shared_ptr<HumanoidContainer> humanoidDefault;
	std::unordered_map<RE::TESRace*, std::shared_ptr<HumanoidContainer>> humanoidRaceMap;
	std::unordered_map<RE::TESRace*, std::shared_ptr<CreatureContainer>> creatureRaceMap;
	std::unordered_map<RE::FormID, std::shared_ptr<ActorData>>           formCache;

	//std::shared_mutex											heatSourceListLock;
	//std::unordered_map<RE::TESObjectCELL*, std::list<RE::TESObjectREFR*>>  heatSourceList;

	std::shared_mutex      heatSourceListLock;
	std::list<RE::NiPoint3> smallHeatSourcePositionCache;
	std::list<RE::NiPoint3> normalHeatSourcePositionCache;
	std::list<RE::NiPoint3> largeHeatSourcePositionCache;

	std::shared_ptr<Container> GetContainer(RE::Actor* a_actor);
	std::shared_ptr<ActorData> GetOrCreateFromCache(RE::Actor* a_actor);

	RE::TESRace* GetRace(std::string a_editorID);
	RE::BGSArtObject* GetArtObject(std::string a_editorID);

	void LoadData();
	void ResetData();

	//void ProcessCellChange(RE::TESObjectCELL* a_cell, bool attached);

	void FormDelete(const RE::TESFormDeleteEvent* a_event);
	void RaceChange(const RE::TESSwitchRaceCompleteEvent* a_event);

	void RegisterEvents();
};
