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

#include "settings.h"
#include "brush.h"
#include "gui.h"
#include "palette_creature.h"
#include "creature_brush.h"
#include "creatures.h"
#include "spawn_brush.h"
#include "materials.h"

namespace
{

CreatureType* GetCreatureType(Brush* brush)
{
	if(!brush) {
		return nullptr;
	}

	CreatureBrush* creatureBrush = brush->asCreature();
	if(!creatureBrush) {
		return nullptr;
	}

	return creatureBrush->getType();
}

bool IsVisibleCreatureBrush(Brush* brush)
{
	const CreatureType* creatureType = GetCreatureType(brush);
	return creatureType && !creatureType->missing;
}

wxString GetCityDisplayName(const std::string& city)
{
	if(city.empty()) {
		return "Unassigned";
	}
	std::string display = city;
	bool capitalize = true;
	for(char& character : display) {
		if(character == '-' || character == '_') {
			character = ' ';
			capitalize = true;
		} else if(capitalize) {
			character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
			capitalize = false;
		}
	}
	if(as_lower_str(display) == "gm island") {
		return "GM Island";
	}
	return wxstr(display);
}

} // namespace

// ============================================================================
// Creature palette

BEGIN_EVENT_TABLE(CreaturePalettePanel, PalettePanel)
	EVT_CHOICE(PALETTE_CREATURE_TILESET_CHOICE, CreaturePalettePanel::OnTilesetChange)

	EVT_TOGGLEBUTTON(PALETTE_CREATURE_BRUSH_BUTTON, CreaturePalettePanel::OnClickCreatureBrushButton)
	EVT_TOGGLEBUTTON(PALETTE_SPAWN_BRUSH_BUTTON, CreaturePalettePanel::OnClickSpawnBrushButton)

	EVT_SPINCTRL(PALETTE_CREATURE_SPAWN_TIME, CreaturePalettePanel::OnChangeSpawnTime)
	EVT_SPINCTRL(PALETTE_CREATURE_SPAWN_SIZE, CreaturePalettePanel::OnChangeSpawnSize)
END_EVENT_TABLE()

CreaturePalettePanel::CreaturePalettePanel(wxWindow* parent, wxWindowID id) :
	PalettePanel(parent, id),
	tileset_choice(nullptr),
	city_label(nullptr),
	city_choice(nullptr),
	creature_list(nullptr),
	creature_brush_button(nullptr),
	spawn_brush_button(nullptr),
	creature_spawntime_spin(nullptr),
	spawn_size_spin(nullptr),
	sort_column(0),
	sort_ascending(true),
	rebuilding_list(false),
	handling_event(false)
{
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	wxSizer* sidesizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Creatures");
	tileset_choice = newd wxChoice(this, PALETTE_CREATURE_TILESET_CHOICE, wxDefaultPosition, wxDefaultSize, (int)0, (const wxString*)nullptr);
	sidesizer->Add(tileset_choice, 0, wxEXPAND);
	wxBoxSizer* citySizer = newd wxBoxSizer(wxHORIZONTAL);
	city_label = newd wxStaticText(this, wxID_ANY, "City");
	city_choice = newd wxChoice(this, PALETTE_CREATURE_CITY_CHOICE);
	city_choice->Bind(wxEVT_CHOICE, &CreaturePalettePanel::OnCityChange, this);
	citySizer->Add(city_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	citySizer->Add(city_choice, 1, wxEXPAND);
	sidesizer->Add(citySizer, 0, wxEXPAND | wxTOP, 4);
	city_label->Hide();
	city_choice->Hide();

	creature_list = newd wxListCtrl(this, PALETTE_CREATURE_LISTBOX, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	creature_list->Bind(wxEVT_SIZE, &CreaturePalettePanel::OnSize, this);
	creature_list->Bind(wxEVT_LIST_ITEM_SELECTED, &CreaturePalettePanel::OnListBoxChange, this);
	creature_list->Bind(wxEVT_LIST_COL_CLICK, &CreaturePalettePanel::OnListColumnClick, this);
	creature_list->InsertColumn(0, "Name", wxLIST_FORMAT_LEFT, 110);
	creature_list->InsertColumn(1, "Title", wxLIST_FORMAT_LEFT, 110);
	creature_list->InsertColumn(2, "Level", wxLIST_FORMAT_LEFT, 55);
	creature_list->InsertColumn(3, "City", wxLIST_FORMAT_LEFT, 90);
	sidesizer->Add(creature_list, 1, wxEXPAND);
	topsizer->Add(sidesizer, 1, wxEXPAND);

	// Brush selection
	sidesizer = newd wxStaticBoxSizer(newd wxStaticBox(this, wxID_ANY, "Brushes", wxDefaultPosition, wxSize(150, 200)), wxVERTICAL);

	//sidesizer->Add(180, 1, wxEXPAND);

	wxFlexGridSizer* grid = newd wxFlexGridSizer(3, 10, 10);
	grid->AddGrowableCol(1);

	grid->Add(newd wxStaticText(this, wxID_ANY, "Spawntime"));
	creature_spawntime_spin = newd wxSpinCtrl(this, PALETTE_CREATURE_SPAWN_TIME, i2ws(g_settings.getInteger(Config::DEFAULT_SPAWNTIME)), wxDefaultPosition, wxSize(50, 20), wxSP_ARROW_KEYS, 0, 3600, g_settings.getInteger(Config::DEFAULT_SPAWNTIME));
	grid->Add(creature_spawntime_spin, 0, wxEXPAND);
	creature_brush_button = newd wxToggleButton(this, PALETTE_CREATURE_BRUSH_BUTTON, "Place Creature");
	grid->Add(creature_brush_button, 0, wxEXPAND);

	grid->Add(newd wxStaticText(this, wxID_ANY, "Spawn size"));
	spawn_size_spin = newd wxSpinCtrl(this, PALETTE_CREATURE_SPAWN_SIZE, i2ws(5), wxDefaultPosition, wxSize(50, 20), wxSP_ARROW_KEYS, 1, g_settings.getInteger(Config::MAX_SPAWN_RADIUS), g_settings.getInteger(Config::CURRENT_SPAWN_RADIUS));
	grid->Add(spawn_size_spin, 0, wxEXPAND);
	spawn_brush_button = newd wxToggleButton(this, PALETTE_SPAWN_BRUSH_BUTTON, "Place Spawn");
	grid->Add(spawn_brush_button, 0, wxEXPAND);

	sidesizer->Add(grid, 0, wxEXPAND);
	topsizer->Add(sidesizer, 0, wxEXPAND);
	SetSizerAndFit(topsizer);

	OnUpdate();
}

CreaturePalettePanel::~CreaturePalettePanel()
{
	////
}

PaletteType CreaturePalettePanel::GetType() const
{
	return TILESET_CREATURE;
}

void CreaturePalettePanel::SelectFirstBrush()
{
	SelectCreatureBrush();
}

Brush* CreaturePalettePanel::GetSelectedBrush() const
{
	if(creature_brush_button->GetValue()) {
		if(creature_list->GetItemCount() == 0) {
			return nullptr;
		}
		long selection = creature_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		Brush* brush = GetCreatureBrush(selection);
		if(brush && brush->isCreature()) {
			g_gui.SetSpawnTime(creature_spawntime_spin->GetValue());
			return brush;
		}
	} else if(spawn_brush_button->GetValue()) {
		g_settings.setInteger(Config::CURRENT_SPAWN_RADIUS, spawn_size_spin->GetValue());
		g_settings.setInteger(Config::DEFAULT_SPAWNTIME, creature_spawntime_spin->GetValue());
		return g_gui.spawn_brush;
	}
	return nullptr;
}

bool CreaturePalettePanel::SelectBrush(const Brush* whatbrush)
{
	if(!whatbrush)
		return false;

	if(whatbrush->isCreature()) {
		int current_index = tileset_choice->GetSelection();
		if(current_index != wxNOT_FOUND) {
			const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(current_index));
			// Select first house
			for(BrushVector::const_iterator iter = tsc->brushlist.begin(); iter != tsc->brushlist.end(); ++iter) {
				if(*iter == whatbrush) {
					if(std::find(creature_brushes.begin(), creature_brushes.end(), *iter) == creature_brushes.end() &&
						city_choice->IsShown() && city_choice->GetCount() > 0) {
						city_choice->SetSelection(0);
						SelectTileset(static_cast<size_t>(current_index), false);
					}
					SelectCreature(whatbrush->getName());
					return true;
				}
			}
		}
		// Not in the current display, search the hidden one's
		for(size_t i = 0; i < tileset_choice->GetCount(); ++i) {
			if(current_index != (int)i) {
				const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(i));
				for(BrushVector::const_iterator iter = tsc->brushlist.begin();
						iter != tsc->brushlist.end();
						++iter)
				{
					if(*iter == whatbrush) {
						SelectTileset(i);
						SelectCreature(whatbrush->getName());
						return true;
					}
				}
			}
		}
	} else if(whatbrush->isSpawn()) {
		SelectSpawnBrush();
		return true;
	}
	return false;
}

int CreaturePalettePanel::GetSelectedBrushSize() const
{
	return spawn_size_spin->GetValue();
}

void CreaturePalettePanel::OnUpdate()
{
	tileset_choice->Clear();
	g_materials.createOtherTileset();

	for(TilesetContainer::const_iterator iter = g_materials.tilesets.begin(); iter != g_materials.tilesets.end(); ++iter) {
		const TilesetCategory* tsc = iter->second->getCategory(TILESET_CREATURE);
		if(tsc && tsc->size() > 0) {
			tileset_choice->Append(wxstr(iter->second->name), const_cast<TilesetCategory*>(tsc));
		} else if(iter->second->name == "NPCs" || iter->second->name == "Others") {
			Tileset* ts = const_cast<Tileset*>(iter->second);
			TilesetCategory* rtsc = ts->getCategory(TILESET_CREATURE);
			tileset_choice->Append(wxstr(ts->name), rtsc);
		}
	}
	SelectTileset(0);
}

void CreaturePalettePanel::OnUpdateBrushSize(BrushShape shape, int size)
{
	return spawn_size_spin->SetValue(size);
}

void CreaturePalettePanel::OnSwitchIn()
{
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SetBrushSize(spawn_size_spin->GetValue());
}

void CreaturePalettePanel::SelectTileset(size_t index, bool rebuildCityChoices)
{
	if(index >= tileset_choice->GetCount()) {
		creature_list->DeleteAllItems();
		creature_brushes.clear();
		city_filter_values.clear();
		city_choice->Clear();
		city_label->Hide();
		city_choice->Hide();
		creature_brush_button->Enable(false);
		return;
	}

	creature_brushes.clear();
	if(tileset_choice->GetCount() == 0) {
		// No tilesets :(
		creature_brush_button->Enable(false);
	} else {
		const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(index));
		if(!tsc) {
			creature_brush_button->Enable(false);
			return;
		}
		const bool npcTileset = tileset_choice->GetString(index).CmpNoCase("NPCs") == 0;
		std::string selectedCity;
		const int oldCitySelection = city_choice->GetSelection();
		if(oldCitySelection >= 0 && oldCitySelection < static_cast<int>(city_filter_values.size())) {
			selectedCity = city_filter_values[oldCitySelection];
		}

		if(rebuildCityChoices) {
			city_choice->Clear();
			city_filter_values.clear();
			if(npcTileset) {
				std::vector<std::string> cities;
				bool hasUnassigned = false;
				for(Brush* brush : tsc->brushlist) {
					const CreatureType* type = GetCreatureType(brush);
					if(!type || type->missing) continue;
					if(type->city.empty()) {
						hasUnassigned = true;
					} else if(std::find(cities.begin(), cities.end(), type->city) == cities.end()) {
						cities.push_back(type->city);
					}
				}
				std::sort(cities.begin(), cities.end(), [](const std::string& left, const std::string& right) {
					return GetCityDisplayName(left).CmpNoCase(GetCityDisplayName(right)) < 0;
				});
				city_choice->Append("All cities");
				city_filter_values.push_back("");
				for(const std::string& city : cities) {
					city_choice->Append(GetCityDisplayName(city));
					city_filter_values.push_back(city);
				}
				if(hasUnassigned) {
					city_choice->Append("Unassigned");
					city_filter_values.push_back("__unassigned__");
				}
				int selection = 0;
				for(size_t cityIndex = 0; cityIndex < city_filter_values.size(); ++cityIndex) {
					if(as_lower_str(city_filter_values[cityIndex]) == as_lower_str(selectedCity)) {
						selection = static_cast<int>(cityIndex);
						break;
					}
				}
				city_choice->SetSelection(selection);
				selectedCity = city_filter_values[selection];
			}
		}

		city_label->Show(npcTileset);
		city_choice->Show(npcTileset);
		if(npcTileset && !rebuildCityChoices) {
			const int selection = city_choice->GetSelection();
			selectedCity = selection >= 0 && selection < static_cast<int>(city_filter_values.size()) ?
				city_filter_values[selection] : "";
		}

		for(BrushVector::const_iterator iter = tsc->brushlist.begin();
				iter != tsc->brushlist.end();
				++iter)
		{
			const CreatureType* type = GetCreatureType(*iter);
			const bool cityMatches = !npcTileset || selectedCity.empty() ||
				(selectedCity == "__unassigned__" ? type && type->city.empty() :
					type && as_lower_str(type->city) == as_lower_str(selectedCity));
			if(IsVisibleCreatureBrush(*iter) && cityMatches) {
				creature_brushes.push_back(*iter);
			}
		}
		SortCreatureBrushes();
		PopulateCreatureList();

		tileset_choice->SetSelection(index);
		Layout();
	}
}

void CreaturePalettePanel::SelectCreature(size_t index)
{
	// Save the old g_settings
	if(index >= static_cast<size_t>(creature_list->GetItemCount())) {
		SelectCreatureBrush();
		return;
	}

	creature_list->SetItemState(index, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
	creature_list->EnsureVisible(index);

	SelectCreatureBrush();
}

void CreaturePalettePanel::SelectCreature(std::string name)
{
	if(creature_list->GetItemCount() > 0) {
		for(size_t i = 0; i < creature_brushes.size(); ++i) {
			Brush* brush = creature_brushes[i];
			if(brush && brush->getName() == name) {
				creature_list->SetItemState(i, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
				creature_list->EnsureVisible(i);
				SelectCreatureBrush();
				return;
			}
		}
		creature_list->SetItemState(0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
		creature_list->EnsureVisible(0);
	}

	SelectCreatureBrush();
}

void CreaturePalettePanel::SelectCreatureBrush()
{
	if(creature_list->GetItemCount() > 0) {
		creature_brush_button->Enable(true);
		creature_brush_button->SetValue(true);
		spawn_brush_button->SetValue(false);
	} else {
		creature_brush_button->Enable(false);
		SelectSpawnBrush();
	}
}

void CreaturePalettePanel::SelectSpawnBrush()
{
	//g_gui.house_exit_brush->setHouse(house);
	creature_brush_button->SetValue(false);
	spawn_brush_button->SetValue(true);
}

void CreaturePalettePanel::SortCreatureBrushes()
{
	const int column = sort_column;
	const bool ascending = sort_ascending;
	std::sort(creature_brushes.begin(), creature_brushes.end(), [column, ascending](Brush* left, Brush* right) {
		const CreatureType* leftType = GetCreatureType(left);
		const CreatureType* rightType = GetCreatureType(right);
		if(!leftType || !rightType) {
			return ascending ? leftType != nullptr : rightType != nullptr;
		}

		if(column == 2) {
			const bool leftMissing = leftType->professionLevel < 0;
			const bool rightMissing = rightType->professionLevel < 0;
			if(leftMissing != rightMissing) {
				return !leftMissing;
			}
			if(leftType->professionLevel != rightType->professionLevel) {
				return ascending ? leftType->professionLevel < rightType->professionLevel : leftType->professionLevel > rightType->professionLevel;
			}
		}

		auto columnValue = [column](const CreatureType* type) {
			switch(column) {
				case 1: return wxstr(type->title);
				case 3: return GetCityDisplayName(type->city);
				default: return wxstr(type->name);
			}
		};

		int comparison = columnValue(leftType).CmpNoCase(columnValue(rightType));
		if(comparison == 0) {
			comparison = wxstr(leftType->name).CmpNoCase(wxstr(rightType->name));
		}
		if(comparison == 0) {
			comparison = wxstr(leftType->title).CmpNoCase(wxstr(rightType->title));
		}
		return ascending ? comparison < 0 : comparison > 0;
	});
}

void CreaturePalettePanel::PopulateCreatureList(const std::string& selectedCreature)
{
	rebuilding_list = true;
	creature_list->DeleteAllItems();
	for(size_t i = 0; i < creature_brushes.size(); ++i) {
		const CreatureType* creatureType = GetCreatureType(creature_brushes[i]);
		if(!creatureType) {
			continue;
		}
		const long item = creature_list->InsertItem(i, wxstr(creatureType->name));
		creature_list->SetItem(item, 1, wxstr(creatureType->title));
		creature_list->SetItem(item, 2, creatureType->professionLevel >= 0 ? i2ws(creatureType->professionLevel) : wxString());
		creature_list->SetItem(item, 3, GetCityDisplayName(creatureType->city));
	}
	UpdateSortColumnLabels();
	ResizeCreatureListColumns();
	if(!selectedCreature.empty()) {
		SelectCreature(selectedCreature);
	} else {
		SelectCreature(0);
	}
	rebuilding_list = false;
	creature_list->Refresh();
}

void CreaturePalettePanel::UpdateSortColumnLabels()
{
	static const wxString labels[] = { "Name", "Title", "Level", "City" };
	for(int columnIndex = 0; columnIndex < 4; ++columnIndex) {
		wxListItem column;
		wxString label = labels[columnIndex];
		if(columnIndex == sort_column) {
			label += sort_ascending ? " ^" : " v";
		}
		column.SetText(label);
		creature_list->SetColumn(columnIndex, column);
	}
}

void CreaturePalettePanel::ResizeCreatureListColumns()
{
	if(!creature_list) {
		return;
	}

	const int width = creature_list->GetClientSize().GetWidth() - 2;
	if(width < 60) {
		return;
	}
	const bool showCity = city_choice->IsShown();
	const int nameWidth = showCity ? width * 28 / 100 : width * 45 / 100;
	const int titleWidth = showCity ? width * 32 / 100 : width - nameWidth;
	const int levelWidth = showCity ? width * 16 / 100 : 0;
	const int cityWidth = showCity ? width - nameWidth - titleWidth - levelWidth : 0;
	creature_list->SetColumnWidth(0, nameWidth);
	creature_list->SetColumnWidth(1, titleWidth);
	creature_list->SetColumnWidth(2, levelWidth);
	creature_list->SetColumnWidth(3, cityWidth);
}

Brush* CreaturePalettePanel::GetCreatureBrush(size_t index) const
{
	if(index >= creature_brushes.size()) {
		return nullptr;
	}
	return creature_brushes[index];
}

void CreaturePalettePanel::OnTilesetChange(wxCommandEvent& event)
{
	const int selection = event.GetSelection();
	if(selection != wxNOT_FOUND) {
		SelectTileset(selection);
	}
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnCityChange(wxCommandEvent& event)
{
	const int tilesetSelection = tileset_choice->GetSelection();
	if(tilesetSelection != wxNOT_FOUND) {
		SelectTileset(static_cast<size_t>(tilesetSelection), false);
	}
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnListBoxChange(wxListEvent& event)
{
	if(rebuilding_list) {
		return;
	}
	if(event.GetIndex() < 0 || event.GetIndex() >= static_cast<long>(creature_brushes.size())) {
		return;
	}

	SelectCreatureBrush();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnListColumnClick(wxListEvent& event)
{
	const int column = event.GetColumn();
	if(column < 0 || column > 3) {
		return;
	}

	std::string selectedCreature;
	const long selection = creature_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	const CreatureType* selectedType = GetCreatureType(GetCreatureBrush(selection));
	if(selectedType) {
		selectedCreature = selectedType->name;
	}

	if(sort_column == column) {
		sort_ascending = !sort_ascending;
	} else {
		sort_column = column;
		sort_ascending = true;
	}
	SortCreatureBrushes();
	PopulateCreatureList(selectedCreature);
}

void CreaturePalettePanel::OnSize(wxSizeEvent& event)
{
	event.Skip();
	if(creature_list) {
		// This handler is bound to the list itself, so its client width is already
		// final and the columns can be adjusted without a delayed repaint.
		ResizeCreatureListColumns();
	}
}

void CreaturePalettePanel::OnClickCreatureBrushButton(wxCommandEvent& event)
{
	SelectCreatureBrush();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnClickSpawnBrushButton(wxCommandEvent& event)
{
	SelectSpawnBrush();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnChangeSpawnTime(wxSpinEvent& event)
{
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SetSpawnTime(event.GetPosition());
}

void CreaturePalettePanel::OnChangeSpawnSize(wxSpinEvent& event)
{
	if(!handling_event) {
		handling_event = true;
		g_gui.ActivatePalette(GetParentPalette());
		g_gui.SetBrushSize(event.GetPosition());
		handling_event = false;
	}
}
