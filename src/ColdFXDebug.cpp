#include "ColdFX.h"

#include "Util.h"

#include <TrueHUDAPI.h>
extern TRUEHUD_API::IVTrueHUD3* g_TrueHUDInterface;


void ColdFX::DebugCurrentHeatGetValueBetweenTwoFixedColors(float value, uint8_t& red, uint8_t& green, uint8_t& blue)
{
	uint8_t aR = 55;
	uint8_t aG = 145;
	uint8_t aB = 245;
	uint8_t bR = 245;
	uint8_t bG = 155;
	uint8_t bB = 55;
	red = (uint8_t)((float)(bR - aR) * value + aR);    // Evaluated as -255*value + 255.
	green = (uint8_t)((float)(bG - aG) * value + aG);  // Evaluates as 0.
	blue = (uint8_t)((float)(bB - aB) * value + aB);   // Evaluates as 255*value + 0.
}

uint32_t ColdFX::DebugCreateRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return ((r & 0xff) << 24) + ((g & 0xff) << 16) + ((b & 0xff) << 8) + (a & 0xff);
}

void ColdFX::DebugDrawHeatSource(RE::NiPoint3 a_position, float a_innerRadius, float a_outerRadius, float a_heat, float a_heatPct)
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;

	auto storage = DataStorage::GetSingleton();

	float range = (float)(storage->Sources.iHeatSourceMax + storage->Sources.iColdSourceMax);
	float heatBalance = (a_heat + storage->Sources.iColdSourceMax) / range;

	DebugCurrentHeatGetValueBetweenTwoFixedColors(heatBalance, red, green, blue);
	auto innerColor = DebugCreateRGBA(red, green, blue, 64);

	DebugCurrentHeatGetValueBetweenTwoFixedColors(std::lerp(heatBalance, 0.5f, a_heatPct), red, green, blue);
	auto outerColor = DebugCreateRGBA(red, green, blue, (uint8_t)std::lerp(64, 0, a_heatPct));

	g_TrueHUDInterface->DrawSphere(a_position, a_innerRadius, 1U, 0, innerColor);
	g_TrueHUDInterface->DrawSphere(a_position, a_outerRadius, 1U, 0, outerColor);
}

void ColdFX::DebugDrawDamageNodes(RE::Actor* a_actor)
{
	uint32_t colors[6] = { (uint32_t)0xfc0303ff,
		(uint32_t)0xfcdf03ff,
		(uint32_t)0x73fc03ff,
		(uint32_t)0x03fca5ff,
		(uint32_t)0x0362fcff,
		(uint32_t)0xce03fcff };
	for (int i = 0; i < 6; i++) {
		if (auto node = GetDamageNode(a_actor, i)) {
			g_TrueHUDInterface->DrawSphere(node->world.translate, 5, 1U, 0, colors[i]);
		}
	}
}
