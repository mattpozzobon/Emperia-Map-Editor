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

#ifndef RME_OUTFIT_H_
#define RME_OUTFIT_H_

enum OutfitSlot {
	OUTFIT_SLOT_HAIR = 0,
	OUTFIT_SLOT_HEAD,
	OUTFIT_SLOT_BODY,
	OUTFIT_SLOT_LEGS,
	OUTFIT_SLOT_FEET,
	OUTFIT_SLOT_LEFT_HAND,
	OUTFIT_SLOT_RIGHT_HAND,
	OUTFIT_SLOT_BACKPACK,
	OUTFIT_SLOT_BELT,
	OUTFIT_SLOT_COUNT
};

struct OutfitSlotColors {
	OutfitSlotColors() : yellow(0), red(0), green(0), blue(0), hasColors(false) {}

	int yellow;
	int red;
	int green;
	int blue;
	bool hasColors;
};

struct OutfitSpriteSlot {
	OutfitSpriteSlot() : id(0), rarity(0), level(0) {}

	int id;
	OutfitSlotColors colors;
	int rarity;
	int level;
};

struct Outfit {
	Outfit() :
		lookType(0),
		lookItem(0),
		lookMount(0),
		lookAddon(0),
		lookHead(0),
		lookBody(0),
		lookLegs(0),
		lookFeet(0),
		renderHelmet(true)
	{}
	~Outfit() {}

	int lookType;
	int lookItem;
	int lookMount;
	int lookAddon;
	int lookHead;
	int lookBody;
	int lookLegs;
	int lookFeet;
	bool renderHelmet;
	OutfitSpriteSlot sprites[OUTFIT_SLOT_COUNT];

	bool hasCompositeSprites() const {
		for(int i = 0; i < OUTFIT_SLOT_COUNT; ++i) {
			if(sprites[i].id > 0) {
				return true;
			}
		}
		return false;
	}

	Outfit getColorizedSlotOutfit(int slot) const {
		Outfit colorized = *this;
		if(slot < 0 || slot >= OUTFIT_SLOT_COUNT) {
			return colorized;
		}

		const OutfitSlotColors& colors = sprites[slot].colors;
		if(colors.hasColors) {
			colorized.lookHead = colors.yellow;
			colorized.lookBody = colors.red;
			colorized.lookLegs = colors.green;
			colorized.lookFeet = colors.blue;
		}
		return colorized;
	}

	uint32_t getColorHash() const {
		return lookHead << 24 | lookBody << 16 | lookLegs << 8 | lookFeet;
	}
};

#endif
