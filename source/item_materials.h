//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_ITEM_MATERIALS_H_
#define RME_ITEM_MATERIALS_H_

#include <array>

class Map;
class wxChoice;

constexpr size_t ITEM_MATERIAL_GROUP_COUNT = 4;

struct ItemMaterialChoiceData {
	std::array<std::vector<uint8_t>, ITEM_MATERIAL_GROUP_COUNT> ids;
	int primaryGroup = -1;
};

ItemMaterialChoiceData PopulateItemMaterialChoices(
	wxChoice* choices[ITEM_MATERIAL_GROUP_COUNT],
	const Map* map,
	uint16_t itemId,
	uint8_t selectedPrimaryId,
	uint32_t selectedComposition);

void GetSelectedItemMaterials(
	wxChoice* const choices[ITEM_MATERIAL_GROUP_COUNT],
	const ItemMaterialChoiceData& data,
	uint8_t& primaryId,
	uint32_t& composition);

#endif
