//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_VIEW_OVERLAY_PANEL_H_
#define RME_VIEW_OVERLAY_PANEL_H_

#include "settings.h"

class ViewOverlayPanel final : public wxPanel
{
public:
	explicit ViewOverlayPanel(wxWindow* parent);
	void RefreshState();

private:
	void AddSection(wxSizer* parent, const wxString& title,
		std::initializer_list<std::tuple<wxString, wxString, Config::Key>> entries);
	std::map<Config::Key, wxCheckBox*> controls;
};

#endif
