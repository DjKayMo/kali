/*
 *  xfdesktop - xfce4's desktop manager
 *
 *  Copyright (c) 2004 Brian Tarricone, <bjt23@cornell.edu>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Random portions taken from or inspired by the original xfdesktop for xfce4:
 *     Copyright (C) 2002-2003 Jasper Huijsmans (huysmans@users.sourceforge.net)
 *     Copyright (C) 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <glib.h>
#include <gmodule.h>
#include <gtk/gtk.h>

#include <libxfce4util/i18n.h>

#include "menu.h"
#ifdef USE_DESKTOP_MENU
#include "desktop-menu-stub.h"
#include "xfdesktop-common.h"
#endif

#ifdef USE_DESKTOP_MENU
GModule *module_desktop_menu = NULL;
XfceDesktopMenu *desktop_menu = NULL;
#endif

#ifdef USE_DESKTOP_MENU
static void
_stop_menu_module() {
	if(module_desktop_menu && desktop_menu) {
		xfce_desktop_menu_stop_autoregen(desktop_menu);
		xfce_desktop_menu_destroy(desktop_menu);
	}
	desktop_menu = NULL;
	if(module_desktop_menu)
		xfce_desktop_menu_stub_cleanup(module_desktop_menu);
	module_desktop_menu = NULL;
}

static gboolean
_start_menu_module()
{
	if(!module_desktop_menu)
		module_desktop_menu = xfce_desktop_menu_stub_init();
	if(module_desktop_menu && !desktop_menu) {
		desktop_menu = xfce_desktop_menu_new(NULL, TRUE);
		xfce_desktop_menu_start_autoregen(desktop_menu, 10);
		return TRUE;
	} else if(!module_desktop_menu) {
		g_warning(_("%s: Unable to initialise menu module. Right-click menu will be unavailable.\n"), PACKAGE);
		return FALSE;
	} else
		return TRUE;
}
#endif

void
popup_desktop_menu(GdkScreen *gscreen, gint button, guint32 time)
{
#if USE_DESKTOP_MENU
	GtkWidget *menu_widget;
	
	if(!module_desktop_menu)
		return;
	
	if(!desktop_menu)
		desktop_menu = xfce_desktop_menu_new(NULL, FALSE);
	else if(xfce_desktop_menu_need_update(desktop_menu))
		xfce_desktop_menu_force_regen(desktop_menu);
	
	if(desktop_menu) {
		menu_widget = xfce_desktop_menu_get_widget(desktop_menu);
		gtk_menu_set_screen(GTK_MENU(menu_widget), gscreen);
		gtk_menu_popup(GTK_MENU(menu_widget), NULL, NULL, NULL, NULL,
				button, time);
	}
#endif
}

void
menu_init(McsClient *mcs_client)
{	
#ifdef USE_DESKTOP_MENU
	McsSetting *setting = NULL;
	
	if(!mcs_client)
		_start_menu_module();
	else {
		if(MCS_SUCCESS == mcs_client_get_setting(mcs_client, "showdm",
				BACKDROP_CHANNEL, &setting))
		{
			if(setting->data.v_int)
				_start_menu_module();
			else
				_stop_menu_module();
			mcs_setting_free(setting);
			setting = NULL;
		} else
			_start_menu_module();
		
		if(module_desktop_menu && desktop_menu) {
			if(MCS_SUCCESS == mcs_client_get_setting(mcs_client, "showdmi",
					BACKDROP_CHANNEL, &setting))
			{
				if(!setting->data.v_int)
					xfce_desktop_menu_set_show_icons(desktop_menu, FALSE);
				mcs_setting_free(setting);
				setting = NULL;
			}
		}
	}
#endif
}

gboolean
menu_settings_changed(McsClient *client, McsAction action, McsSetting *setting,
		gpointer user_data)
{
#ifdef USE_DESKTOP_MENU
	McsSetting *setting1 = NULL;
	
	switch(action) {
		case MCS_ACTION_NEW:
		case MCS_ACTION_CHANGED:
			if(!strcmp(setting->name, "showdm")) {
				if(setting->data.v_int && !module_desktop_menu) {
					_start_menu_module();
					if(desktop_menu
							&& MCS_SUCCESS == mcs_client_get_setting(client,
								"showdmi", BACKDROP_CHANNEL, &setting1))
					{
						if(!setting1->data.v_int)
							xfce_desktop_menu_set_show_icons(desktop_menu, FALSE);
						mcs_setting_free(setting1);
						setting1 = NULL;
					}
				} else if(!setting->data.v_int && module_desktop_menu)
					_stop_menu_module();
				return TRUE;
			} else if(!strcmp(setting->name, "showdmi")) {
				if(desktop_menu)
					xfce_desktop_menu_set_show_icons(desktop_menu, setting->data.v_int);
				return TRUE;
			}
			break;
		
		case MCS_ACTION_DELETED:
			break;
	}
#endif
	
	return FALSE;	
}

void
menu_cleanup()
{
#ifdef USE_DESKTOP_MENU
	_stop_menu_module();
#endif
}
