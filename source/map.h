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

#ifndef RME_MAP_H_
#define RME_MAP_H_

#include "basemap.h"
#include "tile.h"
#include "town.h"
#include "house.h"
#include "spawn.h"
#include "complexitem.h"
#include "waypoints.h"
#include "templates.h"

#include <vector>
#include <string>
#include <set>
#include <map>

struct ZoneResourceDef {
	// The profession catalog's resource node item id, serialized as text in a
	// zone spawn table so it remains stable across Data Editor updates.
	std::string id;
	std::string name;      // Result item name, e.g. "copper ore"
	std::string type;      // skinning, mining, herbalism, fishing, chopping
	std::string groupId;   // profession + result item id
	std::string variant;   // Resource node name, e.g. "small copper vein"
	std::string size;      // small, medium, large
	uint32_t itemId = 0;   // Resource node/corpse item id
	uint32_t resultItemId = 0;
	uint32_t tier = 0;
	uint32_t sizeMultiplierBps = 10000;
	std::vector<std::string> zoneTypes; // Empty means usable in every zone type
};

struct ZoneResourceSpawnEntry {
	std::string resourceId;
	double chancePercent = 0.0;
};

struct ZoneResourceConfig {
	int maxNodes = 0;
	int minDistanceBetweenNodes = 0;
	int spawnIntervalSeconds = 0;
	std::vector<ZoneResourceSpawnEntry> spawnTable;
};

struct ZoneConfig {
	std::string name;
	// Original JSON filename. Used to remove the stale file after a rename.
	std::string sourceFileName;
	std::string displayName;
	std::string category; // city, town, forest, plains, mountain, cave, water, desert
	// The waypoint named by `name` anchors the primary painted area. Each
	// additional anchor links another connected painted area (on any floor)
	// to this same logical zone.
	std::vector<Position> additionalAreas;
	std::string difficulty;
	std::string music;
	// Each gathering profession has independent spawn rules and probabilities.
	std::map<std::string, ZoneResourceConfig> resourceTypes;
};

class Map;

uint32_t getZoneCategoryFlag(const std::string& category);
std::string getZoneCategoryFromFlags(uint32_t flags);
std::string getZoneCategoryDisplayName(const std::string& category);
std::vector<Position> getZoneAreaAnchors(const Map& map, const ZoneConfig& config);
std::set<Position> collectZoneTiles(const Map& map, const ZoneConfig& config);
bool zoneContainsPosition(const Map& map, const ZoneConfig& config, const Position& position);

class Map : public BaseMap
{
public:
	// ctor and dtor
	Map();
	virtual ~Map();

	// Operations on the entire map
	void cleanInvalidTiles(bool showdialog = false);
	// Save a bmp image of the minimap
	bool exportMinimap(FileName filename, int floor = rme::MapGroundLayer, bool showdialog = false);
	//
	bool convert(MapVersion to, bool showdialog = false);
	bool convert(const ConversionMap& cm, bool showdialog = false);

	// Query information about the map

	MapVersion getVersion() const noexcept { return mapVersion; }
	// Returns true if any change has been done since last save
	bool hasChanged() const noexcept { return has_changed; }
	// Makes a change, doesn't matter what. Just so that it asks when saving (Also adds a * to the window title)
	bool doChange();
	// Clears any changes
	bool clearChanges();

	// Errors/warnings
	bool hasWarnings() const { return !warnings.empty(); }
	const wxArrayString& getWarnings() const noexcept { return warnings; }
	bool hasError() const { return !error.empty(); }
	const wxString& getError() const noexcept { return error; }

	// Mess with spawns
	bool addSpawn(Tile* spawn);
	void removeSpawn(Tile* tile);
	void removeSpawn(const Position& position) { removeSpawn(getTile(position)); }

	// Returns all possible spawns on the target tile
	SpawnList getSpawnList(const Tile* tile) const;
	SpawnList getSpawnList(const Position& position) const;
	SpawnList getSpawnList(int x, int y, int z) const;

	// Returns true if the map has been saved
	// ie. it knows which file it should be saved to
	bool hasFile() const noexcept { return !filename.empty(); }
	const std::string& getFilename() const noexcept { return filename; }
	const std::string& getName() const noexcept { return name; }
	void setName(const std::string& _name) noexcept { name = _name; }

	// Get map data
	int getWidth() const noexcept { return width; }
	int getHeight() const noexcept { return height; }
	const std::string& getMapDescription() const noexcept { return description; }
	const std::string& getHouseFilename() const noexcept { return housefile; }
	const std::string& getSpawnFilename() const noexcept { return spawnfile; }

	// Set some map data
	void setWidth(int new_width);
	void setHeight(int new_height);
	void setMapDescription(const std::string& new_description);
	void setHouseFilename(const std::string& new_housefile);
	void setSpawnFilename(const std::string& new_spawnfile);

	void flagAsNamed() noexcept { unnamed = false; }

	bool hasUniqueId(uint16_t uid) const;
	bool ensureZoneForWaypoint(const Waypoint& waypoint);
	bool hasRewardId(uint32_t rewardId, const Item* except = nullptr);
	uint32_t allocateRewardId();
	bool validateRewardIds(std::string& validationError);

protected:
	// Loads a map
	bool open(const std::string identifier);

protected:
	void removeSpawnInternal(Tile* tile);

	wxArrayString warnings;
	wxString error;

	std::string name; // The map name, NOT the same as filename
	std::string filename; // the maps filename
	std::string description; // The description of the map

	MapVersion mapVersion;

	// Map Width and Height - for info purposes
	uint16_t width, height;

	std::string spawnfile; // The maps spawnfile
	std::string housefile; // The housefile
	uint32_t rewardIdSequence; // Monotonic identity allocator for reward containers

public:
	Towns towns;
	Houses houses;
	Spawns spawns;
	std::vector<ZoneConfig> zoneConfigs;
	std::vector<ZoneResourceDef> zoneResourceDefs;

protected:
	void updateUniqueIds(Tile* old_tile, Tile* new_tile) override;
	void addUniqueId(uint16_t uid);
	void removeUniqueId(uint16_t uid);

	bool has_changed; // If the map has changed
	bool unnamed; // If the map has yet to receive a name

	friend class IOMapOTBM;
	friend class IOMapOTMM;
	friend class Editor;

public:
	Waypoints waypoints;

private:
	std::vector<uint16_t> uniqueIds;
};

template <typename ForeachType>
inline void foreach_ItemOnMap(Map& map, ForeachType& foreach, bool selectedTiles)
{
	MapIterator tileiter = map.begin();
	MapIterator end = map.end();
	long long done = 0;

	while(tileiter != end) {
		++done;
		Tile* tile = (*tileiter)->get();
		if(selectedTiles && !tile->isSelected()) {
			++tileiter;
			continue;
		}

		if(tile->ground) {
			foreach(map, tile, tile->ground, done);
		}

		std::queue<Container*> containers;
		for(ItemVector::iterator itemiter = tile->items.begin(); itemiter != tile->items.end(); ++itemiter) {
			Item* item = *itemiter;
			Container* container = dynamic_cast<Container*>(item);
			foreach(map, tile, item, done);
			if(container) {
				containers.push(container);

				do {
					container = containers.front();
					ItemVector& v = container->getVector();
					for(ItemVector::iterator containeriter = v.begin(); containeriter != v.end(); ++containeriter) {
						Item* i = *containeriter;
						Container* c = dynamic_cast<Container*>(i);
						foreach(map, tile, i, done);
						if(c) {
							containers.push(c);
						}
					}
					containers.pop();
				} while(containers.size());
			}
		}
		++tileiter;
	}
}

template <typename ForeachType>
inline void foreach_TileOnMap(Map& map, ForeachType& foreach)
{
	MapIterator tileiter = map.begin();
	MapIterator end = map.end();
	long long done = 0;

	while(tileiter != end)
		foreach(map, (*tileiter++)->get(), ++done);
}

template <typename RemoveIfType>
inline long long remove_if_TileOnMap(Map& map, RemoveIfType& remove_if)
{
	MapIterator tileiter = map.begin();
	MapIterator end = map.end();
	long long done = 0;
	long long removed = 0;
	long long total = map.getTileCount();

	while(tileiter != end) {
		Tile* tile = (*tileiter)->get();
		if(remove_if(map, tile, removed, done, total)) {
			map.setTile(tile->getPosition(), nullptr, true);
			++removed;
		}
		++tileiter;
		++done;
	}

	return removed;
}

template <typename RemoveIfType>
inline int64_t RemoveItemOnMap(Map& map, RemoveIfType& condition, bool selectedOnly) {
	int64_t done = 0;
	int64_t removed = 0;

	MapIterator it = map.begin();
	MapIterator end = map.end();

	while(it != end) {
		++done;
		Tile* tile = (*it)->get();
		if(selectedOnly && !tile->isSelected()) {
			++it;
			continue;
		}

		if(tile->ground) {
			if(condition(map, tile->ground, removed, done)) {
				delete tile->ground;
				tile->ground = nullptr;
				++removed;
			}
		}

		for(auto iit = tile->items.begin(); iit != tile->items.end();) {
			Item* item = *iit;
			if(condition(map, item, removed, done)) {
				iit = tile->items.erase(iit);
				delete item;
				++removed;
			}
			else
				++iit;
		}
		++it;
	}
	return removed;
}

#endif
