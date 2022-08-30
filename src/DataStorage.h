#pragma once

#include <iostream>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>
#include <shared_mutex>
using json = nlohmann::json;

namespace ColdFXData
{
	struct Container
	{
		RE::BGSArtObject* Breath;
		RE::BGSArtObject* BreathFast;
		RE::BGSArtObject* BreathFrigid;
		RE::BGSArtObject* BreathTwo;
		float             TempFrequency;
	};
	class ActorData
	{
	public:
		float breathDelay = 0;
		float activityLevel = 1.0f;
		float dispersalPercent = 0.0f;
		float localTemp = 0;
		float heat = 0;
		RE::NiPoint3 heatSourcePosition;
	};
}

using namespace ColdFXData;

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

	bool debugDrawHeatSources = true;

	struct HeatSourceData
	{
		RE::NiPoint3 position;
		float        radius;
		float        heat;
	};

	std::list<HeatSourceData> heatSourceCache;

	RE::BGSArtObject* breathFirstPerson;

	std::shared_ptr<Container>                                   containerDefault;
	std::unordered_map<RE::TESRace*, std::shared_ptr<Container>> containerMap;

	std::unordered_map<RE::FormID, std::shared_ptr<ActorData>> formCache;

	std::shared_ptr<Container> GetContainer(RE::Actor* a_actor);

	void                       EraseCache();
	void                       DeleteFromCache(RE::FormID a_formID);
	void                       GarbageCollectCache();
	std::shared_ptr<ActorData> GetOrCreateFromCache(RE::Actor* a_actor);

	RE::TESRace*      GetRace(std::string a_editorID);
	RE::BGSArtObject* GetArtObject(std::string a_editorID);

	void LoadData();
	void ResetData();



	void RegisterEvents();
};
