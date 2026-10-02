//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "view_overlay_panel.h"
#include "application.h"
#include "gui.h"

ViewOverlayPanel::ViewOverlayPanel(wxWindow* parent) :
	wxPanel(parent, wxID_ANY)
{
	auto* rootSizer = newd wxBoxSizer(wxVERTICAL);

	auto* heading = newd wxStaticText(this, wxID_ANY, "Map display");
	const wxFont headingFont = heading->GetFont().Bold().Larger();
	heading->SetFont(headingFont);
	rootSizer->Add(heading, 0, wxLEFT | wxRIGHT | wxTOP, FROM_DIP(this, 12));

	auto* description = newd wxStaticText(
		this,
		wxID_ANY,
		"Toggle overlays without leaving the map.\nKeyboard shortcuts stay synchronized."
	);
	description->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
	rootSizer->Add(description, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM,
		FROM_DIP(this, 12));

	AddSection(rootSizer, "Zones and areas", {
		{ "Special tile zones", "E", Config::SHOW_SPECIAL_TILES },
		{ "World zones", "Z", Config::SHOW_ZONES },
		{ "Spawn areas", "S", Config::SHOW_SPAWNS },
		{ "Houses", "Ctrl+H", Config::SHOW_HOUSES },
	});

	AddSection(rootSizer, "Labels and entities", {
		{ "Names and tooltips", "Y", Config::SHOW_TOOLTIPS },
		{ "Creatures", "F", Config::SHOW_CREATURES },
	});

	rootSizer->AddStretchSpacer();
	SetSizer(rootSizer);
	RefreshState();
}

void ViewOverlayPanel::AddSection(
	wxSizer* parent,
	const wxString& title,
	std::initializer_list<std::tuple<wxString, wxString, Config::Key>> entries)
{
	auto* section = newd wxStaticBoxSizer(wxVERTICAL, this, title);
	for(const auto& entry : entries) {
		const wxString& label = std::get<0>(entry);
		const wxString& shortcut = std::get<1>(entry);
		const Config::Key setting = std::get<2>(entry);

		auto* row = newd wxBoxSizer(wxHORIZONTAL);
		auto* checkbox = newd wxCheckBox(section->GetStaticBox(), wxID_ANY, label);
		auto* shortcutLabel = newd wxStaticText(
			section->GetStaticBox(), wxID_ANY, shortcut,
			wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT
		);
		shortcutLabel->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
		checkbox->SetToolTip(label + " (" + shortcut + ")");
		checkbox->Bind(wxEVT_CHECKBOX, [this, setting](wxCommandEvent& event) {
			g_settings.setInteger(setting, event.IsChecked());
			g_gui.root->UpdateMenubar();
			g_gui.RefreshView();
		});

		controls[setting] = checkbox;
		row->Add(checkbox, 1, wxALIGN_CENTER_VERTICAL);
		row->Add(shortcutLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FROM_DIP(this, 8));
		section->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM,
			FROM_DIP(this, 6));
	}
	parent->Add(section, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM,
		FROM_DIP(this, 10));
}

void ViewOverlayPanel::RefreshState()
{
	for(const auto& entry : controls) {
		entry.second->SetValue(g_settings.getBoolean(entry.first));
	}
}
