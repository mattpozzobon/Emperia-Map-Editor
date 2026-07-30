//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "materials.h"
#include "gui.h"
#include <string.h> // memcpy

#include "items.h"
#include "item.h"
#include "outfit.h"

ItemDatabase g_items;

static ItemTypes_t itemTypeFromIdentityCode(uint8_t identity)
{
	switch(identity) {
		case 1: return ITEM_TYPE_BED;
		case 2: return ITEM_TYPE_CONTAINER;
		case 3: return ITEM_TYPE_CORPSE;
		case 4: return ITEM_TYPE_DEPOT;
		case 5: return ITEM_TYPE_DOOR_CLOSED;
		case 6: return ITEM_TYPE_DOOR_OPEN;
		case 7: return ITEM_TYPE_FLUIDCONTAINER;
		case 8: return ITEM_TYPE_KEY;
		case 9: return ITEM_TYPE_MAGICFIELD;
		case 10: return ITEM_TYPE_MAILBOX;
		case 11: return ITEM_TYPE_RUNE;
		case 12: return ITEM_TYPE_SPLASH;
		case 13: return ITEM_TYPE_STAIR;
		case 14: return ITEM_TYPE_TELEPORT;
		case 15: return ITEM_TYPE_TRASHHOLDER;
		case 16: return ITEM_TYPE_LEVER;
		case 17: return ITEM_TYPE_CHEST;
		case 18: return ITEM_TYPE_WINDOW_CLOSED;
		case 19: return ITEM_TYPE_WALL;
		case 20: return ITEM_TYPE_READABLE;
		case 21: return ITEM_TYPE_TRAPDOOR;
		case 22: return ITEM_TYPE_TASKBOARD;
		case 23: return ITEM_TYPE_WINDOW_OPEN;
		default: return ITEM_TYPE_NONE;
	}
}

ItemType::ItemType() :
	sprite(nullptr),
	id(0),
	appearanceID(0),
	brush(nullptr),
	doodad_brush(nullptr),
	raw_brush(nullptr),
	is_metaitem(false),
	has_raw(false),
	in_other_tileset(false),
	group(ITEM_GROUP_NONE),
	type(ITEM_TYPE_NONE),
	volume(0),
	containerSize(0),
	maxTextLen(0),
	//writeOnceItemID(0),
	ground_equivalent(0),
	border_group(0),
	has_equivalent(false),
	wall_hate_me(false),
	name(""),
	description(""),
	weight(0.0f),
	attack(0),
	defense(0),
	armor(0),
	charges(0),
	harvestType(HARVEST_NONE),
	harvestResultItemId(0),
	harvestTier(0),
	harvestRequiredToolType(0),
	marketable(false),
	autoLootable(false),
	client_chargeable(false),
	extra_chargeable(false),
	ignoreLook(false),

	isHangable(false),
	hookEast(false),
	hookSouth(false),
	canReadText(false),
	canWriteText(false),
	replaceable(true),
	decays(false),
	stackable(false),
	moveable(true),
	alwaysOnBottom(false),
	pickupable(false),
	rotable(false),
	isBorder(false),
	isOptionalBorder(false),
	isWall(false),
	isBrushDoor(false),
	isOpen(false),
	isTable(false),
	isCarpet(false),

	floorChangeDown(false),
	floorChangeNorth(false),
	floorChangeSouth(false),
	floorChangeEast(false),
	floorChangeWest(false),
	floorChange(false),

	unpassable(false),
	blockPickupable(false),
	blockMissiles(false),
	blockPathfinder(false),
	hasElevation(false),

	alwaysOnTopOrder(0),
	rotateTo(0),
	border_alignment(BORDER_NONE)
{
	////
}

bool ItemType::isFloorChange() const noexcept
{
	return floorChange
		|| floorChangeDown
		|| floorChangeNorth
		|| floorChangeSouth
		|| floorChangeEast
		|| floorChangeWest;
}

ItemDatabase::ItemDatabase() :
	// Version information
	MajorVersion(0),
	MinorVersion(0),
	BuildNumber(0),

	// Count of GameSprite types
	item_count(0),
	effect_count(0),
	monster_count(0),
	distance_count(0),

	minClientID(0),
	maxClientID(0),

	maxItemId(0)
{
	////
}

ItemDatabase::~ItemDatabase()
{
	clear();
}

void ItemDatabase::clear()
{
	for(size_t i = 0; i < items.size(); i++) {
		delete items[i];
		items.set(i, nullptr);
	}
	maxItemId = 0;
	appearanceAliases.clear();
}

bool ItemDatabase::loadFromPackageJson(const FileName& datafile, wxString& error, wxArrayString& warnings)
{
	std::ifstream input(nstr(datafile.GetFullPath()));
	if(!input.is_open()) {
		error = "Could not open " + datafile.GetFullPath();
		return false;
	}

	nlohmann::json definitions;
	try {
		input >> definitions;
	} catch(const nlohmann::json::exception& exception) {
		error = "Could not parse items.json: " + wxString::FromUTF8(exception.what());
		return false;
	}

	if(!definitions.is_object()) {
		error = "items.json must contain an object keyed by public item ID.";
		return false;
	}

	const std::map<uint16_t, uint16_t>& appearances = g_gui.gfx.getItemAppearances();
	const std::map<uint16_t, uint8_t>& identities = g_gui.gfx.getItemIdentities();
	if(appearances.empty()) {
		error = "EOBJ has no public item mappings.";
		return false;
	}

	clear();
	MajorVersion = 3;
	MinorVersion = 57;
	BuildNumber = 62;

	std::set<uint16_t> coveredAppearances;
	std::set<uint16_t> emittedItemIds;

	for(const auto& definition : definitions.items()) {
		uint32_t parsedId = 0;
		try {
			parsedId = static_cast<uint32_t>(std::stoul(definition.key()));
		} catch(const std::exception&) {
			warnings.push_back("items.json: Ignored invalid item ID \"" + wxstr(definition.key()) + "\".");
			continue;
		}
		if(parsedId == 0 || parsedId > std::numeric_limits<uint16_t>::max() || !definition.value().is_object()) {
			warnings.push_back("items.json: Ignored invalid item definition \"" + wxstr(definition.key()) + "\".");
			continue;
		}

		const uint16_t itemId = static_cast<uint16_t>(parsedId);
		const nlohmann::json& entry = definition.value();
		const uint8_t group = entry.value("group", static_cast<uint8_t>(ITEM_GROUP_NONE));
		if(group == ITEM_GROUP_DEPRECATED) {
			continue;
		}

		const auto appearance = appearances.find(itemId);
		if(appearance == appearances.end()) {
			error = wxString::Format("Item ID %u has no appearance mapping in EOBJ.", itemId);
			clear();
			return false;
		}

		ItemType* item = newd ItemType();
		item->id = itemId;
		item->appearanceID = appearance->second;
		item->sprite = static_cast<GameSprite*>(g_gui.gfx.getSprite(item->appearanceID));
		item->group = group < ITEM_GROUP_LAST ? static_cast<ItemGroup_t>(group) : ITEM_GROUP_NONE;
		if(group >= ITEM_GROUP_LAST) {
			warnings.push_back(wxString::Format("items.json: Item %u has unknown group %u.", itemId, group));
		}
		if(item->group == ITEM_GROUP_CONTAINER) {
			item->type = ITEM_TYPE_CONTAINER;
		}
		const auto identity = identities.find(itemId);
		if(identity != identities.end()) {
			item->type = itemTypeFromIdentityCode(identity->second);
			item->isOpen = item->type == ITEM_TYPE_DOOR_OPEN || item->type == ITEM_TYPE_WINDOW_OPEN;
			item->isWall = item->type == ITEM_TYPE_WALL;
		}

		const uint32_t flags = entry.value("flags", 0u);
		item->unpassable = (flags & FLAG_UNPASSABLE) != 0;
		item->blockMissiles = (flags & FLAG_BLOCK_MISSILES) != 0;
		item->blockPathfinder = (flags & FLAG_BLOCK_PATHFINDER) != 0;
		item->hasElevation = (flags & FLAG_HAS_ELEVATION) != 0;
		item->pickupable = (flags & FLAG_PICKUPABLE) != 0;
		item->moveable = (flags & FLAG_MOVEABLE) != 0;
		item->stackable = (flags & FLAG_STACKABLE) != 0;
		item->floorChangeDown = (flags & FLAG_FLOORCHANGEDOWN) != 0;
		item->floorChangeNorth = (flags & FLAG_FLOORCHANGENORTH) != 0;
		item->floorChangeEast = (flags & FLAG_FLOORCHANGEEAST) != 0;
		item->floorChangeSouth = (flags & FLAG_FLOORCHANGESOUTH) != 0;
		item->floorChangeWest = (flags & FLAG_FLOORCHANGEWEST) != 0;
		item->floorChange = item->floorChangeDown || item->floorChangeNorth || item->floorChangeEast || item->floorChangeSouth || item->floorChangeWest;
		item->alwaysOnBottom = (flags & FLAG_ALWAYSONTOP) != 0;
		item->canReadText = (flags & FLAG_READABLE) != 0;
		item->rotable = (flags & FLAG_ROTABLE) != 0;
		item->isHangable = (flags & FLAG_HANGABLE) != 0;
		item->hookEast = (flags & FLAG_HOOK_EAST) != 0;
		item->hookSouth = (flags & FLAG_HOOK_SOUTH) != 0;
		item->allowDistRead = (flags & FLAG_ALLOWDISTREAD) != 0;
		item->client_chargeable = (flags & FLAG_CLIENTCHARGES) != 0;
		item->ignoreLook = (flags & FLAG_IGNORE_LOOK) != 0;
		item->alwaysOnTopOrder = entry.value("topOrder", 0);

		const auto propertiesIt = entry.find("properties");
		if(propertiesIt != entry.end() && propertiesIt->is_object()) {
			const nlohmann::json& properties = *propertiesIt;
			item->name = properties.value("1", std::string());
			item->description = properties.value("3", std::string());

			if(properties.contains("160") && properties["160"].is_number()) item->weight = properties["160"].get<float>() / 100.f;
			if(properties.contains("26") && properties["26"].is_number_integer()) item->armor = properties["26"].get<int>();
			if(properties.contains("22") && properties["22"].is_number_integer()) item->defense = properties["22"].get<int>();
			if(properties.contains("132") && properties["132"].is_number_unsigned()) item->rotateTo = properties["132"].get<uint16_t>();

			uint32_t volume = 0;
			if(properties.contains("90") && properties["90"].is_number_unsigned()) {
				volume = properties["90"].get<uint32_t>();
				item->containerSize = static_cast<uint16_t>(std::min<uint32_t>(volume, std::numeric_limits<uint16_t>::max()));
			}
			if(properties.contains("144") && properties["144"].is_array()) {
				for(const nlohmann::json& slotDefinition : properties["144"]) {
					int outfitSlot = -1;
					const auto allowedTypes = slotDefinition.find("allowedItemTypes");
					if(allowedTypes != slotDefinition.end() && allowedTypes->is_array()) {
						for(const nlohmann::json& allowedType : *allowedTypes) {
							if(!allowedType.is_number_integer()) {
								continue;
							}
							switch(allowedType.get<int>()) {
								case 10: outfitSlot = OUTFIT_SLOT_HEAD; break;
								case 11: outfitSlot = OUTFIT_SLOT_BODY; break;
								case 12: outfitSlot = OUTFIT_SLOT_LEGS; break;
								case 13: outfitSlot = OUTFIT_SLOT_FEET; break;
								default: break;
							}
							if(outfitSlot >= 0) {
								break;
							}
						}
					}
					item->mannequinOutfitSlots.push_back(outfitSlot);
				}
				volume += static_cast<uint32_t>(properties["144"].size());
			}
			item->volume = static_cast<uint16_t>(std::min<uint32_t>(volume, std::numeric_limits<uint16_t>::max()));

			if(properties.contains("150") && properties["150"].is_boolean()) item->canReadText = properties["150"].get<bool>();
			if(properties.contains("151") && properties["151"].is_boolean()) {
				item->canWriteText = properties["151"].get<bool>();
				item->canReadText = item->canReadText || item->canWriteText;
			}
			if(properties.contains("165") && properties["165"].is_number_unsigned()) {
				item->maxTextLen = properties["165"].get<uint16_t>();
				item->canReadText = item->canReadText || item->maxTextLen > 0;
			}
			item->decays = properties.contains("124");
			if(properties.contains("120") && properties["120"].is_number_unsigned()) {
				item->charges = properties["120"].get<uint32_t>();
				item->extra_chargeable = true;
			}
			if(properties.contains("270") && properties["270"].is_number_unsigned()) {
				const uint8_t value = properties["270"].get<uint8_t>();
				if(value <= HARVEST_CHOPPING) item->harvestType = static_cast<HarvestType_t>(value);
			}
			if(properties.contains("271") && properties["271"].is_number_unsigned()) item->harvestResultItemId = properties["271"].get<uint16_t>();
			if(properties.contains("274") && properties["274"].is_number_unsigned()) item->harvestTier = properties["274"].get<uint8_t>();
			if(properties.contains("276") && properties["276"].is_number_unsigned()) item->harvestRequiredToolType = properties["276"].get<uint8_t>();
			if(properties.contains("291")) {
				item->marketable = properties["291"].is_boolean()
					? properties["291"].get<bool>()
					: properties["291"].is_number_integer() && properties["291"].get<int>() != 0;
			}
			if(properties.contains("292")) {
				item->autoLootable = properties["292"].is_boolean()
					? properties["292"].get<bool>()
					: properties["292"].is_number_integer() && properties["292"].get<int>() != 0;
			}

			const uint8_t floorchange = properties.value("112", static_cast<uint8_t>(0));
			if(floorchange == 5) item->floorChangeDown = true;
			else if(floorchange == 1) item->floorChangeNorth = true;
			else if(floorchange == 3 || floorchange == 6) item->floorChangeSouth = true;
			else if(floorchange == 2 || floorchange == 7) item->floorChangeEast = true;
			else if(floorchange == 4) item->floorChangeWest = true;
			if(floorchange != 0) item->floorChange = true;
		}

		if(items[itemId]) {
			delete items[itemId];
		}
		items.set(itemId, item);
		maxItemId = std::max(maxItemId, itemId);
		coveredAppearances.insert(item->appearanceID);
		emittedItemIds.insert(itemId);

		if(item->appearanceID != itemId) {
			const auto alias = appearanceAliases.find(item->appearanceID);
			if(alias == appearanceAliases.end() || itemId < alias->second) {
				appearanceAliases[item->appearanceID] = itemId;
			}
		}
	}

	for(uint32_t appearanceId = g_gui.gfx.getItemSpriteMinID(); appearanceId <= g_gui.gfx.getItemSpriteMaxID(); ++appearanceId) {
		const uint16_t id = static_cast<uint16_t>(appearanceId);
		if(coveredAppearances.count(id) != 0 || emittedItemIds.count(id) != 0) {
			continue;
		}
		ItemType* item = newd ItemType();
		item->id = id;
		item->appearanceID = id;
		item->sprite = static_cast<GameSprite*>(g_gui.gfx.getSprite(id));
		items.set(id, item);
		maxItemId = std::max(maxItemId, id);
	}

	return true;
}

bool ItemDatabase::loadMetaItem(pugi::xml_node node)
{
	if(const pugi::xml_attribute attribute = node.attribute("id")) {
		const uint16_t id = attribute.as_ushort();
		if(id == 0 || items[id]) {
			return false;
		}

		ItemType* item = new ItemType();
		item->is_metaitem = true;
		item->id = id;
		items.set(id, item);
		return true;
	}
	return false;
}

const ItemType& ItemDatabase::getItemType(uint16_t id) const
{
	id = resolvePublicItemId(id);
	if(id == 0 || id > maxItemId)
		return dummy;

	const ItemType* type = items[id];
	if(type) return *type;

	return dummy;
}

ItemType* ItemDatabase::getRawItemType(uint16_t id)
{
	id = resolvePublicItemId(id);
	if(id == 0 || id > maxItemId)
		return nullptr;
	return items[id];
}

uint16_t ItemDatabase::resolvePublicItemId(uint16_t id) const
{
	if(id == 0) {
		return 0;
	}
	if(id <= maxItemId && items[id] != nullptr) {
		return id;
	}
	const auto alias = appearanceAliases.find(id);
	return alias != appearanceAliases.end() ? alias->second : id;
}

bool ItemDatabase::isValidID(uint16_t id) const
{
	id = resolvePublicItemId(id);
	if(id == 0 || id > maxItemId)
		return false;
	return items[id] != nullptr;
}
