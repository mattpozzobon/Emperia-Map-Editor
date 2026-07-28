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
#include "brush.h"
#include "editor.h"

#include "items.h"
#include "map.h"
#include "item.h"
#include "complexitem.h"
#include "raw_brush.h"

#include "palette_window.h"
#include "gui.h"
#include "application.h"
#include "common_windows.h"
#include "positionctrl.h"

#include "iominimap.h"

#include <wx/statline.h>
#include <wx/tokenzr.h>

#ifdef _MSC_VER
	#pragma warning(disable:4018) // signed/unsigned mismatch
#endif

// ============================================================================
// Map Properties Window

BEGIN_EVENT_TABLE(MapPropertiesWindow, wxDialog)
	EVT_CHOICE(MAP_PROPERTIES_VERSION, MapPropertiesWindow::OnChangeVersion)
	EVT_BUTTON(wxID_OK, MapPropertiesWindow::OnClickOK)
	EVT_BUTTON(wxID_CANCEL, MapPropertiesWindow::OnClickCancel)
END_EVENT_TABLE()

MapPropertiesWindow::MapPropertiesWindow(wxWindow* parent, MapTab* view, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Map Properties", wxDefaultPosition, wxSize(300, 200), wxRESIZE_BORDER | wxCAPTION),
	view(view),
	editor(editor)
{
	// Setup data variabels
	const Map& map = editor.getMap();

	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	wxFlexGridSizer* grid_sizer = newd wxFlexGridSizer(2, 10, 10);
	grid_sizer->AddGrowableCol(1);

	// Description
	grid_sizer->Add(newd wxStaticText(this, wxID_ANY, "Map Description"));
	description_ctrl = newd wxTextCtrl(this, wxID_ANY, wxstr(map.getMapDescription()), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	grid_sizer->Add(description_ctrl, wxSizerFlags(1).Expand());

	// Map version
	grid_sizer->Add(newd wxStaticText(this, wxID_ANY, "Map Version"));
	version_choice = newd wxChoice(this, MAP_PROPERTIES_VERSION);
	version_choice->Append("OTServ 0.5.0");
	version_choice->Append("OTServ 0.6.0");
	version_choice->Append("OTServ 0.6.1");
	version_choice->Append("OTServ 0.7.0 (revscriptsys)");

	switch(map.getVersion().otbm) {
		case MAP_OTBM_1:
			version_choice->SetSelection(0);
			break;
		case MAP_OTBM_2:
			version_choice->SetSelection(1);
			break;
		case MAP_OTBM_3:
			version_choice->SetSelection(2);
			break;
		case MAP_OTBM_4:
			version_choice->SetSelection(3);
			break;
		default:
			version_choice->SetSelection(0);
	}

	grid_sizer->Add(version_choice, wxSizerFlags(1).Expand());

	// Version
	grid_sizer->Add(newd wxStaticText(this, wxID_ANY, "Client Version"));
	protocol_choice = newd wxChoice(this, wxID_ANY);

	protocol_choice->SetStringSelection(wxstr(g_gui.GetCurrentVersion().getName()));

	grid_sizer->Add(protocol_choice, wxSizerFlags(1).Expand());

	// Dimensions
	grid_sizer->Add(newd wxStaticText(this, wxID_ANY, "Map Dimensions"));
	{
		wxSizer* subsizer = newd wxBoxSizer(wxHORIZONTAL);
		subsizer->Add(
			width_spin =
				newd wxSpinCtrl(this, wxID_ANY, wxstr(i2s(map.getWidth())),
				wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, rme::MapMinWidth, rme::MapMaxWidth), wxSizerFlags(1).Expand()
			);
		subsizer->Add(
			height_spin =
				newd wxSpinCtrl(this, wxID_ANY, wxstr(i2s(map.getHeight())),
				wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, rme::MapMinHeight, rme::MapMaxHeight), wxSizerFlags(1).Expand()
			);
		grid_sizer->Add(subsizer, 1, wxEXPAND);
	}

	// External files
	grid_sizer->Add(
		newd wxStaticText(this, wxID_ANY, "External Housefile")
		);

	grid_sizer->Add(
		house_filename_ctrl =
			newd wxTextCtrl(this, wxID_ANY, wxstr(map.getHouseFilename())), 1, wxEXPAND
		);

	grid_sizer->Add(
		newd wxStaticText(this, wxID_ANY, "External Spawnfile")
		);

	grid_sizer->Add(
		spawn_filename_ctrl =
			newd wxTextCtrl(this, wxID_ANY, wxstr(map.getSpawnFilename())), 1, wxEXPAND
		);

	topsizer->Add(grid_sizer, wxSizerFlags(1).Expand().Border(wxALL, 20));

	wxSizer* subsizer = newd wxBoxSizer(wxHORIZONTAL);
	subsizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	subsizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center());
	topsizer->Add(subsizer, wxSizerFlags(0).Center().Border(wxLEFT | wxRIGHT | wxBOTTOM, 20));

	SetSizerAndFit(topsizer);
	Centre(wxBOTH);
	UpdateProtocolList();

	ClientVersion* current_version = ClientVersion::get(map.getVersion().client);
	protocol_choice->SetStringSelection(wxstr(current_version->getName()));
}

void MapPropertiesWindow::UpdateProtocolList()
{
	wxString ver = version_choice->GetStringSelection();
	wxString client = protocol_choice->GetStringSelection();

	protocol_choice->Clear();

	ClientVersionList versions;
	if(g_settings.getInteger(Config::USE_OTBM_4_FOR_ALL_MAPS)) {
		versions = ClientVersion::getAllVisible();
	} else {
		MapVersionID map_version = MAP_OTBM_1;
		if(ver.Contains("0.5.0"))
			map_version = MAP_OTBM_1;
		else if(ver.Contains("0.6.0"))
			map_version = MAP_OTBM_2;
		else if(ver.Contains("0.6.1"))
			map_version = MAP_OTBM_3;
		else if(ver.Contains("0.7.0"))
			map_version = MAP_OTBM_4;

		ClientVersionList protocols = ClientVersion::getAllForOTBMVersion(map_version);
		for(ClientVersionList::const_iterator p = protocols.begin(); p != protocols.end(); ++p)
			protocol_choice->Append(wxstr((*p)->getName()));
	}
	protocol_choice->SetSelection(0);
	protocol_choice->SetStringSelection(client);
}

void MapPropertiesWindow::OnChangeVersion(wxCommandEvent&)
{
	UpdateProtocolList();
}

struct MapConversionContext
{
	struct CreatureInfo
	{
		std::string name;
		bool is_npc;
		Outfit outfit;
	};
	typedef std::map<std::string, CreatureInfo> CreatureMap;
	CreatureMap creature_types;

	void operator()(Map& map, Tile* tile, long long done)
	{
		if(tile->creature) {
			CreatureMap::iterator f = creature_types.find(tile->creature->getName());
			if(f == creature_types.end()) {
				CreatureInfo info = {
					tile->creature->getName(),
					tile->creature->isNpc(),
					tile->creature->getLookType()
				};
				creature_types[tile->creature->getName()] = info;
			}
		}
	}
};

void MapPropertiesWindow::OnClickOK(wxCommandEvent& WXUNUSED(event))
{
	Map& map = editor.getMap();
	MapVersion old_ver = map.getVersion();
	MapVersion new_ver;

	wxString ver = version_choice->GetStringSelection();

	new_ver.client = ClientVersion::get(nstr(protocol_choice->GetStringSelection()))->getID();
	if(ver.Contains("0.5.0")) {
		new_ver.otbm = MAP_OTBM_1;
	} else if(ver.Contains("0.6.0")) {
		new_ver.otbm = MAP_OTBM_2;
	} else if(ver.Contains("0.6.1")) {
		new_ver.otbm = MAP_OTBM_3;
	} else if(ver.Contains("0.7.0")) {
		new_ver.otbm = MAP_OTBM_4;
	}

	if(new_ver.client != old_ver.client) {
		if(g_gui.GetOpenMapCount() > 1) {
			g_gui.PopupDialog(this, "Error",
				"You can not change editor version with multiple maps open", wxOK);
			return;
		}
		wxString error;
		wxArrayString warnings;

		// Switch version
		g_gui.GetCurrentEditor()->getSelection().clear();
		g_gui.GetCurrentEditor()->clearActions();

		if(new_ver.client < old_ver.client) {
			int ret = g_gui.PopupDialog(this, "Notice",
				"Converting to a previous version may have serious side-effects, are you sure you want to do this?", wxYES | wxNO);
			if(ret != wxID_YES) {
				return;
			}
			UnnamedRenderingLock();

			// Remember all creatures types on the map
			MapConversionContext conversion_context;
			foreach_TileOnMap(map, conversion_context);

			// Perform the conversion
			map.convert(new_ver, true);

			// Load the new version
			if(!g_gui.LoadVersion(new_ver.client, error, warnings)) {
				g_gui.ListDialog(this, "Warnings", warnings);
				g_gui.PopupDialog(this, "Map Loader Error", error, wxOK);
				g_gui.PopupDialog(this, "Conversion Error", "Could not convert map. The map will now be closed.", wxOK);

				EndModal(0);
				return;
			}

			// Remove all creatures that were present are present in the new version
			for(MapConversionContext::CreatureMap::iterator cs = conversion_context.creature_types.begin(); cs != conversion_context.creature_types.end();) {
				if(g_creatures[cs->first])
					cs = conversion_context.creature_types.erase(cs);
				else
					++cs;
			}

			if(conversion_context.creature_types.size() > 0) {
				int add = g_gui.PopupDialog(this, "Unrecognized creatures", "There were creatures on the old version that are not present in this and were on the map, do you want to add them to this version as well?", wxYES | wxNO);
				if(add == wxID_YES) {
					for(MapConversionContext::CreatureMap::iterator cs = conversion_context.creature_types.begin(); cs != conversion_context.creature_types.end(); ++cs) {
						MapConversionContext::CreatureInfo info = cs->second;
						g_creatures.addCreatureType(info.name, info.is_npc, info.outfit);
					}
				}
			}

			map.cleanInvalidTiles(true);
		} else  {
			UnnamedRenderingLock();
			if(!g_gui.LoadVersion(new_ver.client, error, warnings)) {
				g_gui.ListDialog(this, "Warnings", warnings);
				g_gui.PopupDialog(this, "Map Loader Error", error, wxOK);
				g_gui.PopupDialog(this, "Conversion Error", "Could not convert map. The map will now be closed.", wxOK);

				EndModal(0);
				return;
			}
			map.convert(new_ver, true);
		}
	} else {
		map.convert(new_ver, true);
	}

	map.setMapDescription(nstr(description_ctrl->GetValue()));
	map.setHouseFilename(nstr(house_filename_ctrl->GetValue()));
	map.setSpawnFilename(nstr(spawn_filename_ctrl->GetValue()));

	// Only resize if we have to
	int new_map_width = width_spin->GetValue();
	int new_map_height = height_spin->GetValue();
	if(new_map_width != map.getWidth() || new_map_height != map.getHeight()) {
		map.setWidth(new_map_width);
		map.setHeight(new_map_height);
		g_gui.FitViewToMap(view);
	}
	g_gui.RefreshPalettes();

	EndModal(1);
}

void MapPropertiesWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event))
{
	// Just close this window
	EndModal(1);
}

MapPropertiesWindow::~MapPropertiesWindow() = default;

// ============================================================================
// Map Import Window

BEGIN_EVENT_TABLE(ImportMapWindow, wxDialog)
	EVT_BUTTON(MAP_WINDOW_FILE_BUTTON, ImportMapWindow::OnClickBrowse)
	EVT_BUTTON(wxID_OK, ImportMapWindow::OnClickOK)
	EVT_BUTTON(wxID_CANCEL, ImportMapWindow::OnClickCancel)
END_EVENT_TABLE()

ImportMapWindow::ImportMapWindow(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Import Map", wxDefaultPosition, wxSize(420, 315)),
	editor(editor)
{
	wxBoxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxStaticBoxSizer* tmpsizer;

	// File
	tmpsizer = newd wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "Map File"), wxHORIZONTAL);
	file_text_field = newd wxTextCtrl(tmpsizer->GetStaticBox(), wxID_ANY, "", wxDefaultPosition, wxSize(300, 23));
	tmpsizer->Add(file_text_field, 0, wxALL, 5);
	wxButton* browse_button = newd wxButton(tmpsizer->GetStaticBox(), MAP_WINDOW_FILE_BUTTON, "Browse...", wxDefaultPosition, wxSize(80, 23));
	tmpsizer->Add(browse_button, 0, wxALL, 5);
	sizer->Add(tmpsizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5);

	// Import offset
	tmpsizer = newd wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "Import Offset"), wxHORIZONTAL);
	tmpsizer->Add(newd wxStaticText(tmpsizer->GetStaticBox(), wxID_ANY, "Offset X:"), 0, wxALL | wxEXPAND, 5);
	x_offset_ctrl = newd wxSpinCtrl(tmpsizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(60, 23), wxSP_ARROW_KEYS, -rme::MapMaxHeight, rme::MapMaxHeight);
	tmpsizer->Add(x_offset_ctrl, 0, wxALL, 5);
	tmpsizer->Add(newd wxStaticText(tmpsizer->GetStaticBox(), wxID_ANY, "Offset Y:"), 0, wxALL, 5);
	y_offset_ctrl = newd wxSpinCtrl(tmpsizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(60, 23), wxSP_ARROW_KEYS, -rme::MapMaxHeight, rme::MapMaxHeight);
	tmpsizer->Add(y_offset_ctrl, 0, wxALL, 5);
	tmpsizer->Add(newd wxStaticText(tmpsizer->GetStaticBox(), wxID_ANY, "Offset Z:"), 0, wxALL, 5);
	z_offset_ctrl = newd wxSpinCtrl(tmpsizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(60, 23), wxSP_ARROW_KEYS, -rme::MapMaxLayer, rme::MapMaxLayer);
	tmpsizer->Add(z_offset_ctrl, 0, wxALL, 5);
	sizer->Add(tmpsizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);

	// Import options
	wxArrayString house_choices;
	house_choices.Add("Smart Merge");
	house_choices.Add("Insert");
	house_choices.Add("Merge");
	house_choices.Add("Don't Import");

	// House options
	tmpsizer = newd wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "House Import Behaviour"), wxVERTICAL);
	house_options = newd wxChoice(tmpsizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, house_choices);
	house_options->SetSelection(0);
	tmpsizer->Add(house_options, 0, wxALL | wxEXPAND, 5);
	sizer->Add(tmpsizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);

	// Import options
	wxArrayString spawn_choices;
	spawn_choices.Add("Merge");
	spawn_choices.Add("Don't Import");

	// Spawn options
	tmpsizer = newd wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "Spawn Import Behaviour"), wxVERTICAL);
	spawn_options = newd wxChoice(tmpsizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, spawn_choices);
	spawn_options->SetSelection(0);
	tmpsizer->Add(spawn_options, 0, wxALL | wxEXPAND, 5);
	sizer->Add(tmpsizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);

	// OK/Cancel buttons
	wxBoxSizer* buttons = newd wxBoxSizer(wxHORIZONTAL);
	buttons->Add(newd wxButton(this, wxID_OK, "Ok"), 0, wxALL, 5);
	buttons->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), 0, wxALL, 5);
	sizer->Add(buttons, wxSizerFlags(1).Center());

	SetSizer(sizer);
	Layout();
	Centre(wxBOTH);
}

ImportMapWindow::~ImportMapWindow() = default;

void ImportMapWindow::OnClickBrowse(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog dialog(this, "Import...", "", "", "*.otbm", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	int ok = dialog.ShowModal();

	if(ok == wxID_OK)
		file_text_field->ChangeValue(dialog.GetPath());
}

void ImportMapWindow::OnClickOK(wxCommandEvent& WXUNUSED(event))
{
	if(Validate() && TransferDataFromWindow()) {
		wxFileName fn = file_text_field->GetValue();
		if(!fn.FileExists()) {
			g_gui.PopupDialog(this, "Error", "The specified map file doesn't exist", wxOK);
			return;
		}

		ImportType spawn_import_type = IMPORT_DONT;
		ImportType house_import_type = IMPORT_DONT;

		switch(spawn_options->GetSelection()) {
			case 0: spawn_import_type = IMPORT_MERGE; break;
			case 1: spawn_import_type = IMPORT_DONT; break;
		}

		switch(house_options->GetSelection()) {
			case 0: house_import_type = IMPORT_SMART_MERGE; break;
			case 1: house_import_type = IMPORT_MERGE; break;
			case 2: house_import_type = IMPORT_INSERT; break;
			case 3: house_import_type = IMPORT_DONT; break;
		}

		EndModal(1);

		editor.importMap(fn, x_offset_ctrl->GetValue(), y_offset_ctrl->GetValue(), z_offset_ctrl->GetValue(), house_import_type, spawn_import_type);
	}
}

void ImportMapWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event))
{
	// Just close this window
	EndModal(0);
}


// ============================================================================
// Export Minimap window

BEGIN_EVENT_TABLE(ExportMiniMapWindow, wxDialog)
	EVT_BUTTON(MAP_WINDOW_FILE_BUTTON, ExportMiniMapWindow::OnClickBrowse)
	EVT_BUTTON(wxID_OK, ExportMiniMapWindow::OnClickOK)
	EVT_BUTTON(wxID_CANCEL, ExportMiniMapWindow::OnClickCancel)
	EVT_CHOICE(wxID_ANY, ExportMiniMapWindow::OnExportTypeChange)
END_EVENT_TABLE()

ExportMiniMapWindow::ExportMiniMapWindow(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Export Minimap", wxDefaultPosition, wxSize(400, 300)),
	editor(editor)
{
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxSizer* tmpsizer;

	// Error field
	error_field = newd wxStaticText(this, wxID_VIEW_DETAILS, "", wxDefaultPosition, wxDefaultSize);
	error_field->SetForegroundColour(*wxRED);
	tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
	tmpsizer->Add(error_field, 0, wxALL, 5);
	sizer->Add(tmpsizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5);

	// Output folder
	directory_text_field = newd wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize);
	directory_text_field->Bind(wxEVT_KEY_UP, &ExportMiniMapWindow::OnDirectoryChanged, this);
	directory_text_field->SetValue(wxString(g_settings.getString(Config::MINIMAP_EXPORT_DIR)));
	tmpsizer = newd wxStaticBoxSizer(wxHORIZONTAL, this, "Output Folder");
	tmpsizer->Add(directory_text_field, 1, wxALL, 5);
	tmpsizer->Add(newd wxButton(this, MAP_WINDOW_FILE_BUTTON, "Browse"), 0, wxALL, 5);
	sizer->Add(tmpsizer, 0, wxALL | wxEXPAND, 5);

	// File name
	wxString mapName(editor.getMap().getName().c_str(), wxConvUTF8);
	file_name_text_field = newd wxTextCtrl(this, wxID_ANY, mapName.BeforeLast('.'), wxDefaultPosition, wxDefaultSize);
	file_name_text_field->Bind(wxEVT_KEY_UP, &ExportMiniMapWindow::OnFileNameChanged, this);
	tmpsizer = newd wxStaticBoxSizer(wxHORIZONTAL, this, "File Name");
	tmpsizer->Add(file_name_text_field, 1, wxALL, 5);
	sizer->Add(tmpsizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5);

	// Format options
	wxArrayString format_choices;
	format_choices.Add(".otmm (Client Minimap)");
	format_choices.Add(".png (PNG Image)");
	format_choices.Add(".bmp (Bitmap Image)");
	format_options = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, format_choices);
	format_options->SetSelection(0);
	tmpsizer->Add(format_options, 1, wxALL, 5);

	// Export options
	wxArrayString choices;
	choices.Add("All Floors");
	choices.Add("Ground Floor");
	choices.Add("Specific Floor");

	if(editor.hasSelection())
		choices.Add("Selected Area");

	// Area options
	tmpsizer = newd wxStaticBoxSizer(wxHORIZONTAL, this, "Area Options");
	floor_options = newd wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices);
	floor_number = newd wxSpinCtrl(this, wxID_ANY, i2ws(rme::MapGroundLayer), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, rme::MapMinLayer, rme::MapMaxLayer, rme::MapGroundLayer);
	floor_number->Enable(false);
	floor_options->SetSelection(0);
	tmpsizer->Add(floor_options, 1, wxALL, 5);
	tmpsizer->Add(floor_number, 0, wxALL, 5);
	sizer->Add(tmpsizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5);

	// OK/Cancel buttons
	tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
	tmpsizer->Add(ok_button = newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	tmpsizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center());
	sizer->Add(tmpsizer, 0, wxCENTER, 10);

	SetSizer(sizer);
	Layout();
	Centre(wxBOTH);
	CheckValues();
}

ExportMiniMapWindow::~ExportMiniMapWindow() = default;

void ExportMiniMapWindow::OnExportTypeChange(wxCommandEvent& event)
{
	floor_number->Enable(event.GetSelection() == 2);
}

void ExportMiniMapWindow::OnClickBrowse(wxCommandEvent& WXUNUSED(event))
{
	wxDirDialog dialog(NULL, "Select the output folder", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	if(dialog.ShowModal() == wxID_OK) {
		const wxString& directory = dialog.GetPath();
		directory_text_field->ChangeValue(directory);
	}
	CheckValues();
}

void ExportMiniMapWindow::OnDirectoryChanged(wxKeyEvent& event)
{
	CheckValues();
	event.Skip();
}

void ExportMiniMapWindow::OnFileNameChanged(wxKeyEvent& event)
{
	CheckValues();
	event.Skip();
}

void ExportMiniMapWindow::OnClickOK(wxCommandEvent& WXUNUSED(event))
{
	g_gui.CreateLoadBar("Exporting minimap...");

	auto format = static_cast<MinimapExportFormat>(format_options->GetSelection());
	auto mode = static_cast<MinimapExportMode>(floor_options->GetSelection());
	std::string directory = directory_text_field->GetValue().ToStdString();
	std::string file_name = file_name_text_field->GetValue().ToStdString();
	int floor = floor_number->GetValue();

	g_settings.setString(Config::MINIMAP_EXPORT_DIR, directory);

	IOMinimap io(&editor, format, mode, true);
	if (!io.saveMinimap(directory, file_name, floor)) {
		g_gui.PopupDialog("Error", io.getError(), wxOK);
	}

	g_gui.DestroyLoadBar();
	EndModal(wxID_OK);
}

void ExportMiniMapWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event))
{
	// Just close this window
	EndModal(wxID_CANCEL);
}

void ExportMiniMapWindow::CheckValues()
{
	if(directory_text_field->IsEmpty()) {
		error_field->SetLabel("Type or select an output folder.");
		ok_button->Enable(false);
		return;
	}

	if(file_name_text_field->IsEmpty()) {
		error_field->SetLabel("Type a name for the file.");
		ok_button->Enable(false);
		return;
	}

	FileName directory(directory_text_field->GetValue());

	if(!directory.Exists()) {
		error_field->SetLabel("Output folder not found.");
		ok_button->Enable(false);
		return;
	}

	if(!directory.IsDirWritable()) {
		error_field->SetLabel("Output folder is not writable.");
		ok_button->Enable(false);
		return;
	}

	error_field->SetLabel(wxEmptyString);
	ok_button->Enable(true);
}

// ============================================================================
// Numkey forwarding text control

BEGIN_EVENT_TABLE(KeyForwardingTextCtrl, wxTextCtrl)
	EVT_KEY_DOWN(KeyForwardingTextCtrl::OnKeyDown)
END_EVENT_TABLE()

void KeyForwardingTextCtrl::OnKeyDown(wxKeyEvent& event)
{
	if(event.GetKeyCode() == WXK_UP || event.GetKeyCode() == WXK_DOWN ||
		event.GetKeyCode() == WXK_PAGEDOWN || event.GetKeyCode() == WXK_PAGEUP) {
		GetParent()->GetEventHandler()->AddPendingEvent(event);
	} else {
		event.Skip();
	}
}

// ============================================================================
// Find Item Dialog (Jump to item)

BEGIN_EVENT_TABLE(FindDialog, wxDialog)
	EVT_TIMER(wxID_ANY, FindDialog::OnTextIdle)
	EVT_TEXT(JUMP_DIALOG_TEXT, FindDialog::OnTextChange)
	EVT_KEY_DOWN(FindDialog::OnKeyDown)
	EVT_TEXT_ENTER(JUMP_DIALOG_TEXT, FindDialog::OnClickOK)
	EVT_LISTBOX_DCLICK(JUMP_DIALOG_LIST, FindDialog::OnClickList)
	EVT_BUTTON(wxID_OK, FindDialog::OnClickOK)
	EVT_BUTTON(wxID_CANCEL, FindDialog::OnClickCancel)
END_EVENT_TABLE()

FindDialog::FindDialog(wxWindow* parent, wxString title) :
	wxDialog(g_gui.root, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX),
	idle_input_timer(this),
	result_brush(nullptr),
	result_id(0)
{
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	search_field = newd KeyForwardingTextCtrl(this, JUMP_DIALOG_TEXT, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	search_field->SetFocus();
	sizer->Add(search_field, 0, wxEXPAND);

	item_list = newd FindDialogListBox(this, JUMP_DIALOG_LIST);
	item_list->SetMinSize(wxSize(470, 400));
	sizer->Add(item_list, wxSizerFlags(1).Expand().Border());

	wxSizer* stdsizer = newd wxBoxSizer(wxHORIZONTAL);
	stdsizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	stdsizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center());
	sizer->Add(stdsizer, wxSizerFlags(0).Center().Border());

	SetSizerAndFit(sizer);
	Centre(wxBOTH);
	// We can't call it here since it calls an abstract function, call in child constructors instead.
	// RefreshContents();
}

FindDialog::~FindDialog() = default;

void FindDialog::OnKeyDown(wxKeyEvent& event)
{
	int w, h;
	item_list->GetSize(&w, &h);
	size_t amount = 1;

	switch(event.GetKeyCode()) {
		case WXK_PAGEUP:
			amount = h / 32 + 1;
			[[fallthrough]];
		case WXK_UP: {
			if(item_list->GetItemCount() > 0) {
				ssize_t n = item_list->GetSelection();
				if(n == wxNOT_FOUND)
					n = 0;
				else if(n != amount && n - amount < n) // latter is needed for unsigned overflow
					n -= amount;
				else
					n = 0;
				item_list->SetSelection(n);
			}
			break;
		}

		case WXK_PAGEDOWN:
			amount = h / 32 + 1;
			[[fallthrough]];
		case WXK_DOWN: {
			if(item_list->GetItemCount() > 0) {
				ssize_t n = item_list->GetSelection();
				size_t itemcount = item_list->GetItemCount();
				if(n == wxNOT_FOUND)
					n = 0;
				else if(static_cast<uint32_t>(n) < itemcount - amount && itemcount - amount < itemcount)
					n += amount;
				else
					n = item_list->GetItemCount() - 1;

				item_list->SetSelection(n);
			}
			break;
		}
		default:
			event.Skip();
			break;
	}
}

void FindDialog::OnTextIdle(wxTimerEvent& WXUNUSED(event))
{
	RefreshContents();
}

void FindDialog::OnTextChange(wxCommandEvent& WXUNUSED(event))
{
	idle_input_timer.Start(800, true);
}

void FindDialog::OnClickList(wxCommandEvent& event)
{
	OnClickListInternal(event);
}

void FindDialog::OnClickOK(wxCommandEvent& WXUNUSED(event))
{
	// This is to get virtual callback
	OnClickOKInternal();
}

void FindDialog::OnClickCancel(wxCommandEvent& WXUNUSED(event))
{
	EndModal(0);
}

void FindDialog::RefreshContents()
{
	// This is to get virtual callback
	RefreshContentsInternal();
}

// ============================================================================
// Find Brush Dialog (Jump to brush)

FindBrushDialog::FindBrushDialog(wxWindow* parent, wxString title) : FindDialog(parent, title)
{
	RefreshContents();
}

FindBrushDialog::~FindBrushDialog() = default;

void FindBrushDialog::OnClickListInternal(wxCommandEvent& event)
{
	Brush* brush = item_list->GetSelectedBrush();
	if(brush) {
		result_brush = brush;
		EndModal(1);
	}
}

void FindBrushDialog::OnClickOKInternal()
{
	// This is kind of stupid as it would fail unless the "Please enter a search string" wasn't there
	if(item_list->GetItemCount() > 0) {
		if(item_list->GetSelection() == wxNOT_FOUND) {
			item_list->SetSelection(0);
		}
		Brush* brush = item_list->GetSelectedBrush();
		if(!brush) {
			// It's either "Please enter a search string" or "No matches"
			// Perhaps we can refresh now?
			std::string search_string = as_lower_str(nstr(search_field->GetValue()));
			bool do_search = (search_string.size() >= 2);

			if(do_search) {
				const BrushMap& map = g_brushes.getMap();
				for(BrushMap::const_iterator iter = map.begin(); iter != map.end(); ++iter) {
					const Brush* brush = iter->second;
					if(as_lower_str(brush->getName()).find(search_string) == std::string::npos)
						continue;

					// Don't match RAWs now.
					if(brush->isRaw())
						continue;

					// Found one!
					result_brush = brush;
					break;
				}

				// Did we not find a matching brush?
				if(!result_brush) {
					// Then let's search the RAWs
					for(int id = 0; id <= g_items.getMaxID(); ++id) {
						const ItemType& type = g_items.getItemType(id);
						if(type.id == 0)
							continue;

						RAWBrush* raw_brush = type.raw_brush;
						if(!raw_brush)
							continue;

						if(as_lower_str(raw_brush->getName()).find(search_string) == std::string::npos)
							continue;

						// Found one!
						result_brush = raw_brush;
						break;
					}
				}
				// Done!
			}
		} else {
			result_brush = brush;
		}
	}
	EndModal(1);
}

void FindBrushDialog::RefreshContentsInternal()
{
	item_list->Clear();

	std::string search_string = as_lower_str(nstr(search_field->GetValue()));
	bool do_search = (search_string.size() >= 2);

	if(do_search) {

		bool found_search_results = false;

		const BrushMap& brushes_map = g_brushes.getMap();

		// We store the raws so they display last of all results
		std::deque<const RAWBrush*> raws;

		for(BrushMap::const_iterator iter = brushes_map.begin(); iter != brushes_map.end(); ++iter) {
			const Brush* brush = iter->second;

			if(as_lower_str(brush->getName()).find(search_string) == std::string::npos)
				continue;

			if(brush->isRaw())
				continue;

			found_search_results = true;
			item_list->AddBrush(const_cast<Brush*>(brush));
		}

		for(int id = 0; id <= g_items.getMaxID(); ++id) {
			const ItemType& type = g_items.getItemType(id);
			if(type.id == 0)
				continue;

			RAWBrush* raw_brush = type.raw_brush;
			if(!raw_brush)
				continue;

			if(as_lower_str(raw_brush->getName()).find(search_string) == std::string::npos)
				continue;

			found_search_results = true;
			item_list->AddBrush(raw_brush);
		}

		while(raws.size() > 0) {
			item_list->AddBrush(const_cast<RAWBrush*>(raws.front()));
			raws.pop_front();
		}

		if(found_search_results) {
			item_list->SetSelection(0);
		} else {
			item_list->SetNoMatches();
		}

	}
	item_list->Refresh();
}

// ============================================================================
// Listbox in find item / brush stuff

FindDialogListBox::FindDialogListBox(wxWindow* parent, wxWindowID id) :
	wxVListBox(parent, id, wxDefaultPosition, wxDefaultSize, wxLB_SINGLE),
	cleared(false),
	no_matches(false)
{
	Clear();
}

FindDialogListBox::~FindDialogListBox()
{
	////
}

void FindDialogListBox::Clear()
{
	cleared = true;
	no_matches = false;
	brushlist.clear();
	SetItemCount(1);
}

void FindDialogListBox::SetNoMatches()
{
	cleared = false;
	no_matches = true;
	brushlist.clear();
	SetItemCount(1);
}

void FindDialogListBox::AddBrush(Brush* brush)
{
	if(cleared || no_matches)
		SetItemCount(0);

	cleared = false;
	no_matches = false;

	SetItemCount(GetItemCount() + 1);
	brushlist.push_back(brush);
}

Brush* FindDialogListBox::GetSelectedBrush()
{
	ssize_t n = GetSelection();
	if(n == wxNOT_FOUND || no_matches || cleared)
		return nullptr;
	return brushlist[n];
}

void FindDialogListBox::OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const
{
	if(no_matches) {
		dc.DrawText("No matches for your search.", rect.GetX() + 40, rect.GetY() + 6);
	} else if(cleared) {
		dc.DrawText("Please enter your search string.", rect.GetX() + 40, rect.GetY() + 6);
	} else {
		ASSERT(n < brushlist.size());
		Sprite* spr = g_gui.gfx.getSprite(brushlist[n]->getLookID());
		if(spr) {
			spr->DrawTo(&dc, SPRITE_SIZE_32x32, rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
		} else {
			auto creatureType = g_creatures[brushlist[n]->getName()];
			if (!creatureType) {
				return;
			}

			auto creatureSprite = g_gui.gfx.getCreatureSprite(creatureType->outfit.lookType);
			if (creatureSprite) {
				creatureSprite->DrawTo(&dc, rect, creatureType->outfit);
			}
		}

		if(IsSelected(n)) {
			if(HasFocus())
				dc.SetTextForeground(wxColor(0xFF, 0xFF, 0xFF));
			else
				dc.SetTextForeground(wxColor(0x00, 0x00, 0xFF));
		} else {
			dc.SetTextForeground(wxColor(0x00, 0x00, 0x00));
		}

		dc.DrawText(wxstr(brushlist[n]->getName()), rect.GetX() + 40, rect.GetY() + 6);
	}
}

wxCoord FindDialogListBox::OnMeasureItem(size_t n) const
{
	return 32;
}

// ============================================================================
// wxListBox that can be sorted

SortableListBox::SortableListBox(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size)
: wxListBox(parent, id, pos, size, 0, nullptr, wxLB_SINGLE | wxLB_NEEDED_SB)
{}

SortableListBox::~SortableListBox() {}

void SortableListBox::Sort() {

	if(GetCount() == 0)
		return;

	wxASSERT_MSG(GetClientDataType() != wxClientData_Object, "Sorting a list with data of type wxClientData_Object is currently not implemented");

	DoSort();
}

void SortableListBox::DoSort() {
	size_t count = GetCount();
	int selection = GetSelection();
	wxClientDataType dataType = GetClientDataType();

	wxArrayString stringList;
	wxArrayPtrVoid dataList;

	for(size_t i = 0; i < count; ++i) {
		stringList.Add(GetString(i));
		if(dataType == wxClientData_Void)
			dataList.Add(GetClientData(i));
	}

	//Insertion sort
	for(size_t i = 0; i < count; ++i) {
		size_t j = i;
		while(j > 0 && stringList[j].CmpNoCase(stringList[j - 1]) < 0) {

			wxString tmpString = stringList[j];
			stringList[j] = stringList[j - 1];
			stringList[j - 1] = tmpString;

			if(dataType == wxClientData_Void) {
				void* tmpData = dataList[j];
				dataList[j] = dataList[j - 1];
				dataList[j - 1] = tmpData;
			}

			if(selection == j - 1)
				selection++;
			else if(selection == j) {
				selection--;
			}

			j--;
		}
	}

	Freeze();
	Clear();
	for(size_t i = 0; i < count; ++i) {
		if(dataType == wxClientData_Void)
			Append(stringList[i], dataList[i]);
		else
			Append(stringList[i]);
	}
	Thaw();

	SetSelection(selection);
}

// ============================================================================
// Object properties base

ObjectPropertiesWindowBase::ObjectPropertiesWindowBase(wxWindow* parent, wxString title, const Map* map, const Tile* tile, Item* item, wxPoint position /* = wxDefaultPosition */) :
wxDialog(parent, wxID_ANY, title,
	position, wxSize(600, 400), wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER),
	edit_map(map),
	edit_tile(tile),
	edit_item(item),
	edit_creature(nullptr),
	edit_spawn(nullptr)
{
	////
}

ObjectPropertiesWindowBase::ObjectPropertiesWindowBase(wxWindow* parent, wxString title, const Map* map, const Tile* tile, Creature* creature, wxPoint position /* = wxDefaultPosition */) :
wxDialog(parent, wxID_ANY, title,
	position, wxSize(600, 400), wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER),
	edit_map(map),
	edit_tile(tile),
	edit_item(nullptr),
	edit_creature(creature),
	edit_spawn(nullptr)
{
	////
}

ObjectPropertiesWindowBase::ObjectPropertiesWindowBase(wxWindow* parent, wxString title, const Map* map, const Tile* tile, Spawn* spawn, wxPoint position /* = wxDefaultPosition */) :
wxDialog(parent, wxID_ANY, title,
	position, wxSize(600, 400), wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER),
	edit_map(map),
	edit_tile(tile),
	edit_item(nullptr),
	edit_creature(nullptr),
	edit_spawn(spawn)
{
	////
}

Item* ObjectPropertiesWindowBase::getItemBeingEdited()
{
	return edit_item;
}

// ============================================================================
// Edit Towns Dialog

BEGIN_EVENT_TABLE(EditTownsDialog, wxDialog)
	EVT_LISTBOX(EDIT_TOWNS_LISTBOX, EditTownsDialog::OnListBoxChange)

	EVT_BUTTON(EDIT_TOWNS_SELECT_TEMPLE, EditTownsDialog::OnClickSelectTemplePosition)
	EVT_BUTTON(EDIT_TOWNS_ADD, EditTownsDialog::OnClickAdd)
	EVT_BUTTON(EDIT_TOWNS_REMOVE, EditTownsDialog::OnClickRemove)
	EVT_BUTTON(wxID_OK, EditTownsDialog::OnClickOK)
	EVT_BUTTON(wxID_CANCEL, EditTownsDialog::OnClickCancel)
END_EVENT_TABLE()

EditTownsDialog::EditTownsDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Towns", wxDefaultPosition, wxSize(280,330)),
	editor(editor)
{
	const Map& map = editor.getMap();

	// Create topsizer
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxSizer* tmpsizer;

	for(TownMap::const_iterator town_iter = map.towns.begin(); town_iter != map.towns.end(); ++town_iter) {
		Town* town = town_iter->second;
		town_list.push_back(newd Town(*town));
		if(max_town_id < town->getID()) {
			max_town_id = town->getID();
		}
	}

	// Town list
	town_listbox = newd wxListBox(this, EDIT_TOWNS_LISTBOX, wxDefaultPosition, wxSize(240, 100));
	sizer->Add(town_listbox, 1, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);

	tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
	tmpsizer->Add(newd wxButton(this, EDIT_TOWNS_ADD, "Add"), 0, wxTOP, 5);
	tmpsizer->Add(remove_button = newd wxButton(this, EDIT_TOWNS_REMOVE, "Remove"), 0, wxRIGHT | wxTOP, 5);
	sizer->Add(tmpsizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

	// House options
	tmpsizer = newd wxStaticBoxSizer(wxHORIZONTAL, this, "Name / ID");
	name_field = newd wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(190,20), 0, wxTextValidator(wxFILTER_ASCII, &town_name));
	tmpsizer->Add(name_field, 2, wxEXPAND | wxLEFT | wxBOTTOM, 5);

	id_field = newd wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(40,20), 0, wxTextValidator(wxFILTER_NUMERIC, &town_id));
	id_field->Enable(false);
	tmpsizer->Add(id_field, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
	sizer->Add(tmpsizer, 0, wxEXPAND | wxALL, 10);

	// Temple position
	temple_position = newd PositionCtrl(this, "Temple Position", 0, 0, 0, map.getWidth(), map.getHeight());
	select_position_button = newd wxButton(this, EDIT_TOWNS_SELECT_TEMPLE, "Go To");
	temple_position->Add(select_position_button, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);
	sizer->Add(temple_position, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

	// OK/Cancel buttons
	tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
	tmpsizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	tmpsizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center());
	sizer->Add(tmpsizer, 0, wxCENTER | wxALL, 10);

	SetSizerAndFit(sizer);
	Centre(wxBOTH);
	BuildListBox(true);
}

EditTownsDialog::~EditTownsDialog()
{
	for(std::vector<Town*>::iterator town_iter = town_list.begin(); town_iter != town_list.end(); ++town_iter) {
		delete *town_iter;
	}
}

void EditTownsDialog::BuildListBox(bool doselect)
{
	long tmplong = 0;
	max_town_id = 0;
	wxArrayString town_name_list;
	uint32_t selection_before = 0;

	if(doselect && id_field->GetValue().ToLong(&tmplong)) {
		uint32_t old_town_id = tmplong;

		for(std::vector<Town*>::iterator town_iter = town_list.begin(); town_iter != town_list.end(); ++town_iter) {
			if(old_town_id == (*town_iter)->getID()) {
				selection_before = (*town_iter)->getID();
				break;
			}
		}
	}

	for(std::vector<Town*>::iterator town_iter = town_list.begin(); town_iter != town_list.end(); ++town_iter) {
		Town* town = *town_iter;
		town_name_list.Add(wxstr(town->getName()));
		if(max_town_id < town->getID()) {
			max_town_id = town->getID();
		}
	}

	town_listbox->Set(town_name_list);
	remove_button->Enable(town_listbox->GetCount() != 0);
	select_position_button->Enable(false);

	if(doselect) {
		if(selection_before) {
			int i = 0;
			for(std::vector<Town*>::iterator town_iter = town_list.begin(); town_iter != town_list.end(); ++town_iter) {
				if(selection_before == (*town_iter)->getID()) {
					town_listbox->SetSelection(i);
					return;
				}
				++i;
			}
		}
		UpdateSelection(0);
	}
}

void EditTownsDialog::UpdateSelection(int new_selection)
{
	long tmplong;

	// Save old values
	if(town_list.size() > 0) {
		if(id_field->GetValue().ToLong(&tmplong)) {
			uint32_t old_town_id = tmplong;

			Town* old_town = nullptr;

			for(std::vector<Town*>::iterator town_iter = town_list.begin(); town_iter != town_list.end(); ++town_iter) {
				if(old_town_id == (*town_iter)->getID()) {
					old_town = *town_iter;
					break;
				}
			}

			if(old_town) {
				Position templepos = temple_position->GetPosition();

				//printf("Changed town %d:%s\n", old_town_id, old_town->getName().c_str());
				//printf("New values %d:%s:%d:%d:%d\n", town_id, town_name.c_str(), templepos.x, templepos.y, templepos.z);
				old_town->setTemplePosition(templepos);

				wxString new_name = name_field->GetValue();
				wxString old_name = wxstr(old_town->getName());

				old_town->setName(nstr(new_name));
				if(new_name != old_name) {
					// Name has changed, update list
					BuildListBox(false);
				}
			}
		}
	}

	// Clear fields
	town_name.Clear();
	town_id.Clear();

	if(town_list.size() > size_t(new_selection)) {
		name_field->Enable(true);
		temple_position->Enable(true);
		select_position_button->Enable(true);

		// Change the values to reflect the newd selection
		Town* town = town_list[new_selection];
		ASSERT(town);

		//printf("Selected %d:%s\n", new_selection, town->getName().c_str());
		town_name << wxstr(town->getName());
		name_field->SetValue(town_name);
		town_id << long(town->getID());
		id_field->SetValue(town_id);
		temple_position->SetPosition(town->getTemplePosition());
		town_listbox->SetSelection(new_selection);
	} else {
		name_field->Enable(false);
		temple_position->Enable(false);
		select_position_button->Enable(false);
	}
	Refresh();
}

void EditTownsDialog::OnListBoxChange(wxCommandEvent& event)
{
	UpdateSelection(event.GetSelection());
}

void EditTownsDialog::OnClickSelectTemplePosition(wxCommandEvent& WXUNUSED(event))
{
	Position templepos = temple_position->GetPosition();
	g_gui.SetScreenCenterPosition(templepos);
}

void EditTownsDialog::OnClickAdd(wxCommandEvent& WXUNUSED(event))
{
	Town* new_town = newd Town(++max_town_id);
	new_town->setName("Unnamed Town");
	new_town->setTemplePosition(Position(0,0,0));
	town_list.push_back(new_town);

	BuildListBox(false);
	UpdateSelection(town_list.size()-1);
	town_listbox->SetSelection(town_list.size()-1);
}

void EditTownsDialog::OnClickRemove(wxCommandEvent& WXUNUSED(event))
{
	long tmplong;
	if(id_field->GetValue().ToLong(&tmplong)) {
		uint32_t old_town_id = tmplong;

		Town* town = nullptr;

		std::vector<Town*>::iterator town_iter = town_list.begin();

		int selection_index = 0;
		while(town_iter != town_list.end()) {
			if(old_town_id == (*town_iter)->getID()) {
				town = *town_iter;
				break;
			}
			++selection_index;
			++town_iter;
		}
		if(!town) return;

		const Map& map = editor.getMap();
		for(const auto& pair : map.houses) {
			if(pair.second->townid == town->getID()) {
				g_gui.PopupDialog(this, "Error", "You cannot delete a town which still has houses associated with it.", wxOK);
				return;
			}
		}

		delete town;
		town_list.erase(town_iter);
		BuildListBox(false);
		UpdateSelection(selection_index-1);
	}
}

void EditTownsDialog::OnClickOK(wxCommandEvent& WXUNUSED(event))
{
	long tmplong = 0;

	if(Validate() && TransferDataFromWindow()) {
		// Save old values
		if(town_list.size() > 0 && id_field->GetValue().ToLong(&tmplong)) {
			uint32_t old_town_id = tmplong;

			Town* old_town = nullptr;

			for(std::vector<Town*>::iterator town_iter = town_list.begin(); town_iter != town_list.end(); ++town_iter) {
				if(old_town_id == (*town_iter)->getID()) {
					old_town = *town_iter;
					break;
				}
			}

			if(old_town) {
				Position templepos = temple_position->GetPosition();

				//printf("Changed town %d:%s\n", old_town_id, old_town->getName().c_str());
				//printf("New values %d:%s:%d:%d:%d\n", town_id, town_name.c_str(), templepos.x, templepos.y, templepos.z);
				old_town->setTemplePosition(templepos);

				wxString new_name = name_field->GetValue();
				wxString old_name = wxstr(old_town->getName());

				old_town->setName(nstr(new_name));
				if(new_name != old_name) {
					// Name has changed, update list
					BuildListBox(true);
				}
			}
		}

		Towns& towns = editor.getMap().towns;

		// Verify the newd information
		for(std::vector<Town*>::iterator town_iter = town_list.begin(); town_iter != town_list.end(); ++town_iter) {
			Town* town = *town_iter;
			if(town->getName() == "") {
				g_gui.PopupDialog(this, "Error", "You can't have a town with an empty name.", wxOK);
				return;
			}
			if(!town->getTemplePosition().isValid() ||
				town->getTemplePosition().x > editor.getMap().getWidth() ||
				town->getTemplePosition().y > editor.getMap().getHeight()) {
				wxString msg;
				msg << "The town " << wxstr(town->getName()) << " has an invalid temple position.";
				g_gui.PopupDialog(this, "Error", msg, wxOK);
				return;
			}
		}

		// Clear old towns
		towns.clear();

		// Build the newd town map
		for(std::vector<Town*>::iterator town_iter = town_list.begin(); town_iter != town_list.end(); ++town_iter) {
			towns.addTown(*town_iter);
		}
		town_list.clear();
		editor.getMap().doChange();

		EndModal(1);
		g_gui.RefreshPalettes();
	}
}

void EditTownsDialog::OnClickCancel(wxCommandEvent& WXUNUSED(event))
{
	// Just close this window
	EndModal(0);
}

// ============================================================================
// Go To Position Dialog
// Jump to a position on the map by entering XYZ coordinates

BEGIN_EVENT_TABLE(GotoPositionDialog, wxDialog)
	EVT_BUTTON(wxID_OK, GotoPositionDialog::OnClickOK)
	EVT_BUTTON(wxID_CANCEL, GotoPositionDialog::OnClickCancel)
END_EVENT_TABLE()

GotoPositionDialog::GotoPositionDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Go To Position", wxDefaultPosition, wxDefaultSize),
	editor(editor)
{
	const Map& map = editor.getMap();

	// create topsizer
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	posctrl = newd PositionCtrl(this, "Destination", map.getWidth() / 2, map.getHeight() / 2, rme::MapGroundLayer, map.getWidth(), map.getHeight());
	sizer->Add(posctrl, 0, wxTOP | wxLEFT | wxRIGHT, 20);

	// OK/Cancel buttons
	wxSizer* tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
	tmpsizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	tmpsizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center());
	sizer->Add(tmpsizer, 0, wxALL | wxCENTER, 20); // Border to top too

	SetSizerAndFit(sizer);
	Centre(wxBOTH);
}

void GotoPositionDialog::OnClickCancel(wxCommandEvent &)
{
	EndModal(0);
}

void GotoPositionDialog::OnClickOK(wxCommandEvent &)
{
	g_gui.SetScreenCenterPosition(posctrl->GetPosition());
	EndModal(1);
}

// ============================================================================
// Zone Configuration Dialog

BEGIN_EVENT_TABLE(ZoneConfigDialog, wxDialog)
	EVT_LISTBOX(ZONE_CONFIG_LIST, ZoneConfigDialog::OnListSelect)
	EVT_CHOICE(ZONE_CONFIG_FILTER, ZoneConfigDialog::OnFilterType)
	EVT_BUTTON(ZONE_CONFIG_ADD, ZoneConfigDialog::OnAddZone)
	EVT_BUTTON(ZONE_CONFIG_REMOVE, ZoneConfigDialog::OnRemoveZone)
	EVT_CHOICE(ZONE_CONFIG_CATEGORY, ZoneConfigDialog::OnCategoryChanged)
	EVT_BUTTON(ZONE_CONFIG_AREA_REMOVE, ZoneConfigDialog::OnAreaRemove)
	EVT_BUTTON(wxID_OK, ZoneConfigDialog::OnClickOK)
	EVT_BUTTON(wxID_CANCEL, ZoneConfigDialog::OnClickCancel)
	EVT_CHECKBOX(ZONE_CONFIG_HAS_RESOURCES, ZoneConfigDialog::OnResourcesCheck)
	EVT_BUTTON(ZONE_CONFIG_SPAWN_ADD, ZoneConfigDialog::OnSpawnAdd)
	EVT_BUTTON(ZONE_CONFIG_SPAWN_REMOVE, ZoneConfigDialog::OnSpawnRemove)
END_EVENT_TABLE()

namespace {
	const char* ZoneCategoryValues[] = {
		"", "city", "town", "forest", "plains", "mountain", "cave", "water",
		"desert", "market", "temple", "depot", "library", "shop", "bank", "tavern"
	};
	constexpr int ZoneCategoryCount = sizeof(ZoneCategoryValues) / sizeof(ZoneCategoryValues[0]);

	wxArrayString BuildZoneCategoryChoices(const wxString& firstLabel)
	{
		wxArrayString choices;
		choices.Add(firstLabel);
		for(int index = 1; index < ZoneCategoryCount; ++index) {
			choices.Add(wxstr(getZoneCategoryDisplayName(ZoneCategoryValues[index])));
		}
		return choices;
	}

	int FindZoneCategoryIndex(const std::string& category)
	{
		for(int index = 1; index < ZoneCategoryCount; ++index) {
			if(category == ZoneCategoryValues[index]) {
				return index;
			}
		}
		return 0;
	}

	std::string GetResourceType(const ZoneResourceDef& def)
	{
		return as_lower_str(def.type);
	}

	std::string GetResourceGroupId(const ZoneResourceDef& def)
	{
		return as_lower_str(def.groupId);
	}

	std::string GetResourceVariant(const ZoneResourceDef& def)
	{
		return as_lower_str(def.variant);
	}

	bool IsResourceCompatibleWithZone(const ZoneResourceDef& def, const std::string& category)
	{
		if(category.empty()) {
			return true;
		}
		if(def.zoneTypes.empty()) {
			return true;
		}
		for(const std::string& zoneType : def.zoneTypes) {
			if(as_lower_str(zoneType) == as_lower_str(category)) {
				return true;
			}
		}
		return false;
	}

	wxString GetResourceTypeDisplayName(const std::string& type)
	{
		if(type == "ore") return "Ore";
		if(type == "crystal") return "Crystal";
		if(type == "plant") return "Plant";
		if(type == "fungus") return "Fungus";
		return "Other";
	}

	wxString GetResourceVariantDisplayName(const std::string& variant)
	{
		if(variant == "small") return "Small";
		if(variant == "medium") return "Medium";
		if(variant == "large") return "Large";
		if(variant == "default") return "Default";
		return wxstr(variant);
	}
}

ZoneConfigDialog::ZoneConfigDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Zone Configuration", wxDefaultPosition, wxSize(860, 760),
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	editor(editor),
	currentIndex(-1)
{
	// Copy configs and resource definitions from map
	configs = editor.getMap().zoneConfigs;
	resourceDefs = editor.getMap().zoneResourceDefs;

	wxBoxSizer* topSizer = newd wxBoxSizer(wxHORIZONTAL);

	// Left panel: zone list + add/remove
	wxBoxSizer* leftSizer = newd wxBoxSizer(wxVERTICAL);
	leftSizer->Add(newd wxStaticText(this, wxID_ANY, "Filter zones by type:"), 0, wxBOTTOM, 4);
	filter_choice = newd wxChoice(
		this, ZONE_CONFIG_FILTER, wxDefaultPosition, wxSize(220, -1),
		BuildZoneCategoryChoices("All types")
	);
	filter_choice->SetSelection(0);
	leftSizer->Add(filter_choice, 0, wxEXPAND | wxBOTTOM, 8);

	leftSizer->Add(newd wxStaticText(this, wxID_ANY, "Zones:"), 0, wxBOTTOM, 4);
	zone_listbox = newd wxListBox(this, ZONE_CONFIG_LIST, wxDefaultPosition, wxSize(220, 300));
	leftSizer->Add(zone_listbox, 1, wxEXPAND);

	leftSizer->Add(newd wxStaticText(this, wxID_ANY, "Create from marker waypoint:"), 0, wxTOP, 8);
	waypoint_picker = newd wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(220, -1));
	leftSizer->Add(waypoint_picker, 0, wxEXPAND | wxTOP, 2);

	wxBoxSizer* btnSizer = newd wxBoxSizer(wxHORIZONTAL);
	btnSizer->Add(newd wxButton(this, ZONE_CONFIG_ADD, "Create zone"), 1, wxRIGHT, 4);
	btnSizer->Add(newd wxButton(this, ZONE_CONFIG_REMOVE, "Delete zone"), 1);
	leftSizer->Add(btnSizer, 0, wxEXPAND | wxTOP, 4);

	topSizer->Add(leftSizer, 0, wxEXPAND | wxALL, 8);

	// Right panel: zone config fields
	wxBoxSizer* rightSizer = newd wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* grid = newd wxFlexGridSizer(2, 4, 4);
	grid->AddGrowableCol(1);

	grid->Add(newd wxStaticText(this, wxID_ANY, "Marker waypoint:"), 0, wxALIGN_CENTER_VERTICAL);
	marker_picker = newd wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(200, -1));
	grid->Add(marker_picker, 1, wxEXPAND);
	marker_picker->SetToolTip(
		"Select any unassigned waypoint. Missing markers can be replaced directly here."
	);
	marker_picker->Bind(wxEVT_CHOICE, &ZoneConfigDialog::OnMarkerChanged, this);

	grid->Add(newd wxStaticText(this, wxID_ANY, "Display Name:"), 0, wxALIGN_CENTER_VERTICAL);
	display_name_field = newd wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(200, -1));
	grid->Add(display_name_field, 1, wxEXPAND);

	grid->Add(newd wxStaticText(this, wxID_ANY, "Zone type:"), 0, wxALIGN_CENTER_VERTICAL);
	category_choice = newd wxChoice(
		this, ZONE_CONFIG_CATEGORY, wxDefaultPosition, wxSize(200, -1),
		BuildZoneCategoryChoices("Select a type")
	);
	category_choice->SetSelection(0);
	grid->Add(category_choice, 1, wxEXPAND);

	grid->Add(newd wxStaticText(this, wxID_ANY, "Difficulty:"), 0, wxALIGN_CENTER_VERTICAL);
	difficulty_field = newd wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(200, -1));
	grid->Add(difficulty_field, 1, wxEXPAND);

	grid->Add(newd wxStaticText(this, wxID_ANY, "Music:"), 0, wxALIGN_CENTER_VERTICAL);
	music_field = newd wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(200, -1));
	grid->Add(music_field, 1, wxEXPAND);

	grid->Add(newd wxStaticText(this, wxID_ANY, "Included tiles:"), 0, wxALIGN_CENTER_VERTICAL);
	area_label = newd wxStaticText(this, wxID_ANY, "—", wxDefaultPosition, wxSize(200, -1));
	grid->Add(area_label, 1, wxEXPAND);

	rightSizer->Add(grid, 0, wxEXPAND);

	rightSizer->Add(newd wxStaticText(this, wxID_ANY, "Included areas and floors:"), 0, wxTOP, 8);
	area_list = newd wxListBox(this, ZONE_CONFIG_AREA_LIST, wxDefaultPosition, wxSize(260, 90));
	rightSizer->Add(area_list, 0, wxEXPAND | wxTOP, 2);
	area_remove_button = newd wxButton(
		this, ZONE_CONFIG_AREA_REMOVE, "Remove selected additional area"
	);
	rightSizer->Add(area_remove_button, 0, wxALIGN_RIGHT | wxTOP, 4);
	area_list->Bind(wxEVT_LISTBOX, [this](wxCommandEvent&) {
		area_remove_button->Enable(area_list->GetSelection() > 0);
	});

	// Resources section
	rightSizer->Add(newd wxStaticLine(this), 0, wxEXPAND | wxTOP | wxBOTTOM, 8);
	has_resources_check = newd wxCheckBox(this, ZONE_CONFIG_HAS_RESOURCES, "Has Resources");
	rightSizer->Add(has_resources_check, 0, wxBOTTOM, 4);

	wxFlexGridSizer* resGrid = newd wxFlexGridSizer(2, 4, 4);
	resGrid->AddGrowableCol(1);

	resGrid->Add(newd wxStaticText(this, wxID_ANY, "Max Nodes:"), 0, wxALIGN_CENTER_VERTICAL);
	max_nodes_spin = newd wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 0, 9999);
	resGrid->Add(max_nodes_spin);

	resGrid->Add(newd wxStaticText(this, wxID_ANY, "Min Distance:"), 0, wxALIGN_CENTER_VERTICAL);
	min_distance_spin = newd wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 0, 999);
	resGrid->Add(min_distance_spin);

	resGrid->Add(newd wxStaticText(this, wxID_ANY, "Spawn Interval (s):"), 0, wxALIGN_CENTER_VERTICAL);
	spawn_interval_spin = newd wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 0, 99999);
	resGrid->Add(spawn_interval_spin);

	rightSizer->Add(resGrid, 0, wxEXPAND);

	// Spawn table section
	rightSizer->Add(newd wxStaticText(this, wxID_ANY, "Spawn Table:"), 0, wxTOP, 6);
	spawn_list = newd wxListBox(this, ZONE_CONFIG_SPAWN_LIST, wxDefaultPosition, wxSize(200, 80));
	rightSizer->Add(spawn_list, 0, wxEXPAND | wxTOP, 2);

	resource_filter_label = newd wxStaticText(this, wxID_ANY, "");
	resource_filter_label->SetForegroundColour(wxColour(80, 80, 80));
	rightSizer->Add(resource_filter_label, 0, wxTOP | wxBOTTOM, 4);

	wxFlexGridSizer* resourceSelectorSizer = newd wxFlexGridSizer(2, 4, 4);
	resourceSelectorSizer->AddGrowableCol(1);

	resourceSelectorSizer->Add(
		newd wxStaticText(this, wxID_ANY, "Resource type:"), 0, wxALIGN_CENTER_VERTICAL
	);
	resource_type_picker = newd wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(220, -1));
	resourceSelectorSizer->Add(resource_type_picker, 1, wxEXPAND);

	resourceSelectorSizer->Add(
		newd wxStaticText(this, wxID_ANY, "Resource:"), 0, wxALIGN_CENTER_VERTICAL
	);
	resource_group_picker = newd wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(220, -1));
	resourceSelectorSizer->Add(resource_group_picker, 1, wxEXPAND);

	resourceSelectorSizer->Add(
		newd wxStaticText(this, wxID_ANY, "Variant:"), 0, wxALIGN_CENTER_VERTICAL
	);
	resource_variant_picker = newd wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(220, -1));
	resourceSelectorSizer->Add(resource_variant_picker, 1, wxEXPAND);
	rightSizer->Add(resourceSelectorSizer, 0, wxEXPAND | wxTOP, 4);

	wxBoxSizer* spawnAddSizer = newd wxBoxSizer(wxHORIZONTAL);
	spawnAddSizer->AddStretchSpacer();
	spawnAddSizer->Add(newd wxStaticText(this, wxID_ANY, "Weight:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	chance_spin = newd wxSpinCtrl(this, wxID_ANY, "10", wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 1, 9999);
	spawnAddSizer->Add(chance_spin, 0, wxRIGHT, 4);
	spawnAddSizer->Add(newd wxButton(this, ZONE_CONFIG_SPAWN_ADD, "Add"), 0, wxRIGHT, 2);
	spawnAddSizer->Add(newd wxButton(this, ZONE_CONFIG_SPAWN_REMOVE, "Remove"), 0);
	rightSizer->Add(spawnAddSizer, 0, wxEXPAND | wxTOP, 4);

	resource_type_picker->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
		RefreshResourceGroups();
	});
	resource_group_picker->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
		RefreshResourceVariants();
	});

	topSizer->Add(rightSizer, 1, wxEXPAND | wxALL, 8);

	// Main layout
	wxBoxSizer* mainSizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* infoLabel = newd wxStaticText(this, wxID_ANY,
		"A zone has one marker waypoint and may include multiple connected painted areas on any floor.\n"
		"To extend it, paint the same type elsewhere, right-click that area, and choose "
		"\"Add this area to a zone...\".");
	infoLabel->SetForegroundColour(wxColour(80, 80, 80));
	mainSizer->Add(infoLabel, 0, wxALL, 8);

	mainSizer->Add(topSizer, 1, wxEXPAND);

	wxBoxSizer* okCancelSizer = newd wxBoxSizer(wxHORIZONTAL);
	okCancelSizer->AddStretchSpacer();
	okCancelSizer->Add(newd wxButton(this, wxID_OK, "OK"), 0, wxRIGHT, 4);
	okCancelSizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), 0);
	mainSizer->Add(okCancelSizer, 0, wxEXPAND | wxALL, 8);

	SetSizer(mainSizer);

	RefreshResourceControls();

	// Disable right panel until a zone is selected
	marker_picker->Disable();
	display_name_field->Disable();
	category_choice->Disable();
	difficulty_field->Disable();
	music_field->Disable();
	has_resources_check->Disable();
	max_nodes_spin->Disable();
	min_distance_spin->Disable();
	spawn_interval_spin->Disable();
	spawn_list->Disable();
	area_list->Disable();
	area_remove_button->Disable();
	resource_type_picker->Disable();
	resource_group_picker->Disable();
	resource_variant_picker->Disable();
	chance_spin->Disable();

	RefreshList();
	RefreshWaypointPicker();
	SetMinSize(wxSize(760, 700));
	Centre(wxBOTH);
}

ZoneConfigDialog::~ZoneConfigDialog()
{
}

void ZoneConfigDialog::RefreshList()
{
	zone_listbox->Clear();
	visibleZoneIndices.clear();

	const int filterIndex = filter_choice ? filter_choice->GetSelection() : 0;
	const std::string filterCategory =
		filterIndex > 0 && filterIndex < ZoneCategoryCount ? ZoneCategoryValues[filterIndex] : "";

	int selectedListIndex = wxNOT_FOUND;
	for(size_t configIndex = 0; configIndex < configs.size(); ++configIndex) {
		const ZoneConfig& zc = configs[configIndex];
		if(!filterCategory.empty() && zc.category != filterCategory) {
			continue;
		}

		std::set<int> floors;
		for(const Position& anchor : getZoneAreaAnchors(editor.getMap(), zc)) {
			floors.insert(anchor.z);
		}

		wxString label = wxstr(zc.displayName.empty() ? zc.name : zc.displayName);
		label += "  [" + wxstr(getZoneCategoryDisplayName(zc.category)) + "]";
		if(!floors.empty()) {
			label += wxString::Format("  - %d floor%s", static_cast<int>(floors.size()),
				floors.size() == 1 ? "" : "s");
		}
		zone_listbox->Append(label);
		visibleZoneIndices.push_back(static_cast<int>(configIndex));
		if(static_cast<int>(configIndex) == currentIndex) {
			selectedListIndex = static_cast<int>(visibleZoneIndices.size()) - 1;
		}
	}
	if(selectedListIndex != wxNOT_FOUND) {
		zone_listbox->SetSelection(selectedListIndex);
	}
}

void ZoneConfigDialog::RefreshWaypointPicker()
{
	waypoint_picker->Clear();

	// Collect names already used by configs
	std::set<std::string> usedNames;
	for(const auto& zc : configs) {
		usedNames.insert(as_lower_str(zc.name));
	}

	// Add only waypoints not already configured
	const Waypoints& wps = editor.getMap().waypoints;
	for(auto it = wps.begin(); it != wps.end(); ++it) {
		if(usedNames.find(as_lower_str(it->first)) == usedNames.end()) {
			waypoint_picker->Append(wxstr(it->first));
		}
	}

	if(waypoint_picker->GetCount() > 0)
		waypoint_picker->SetSelection(0);
}

void ZoneConfigDialog::RefreshMarkerPicker(int index)
{
	marker_picker->Clear();
	markerWaypointNames.clear();
	if(index < 0 || index >= static_cast<int>(configs.size())) {
		return;
	}

	const std::string currentName = configs[index].name;
	const std::string currentNameLower = as_lower_str(currentName);
	std::set<std::string> namesUsedByOtherZones;
	for(size_t configIndex = 0; configIndex < configs.size(); ++configIndex) {
		if(static_cast<int>(configIndex) != index) {
			namesUsedByOtherZones.insert(as_lower_str(configs[configIndex].name));
		}
	}

	bool currentMarkerFound = false;
	int selectedIndex = wxNOT_FOUND;
	const Waypoints& waypoints = editor.getMap().waypoints;
	for(auto waypoint = waypoints.begin(); waypoint != waypoints.end(); ++waypoint) {
		const std::string name = waypoint->first;
		const std::string lowerName = as_lower_str(name);
		if(namesUsedByOtherZones.find(lowerName) != namesUsedByOtherZones.end()) {
			continue;
		}

		marker_picker->Append(wxstr(name));
		markerWaypointNames.push_back(name);
		if(lowerName == currentNameLower) {
			currentMarkerFound = true;
			selectedIndex = static_cast<int>(markerWaypointNames.size()) - 1;
		}
	}

	if(!currentMarkerFound && !currentName.empty()) {
		marker_picker->Insert("[Missing] " + wxstr(currentName), 0);
		markerWaypointNames.insert(markerWaypointNames.begin(), currentName);
		selectedIndex = 0;
	}
	if(selectedIndex != wxNOT_FOUND) {
		marker_picker->SetSelection(selectedIndex);
	} else if(marker_picker->GetCount() > 0) {
		marker_picker->SetSelection(0);
	}
}

void ZoneConfigDialog::LoadZoneToUI(int index)
{
	if(index < 0 || index >= (int)configs.size()) {
		RefreshMarkerPicker(-1);
		display_name_field->SetValue("");
		category_choice->SetSelection(0);
		difficulty_field->SetValue("");
		music_field->SetValue("");
		has_resources_check->SetValue(false);
		max_nodes_spin->SetValue(0);
		min_distance_spin->SetValue(0);
		spawn_interval_spin->SetValue(0);
		spawn_list->Clear();
		area_list->Clear();
		area_label->SetLabel(wxT("\u2014"));

		marker_picker->Disable();
		display_name_field->Disable();
		category_choice->Disable();
		difficulty_field->Disable();
		music_field->Disable();
		has_resources_check->Disable();
		max_nodes_spin->Disable();
		min_distance_spin->Disable();
		spawn_interval_spin->Disable();
		spawn_list->Disable();
		area_list->Disable();
		area_remove_button->Disable();
		resource_type_picker->Disable();
		resource_group_picker->Disable();
		resource_variant_picker->Disable();
		chance_spin->Disable();
		RefreshResourceControls();
		return;
	}

	const ZoneConfig& zc = configs[index];
	marker_picker->Enable();
	display_name_field->Enable();
	category_choice->Enable();
	difficulty_field->Enable();
	music_field->Enable();
	has_resources_check->Enable();
	area_list->Enable();
	area_remove_button->Disable();

	RefreshMarkerPicker(index);
	display_name_field->SetValue(wxstr(zc.displayName));

	category_choice->SetSelection(FindZoneCategoryIndex(zc.category));
	RefreshResourceControls();

	difficulty_field->SetValue(wxstr(zc.difficulty));
	music_field->SetValue(wxstr(zc.music));
	has_resources_check->SetValue(zc.hasResources);

	max_nodes_spin->Enable(zc.hasResources);
	min_distance_spin->Enable(zc.hasResources);
	spawn_interval_spin->Enable(zc.hasResources);
	spawn_list->Enable(zc.hasResources);
	resource_type_picker->Enable(zc.hasResources);
	resource_group_picker->Enable(zc.hasResources);
	resource_variant_picker->Enable(zc.hasResources);
	chance_spin->Enable(zc.hasResources);

	if(zc.hasResources) {
		max_nodes_spin->SetValue(zc.resources.maxNodes);
		min_distance_spin->SetValue(zc.resources.minDistanceBetweenNodes);
		spawn_interval_spin->SetValue(zc.resources.spawnIntervalSeconds);
	} else {
		max_nodes_spin->SetValue(0);
		min_distance_spin->SetValue(0);
		spawn_interval_spin->SetValue(0);
	}

	// Update area label
	if(!zc.category.empty()) {
		int tileCount = CountZoneTiles(zc);
		area_label->SetLabel(wxString::Format("%d tiles", tileCount));
	} else {
		area_label->SetLabel(wxT("\u2014"));
	}

	RefreshSpawnList();
	RefreshAreaList();
}

int ZoneConfigDialog::CountZoneTiles(const ZoneConfig& config) const
{
	return static_cast<int>(collectZoneTiles(editor.getMap(), config).size());
}

void ZoneConfigDialog::RefreshAreaList()
{
	area_list->Clear();
	area_remove_button->Disable();
	if(currentIndex < 0 || currentIndex >= static_cast<int>(configs.size())) {
		return;
	}

	const ZoneConfig& config = configs[currentIndex];
	const uint32_t categoryFlag = getZoneCategoryFlag(config.category);
	bool markerFound = false;
	const std::string markerName = as_lower_str(config.name);
	for(auto it = editor.getMap().waypoints.begin(); it != editor.getMap().waypoints.end(); ++it) {
		if(it->second && as_lower_str(it->second->name) == markerName) {
			const Position& position = it->second->pos;
			area_list->Append(wxString::Format(
				"Marker area - floor %d at %d, %d", position.z, position.x, position.y
			));
			const Tile* tile = editor.getMap().getTile(position);
			if(categoryFlag != 0 && (!tile || !(tile->getMapFlags() & categoryFlag))) {
				area_list->SetString(area_list->GetCount() - 1,
					area_list->GetString(area_list->GetCount() - 1) + "  [type mismatch]");
			}
			markerFound = true;
			break;
		}
	}
	if(!markerFound) {
		area_list->Append("Marker area - waypoint is missing");
	}

	for(const Position& position : config.additionalAreas) {
		area_list->Append(wxString::Format(
			"Additional area - floor %d at %d, %d", position.z, position.x, position.y
		));
		const Tile* tile = editor.getMap().getTile(position);
		if(categoryFlag != 0 && (!tile || !(tile->getMapFlags() & categoryFlag))) {
			area_list->SetString(area_list->GetCount() - 1,
				area_list->GetString(area_list->GetCount() - 1) + "  [type mismatch]");
		}
	}
}

void ZoneConfigDialog::SaveCurrentZone()
{
	if(currentIndex < 0 || currentIndex >= (int)configs.size())
		return;

	ZoneConfig& zc = configs[currentIndex];
	const int markerSelection = marker_picker->GetSelection();
	if(markerSelection >= 0 &&
		markerSelection < static_cast<int>(markerWaypointNames.size())) {
		zc.name = markerWaypointNames[markerSelection];
	}
	zc.displayName = nstr(display_name_field->GetValue());

	// Save category from dropdown
	int catSel = category_choice->GetSelection();
	zc.category = (catSel > 0 && catSel < ZoneCategoryCount) ? ZoneCategoryValues[catSel] : "";

	zc.difficulty = nstr(difficulty_field->GetValue());
	zc.music = nstr(music_field->GetValue());
	zc.hasResources = has_resources_check->GetValue();

	if(zc.hasResources) {
		zc.resources.maxNodes = max_nodes_spin->GetValue();
		zc.resources.minDistanceBetweenNodes = min_distance_spin->GetValue();
		zc.resources.spawnIntervalSeconds = spawn_interval_spin->GetValue();
		// spawnTable is managed directly by OnSpawnAdd/OnSpawnRemove
	} else {
		zc.resources.spawnTable.clear();
		zc.resources.maxNodes = 0;
		zc.resources.minDistanceBetweenNodes = 0;
		zc.resources.spawnIntervalSeconds = 0;
	}
}

void ZoneConfigDialog::OnListSelect(wxCommandEvent& event)
{
	SaveCurrentZone();
	const int listIndex = zone_listbox->GetSelection();
	currentIndex = listIndex >= 0 && listIndex < static_cast<int>(visibleZoneIndices.size()) ?
		visibleZoneIndices[listIndex] : -1;
	LoadZoneToUI(currentIndex);
}

void ZoneConfigDialog::OnFilterType(wxCommandEvent& event)
{
	SaveCurrentZone();
	RefreshList();
	if(zone_listbox->GetSelection() == wxNOT_FOUND) {
		currentIndex = visibleZoneIndices.empty() ? -1 : visibleZoneIndices.front();
		if(currentIndex >= 0) {
			zone_listbox->SetSelection(0);
		}
	}
	LoadZoneToUI(currentIndex);
}

void ZoneConfigDialog::OnAddZone(wxCommandEvent& event)
{
	if(waypoint_picker->GetCount() == 0 || waypoint_picker->GetSelection() == wxNOT_FOUND) {
		wxMessageBox("No available waypoints. Place a waypoint on the map first.", "No Waypoints", wxOK | wxICON_INFORMATION);
		return;
	}

	SaveCurrentZone();

	wxString wpName = waypoint_picker->GetStringSelection();
	ZoneConfig zc;
	zc.name = nstr(wpName);

	// Auto-detect zone category from the tile flags at the waypoint position
	Waypoint* wp = editor.getMap().waypoints.getWaypoint(nstr(wpName));
	if(wp) {
		Tile* tile = editor.getMap().getTile(wp->pos);
		if(tile) {
			zc.category = getZoneCategoryFromFlags(tile->getMapFlags());
		}
	}

	configs.push_back(zc);
	currentIndex = static_cast<int>(configs.size()) - 1;
	filter_choice->SetSelection(0);

	RefreshList();
	RefreshWaypointPicker();

	LoadZoneToUI(currentIndex);
}

void ZoneConfigDialog::OnRemoveZone(wxCommandEvent& event)
{
	if(currentIndex < 0 || currentIndex >= (int)configs.size())
		return;

	configs.erase(configs.begin() + currentIndex);
	currentIndex = -1;
	RefreshList();
	RefreshWaypointPicker();
	LoadZoneToUI(-1);
}

void ZoneConfigDialog::OnCategoryChanged(wxCommandEvent& event)
{
	SaveCurrentZone();
	const int filterIndex = filter_choice->GetSelection();
	if(filterIndex > 0 && configs[currentIndex].category != ZoneCategoryValues[filterIndex]) {
		filter_choice->SetSelection(0);
	}
	RefreshList();
	LoadZoneToUI(currentIndex);
}

void ZoneConfigDialog::OnMarkerChanged(wxCommandEvent& event)
{
	if(currentIndex < 0 || currentIndex >= static_cast<int>(configs.size())) {
		return;
	}
	const int selection = marker_picker->GetSelection();
	if(selection < 0 || selection >= static_cast<int>(markerWaypointNames.size())) {
		return;
	}

	ZoneConfig& config = configs[currentIndex];
	config.name = markerWaypointNames[selection];

	Waypoint* waypoint = editor.getMap().waypoints.getWaypoint(config.name);
	if(waypoint) {
		const Tile* tile = editor.getMap().getTile(waypoint->pos);
		if(tile) {
			const std::string detectedCategory = getZoneCategoryFromFlags(tile->getMapFlags());
			if(!detectedCategory.empty()) {
				config.category = detectedCategory;
				category_choice->SetSelection(FindZoneCategoryIndex(config.category));
			}
		}
	}

	const int filterIndex = filter_choice->GetSelection();
	if(filterIndex > 0 && config.category != ZoneCategoryValues[filterIndex]) {
		filter_choice->SetSelection(0);
	}
	RefreshWaypointPicker();
	RefreshResourceControls();
	RefreshAreaList();
	area_label->SetLabel(
		config.category.empty() ?
			wxString(wxT("\u2014")) :
			wxString::Format("%d tiles", CountZoneTiles(config))
	);
	RefreshList();
}

void ZoneConfigDialog::OnAreaRemove(wxCommandEvent& event)
{
	if(currentIndex < 0 || currentIndex >= static_cast<int>(configs.size())) {
		return;
	}
	const int areaIndex = area_list->GetSelection();
	if(areaIndex <= 0) {
		return;
	}

	std::vector<Position>& areas = configs[currentIndex].additionalAreas;
	const int additionalIndex = areaIndex - 1;
	if(additionalIndex >= 0 && additionalIndex < static_cast<int>(areas.size())) {
		areas.erase(areas.begin() + additionalIndex);
	}
	RefreshAreaList();
	area_label->SetLabel(wxString::Format("%d tiles", CountZoneTiles(configs[currentIndex])));
	RefreshList();
}

void ZoneConfigDialog::OnResourcesCheck(wxCommandEvent& event)
{
	bool checked = has_resources_check->GetValue();
	max_nodes_spin->Enable(checked);
	min_distance_spin->Enable(checked);
	spawn_interval_spin->Enable(checked);
	spawn_list->Enable(checked);
	resource_type_picker->Enable(checked);
	resource_group_picker->Enable(checked);
	resource_variant_picker->Enable(checked);
	chance_spin->Enable(checked);
}

void ZoneConfigDialog::RefreshSpawnList()
{
	spawn_list->Clear();
	if(currentIndex < 0 || currentIndex >= (int)configs.size())
		return;

	const auto& table = configs[currentIndex].resources.spawnTable;
	for(const auto& se : table) {
		wxString label = wxstr(se.resourceId);
		for(const auto& def : resourceDefs) {
			if(def.id == se.resourceId) {
				label = "[" + GetResourceTypeDisplayName(GetResourceType(def)) + "] " +
					wxstr(def.name) + " - " +
					GetResourceVariantDisplayName(GetResourceVariant(def));
				break;
			}
		}
		label += wxString::Format(" - weight %d", se.weight);
		spawn_list->Append(label);
	}
}

void ZoneConfigDialog::RefreshResourceControls()
{
	std::string selectedType;
	const int oldSelection = resource_type_picker->GetSelection();
	if(oldSelection >= 0 && oldSelection < static_cast<int>(visibleResourceTypes.size())) {
		selectedType = visibleResourceTypes[oldSelection];
	}

	resource_type_picker->Clear();
	visibleResourceTypes.clear();

	std::string category;
	if(currentIndex >= 0 && currentIndex < static_cast<int>(configs.size())) {
		category = configs[currentIndex].category;
	}
	resource_filter_label->SetLabel(
		category.empty() ?
			wxString("Showing all resources. Select a zone type to apply compatibility filtering.") :
			wxString("Compatible with this ") + wxstr(getZoneCategoryDisplayName(category)) +
				" zone. Global resources are also included."
	);

	int newSelection = wxNOT_FOUND;
	for(const ZoneResourceDef& def : resourceDefs) {
		if(!IsResourceCompatibleWithZone(def, category)) {
			continue;
		}

		const std::string type = GetResourceType(def);
		if(std::find(visibleResourceTypes.begin(), visibleResourceTypes.end(), type) !=
			visibleResourceTypes.end()) {
			continue;
		}
		resource_type_picker->Append(GetResourceTypeDisplayName(type));
		visibleResourceTypes.push_back(type);
		if(type == selectedType) {
			newSelection = static_cast<int>(visibleResourceTypes.size()) - 1;
		}
	}

	if(resource_type_picker->GetCount() > 0) {
		resource_type_picker->SetSelection(newSelection == wxNOT_FOUND ? 0 : newSelection);
	}
	RefreshResourceGroups();
}

void ZoneConfigDialog::RefreshResourceGroups()
{
	std::string selectedGroup;
	const int oldSelection = resource_group_picker->GetSelection();
	if(oldSelection >= 0 && oldSelection < static_cast<int>(visibleResourceGroups.size())) {
		selectedGroup = visibleResourceGroups[oldSelection];
	}

	resource_group_picker->Clear();
	visibleResourceGroups.clear();

	const int typeSelection = resource_type_picker->GetSelection();
	if(typeSelection < 0 || typeSelection >= static_cast<int>(visibleResourceTypes.size())) {
		RefreshResourceVariants();
		return;
	}
	const std::string& selectedType = visibleResourceTypes[typeSelection];
	const std::string category =
		currentIndex >= 0 && currentIndex < static_cast<int>(configs.size()) ?
			configs[currentIndex].category : "";

	int newSelection = wxNOT_FOUND;
	for(const ZoneResourceDef& def : resourceDefs) {
		if(GetResourceType(def) != selectedType ||
			!IsResourceCompatibleWithZone(def, category)) {
			continue;
		}

		const std::string groupId = GetResourceGroupId(def);
		if(std::find(visibleResourceGroups.begin(), visibleResourceGroups.end(), groupId) !=
			visibleResourceGroups.end()) {
			continue;
		}
		resource_group_picker->Append(wxstr(def.name));
		visibleResourceGroups.push_back(groupId);
		if(groupId == selectedGroup) {
			newSelection = static_cast<int>(visibleResourceGroups.size()) - 1;
		}
	}

	if(resource_group_picker->GetCount() > 0) {
		resource_group_picker->SetSelection(newSelection == wxNOT_FOUND ? 0 : newSelection);
	}
	RefreshResourceVariants();
}

void ZoneConfigDialog::RefreshResourceVariants()
{
	std::string selectedId;
	const int oldSelection = resource_variant_picker->GetSelection();
	if(oldSelection >= 0 && oldSelection < static_cast<int>(visibleResourceIndices.size())) {
		selectedId = resourceDefs[visibleResourceIndices[oldSelection]].id;
	}

	resource_variant_picker->Clear();
	visibleResourceIndices.clear();

	const int typeSelection = resource_type_picker->GetSelection();
	const int groupSelection = resource_group_picker->GetSelection();
	if(typeSelection < 0 || typeSelection >= static_cast<int>(visibleResourceTypes.size()) ||
		groupSelection < 0 || groupSelection >= static_cast<int>(visibleResourceGroups.size())) {
		return;
	}

	const std::string& selectedType = visibleResourceTypes[typeSelection];
	const std::string& selectedGroup = visibleResourceGroups[groupSelection];
	const std::string category =
		currentIndex >= 0 && currentIndex < static_cast<int>(configs.size()) ?
			configs[currentIndex].category : "";

	int newSelection = wxNOT_FOUND;
	for(size_t index = 0; index < resourceDefs.size(); ++index) {
		const ZoneResourceDef& def = resourceDefs[index];
		if(GetResourceType(def) != selectedType ||
			GetResourceGroupId(def) != selectedGroup ||
			!IsResourceCompatibleWithZone(def, category)) {
			continue;
		}

		resource_variant_picker->Append(
			GetResourceVariantDisplayName(GetResourceVariant(def))
		);
		visibleResourceIndices.push_back(static_cast<int>(index));
		if(def.id == selectedId) {
			newSelection = static_cast<int>(visibleResourceIndices.size()) - 1;
		}
	}

	if(resource_variant_picker->GetCount() > 0) {
		resource_variant_picker->SetSelection(newSelection == wxNOT_FOUND ? 0 : newSelection);
	}
}

void ZoneConfigDialog::OnSpawnAdd(wxCommandEvent& event)
{
	if(currentIndex < 0 || currentIndex >= (int)configs.size())
		return;
	if(resource_variant_picker->GetCount() == 0 ||
		resource_variant_picker->GetSelection() == wxNOT_FOUND)
		return;

	int sel = resource_variant_picker->GetSelection();
	if(sel < 0 || sel >= static_cast<int>(visibleResourceIndices.size()))
		return;
	const int resourceIndex = visibleResourceIndices[sel];

	ZoneResourceSpawnEntry se;
	se.resourceId = resourceDefs[resourceIndex].id;
	se.weight = chance_spin->GetValue();
	configs[currentIndex].resources.spawnTable.push_back(se);
	RefreshSpawnList();
}

void ZoneConfigDialog::OnSpawnRemove(wxCommandEvent& event)
{
	if(currentIndex < 0 || currentIndex >= (int)configs.size())
		return;

	int sel = spawn_list->GetSelection();
	if(sel == wxNOT_FOUND)
		return;

	auto& table = configs[currentIndex].resources.spawnTable;
	if(sel >= 0 && sel < (int)table.size()) {
		table.erase(table.begin() + sel);
	}
	RefreshSpawnList();
}

void ZoneConfigDialog::OnClickOK(wxCommandEvent& event)
{
	SaveCurrentZone();

	// Validate every configured marker before committing the dialog changes.
	for(int i = 0; i < (int)configs.size(); ++i) {
		if(configs[i].name.empty()) {
			wxMessageBox("Zone at index " + wxString::Format("%d", i) + " has an empty name. Please fix it.", "Error", wxOK | wxICON_ERROR);
			return;
		}
		if(configs[i].category.empty()) {
			wxMessageBox(
				"Zone \"" + wxstr(configs[i].name) +
					"\" does not have a zone type. Select a type before saving.",
				"Zone Type Required",
				wxOK | wxICON_ERROR
			);
			return;
		}
		bool markerFound = false;
		const std::string markerName = as_lower_str(configs[i].name);
		for(auto waypoint = editor.getMap().waypoints.begin();
			waypoint != editor.getMap().waypoints.end(); ++waypoint) {
			if(waypoint->second && as_lower_str(waypoint->second->name) == markerName) {
				markerFound = true;
				break;
			}
		}
		if(!markerFound) {
			wxMessageBox(
				"Zone \"" + wxstr(configs[i].name) +
					"\" no longer has its marker waypoint. Select another marker in the "
					"zone configuration or restore the missing waypoint before saving.",
				"Marker Waypoint Missing",
				wxOK | wxICON_ERROR
			);
			return;
		}
		const uint32_t categoryFlag = getZoneCategoryFlag(configs[i].category);
		for(const Position& anchor : getZoneAreaAnchors(editor.getMap(), configs[i])) {
			const Tile* tile = editor.getMap().getTile(anchor);
			if(!tile || !(tile->getMapFlags() & categoryFlag)) {
				wxMessageBox(
					"An area of zone \"" + wxstr(configs[i].name) +
						wxString::Format("\" at %d, %d, floor %d is not painted as ",
							anchor.x, anchor.y, anchor.z) +
						wxstr(getZoneCategoryDisplayName(configs[i].category)) + ".\n" +
						"Repaint that tile with the selected type or remove the additional area.",
					"Zone Area Type Mismatch",
					wxOK | wxICON_ERROR
				);
				return;
			}
		}
	}

	editor.getMap().zoneConfigs = configs;
	editor.getMap().doChange();
	EndModal(wxID_OK);
}

void ZoneConfigDialog::OnClickCancel(wxCommandEvent& event)
{
	EndModal(wxID_CANCEL);
}
