#include "Util.h"

RE::NiNode* GetDamageNode(RE::Actor* a_actor, int a_node)
{
	if (a_actor->currentProcess && a_actor->currentProcess->middleHigh && a_actor->currentProcess->middleHigh->damageRootNode && a_node >= 0 && a_node < 6) {
		auto nodes = a_actor->currentProcess->middleHigh->damageRootNode;
		if (nodes[a_node]) {
			return nodes[a_node];
		}
	}
	return nullptr;
}

bool IsTurning(RE::Actor* a_actor)
{
	auto state = a_actor->actorState1.unk04;

	if (state != 0) {
		return true;
	}
	return false;
}

float GetSubmergedLevel_Impl(RE::Actor* a_actor, float a_zPos, RE::TESObjectCELL* a_cell)
{
	using func_t = decltype(&GetSubmergedLevel_Impl);
	REL::Relocation<func_t> func{ REL::RelocationID(36452, 37448) };  // 1.5.97 1405E1510
	return func(a_actor, a_zPos, a_cell);
}

float GetSubmergedLevel(RE::Actor* a_actor)
{
	return GetSubmergedLevel_Impl(a_actor, a_actor->GetPositionZ(), a_actor->GetParentCell());
}

bool IsSubmerged(RE::Actor* a_actor)
{
	if (auto node = GetDamageNode(a_actor, 1)) {
		auto level = GetSubmergedLevel_Impl(a_actor, node->world.translate.z, a_actor->GetParentCell()); 
		return level;
	}
	return GetSubmergedLevel(a_actor) >= 0.9f;
}
