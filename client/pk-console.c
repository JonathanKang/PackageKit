/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <dbus/dbus-glib.h>

#include <pk-debug.h>
#include <pk-task-client.h>

/**
 * pk_console_package_cb:
 **/
static gchar *
pk_console_make_space (const gchar *data, guint length, guint *extra)
{
	gint size;
	gchar *padding;
	*extra = 0;

	size = length;
	if (data != NULL) {
		size = (length - strlen(data));
		if (size < 0) {
			*extra = -size;
			size = 0;
		}
	}
	padding = g_strnfill (size, ' ');
	return padding;
}

/**
 * pk_console_package_cb:
 **/
static void
pk_console_package_cb (PkTaskClient *tclient, guint value, const gchar *package_id, const gchar *summary, gpointer data)
{
	PkPackageIdent *ident;
	PkPackageIdent *spacing;
	const gchar *installed;
	guint extra;

	if (value == 0) {
		installed = "no  ";
	} else {
		installed = "yes ";
	}

	spacing = pk_task_package_ident_new ();
	ident = pk_task_package_ident_from_string (package_id);

	/* these numbers are guesses */
	extra = 0;
	spacing->name = pk_console_make_space (ident->name, 20, &extra);
	spacing->version = pk_console_make_space (ident->version, 15-extra, &extra);
	spacing->arch = pk_console_make_space (ident->arch, 7-extra, &extra);
	spacing->data = pk_console_make_space (ident->data, 7-extra, &extra);

	/* pretty print */
	g_print ("%s %s%s %s%s %s%s %s%s %s\n", installed,
		 ident->name, spacing->name,
		 ident->version, spacing->version,
		 ident->arch, spacing->arch,
		 ident->data, spacing->data,
		 summary);

	/* free all the data */
	pk_task_package_ident_free (ident);
	pk_task_package_ident_free (spacing);
}

/**
 * pk_console_percentage_changed_cb:
 **/
static void
pk_console_percentage_changed_cb (PkTaskClient *tclient, guint percentage, gpointer data)
{
	g_print ("%i%%\n", percentage);
}

/**
 * pk_console_usage:
 **/
static void
pk_console_usage (const gchar *error)
{
	if (error != NULL) {
		g_print ("Error: %s\n", error);
	}
	g_print ("usage:\n");
	g_print ("  pkcon search name power\n");
	g_print ("  pkcon search details power\n");
	g_print ("  pkcon search group system\n");
	g_print ("  pkcon search file libc.so.3\n");
	g_print ("  pkcon sync install gimp;2:2.4.0-0.rc1.1.fc8;i386;development\n");
	g_print ("  pkcon install gimp;2:2.4.0-0.rc1.1.fc8;i386;development\n");
	g_print ("  pkcon sync update\n");
	g_print ("  pkcon refresh\n");
	g_print ("  pkcon force-refresh\n");
	g_print ("  pkcon getdeps gimp;2:2.4.0-0.rc1.1.fc8;i386;development\n");
	g_print ("  pkcon getdesc gimp;2:2.4.0-0.rc1.1.fc8;i386;development\n");
	g_print ("  pkcon debug checkupdate\n");
}

/**
 * pk_console_parse_multiple_commands:
 **/
static void
pk_console_parse_multiple_commands (PkTaskClient *tclient, GPtrArray *array)
{
	const gchar *mode;
	const gchar *value = NULL;
	const gchar *details = NULL;
	guint remove;

	mode = g_ptr_array_index (array, 0);
	if (array->len > 1) {
		value = g_ptr_array_index (array, 1);
	}
	if (array->len > 2) {
		details = g_ptr_array_index (array, 2);
	}
	remove = 1;

	if (strcmp (mode, "search") == 0) {
		if (value == NULL) {
			pk_console_usage ("you need to specify a search type");
			remove = 1;
			goto out;
		} else if (strcmp (value, "name") == 0) {
			if (details == NULL) {
				pk_console_usage ("you need to specify a search term");
				remove = 2;
				goto out;
			} else {
				pk_task_client_set_sync (tclient, TRUE);
				pk_task_client_search_name (tclient, "none", details);
				remove = 3;
			}
		} else if (strcmp (value, "details") == 0) {
			if (details == NULL) {
				pk_console_usage ("you need to specify a search term");
				remove = 2;
				goto out;
			} else {
				pk_task_client_set_sync (tclient, TRUE);
				pk_task_client_search_details (tclient, "none", details);
				remove = 3;
			}
		} else if (strcmp (value, "group") == 0) {
			if (details == NULL) {
				pk_console_usage ("you need to specify a search term");
				remove = 2;
				goto out;
			} else {
				pk_task_client_set_sync (tclient, TRUE);
				pk_task_client_search_group (tclient, "none", details);
				remove = 3;
			}
		} else if (strcmp (value, "file") == 0) {
			if (details == NULL) {
				pk_console_usage ("you need to specify a search term");
				remove = 2;
				goto out;
			} else {
				pk_task_client_set_sync (tclient, TRUE);
				pk_task_client_search_file (tclient, "none", details);
				remove = 3;
			}
		} else {
			pk_console_usage ("invalid search type");
		}
	} else if (strcmp (mode, "install") == 0) {
		if (value == NULL) {
			pk_console_usage ("you need to specify a package to install");
			remove = 1;
			goto out;
		} else {
			pk_task_client_install_package (tclient, value);
			remove = 2;
		}
	} else if (strcmp (mode, "remove") == 0) {
		if (value == NULL) {
			pk_console_usage ("you need to specify a package to remove");
			remove = 1;
			goto out;
		} else {
			pk_task_client_remove_package (tclient, value, FALSE);
			remove = 2;
		}
	} else if (strcmp (mode, "getdeps") == 0) {
		if (value == NULL) {
			pk_console_usage ("you need to specify a package to find the deps for");
			goto out;
		} else {
			pk_task_client_set_sync (tclient, TRUE);
			pk_task_client_get_deps (tclient, value);
			remove = 2;
		}
	} else if (strcmp (mode, "getdesc") == 0) {
		if (value == NULL) {
			pk_console_usage ("you need to specify a package to find the description for");
			goto out;
		} else {
			pk_task_client_set_sync (tclient, TRUE);
			pk_task_client_get_description (tclient, value);
			remove = 2;
		}
	} else if (strcmp (mode, "debug") == 0) {
		pk_debug_init (TRUE);
	} else if (strcmp (mode, "verbose") == 0) {
		pk_debug_init (TRUE);
	} else if (strcmp (mode, "update") == 0) {
		pk_task_client_update_system (tclient);
	} else if (strcmp (mode, "refresh") == 0) {
		pk_task_client_refresh_cache (tclient, FALSE);
	} else if (strcmp (mode, "force-refresh") == 0) {
		pk_task_client_refresh_cache (tclient, TRUE);
	} else if (strcmp (mode, "sync") == 0) {
		pk_task_client_set_sync (tclient, TRUE);
	} else if (strcmp (mode, "async") == 0) {
		pk_task_client_set_sync (tclient, FALSE);
	} else if (strcmp (mode, "checkupdate") == 0) {
		pk_task_client_set_sync (tclient, TRUE);
		pk_task_client_get_updates (tclient);
	} else {
		pk_console_usage ("option not yet supported");
	}

out:
	/* remove the right number of items from the pointer index */
	g_ptr_array_remove_index (array, 0);
	if (remove > 1) {
		g_ptr_array_remove_index (array, 0);
	}
	if (remove > 2) {
		g_ptr_array_remove_index (array, 0);
	}
}

/**
 * pk_console_tidy_up_sync:
 **/
static void
pk_console_tidy_up_sync (PkTaskClient *tclient)
{
	PkTaskClientPackageItem *item;
	GPtrArray *packages;
	guint i;
	guint length;
	gboolean sync;

	sync = pk_task_client_get_sync (tclient);
	if (sync == TRUE) {
		packages = pk_task_client_get_package_buffer (tclient);
		length = packages->len;
		for (i=0; i<length; i++) {
			item = g_ptr_array_index (packages, i);
			pk_console_package_cb (tclient, item->value, item->package_id, item->summary, NULL);
		}
	}
}

/**
 * pk_console_finished_cb:
 **/
static void
pk_console_finished_cb (PkTaskClient *tclient, PkTaskStatus status, guint runtime, gpointer data)
{
	g_print ("Runtime was %i seconds\n", runtime);
}

/**
 * pk_console_error_code_cb:
 **/
static void
pk_console_error_code_cb (PkTaskClient *tclient, PkTaskErrorCode error_code, const gchar *details, gpointer data)
{
	g_print ("Error: %s : %s\n", pk_task_error_code_to_text (error_code), details);
}

/**
 * main:
 **/
int
main (int argc, char *argv[])
{
	DBusGConnection *system_connection;
	GError *error = NULL;
	PkTaskClient *tclient;
	GPtrArray *array;
	guint i;

	if (! g_thread_supported ()) {
		g_thread_init (NULL);
	}
	dbus_g_thread_init ();
	g_type_init ();

	if (!g_thread_supported ())
		g_thread_init (NULL);
	dbus_g_thread_init ();

	/* check dbus connections, exit if not valid */
	system_connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (error) {
		pk_warning ("%s", error->message);
		g_error_free (error);
		g_error ("This program cannot start until you start the dbus system service.");
	}

	if (argc < 2) {
		pk_console_usage (NULL);
		return 1;
	}

	tclient = pk_task_client_new ();
	g_signal_connect (tclient, "package",
			  G_CALLBACK (pk_console_package_cb), NULL);
	g_signal_connect (tclient, "percentage-changed",
			  G_CALLBACK (pk_console_percentage_changed_cb), NULL);
	g_signal_connect (tclient, "finished",
			  G_CALLBACK (pk_console_finished_cb), NULL);
	g_signal_connect (tclient, "error-code",
			  G_CALLBACK (pk_console_error_code_cb), NULL);

	/* add argv to a pointer array */
	array = g_ptr_array_new ();
	for (i=1; i<argc; i++) {
		g_ptr_array_add (array, (gpointer) argv[i]);
	}
	/* process all the commands */
	while (array->len > 0) {
		pk_console_parse_multiple_commands (tclient, array);
	}

	/* if we are sync then print the package lists */
	pk_console_tidy_up_sync (tclient);

	g_ptr_array_free (array, TRUE);
	g_object_unref (tclient);

	return 0;
}
