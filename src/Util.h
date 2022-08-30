
RE::NiNode* GetDamageNode(RE::Actor* a_actor, int a_node);

bool  IsTurning(RE::Actor* a_actor);

float GetSubmergedLevel_Impl(RE::Actor* a_actor, float a_zPos, RE::TESObjectCELL* a_cell);
float GetSubmergedLevel(RE::Actor* a_actor);
bool  IsSubmerged(RE::Actor* a_actor);
