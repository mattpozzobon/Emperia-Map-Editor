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

#include "map_window.h"
#include "gui.h"
#include "sprites.h"
#include "editor.h"

#include <wx/combobox.h>
#include <wx/spinctrl.h>

MapWindow::MapWindow(wxWindow* parent, Editor& editor) :
	wxPanel(parent, PANE_MAIN),
	editor(editor),
	floor_control(nullptr),
	zoom_control(nullptr),
	updating_view_controls(false),
	replaceItemsDialog(nullptr)
{
	int GL_settings[3];
	GL_settings[0] = WX_GL_RGBA;
	GL_settings[1] = WX_GL_DOUBLEBUFFER;
	GL_settings[2] = 0;
	canvas = newd MapCanvas(this, editor, GL_settings);

	vScroll = newd MapScrollBar(this, MAP_WINDOW_VSCROLL, wxVERTICAL, canvas);
	wxPanel* view_controls = newd wxPanel(this, wxID_ANY);
	hScroll = newd MapScrollBar(view_controls, MAP_WINDOW_HSCROLL, wxHORIZONTAL, canvas);

	gem = newd DCButton(this, MAP_WINDOW_GEM, wxDefaultPosition, DC_BTN_NORMAL, RENDER_SIZE_16x16, EDITOR_SPRITE_SELECTION_GEM);

	wxBoxSizer* control_sizer = newd wxBoxSizer(wxHORIZONTAL);
	control_sizer->Add(hScroll, 1, wxEXPAND | wxRIGHT, FROM_DIP(this, 8));

	wxStaticText* floor_label = newd wxStaticText(view_controls, wxID_ANY, "Floor");
	floor_control = newd wxSpinCtrl(view_controls, wxID_ANY, wxEmptyString, wxDefaultPosition,
		FROM_DIP(this, wxSize(58, -1)), wxSP_ARROW_KEYS, rme::MapMinLayer, rme::MapMaxLayer);
	floor_control->SetName("Current map floor");
	floor_control->SetToolTip("Current floor (0-15). Ctrl+mouse wheel also changes floors.");

	wxStaticText* zoom_label = newd wxStaticText(view_controls, wxID_ANY, "Zoom");
	wxArrayString zoom_levels;
	zoom_levels.Add("25%");
	zoom_levels.Add("50%");
	zoom_levels.Add("75%");
	zoom_levels.Add("100%");
	zoom_levels.Add("150%");
	zoom_levels.Add("200%");
	zoom_levels.Add("400%");
	zoom_control = newd wxComboBox(view_controls, wxID_ANY, wxEmptyString, wxDefaultPosition,
		FROM_DIP(this, wxSize(76, -1)), zoom_levels, wxCB_DROPDOWN | wxTE_PROCESS_ENTER);
	zoom_control->SetName("Map zoom percentage");
	zoom_control->SetToolTip("Choose or type a zoom percentage from 4% to 800%.");

	control_sizer->Add(floor_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FROM_DIP(this, 4));
	control_sizer->Add(floor_control, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FROM_DIP(this, 8));
	control_sizer->Add(zoom_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FROM_DIP(this, 4));
	control_sizer->Add(zoom_control, 0, wxALIGN_CENTER_VERTICAL);
	view_controls->SetSizer(control_sizer);

	wxFlexGridSizer* topsizer = newd wxFlexGridSizer(2, 0, 0);

	topsizer->AddGrowableCol(0);
	topsizer->AddGrowableRow(0);

	topsizer->Add(canvas, wxSizerFlags(1).Expand());
	topsizer->Add(vScroll, wxSizerFlags(1).Expand());
	topsizer->Add(view_controls, wxSizerFlags(1).Expand());
	topsizer->Add(gem, wxSizerFlags(1));

	SetSizerAndFit(topsizer);

	floor_control->Bind(wxEVT_SPINCTRL, &MapWindow::OnFloorChanged, this);
	zoom_control->Bind(wxEVT_COMBOBOX, &MapWindow::OnZoomChanged, this);
	zoom_control->Bind(wxEVT_TEXT_ENTER, &MapWindow::OnZoomChanged, this);
	UpdateViewControls();
}

MapWindow::~MapWindow()
{
	floor_control->Unbind(wxEVT_SPINCTRL, &MapWindow::OnFloorChanged, this);
	zoom_control->Unbind(wxEVT_COMBOBOX, &MapWindow::OnZoomChanged, this);
	zoom_control->Unbind(wxEVT_TEXT_ENTER, &MapWindow::OnZoomChanged, this);
}

void MapWindow::UpdateViewControls()
{
	if(!floor_control || !zoom_control || updating_view_controls) {
		return;
	}

	updating_view_controls = true;
	floor_control->SetValue(canvas->GetFloor());
	const int zoom_percent = std::clamp(static_cast<int>(std::lround(100.0 / canvas->GetZoom())), 4, 800);
	zoom_control->ChangeValue(wxString::Format("%d%%", zoom_percent));
	updating_view_controls = false;
}

void MapWindow::OnFloorChanged(wxSpinEvent& event)
{
	if(!updating_view_controls) {
		g_gui.ChangeFloor(event.GetValue());
		canvas->SetFocus();
	}
}

void MapWindow::OnZoomChanged(wxCommandEvent& event)
{
	if(updating_view_controls) {
		return;
	}

	wxString value = zoom_control->GetValue();
	value.Replace("%", "");
	value.Trim(true).Trim(false);
	long percentage = 0;
	if(!value.ToLong(&percentage) || percentage < 4 || percentage > 800) {
		UpdateViewControls();
		return;
	}

	canvas->SetZoom(100.0 / static_cast<double>(percentage));
	canvas->SetFocus();
}

void MapWindow::ShowReplaceItemsDialog(bool selectionOnly)
{
	if(replaceItemsDialog)
		return;

	replaceItemsDialog = new ReplaceItemsDialog(this, selectionOnly);
	replaceItemsDialog->Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(MapWindow::OnReplaceItemsDialogClose), NULL, this);
	replaceItemsDialog->Show();
}

void MapWindow::CloseReplaceItemsDialog()
{
	if(replaceItemsDialog)
		replaceItemsDialog->Close();
}

void MapWindow::OnReplaceItemsDialogClose(wxCloseEvent& event)
{
	if(replaceItemsDialog) {
		replaceItemsDialog->Disconnect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(MapWindow::OnReplaceItemsDialogClose), NULL, this);
		replaceItemsDialog->Destroy();
		replaceItemsDialog = nullptr;
	}
}

void MapWindow::SetSize(int x, int y, bool center)
{
	if(x == 0 || y == 0) return;

	int windowSizeX;
	int windowSizeY;

	canvas->GetSize(&windowSizeX, &windowSizeY);

	hScroll->SetScrollbar(center? (x - windowSizeX)/2 : hScroll->GetThumbPosition(), windowSizeX / x,  x, windowSizeX / x);
	vScroll->SetScrollbar(center? (y - windowSizeY)/2 : vScroll->GetThumbPosition(), windowSizeY / y,  y, windowSizeX / y);
	//wxPanel::SetSize(x, y);
}

void MapWindow::UpdateScrollbars(int nx, int ny)
{
	// nx and ny are size of this window
	hScroll->SetScrollbar(hScroll->GetThumbPosition(), nx / std::max(1, hScroll->GetRange()), std::max(1, hScroll->GetRange()), 96);
	vScroll->SetScrollbar(vScroll->GetThumbPosition(), ny / std::max(1, vScroll->GetRange()), std::max(1, vScroll->GetRange()), 96);
}

void MapWindow::UpdateDialogs(bool show)
{
	if(replaceItemsDialog)
		replaceItemsDialog->Show(show);
}

void MapWindow::GetViewStart(int* x, int* y)
{
	*x = hScroll->GetThumbPosition();
	*y = vScroll->GetThumbPosition();
}

void MapWindow::GetViewSize(int* x, int* y)
{
	canvas->GetSize(x, y);
	*x *= canvas->GetContentScaleFactor();
	*y *= canvas->GetContentScaleFactor();
}

void MapWindow::FitToMap()
{
	const Map& map = editor.getMap();
	SetSize(map.getWidth() * rme::TileSize, map.getHeight() * rme::TileSize, true);
}

Position MapWindow::GetScreenCenterPosition()
{
	int x, y;
	canvas->GetScreenCenter(&x, &y);
	return Position(x, y, canvas->GetFloor());
}

void MapWindow::SetScreenCenterPosition(const Position& position, bool showIndicator)
{
	if(!position.isValid())
		return;

	int x = position.x * rme::TileSize;
	int y = position.y * rme::TileSize;
	int z = position.z;
	if(position.z < 8) {
		// Compensate for floor offset above ground
		x -= (rme::MapGroundLayer - z) * rme::TileSize;
		y -= (rme::MapGroundLayer - z) * rme::TileSize;
	}

	const Position& center = GetScreenCenterPosition();
	if(previous_position != center) {
		previous_position.x = center.x;
		previous_position.y = center.y;
		previous_position.z = center.z;
	}

	Scroll(x, y, true);
	canvas->ChangeFloor(z);

	if(showIndicator) {
		canvas->ShowPositionIndicator(position);
		Refresh();
	}
}

void MapWindow::GoToPreviousCenterPosition()
{
	SetScreenCenterPosition(previous_position, true);
}

void MapWindow::Scroll(int x, int y, bool center)
{
	if(center) {
		int windowSizeX, windowSizeY;

		canvas->GetSize(&windowSizeX, &windowSizeY);
		x -= int((windowSizeX * g_gui.GetCurrentZoom()) / 2.0);
		y -= int((windowSizeY * g_gui.GetCurrentZoom()) / 2.0);
	}

	hScroll->SetThumbPosition(x);
	vScroll->SetThumbPosition(y);
	g_gui.UpdateMinimap();
}

void MapWindow::ScrollRelative(int x, int y)
{
	hScroll->SetThumbPosition(hScroll->GetThumbPosition()+x);
	vScroll->SetThumbPosition(vScroll->GetThumbPosition()+y);
	g_gui.UpdateMinimap();
}

void MapWindow::OnGem(wxCommandEvent& WXUNUSED(event))
{
	g_gui.SwitchMode();
}

void MapWindow::OnSize(wxSizeEvent& event)
{
	UpdateScrollbars(event.GetSize().GetWidth(), event.GetSize().GetHeight());
	event.Skip();
}

void MapWindow::OnScroll(wxScrollEvent& event)
{
	Refresh();
}

void MapWindow::OnScrollLineDown(wxScrollEvent& event)
{
	if(event.GetOrientation() == wxHORIZONTAL)
		ScrollRelative(96,0);
	else
		ScrollRelative(0,96);
	Refresh();
}

void MapWindow::OnScrollLineUp(wxScrollEvent& event)
{
	if(event.GetOrientation() == wxHORIZONTAL)
		ScrollRelative(-96,0);
	else
		ScrollRelative(0,-96);
	Refresh();
}

void MapWindow::OnScrollPageDown(wxScrollEvent& event)
{
	if(event.GetOrientation() == wxHORIZONTAL)
		ScrollRelative(5*96,0);
	else
		ScrollRelative(0,5*96);
	Refresh();
}

void MapWindow::OnScrollPageUp(wxScrollEvent& event)
{
	if(event.GetOrientation() == wxHORIZONTAL)
		ScrollRelative(-5*96,0);
	else
		ScrollRelative(0,-5*96);
	Refresh();
}
