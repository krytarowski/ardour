/*
    Copyright (C) 2000 Paul Davis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifdef WAF_BUILD
#include "gtk2ardour-config.h"
#endif

/* this file exists solely to break compilation dependencies that
   would connect changes to the mixer or editor objects.
*/

#include <cstdio>

#include "pbd/error.h"

#include "ardour/session.h"

#include "actions.h"
#include "ardour_ui.h"
#include "public_editor.h"
#include "meterbridge.h"
#include "mixer_ui.h"
#include "keyboard.h"
#include "splash.h"
#include "route_params_ui.h"
#include "opts.h"
#include "utils.h"

#include "i18n.h"

using namespace Gtk;
using namespace PBD;

namespace ARDOUR {
	class Session;
	class Route;
}

using namespace ARDOUR;

void
ARDOUR_UI::we_have_dependents ()
{
	install_actions ();
	ProcessorBox::register_actions ();
	keyboard->setup_keybindings ();
	editor->setup_tooltips ();
	editor->UpdateAllTransportClocks.connect (sigc::mem_fun (*this, &ARDOUR_UI::update_transport_clocks));

	/* catch up on tabbable state */

	std::cerr << "Tab catch up\n";
	
	tabbable_state_change (*editor);
	tabbable_state_change (*mixer);
	tabbable_state_change (*rc_option_editor);
	
	std::cerr << "Tab catch done\n";
	
	/* all actions are defined */

	ActionManager::enable_accelerators ();
	ActionManager::load_menus (ARDOUR_COMMAND_LINE::menus_file);

	editor->track_mixer_selection ();
	mixer->track_editor_selection ();

	/* catch up on parameters */
	
	boost::function<void (std::string)> pc (boost::bind (&ARDOUR_UI::parameter_changed, this, _1));
	Config->map_parameters (pc);

	ARDOUR_UI_UTILS::reset_dpi ();
}

void
ARDOUR_UI::connect_dependents_to_session (ARDOUR::Session *s)
{
	DisplaySuspender ds;
	BootMessage (_("Setup Editor"));
	editor->set_session (s);
	BootMessage (_("Setup Mixer"));
	mixer->set_session (s);
	meterbridge->set_session (s);

	/* its safe to do this now */

	BootMessage (_("Reload Session History"));
	s->restore_history ("");
}

/** The main editor window has been closed */
gint
ARDOUR_UI::exit_on_main_window_close (GdkEventAny * /*ev*/)
{
#ifdef TOP_MENUBAR
	/* just hide the window, and return - the top menu stays up */
	editor->hide ();
	return TRUE;
#else
	/* time to get out of here */
	finish();
	return TRUE;
#endif
}

GtkNotebook*
ARDOUR_UI::tab_window_root_drop (GtkNotebook* src,
				 GtkWidget* w,
				 gint x,
				 gint y,
				 gpointer)
{
	using namespace std;
	Gtk::Notebook* nb = 0;
	Gtk::Window* win = 0;
	Gtkmm2ext::Tabbable* tabbable = 0;


	if (w == GTK_WIDGET(editor->contents().gobj())) {
		tabbable = editor;
	} else if (w == GTK_WIDGET(mixer->contents().gobj())) {
		tabbable = mixer;
	} else if (w == GTK_WIDGET(rc_option_editor->contents().gobj())) {
		tabbable = rc_option_editor;
	} else {
		return 0;
	}

	nb = tabbable->tab_root_drop ();
	win = tabbable->own_window ();

	if (nb) {
		win->move (x, y);
		win->show_all ();
		win->present ();
		return nb->gobj();
	}
	
	return 0; /* what was that? */
}
