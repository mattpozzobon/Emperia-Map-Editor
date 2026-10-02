//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "item_materials.h"
#include "map.h"

#include <fstream>
#include <unordered_map>

namespace
{
	const char* MaterialGroupNames[ITEM_MATERIAL_GROUP_COUNT] = {
		"Metal", "Leather", "Cloth", "Wood",
	};

	struct MaterialVariant {
		uint8_t id = 0;
		uint32_t itemId = 0;
		int tier = 0;
		std::string name;
	};

	struct RecipeMaterialDefaults {
		std::array<uint8_t, ITEM_MATERIAL_GROUP_COUNT> ids{};
		int primaryGroup = -1;
	};

	struct MaterialCatalog {
		std::array<std::vector<MaterialVariant>, ITEM_MATERIAL_GROUP_COUNT> groups;
		std::unordered_map<uint16_t, RecipeMaterialDefaults> recipes;
	};

	std::filesystem::path dataDirectory(const Map* map)
	{
		if(!map || !map->hasFile()) return {};
		return std::filesystem::path(map->getFilename()).parent_path().parent_path();
	}

	MaterialCatalog loadCatalog(const std::filesystem::path& dataPath)
	{
		MaterialCatalog catalog;
		try {
			std::ifstream materialsInput(dataPath / "equipment" / "materials.json");
			nlohmann::json materialGroups;
			materialsInput >> materialGroups;
			if(materialGroups.is_array()) {
				for(size_t groupIndex = 0; groupIndex < std::min(materialGroups.size(), ITEM_MATERIAL_GROUP_COUNT); ++groupIndex) {
					const auto& variants = materialGroups[groupIndex]["variants"];
					if(!variants.is_array()) continue;
					for(const auto& source : variants) {
						const int id = source.value("id", 0);
						if(id <= 0 || id > 0xFF) continue;
						catalog.groups[groupIndex].push_back({
							static_cast<uint8_t>(id), source.value("itemId", 0u),
							source.value("tier", 0), source.value("name", std::string("Material")),
						});
					}
				}
			}

			std::ifstream recipesInput(dataPath / "recipes" / "equipment.json");
			nlohmann::json recipeDocument;
			recipesInput >> recipeDocument;
			if(recipeDocument.is_object() && recipeDocument.contains("recipes") && recipeDocument["recipes"].is_array()) {
				const auto& recipes = recipeDocument["recipes"];
					for(const auto& recipe : recipes) {
						const int resultItemId = recipe.value("resultItemId", 0);
						if(resultItemId <= 0 || resultItemId > 0xFFFF || !recipe.contains("materials") || !recipe["materials"].is_array()) continue;
						RecipeMaterialDefaults defaults;
						int primaryCount = -1;
						for(const auto& material : recipe["materials"]) {
							const int group = material.value("materialGroup", -1);
							if(group < 0 || group >= static_cast<int>(ITEM_MATERIAL_GROUP_COUNT)) continue;
							const auto& variants = catalog.groups[group];
							const MaterialVariant* selected = variants.empty() ? nullptr : &variants.front();
							if(material.contains("allowedItemIds") && material["allowedItemIds"].is_array()) {
								selected = nullptr;
								for(const auto& variant : variants) {
									if(std::find(material["allowedItemIds"].begin(), material["allowedItemIds"].end(), variant.itemId) != material["allowedItemIds"].end()) {
										selected = &variant;
										break;
									}
								}
							}
							if(!selected) continue;
							defaults.ids[group] = selected->id;
							const int count = material.value("count", 0);
							if(count > primaryCount) {
								primaryCount = count;
								defaults.primaryGroup = group;
							}
						}
						if(defaults.primaryGroup >= 0) catalog.recipes[static_cast<uint16_t>(resultItemId)] = defaults;
					}
			}
		} catch(const std::exception&) {
			// Keep the dialog usable when the external catalog is unavailable.
		}
		return catalog;
	}

	const MaterialCatalog& getCatalog(const Map* map)
	{
		static std::unordered_map<std::string, MaterialCatalog> catalogs;
		const std::string key = dataDirectory(map).string();
		auto found = catalogs.find(key);
		if(found == catalogs.end()) found = catalogs.emplace(key, loadCatalog(dataDirectory(map))).first;
		return found->second;
	}

	int findMaterialGroup(const MaterialCatalog& catalog, uint8_t materialId)
	{
		for(size_t group = 0; group < ITEM_MATERIAL_GROUP_COUNT; ++group) {
			for(const auto& variant : catalog.groups[group]) {
				if(variant.id == materialId) return static_cast<int>(group);
			}
		}
		return -1;
	}
}

ItemMaterialChoiceData PopulateItemMaterialChoices(
	wxChoice* choices[ITEM_MATERIAL_GROUP_COUNT], const Map* map, uint16_t itemId,
	uint8_t selectedPrimaryId, uint32_t selectedComposition)
{
	ItemMaterialChoiceData data;
	const MaterialCatalog& catalog = getCatalog(map);
	std::array<uint8_t, ITEM_MATERIAL_GROUP_COUNT> selections{};
	const auto recipe = catalog.recipes.find(itemId);
	if(recipe != catalog.recipes.end()) {
		selections = recipe->second.ids;
		data.primaryGroup = recipe->second.primaryGroup;
	}

	if(selectedPrimaryId != 0) {
		const uint8_t authored[] = {
			selectedPrimaryId, static_cast<uint8_t>(selectedComposition & 0xFF),
			static_cast<uint8_t>((selectedComposition >> 8) & 0xFF),
			static_cast<uint8_t>((selectedComposition >> 16) & 0xFF),
		};
		for(uint8_t materialId : authored) {
			if(materialId == 0) continue;
			const int group = findMaterialGroup(catalog, materialId);
			if(group >= 0) selections[group] = materialId;
		}
		data.primaryGroup = findMaterialGroup(catalog, selectedPrimaryId);
	}

	const bool hasRecipe = recipe != catalog.recipes.end();
	for(size_t group = 0; group < ITEM_MATERIAL_GROUP_COUNT; ++group) {
		wxChoice* choice = choices[group];
		choice->Clear();
		choice->Append("Not used");
		data.ids[group].push_back(0);
		for(const auto& variant : catalog.groups[group]) {
			wxString label = wxstr(variant.name);
			if(variant.tier > 0) label += " (Tier " + i2ws(variant.tier) + ")";
			choice->Append(label);
			data.ids[group].push_back(variant.id);
		}
		auto selected = std::find(data.ids[group].begin(), data.ids[group].end(), selections[group]);
		if(selected == data.ids[group].end() && selections[group] != 0) {
			choice->Append("Unknown material (ID " + i2ws(selections[group]) + ")");
			data.ids[group].push_back(selections[group]);
			selected = data.ids[group].end() - 1;
		}
		choice->SetSelection(static_cast<int>(std::distance(data.ids[group].begin(), selected)));
		choice->Enable(selections[group] != 0 || !hasRecipe);
	}
	return data;
}

void GetSelectedItemMaterials(
	wxChoice* const choices[ITEM_MATERIAL_GROUP_COUNT], const ItemMaterialChoiceData& data,
	uint8_t& primaryId, uint32_t& composition)
{
	std::array<uint8_t, ITEM_MATERIAL_GROUP_COUNT> selected{};
	for(size_t group = 0; group < ITEM_MATERIAL_GROUP_COUNT; ++group) {
		const int row = choices[group] ? choices[group]->GetSelection() : -1;
		if(row >= 0 && static_cast<size_t>(row) < data.ids[group].size()) selected[group] = data.ids[group][row];
	}
	int primaryGroup = data.primaryGroup;
	if(primaryGroup < 0 || selected[primaryGroup] == 0) {
		primaryGroup = -1;
		for(size_t group = 0; group < ITEM_MATERIAL_GROUP_COUNT; ++group) {
			if(selected[group] != 0) { primaryGroup = static_cast<int>(group); break; }
		}
	}
	primaryId = primaryGroup >= 0 ? selected[primaryGroup] : 0;
	composition = 0;
	int shift = 0;
	for(size_t group = 0; group < ITEM_MATERIAL_GROUP_COUNT; ++group) {
		if(static_cast<int>(group) == primaryGroup || selected[group] == 0) continue;
		composition |= static_cast<uint32_t>(selected[group]) << shift;
		shift += 8;
	}
}
