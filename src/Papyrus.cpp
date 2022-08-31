#include "Papyrus.h"

#include "DataStorage.h"

namespace Papyrus
{
	void ColdFX_MCM::OnConfigClose(RE::TESQuest*)
	{
		DataStorage::GetSingleton()->LoadSettings();
	}

	bool ColdFX_MCM::Register(RE::BSScript::IVirtualMachine* a_vm)
	{
		a_vm->RegisterFunction("OnConfigClose", "ColdFX_MCM", OnConfigClose);

		logger::info("Registered ColdFX_MCM class");
		return true;
	}

	void Register()
	{
		auto papyrus = SKSE::GetPapyrusInterface();
		papyrus->Register(ColdFX_MCM::Register);
		logger::info("Registered papyrus functions");
	}
}
