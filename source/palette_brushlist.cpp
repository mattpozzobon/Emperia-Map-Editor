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

#include "palette_brushlist.h"
#include "gui.h"
#include "brush.h"
#include "raw_brush.h"

#include <wx/combobox.h>
#include <wx/settings.h>

namespace
{
	wxString BrushSearchLabel(const Brush* brush)
	{
		if(brush->isRaw()) {
			return wxstr(brush->getName());
		}
		return wxString::Format("%s (ID: %d)", wxstr(brush->getName()), brush->getLookID());
	}

	wxString BrushSearchItemID(const Brush* brush)
	{
		if(brush->isRaw()) {
			return wxString::Format("%u", static_cast<const RAWBrush*>(brush)->getItemID());
		}
		return wxString::Format("%d", brush->getLookID());
	}
}

Brush* BrushBoxInterface::GetBrush(size_t index) const
{
	return index < brushes.size() ? brushes[index] : nullptr;
}

// ============================================================================
// Brush Palette Panel
// A common class for terrain/doodad/item/raw palette

BEGIN_EVENT_TABLE(BrushPalettePanel, PalettePanel)
	EVT_CHOICEBOOK_PAGE_CHANGING(wxID_ANY, BrushPalettePanel::OnSwitchingPage)
	EVT_CHOICEBOOK_PAGE_CHANGED(wxID_ANY, BrushPalettePanel::OnPageChanged)
END_EVENT_TABLE()

BrushPalettePanel::BrushPalettePanel(wxWindow* parent, const TilesetContainer& tilesets, TilesetCategoryType category, wxWindowID id) :
	PalettePanel(parent, id),
	palette_type(category),
	choicebook(nullptr),
	search_control(nullptr),
	search_candidates(),
	search_results(),
	updating_search(false),
	restore_hotkeys_on_blur(false),
	size_panel(nullptr)
{
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	search_control = newd wxComboBox(
		this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		0, nullptr, wxCB_DROPDOWN | wxTE_PROCESS_ENTER);
	search_control->SetHint("Find item by name or ID");
	search_control->SetName("Item finder");
	search_control->SetToolTip("Type an item name or ID, then choose a result.");
	search_control->Bind(wxEVT_TEXT, &BrushPalettePanel::OnSearchChanged, this);
	search_control->Bind(wxEVT_COMBOBOX, &BrushPalettePanel::OnSearchSelected, this);
	search_control->Bind(wxEVT_SET_FOCUS, &BrushPalettePanel::OnSearchFocus, this);
	search_control->Bind(wxEVT_KILL_FOCUS, &BrushPalettePanel::OnSearchBlur, this);
	search_control->Bind(wxEVT_KEY_DOWN, &BrushPalettePanel::OnSearchKeyDown, this);
	topsizer->Add(search_control, 0, wxEXPAND | wxALL, FROM_DIP(this, 6));

	// Create the tileset panel
	wxSizer* ts_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Tileset");
	wxChoicebook* tmp_choicebook = newd wxChoicebook(this, wxID_ANY);
	tmp_choicebook->SetMinSize(FROM_DIP(this, wxSize(170, 180)));
	ts_sizer->Add(tmp_choicebook, 1, wxEXPAND);
	topsizer->Add(ts_sizer, 1, wxEXPAND);

	for(TilesetContainer::const_iterator iter = tilesets.begin(); iter != tilesets.end(); ++iter) {
		const TilesetCategory* tcg = iter->second->getCategory(category);
		if(tcg && tcg->size() > 0) {
			BrushPanel* panel = newd BrushPanel(tmp_choicebook);
			panel->AssignTileset(tcg);
			tmp_choicebook->AddPage(panel, wxstr(iter->second->name));
		}
	}

	std::set<Brush*> unique_search_candidates;
	for(const auto& entry : g_brushes.getMap()) {
		Brush* brush = entry.second;
		if(brush && brush->visibleInPalette() && unique_search_candidates.insert(brush).second) {
			search_candidates.push_back(brush);
		}
	}

	UpdateSearchResults(wxEmptyString);
	SetSizerAndFit(topsizer);

	choicebook = tmp_choicebook;
}

BrushPalettePanel::~BrushPalettePanel()
{
	if(restore_hotkeys_on_blur) {
		g_gui.EnableHotkeys();
	}
	search_control->Unbind(wxEVT_TEXT, &BrushPalettePanel::OnSearchChanged, this);
	search_control->Unbind(wxEVT_COMBOBOX, &BrushPalettePanel::OnSearchSelected, this);
	search_control->Unbind(wxEVT_SET_FOCUS, &BrushPalettePanel::OnSearchFocus, this);
	search_control->Unbind(wxEVT_KILL_FOCUS, &BrushPalettePanel::OnSearchBlur, this);
	search_control->Unbind(wxEVT_KEY_DOWN, &BrushPalettePanel::OnSearchKeyDown, this);
}

void BrushPalettePanel::InvalidateContents()
{
	for(size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		panel->InvalidateContents();
	}
	PalettePanel::InvalidateContents();
}

void BrushPalettePanel::LoadCurrentContents()
{
	wxWindow* page = choicebook->GetCurrentPage();
	BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
	if(panel) {
		panel->OnSwitchIn();
	}
	PalettePanel::LoadCurrentContents();
}

void BrushPalettePanel::LoadAllContents()
{
	for(size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		panel->LoadContents();
	}
	PalettePanel::LoadAllContents();
}

PaletteType BrushPalettePanel::GetType() const
{
	return palette_type;
}

void BrushPalettePanel::SetListType(BrushListType ltype)
{
	if(!choicebook) return;
	for(size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		panel->SetListType(ltype);
	}
}

void BrushPalettePanel::SetListType(wxString ltype)
{
	if(!choicebook) return;
	for(size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		panel->SetListType(ltype);
	}
}

Brush* BrushPalettePanel::GetSelectedBrush() const
{
	if(!choicebook) return nullptr;
	wxWindow* page = choicebook->GetCurrentPage();
	BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
	Brush* res = nullptr;
	if(panel) {
		for(ToolBarList::const_iterator iter = tool_bars.begin(); iter != tool_bars.end(); ++iter) {
			res = (*iter)->GetSelectedBrush();
			if(res) return res;
		}
		res = panel->GetSelectedBrush();
	}
	return res;
}

void BrushPalettePanel::SelectFirstBrush()
{
	if(!choicebook) return;
	wxWindow* page = choicebook->GetCurrentPage();
	BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
	panel->SelectFirstBrush();
}

bool BrushPalettePanel::SelectBrush(const Brush* whatbrush)
{
	if(!choicebook) {
		return false;
	}

	BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetCurrentPage());
	if(!panel) {
		return false;
	}

	for(PalettePanel* toolBar : tool_bars) {
		if(toolBar->SelectBrush(whatbrush)) {
			panel->SelectBrush(nullptr);
			return true;
		}
	}

	if(panel->SelectBrush(whatbrush)) {
		for(PalettePanel* toolBar : tool_bars) {
			toolBar->SelectBrush(nullptr);
		}
		return true;
	}

	for(size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		if((int)iz == choicebook->GetSelection()) {
			continue;
		}

		panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		if(panel && panel->SelectBrush(whatbrush)) {
			choicebook->ChangeSelection(iz);
			for(PalettePanel* toolBar : tool_bars) {
				toolBar->SelectBrush(nullptr);
			}
			return true;
		}
	}
	return false;
}

void BrushPalettePanel::OnSwitchingPage(wxChoicebookEvent& event)
{
	event.Skip();
	if(!choicebook) {
		return;
	}
	BrushPanel* old_panel = dynamic_cast<BrushPanel*>(choicebook->GetCurrentPage());
	if(old_panel) {
		old_panel->OnSwitchOut();
		for(ToolBarList::iterator iter = tool_bars.begin(); iter != tool_bars.end(); ++iter) {
			Brush* tmp = (*iter)->GetSelectedBrush();
			if(tmp) {
				remembered_brushes[old_panel] = tmp;
			}
		}
	}

	wxWindow* page = choicebook->GetPage(event.GetSelection());
	BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
	if(panel) {
		panel->OnSwitchIn();
		for(ToolBarList::iterator iter = tool_bars.begin(); iter != tool_bars.end(); ++iter) {
			(*iter)->SelectBrush(remembered_brushes[panel]);
		}
	}
}

void BrushPalettePanel::OnPageChanged(wxChoicebookEvent& event)
{
	if(!choicebook) {
		return;
	}
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void BrushPalettePanel::OnSearchChanged(wxCommandEvent& event)
{
	// Selecting a dropdown row also emits a text event on some platforms.
	// Keep the existing rows intact so the following combo event can resolve it.
	if(!updating_search && search_control->FindString(event.GetString()) == wxNOT_FOUND) {
		const wxString query = event.GetString();
		UpdateSearchResults(query);
		if(!query.Strip(wxString::both).IsEmpty() && !search_results.empty()) {
			// Opening a native editable combo can automatically select its first
			// row and replace the typed text. Restore the query after the native
			// popup has finished opening so subsequent keys append normally.
			search_control->CallAfter([this, query]() {
				if(!search_control->HasFocus()) {
					return;
				}
				updating_search = true;
				search_control->Popup();
				search_control->SetSelection(wxNOT_FOUND);
				search_control->ChangeValue(query);
				search_control->SetInsertionPointEnd();
				updating_search = false;
			});
		}
	}
}

void BrushPalettePanel::OnSearchSelected(wxCommandEvent& event)
{
	if(updating_search) {
		return;
	}
	const int selection = event.GetSelection();
	if(selection != wxNOT_FOUND && static_cast<size_t>(selection) < search_results.size()) {
		Brush* brush = search_results[selection];
		// Let the native combo finish closing before changing palette pages.
		search_control->CallAfter([this, brush]() {
			SelectSearchResult(brush);
		});
	}
}

void BrushPalettePanel::UpdateSearchResults(const wxString& query)
{
	struct SearchMatch {
		Brush* brush;
		wxString label;
		int rank;
	};

	const wxString normalized_query = query.Lower().Strip(wxString::both);
	std::vector<SearchMatch> matches;
	for(Brush* brush : search_candidates) {
		const wxString name = wxstr(brush->getName()).Lower();
		const wxString item_id = BrushSearchItemID(brush);
		const wxString brush_id = wxString::Format("%u", brush->getID());
		int rank = 4;
		if(normalized_query.IsEmpty()) {
			rank = 3;
		} else if(item_id == normalized_query || brush_id == normalized_query) {
			rank = 0;
		} else if(name.StartsWith(normalized_query)) {
			rank = 1;
		} else if(name.Find(normalized_query) != wxNOT_FOUND ||
			item_id.Find(normalized_query) != wxNOT_FOUND ||
			brush_id.Find(normalized_query) != wxNOT_FOUND) {
			rank = 2;
		} else {
			continue;
		}
		matches.push_back({brush, BrushSearchLabel(brush), rank});
	}

	std::stable_sort(matches.begin(), matches.end(), [](const SearchMatch& left, const SearchMatch& right) {
		if(left.rank != right.rank) {
			return left.rank < right.rank;
		}
		return left.label.CmpNoCase(right.label) < 0;
	});

	constexpr size_t maximum_results = 200;
	updating_search = true;
	search_control->Freeze();
	search_control->Clear();
	search_results.clear();
	for(size_t index = 0; index < std::min(matches.size(), maximum_results); ++index) {
		search_control->Append(matches[index].label);
		search_results.push_back(matches[index].brush);
	}
	search_control->ChangeValue(query);
	search_control->SetInsertionPointEnd();
	search_control->Thaw();
	updating_search = false;
}

void BrushPalettePanel::SelectSearchResult(Brush* brush)
{
	if(!brush) {
		return;
	}
	search_control->ChangeValue(BrushSearchLabel(brush));
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush(brush, brush->isRaw() ? TILESET_ITEM : TILESET_UNKNOWN);
}

void BrushPalettePanel::OnSearchFocus(wxFocusEvent& event)
{
	restore_hotkeys_on_blur = g_gui.AreHotkeysEnabled();
	if(restore_hotkeys_on_blur) {
		g_gui.DisableHotkeys();
	}
	event.Skip();
}

void BrushPalettePanel::OnSearchBlur(wxFocusEvent& event)
{
	if(restore_hotkeys_on_blur) {
		g_gui.EnableHotkeys();
		restore_hotkeys_on_blur = false;
	}
	event.Skip();
}

void BrushPalettePanel::OnSearchKeyDown(wxKeyEvent& event)
{
	if(event.GetKeyCode() == WXK_ESCAPE && !search_control->GetValue().IsEmpty()) {
		UpdateSearchResults(wxEmptyString);
		return;
	}
	if(event.GetKeyCode() == WXK_RETURN && !search_results.empty()) {
		const int selection = search_control->GetSelection();
		const size_t result_index = selection == wxNOT_FOUND ? 0 : static_cast<size_t>(selection);
		if(result_index < search_results.size()) {
			SelectSearchResult(search_results[result_index]);
		}
		return;
	}
	event.Skip();
}

void BrushPalettePanel::OnSwitchIn() {
	LoadCurrentContents();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SetBrushSizeInternal(last_brush_size);
	OnUpdateBrushSize(g_gui.GetBrushShape(), last_brush_size);
}

// ============================================================================
// Brush Panel
// A container of brush buttons

BEGIN_EVENT_TABLE(BrushPanel, wxPanel)
	// Listbox style
	EVT_LISTBOX(wxID_ANY, BrushPanel::OnClickListBoxRow)
END_EVENT_TABLE()

BrushPanel::BrushPanel(wxWindow *parent) :
	wxPanel(parent, wxID_ANY),
	tileset(nullptr),
	brushbox(nullptr),
	loaded(false),
	list_type(BRUSHLIST_LISTBOX)
{
	sizer = newd wxBoxSizer(wxVERTICAL);
	SetSizerAndFit(sizer);
}

BrushPanel::~BrushPanel()
{
	////
}

void BrushPanel::AssignTileset(const TilesetCategory* _tileset)
{
	if(_tileset != tileset) {
		InvalidateContents();
		tileset = _tileset;
	}
}

void BrushPanel::SetListType(BrushListType ltype)
{
	if(list_type != ltype) {
		InvalidateContents();
		list_type = ltype;
	}
}

void BrushPanel::SetListType(wxString ltype)
{
	if(ltype == "small icons") {
		SetListType(BRUSHLIST_SMALL_ICONS);
	} else if(ltype == "large icons") {
		SetListType(BRUSHLIST_LARGE_ICONS);
	} else if(ltype == "listbox") {
		SetListType(BRUSHLIST_LISTBOX);
	} else if(ltype == "textlistbox") {
		SetListType(BRUSHLIST_TEXT_LISTBOX);
	}
}

void BrushPanel::InvalidateContents()
{
	sizer->Clear(true);
	loaded = false;
	brushbox = nullptr;
}

void BrushPanel::LoadContents()
{
	if(loaded) {
		return;
	}
	loaded = true;
	ASSERT(tileset != nullptr);

	switch (list_type) {
		case BRUSHLIST_LARGE_ICONS:
			brushbox = newd BrushIconBox(this, tileset, tileset->brushlist, RENDER_SIZE_32x32);
			break;
		case BRUSHLIST_SMALL_ICONS:
			brushbox = newd BrushIconBox(this, tileset, tileset->brushlist, RENDER_SIZE_16x16);
			break;
		case BRUSHLIST_LISTBOX:
			brushbox = newd BrushListBox(this, tileset, tileset->brushlist);
			break;
		default:
			break;
	}
	ASSERT(brushbox != nullptr);
	sizer->Add(brushbox->GetSelfWindow(), 1, wxEXPAND);
	Fit();
	brushbox->SelectFirstBrush();
}

void BrushPanel::SelectFirstBrush()
{
	if(loaded) {
		ASSERT(brushbox != nullptr);
		brushbox->SelectFirstBrush();
	}
}

Brush* BrushPanel::GetSelectedBrush() const
{
	if(loaded) {
		ASSERT(brushbox != nullptr);
		return brushbox->GetSelectedBrush();
	}

	if(tileset && tileset->size() > 0) {
		return tileset->brushlist[0];
	}
	return nullptr;
}

bool BrushPanel::SelectBrush(const Brush* whatbrush)
{
	if(loaded) {
		//std::cout << loaded << std::endl;
		//std::cout << brushbox << std::endl;
		ASSERT(brushbox != nullptr);
		return brushbox->SelectBrush(whatbrush);
	}

	for(BrushVector::const_iterator iter = tileset->brushlist.begin(); iter != tileset->brushlist.end(); ++iter) {
		if(*iter == whatbrush) {
			LoadContents();
			return brushbox->SelectBrush(whatbrush);
		}
	}
	return false;
}

void BrushPanel::OnSwitchIn()
{
	LoadContents();
}

void BrushPanel::OnSwitchOut()
{
	////
}

void BrushPanel::OnClickListBoxRow(wxCommandEvent& event)
{
	ASSERT(tileset->getType() >= TILESET_UNKNOWN && tileset->getType() <= TILESET_HOUSE);
	// We just notify the GUI of the action, it will take care of everything else
	ASSERT(brushbox);
	size_t n = event.GetSelection();
	Brush* brush = brushbox->GetBrush(n);


	wxWindow* w = this;
	while((w = w->GetParent()) && dynamic_cast<PaletteWindow*>(w) == nullptr);

	if(w)
		g_gui.ActivatePalette(static_cast<PaletteWindow*>(w));

	if(brush) {
		g_gui.SelectBrush(brush, tileset->getType());
	}
}

// ============================================================================
// BrushIconBox

BEGIN_EVENT_TABLE(BrushIconBox, wxScrolledWindow)
	// Listbox style
	EVT_TOGGLEBUTTON(wxID_ANY, BrushIconBox::OnClickBrushButton)
END_EVENT_TABLE()

BrushIconBox::BrushIconBox(wxWindow *parent, const TilesetCategory *_tileset, const BrushVector& brushes, RenderSize rsz) :
	wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL),
	BrushBoxInterface(_tileset, brushes),
	icon_size(rsz)
{
	ASSERT(tileset->getType() >= TILESET_UNKNOWN && tileset->getType() <= TILESET_HOUSE);
	int width;
	if(icon_size == RENDER_SIZE_32x32) {
		width = std::max(g_settings.getInteger(Config::PALETTE_COL_COUNT) / 2 + 1, 1);
	} else {
		width = std::max(g_settings.getInteger(Config::PALETTE_COL_COUNT) + 1, 1);
	}

	// Create buttons
	wxSizer* stacksizer = newd wxBoxSizer(wxVERTICAL);
	wxSizer* rowsizer = nullptr;
	int item_counter = 0;
	for(BrushVector::const_iterator iter = this->brushes.begin(); iter != this->brushes.end(); ++iter) {
		ASSERT(*iter);
		++item_counter;

		if(!rowsizer) {
			rowsizer = newd wxBoxSizer(wxHORIZONTAL);
		}

		BrushButton* bb = newd BrushButton(this, *iter, rsz);
		rowsizer->Add(bb);
		brush_buttons.push_back(bb);

		if(item_counter % width == 0) { // newd row
			stacksizer->Add(rowsizer);
			rowsizer = nullptr;
		}
	}
	if(rowsizer) {
		stacksizer->Add(rowsizer);
	}

	const int density = std::clamp(g_settings.getInteger(Config::UI_DENSITY), 0, 2);
	const int scroll_units[] = {16, 20, 24};
	const int scroll_unit = FROM_DIP(this, scroll_units[density]);
	SetScrollbars(scroll_unit, scroll_unit, 8, item_counter/width, 0, 0);
	SetSizer(stacksizer);
}

BrushIconBox::~BrushIconBox()
{
	////
}

void BrushIconBox::SelectFirstBrush()
{
	if(!brush_buttons.empty()) {
		DeselectAll();
		brush_buttons[0]->SetValue(true);
		EnsureVisible((size_t)0);
	}
}

Brush* BrushIconBox::GetSelectedBrush() const
{
	if(!tileset) {
		return nullptr;
	}

	for(std::vector<BrushButton*>::const_iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		if((*it)->GetValue()) {
			return (*it)->brush;
		}
	}
	return nullptr;
}

bool BrushIconBox::SelectBrush(const Brush* whatbrush)
{
	DeselectAll();
	for(std::vector<BrushButton*>::iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		if((*it)->brush == whatbrush) {
			(*it)->SetValue(true);
			EnsureVisible(*it);
			return true;
		}
	}
	return false;
}

void BrushIconBox::DeselectAll()
{
	for(std::vector<BrushButton*>::iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		(*it)->SetValue(false);
	}
}

void BrushIconBox::EnsureVisible(BrushButton* btn)
{
	int windowSizeX, windowSizeY;
	GetVirtualSize(&windowSizeX, &windowSizeY);

	int scrollUnitX;
	int scrollUnitY;
	GetScrollPixelsPerUnit(&scrollUnitX, &scrollUnitY);

	wxRect rect = btn->GetRect();
	int y;
	CalcUnscrolledPosition(0, rect.y, nullptr, &y);

	int maxScrollPos = windowSizeY / scrollUnitY;
	int scrollPosY = std::min(maxScrollPos, (y / scrollUnitY));

	int startScrollPosY;
	GetViewStart(nullptr, &startScrollPosY);

	int clientSizeX, clientSizeY;
	GetClientSize(&clientSizeX, &clientSizeY);
	int endScrollPosY = startScrollPosY + clientSizeY / scrollUnitY;

	if(scrollPosY < startScrollPosY || scrollPosY > endScrollPosY){
		//only scroll if the button isnt visible
		Scroll(-1, scrollPosY);
	}
}

void BrushIconBox::EnsureVisible(size_t n)
{
	EnsureVisible(brush_buttons[n]);
}

void BrushIconBox::OnClickBrushButton(wxCommandEvent& event)
{
	wxObject* obj = event.GetEventObject();
	BrushButton* btn = dynamic_cast<BrushButton*>(obj);
	if(btn) {
		wxWindow* w = this;
		while((w = w->GetParent()) && dynamic_cast<PaletteWindow*>(w) == nullptr);
		if(w)
			g_gui.ActivatePalette(static_cast<PaletteWindow*>(w));
		g_gui.SelectBrush(btn->brush, tileset->getType());
	}
}

// ============================================================================
// BrushListBox

BEGIN_EVENT_TABLE(BrushListBox, wxVListBox)
	EVT_KEY_DOWN(BrushListBox::OnKey)
END_EVENT_TABLE()

BrushListBox::BrushListBox(wxWindow* parent, const TilesetCategory* tileset, const BrushVector& brushes) :
	wxVListBox(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLB_SINGLE),
	BrushBoxInterface(tileset, brushes)
{
	SetItemCount(this->brushes.size());
}

BrushListBox::~BrushListBox()
{
	////
}

void BrushListBox::SelectFirstBrush()
{
	if(!brushes.empty()) {
		SetSelection(0);
		wxWindow::ScrollLines(-1);
	}
}

Brush* BrushListBox::GetSelectedBrush() const
{
	if(!tileset) {
		return nullptr;
	}

	int n = GetSelection();
	if(n != wxNOT_FOUND && static_cast<size_t>(n) < brushes.size()) {
		return brushes[n];
	} else if(!brushes.empty()) {
		return brushes[0];
	}
	return nullptr;
}

bool BrushListBox::SelectBrush(const Brush* whatbrush)
{
	for(size_t n = 0; n < brushes.size(); ++n) {
		if(brushes[n] == whatbrush) {
			SetSelection(n);
			return true;
		}
	}
	return false;
}

void BrushListBox::OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const
{
	ASSERT(n < brushes.size());
	Sprite* spr = g_gui.gfx.getSprite(brushes[n]->getLookID());
	if(spr) {
		spr->DrawTo(&dc, SPRITE_SIZE_32x32, rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
	}
	const wxSystemColour text_colour = IsSelected(n) ?
		wxSYS_COLOUR_HIGHLIGHTTEXT : wxSYS_COLOUR_WINDOWTEXT;
	dc.SetTextForeground(wxSystemSettings::GetColour(text_colour));
	const wxString name = wxstr(brushes[n]->getName());
	wxCoord text_width;
	wxCoord text_height;
	dc.GetTextExtent(name, &text_width, &text_height);
	const int density = std::clamp(g_settings.getInteger(Config::UI_DENSITY), 0, 2);
	const int text_offsets[] = {36, 40, 44};
	const int text_x = rect.GetX() + FROM_DIP(this, text_offsets[density]);
	const int text_y = rect.GetY() + std::max(0, (rect.GetHeight() - text_height) / 2);
	dc.DrawText(name, text_x, text_y);
}

wxCoord BrushListBox::OnMeasureItem(size_t n) const
{
	const int density = std::clamp(g_settings.getInteger(Config::UI_DENSITY), 0, 2);
	const int row_heights[] = {28, 36, 44};
	return FROM_DIP(this, row_heights[density]);
}

void BrushListBox::OnKey(wxKeyEvent& event)
{
	switch(event.GetKeyCode()) {
		case WXK_UP:
		case WXK_DOWN:
		case WXK_LEFT:
		case WXK_RIGHT:
			if(g_settings.getInteger(Config::LISTBOX_EATS_ALL_EVENTS)) {
		case WXK_PAGEUP:
		case WXK_PAGEDOWN:
		case WXK_HOME:
		case WXK_END:
			event.Skip(true);
			} else {
			[[fallthrough]];
		default:
			if(g_gui.GetCurrentTab() != nullptr) {
				g_gui.GetCurrentMapTab()->GetEventHandler()->AddPendingEvent(event);
			}
		}
	}
}
