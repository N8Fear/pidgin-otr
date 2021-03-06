/*
 *  Off-the-Record Messaging plugin for pidgin
 *  Copyright (C) 2004-2012  Ian Goldberg, Rob Smits,
 *                           Chris Alexander, Willy Lew,
 *                           Nikita Borisov
 *                           <otr@cypherpunks.ca>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/* config.h */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* system headers */
#include <gtk/gtk.h>

/* libgcrypt headers */
#include <gcrypt.h>

/* libotr headers */
#include <libotr/privkey.h>

/* purple headers */
#include "util.h"
#include "account.h"
#include "notify.h"
#include "gtkutils.h"

#ifdef ENABLE_NLS
/* internationalisation header */
#include <glib/gi18n-lib.h>
#else
#define _(x) (x)
#define N_(x) (x)
#endif

/* purple-otr headers */
#include "dialogs.h"
#include "ui.h"
#include "otr-plugin.h"

struct otrsettingsdata {
    GtkWidget *enablebox;
    GtkWidget *automaticbox;
    GtkWidget *onlyprivatebox;
    GtkWidget *avoidloggingotrbox;
};

struct otroptionsdata {
    GtkWidget *showotrbutton;
};

static struct {
    GtkWidget *accountmenu;
    GtkWidget *fprint_label;
    GtkWidget *generate_button;
    GtkWidget *scrollwin;
    GtkWidget *keylist;
    gint sortcol, sortdir;
    Fingerprint *selected_fprint;
    GtkWidget *connect_button;
    GtkWidget *disconnect_button;
    GtkWidget *forget_button;
    GtkWidget *verify_button;
    struct otrsettingsdata os;
    struct otroptionsdata oo;
} ui_layout;

static const gchar *trust_states[] = {
    N_("Not private"),
    N_("Unverified"),
    N_("Private"),
    N_("Finished")
};

static void account_menu_changed_cb(GtkWidget *item, PurpleAccount *account,
	void *data)
{
    const char *accountname;
    const char *protocol;
    GtkWidget *fprint = ui_layout.fprint_label;
    char *s = NULL;
    char *fingerprint;

    if (account) {
	char fingerprint_buf[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
	accountname = purple_account_get_username(account);
	protocol = purple_account_get_protocol_id(account);
	fingerprint = otrl_privkey_fingerprint(otrg_plugin_userstate,
		fingerprint_buf, accountname, protocol);

	if (fingerprint) {
	    s = g_strdup_printf(_("Fingerprint: %.80s"), fingerprint);
	    if (ui_layout.generate_button)
		gtk_widget_set_sensitive(ui_layout.generate_button, 0);
	} else {
	    s = g_strdup(_("No key present"));
	    if (ui_layout.generate_button)
		gtk_widget_set_sensitive(ui_layout.generate_button, 1);
	}
    } else {
	s = g_strdup(_("No account available"));
	if (ui_layout.generate_button)
	    gtk_widget_set_sensitive(ui_layout.generate_button, 0);
    }
    if (fprint) {
	gtk_label_set_text(GTK_LABEL(fprint), s ? s : "");
	gtk_widget_show(fprint);
    }
    if (s) {
	g_free(s);
    }
}

/* Call this function when the DSA key is updated; it will redraw the
 * UI, if visible. */
static void otrg_gtk_ui_update_fingerprint(void)
{
    if (ui_layout.accountmenu) {
	g_signal_emit_by_name(G_OBJECT(ui_layout.accountmenu), "changed");
    }
}

static void account_menu_added_removed_cb(PurpleAccount *account, void *data)
{
    otrg_gtk_ui_update_fingerprint();
}

static void clist_all_unselected(void)
{
    if (ui_layout.connect_button) {
	gtk_widget_set_sensitive(ui_layout.connect_button, 0);
    }
    if (ui_layout.disconnect_button) {
	gtk_widget_set_sensitive(ui_layout.disconnect_button, 0);
    }
    if (ui_layout.forget_button) {
	gtk_widget_set_sensitive(ui_layout.forget_button, 0);
    }
    if (ui_layout.verify_button) {
	gtk_widget_set_sensitive(ui_layout.verify_button, 0);
    }
    ui_layout.selected_fprint = NULL;
}

/* Update the keylist, if it's visible */
static void otrg_gtk_ui_update_keylist(void)
{
    gchar *titles[5];
    char hash[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
    ConnContext * context;
    Fingerprint * fingerprint;
    int selected_row = -1;

    GtkWidget *keylist = ui_layout.keylist;

    if (keylist == NULL)
	return;

    gtk_clist_freeze(GTK_CLIST(keylist));
    gtk_clist_clear(GTK_CLIST(keylist));

    for (context = otrg_plugin_userstate->context_root; context != NULL;
	    context = context->next) {
	int i;
	PurplePlugin *p;
	const char *proto_name;

	if (context->m_context != context) continue;

	fingerprint = context->fingerprint_root.next;
	/* If there's no fingerprint, don't add it to the known
	 * fingerprints list */
	while(fingerprint) {
	    ConnContext *context_iter;
	    TrustLevel best_level = TRUST_NOT_PRIVATE;
	    int used = 0;

	    titles[0] = context->username;
	    titles[1] = _("Unused");

	    for (context_iter = context->m_context;
		    context_iter && context_iter->m_context == context->m_context;
		    context_iter = context_iter->next) {

		TrustLevel this_level = TRUST_NOT_PRIVATE;

		if (context_iter->active_fingerprint == fingerprint) {
		    this_level = otrg_plugin_context_to_trust(context_iter);
		    used = 1;

		    if (this_level == TRUST_PRIVATE) {
			best_level = TRUST_PRIVATE;
		    } else if (this_level == TRUST_UNVERIFIED
			    && best_level != TRUST_PRIVATE) {
			best_level = TRUST_UNVERIFIED;
		    } else if (this_level == TRUST_FINISHED
			    && best_level == TRUST_NOT_PRIVATE) {
			best_level = TRUST_FINISHED;
		    }
		}
	    }

	    if (used) {
		titles[1] = (gchar *)
		    _(trust_states[best_level]);
	    }
	    titles[2] = otrg_fingerprint_is_trusted(fingerprint) ?
		    _("Yes") : _("No");
	    otrl_privkey_hash_to_human(hash, fingerprint->fingerprint);
	    titles[3] = hash;
	    p = purple_find_prpl(context->protocol);
	    proto_name = (p && p->info->name) ? p->info->name : _("Unknown");
	    titles[4] = g_strdup_printf("%s (%s)", context->accountname,
		    proto_name);
	    i = gtk_clist_append(GTK_CLIST(keylist), titles);
	    g_free(titles[4]);
	    gtk_clist_set_row_data(GTK_CLIST(keylist), i, fingerprint);
	    if (ui_layout.selected_fprint == fingerprint) {
		selected_row = i;
	    }
	    fingerprint = fingerprint->next;
	}
    }

    if (selected_row >= 0) {
	gtk_clist_select_row(GTK_CLIST(keylist), selected_row, 0);
    } else {
	clist_all_unselected();
    }

    gtk_clist_sort(GTK_CLIST(keylist));

    gtk_clist_thaw(GTK_CLIST(keylist));

}

static void generate(GtkWidget *widget, gpointer data)
{
    PurpleAccount *account;
    account = pidgin_account_option_menu_get_selected(ui_layout.accountmenu);

    if (account == NULL) return;

    otrg_plugin_privkeygen_start(purple_account_get_username(account),
	    purple_account_get_protocol_id(account));
}

static void ui_destroyed(GtkObject *object)
{
    /* If this is called, we need to invalidate the stored pointers in
     * the ui_layout struct. */
    ui_layout.accountmenu = NULL;
    ui_layout.fprint_label = NULL;
    ui_layout.generate_button = NULL;
    ui_layout.scrollwin = NULL;
    ui_layout.keylist = NULL;
    ui_layout.selected_fprint = NULL;
    ui_layout.connect_button = NULL;
    ui_layout.disconnect_button = NULL;
    ui_layout.forget_button = NULL;
    ui_layout.verify_button = NULL;
    ui_layout.os.enablebox = NULL;
    ui_layout.os.automaticbox = NULL;
    ui_layout.os.onlyprivatebox = NULL;
}

static void clist_selected(GtkWidget *widget, gint row, gint column,
	GdkEventButton *event, gpointer data)
{
    int connect_sensitive = 0;
    int disconnect_sensitive = 0;
    int forget_sensitive = 0;
    int verify_sensitive = 0;
    Fingerprint *f = gtk_clist_get_row_data(GTK_CLIST(ui_layout.keylist),
	    row);
    ConnContext *context_iter;

    if (f) {
	verify_sensitive = 1;
	forget_sensitive = 1;

	if (f->context && f->context->m_context) {
	    for (context_iter = f->context;
		    context_iter && context_iter->m_context ==
		    f->context->m_context;
		    context_iter = context_iter->next) {

		if (context_iter->msgstate == OTRL_MSGSTATE_ENCRYPTED &&
		    context_iter->active_fingerprint == f) {
		    disconnect_sensitive = 1;
		    forget_sensitive = 0;
		}
		else if (context_iter->msgstate == OTRL_MSGSTATE_FINISHED) {
		    disconnect_sensitive = 1;
		    connect_sensitive = 1;
		}
		else if (context_iter->msgstate == OTRL_MSGSTATE_PLAINTEXT) {
		    connect_sensitive = 1;
		}
	    }
	}
    }

    gtk_widget_set_sensitive(ui_layout.connect_button,
	    connect_sensitive);
    gtk_widget_set_sensitive(ui_layout.disconnect_button,
	    disconnect_sensitive);
    gtk_widget_set_sensitive(ui_layout.forget_button, forget_sensitive);
    gtk_widget_set_sensitive(ui_layout.verify_button, verify_sensitive);
    ui_layout.selected_fprint = f;
}

static void clist_unselected(GtkWidget *widget, gint row, gint column,
	GdkEventButton *event, gpointer data)
{
    clist_all_unselected();
}

/* For a given fingerprint, find the master context that the fingerprint is
 * pointing to, iterate through it and all its children.
 * Of the contexts that are using this fingerprint, return a value that
 * corresponds to the "best" trust level among these.
 */
static int fngsortval(Fingerprint *f)
{
    int result = 200;
    ConnContext *context_iter;

    for (context_iter = f->context->m_context;
	context_iter && context_iter->m_context == f->context->m_context;
	context_iter = context_iter->next) {

	int is_active = 0;
	TrustLevel level;

	if  (context_iter->msgstate == OTRL_MSGSTATE_ENCRYPTED &&
	    context_iter->active_fingerprint == f) {
	    is_active = 1;
	}

	level = otrg_plugin_context_to_trust(context_iter);

	if (level == TRUST_PRIVATE) {
	    if (is_active) {
		result = 0;
		break;
	    } else {
		result = result > 100 ? 100 : result;
	    }
	} else if (level == TRUST_UNVERIFIED) {
	    if (is_active) {
		result = 1;
	    } else {
		result = result > 100 ? 100 : result;
	    }
	} else if (level == TRUST_FINISHED) {
	    result = result > 2 ? 2 : result;
	} else if (level == TRUST_NOT_PRIVATE) {
	    result = result > 3 ? 3 : result;
	}
    }

    return result;
}

static gint statuscmp(GtkCList *clist, gconstpointer ptr1, gconstpointer ptr2)
{
    const GtkCListRow *a = ptr1;
    const GtkCListRow *b = ptr2;
    int as = fngsortval(a->data);
    int bs = fngsortval(b->data);
    return (as - bs);
}

static void clist_click_column(GtkCList *clist, gint column, gpointer data)
{
    if (ui_layout.sortcol == column) {
	ui_layout.sortdir = -(ui_layout.sortdir);
    } else {
	ui_layout.sortcol = column;
	ui_layout.sortdir = 1;
    }

    gtk_clist_set_sort_column(clist, ui_layout.sortcol);
    gtk_clist_set_sort_type(clist,
	    ui_layout.sortdir == 1 ? GTK_SORT_ASCENDING : GTK_SORT_DESCENDING);
    if (column == 1) {
	gtk_clist_set_compare_func(clist, statuscmp);
    } else {
	/* Just use the default compare function for the rest of the
	 * columns */
	gtk_clist_set_compare_func(clist, NULL);
    }
    gtk_clist_sort(clist);
}

static void connect_connection(GtkWidget *widget, gpointer data)
{
    /* Send an OTR Query to the other side. */
    ConnContext *context;

    if (ui_layout.selected_fprint == NULL) return;

    context = ui_layout.selected_fprint->context;
    otrg_ui_connect_connection(context);
}

static void disconnect_connection(GtkWidget *widget, gpointer data)
{
    /* Forget whatever state we've got with this context */
    ConnContext *context;
    ConnContext *context_iter;

    if (ui_layout.selected_fprint == NULL) return;

    context = ui_layout.selected_fprint->context;
    if (context == NULL) return;

    for (context_iter = context->m_context;
	    context_iter && context_iter->m_context == context->m_context;
	    context_iter = context_iter->next) {

	/* Don't do anything with fingerprints other than the active one
	 * if we're in the ENCRYPTED state */
	if (context_iter->msgstate == OTRL_MSGSTATE_ENCRYPTED &&
	    context_iter->active_fingerprint == ui_layout.selected_fprint) {
	    otrg_ui_disconnect_connection(context_iter);
	}
    }

}

static void forget_fingerprint(GtkWidget *widget, gpointer data)
{
    Fingerprint *fingerprint = ui_layout.selected_fprint;

    otrg_ui_forget_fingerprint(fingerprint);
}

static void verify_fingerprint(GtkWidget *widget, gpointer data)
{
    Fingerprint *fingerprint = ui_layout.selected_fprint;

    otrg_dialog_verify_fingerprint(fingerprint);
}

static void otrsettings_clicked_cb(GtkButton *button,
	struct otrsettingsdata *os)
{
    gtk_widget_set_sensitive(os->enablebox, TRUE);
    if (gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(os->enablebox))) {
	gtk_widget_set_sensitive(os->automaticbox, TRUE);
	if (gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(os->automaticbox))) {
	    gtk_widget_set_sensitive(os->onlyprivatebox, TRUE);
	} else {
	    gtk_widget_set_sensitive(os->onlyprivatebox, FALSE);
	}
	gtk_widget_set_sensitive(os->avoidloggingotrbox, TRUE);
    } else {
	gtk_widget_set_sensitive(os->automaticbox, FALSE);
	gtk_widget_set_sensitive(os->onlyprivatebox, FALSE);
	gtk_widget_set_sensitive(os->avoidloggingotrbox, FALSE);
    }
}

static void create_otrsettings_buttons(struct otrsettingsdata *os,
	GtkWidget *vbox)
{
    GtkWidget *tempbox1, *tempbox2;

    os->enablebox = gtk_check_button_new_with_label(_("Enable private "
	    "messaging"));
    os->automaticbox = gtk_check_button_new_with_label(_("Automatically "
	    "initiate private messaging"));
    os->onlyprivatebox = gtk_check_button_new_with_label(_("Require private "
	    "messaging"));
    os->avoidloggingotrbox = gtk_check_button_new_with_label(
	    _("Don't log OTR conversations"));

    gtk_box_pack_start(GTK_BOX(vbox), os->enablebox,
	    FALSE, FALSE, 0);
    tempbox1 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), tempbox1,
	    FALSE, FALSE, 0);
    tempbox2 = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tempbox1), tempbox2, FALSE, FALSE, 5);

    gtk_box_pack_start(GTK_BOX(tempbox2), os->automaticbox,
	    FALSE, FALSE, 0);
    tempbox1 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tempbox2), tempbox1, FALSE, FALSE, 0);
    tempbox2 = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tempbox1), tempbox2, FALSE, FALSE, 5);

    gtk_box_pack_start(GTK_BOX(tempbox2), os->onlyprivatebox,
	    FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), os->avoidloggingotrbox, FALSE, FALSE, 5);

    g_signal_connect(G_OBJECT(os->enablebox), "clicked",
	    G_CALLBACK(otrsettings_clicked_cb), os);
    g_signal_connect(G_OBJECT(os->automaticbox), "clicked",
	    G_CALLBACK(otrsettings_clicked_cb), os);
    g_signal_connect(G_OBJECT(os->onlyprivatebox), "clicked",
	    G_CALLBACK(otrsettings_clicked_cb), os);
    g_signal_connect(G_OBJECT(os->avoidloggingotrbox), "clicked",
	    G_CALLBACK(otrsettings_clicked_cb), os);
}

static void otroptions_clicked_cb(GtkButton *button,
	struct otroptionsdata *oo)
{
    /* This doesn't really do anything useful right now, but is here for
     * future expansion purposes. */
    gtk_widget_set_sensitive(oo->showotrbutton, TRUE);
}

static void create_otroptions_buttons(struct otroptionsdata *oo,
	GtkWidget *vbox)
{
    oo->showotrbutton = gtk_check_button_new_with_label(
	    _("Show OTR button in toolbar"));

    gtk_box_pack_start(GTK_BOX(vbox), oo->showotrbutton, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(oo->showotrbutton), "clicked",
	    G_CALLBACK(otroptions_clicked_cb), oo);
}

static void load_otrsettings(struct otrsettingsdata *os)
{
    gboolean otrenabled;
    gboolean otrautomatic;
    gboolean otronlyprivate;
    gboolean otravoidloggingotr;

    otrg_prefs_global_load(&otrenabled, &otrautomatic, &otronlyprivate,
	    &otravoidloggingotr);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(os->enablebox),
	    otrenabled);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(os->automaticbox),
	    otrautomatic);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(os->onlyprivatebox),
	    otronlyprivate);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(os->avoidloggingotrbox),
	    otravoidloggingotr);

    otrsettings_clicked_cb(GTK_BUTTON(os->enablebox), os);
}

/* Load the global OTR UI options */
static void otrg_gtk_ui_global_options_load(gboolean *showotrbuttonp)
{
    if (purple_prefs_exists("/OTR/showotrbutton")) {
	*showotrbuttonp = purple_prefs_get_bool("/OTR/showotrbutton");
    } else {
	*showotrbuttonp = TRUE;
    }
}

/* Save the global OTR UI options */
static void otrg_gtk_ui_global_options_save(gboolean showotrbutton)
{
    if (! purple_prefs_exists("/OTR")) {
	purple_prefs_add_none("/OTR");
    }
    if (! purple_prefs_exists("/OTR/showotrbutton")) {
	purple_prefs_add_bool("/OTR/showotrbutton", showotrbutton);
    }
    purple_prefs_set_bool("/OTR/showotrbutton", showotrbutton);
}


static void load_otroptions(struct otroptionsdata *oo)
{
    gboolean showotrbutton;

    otrg_gtk_ui_global_options_load(&showotrbutton);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(oo->showotrbutton),
	    showotrbutton);

    otroptions_clicked_cb(GTK_BUTTON(oo->showotrbutton), oo);
}

/* Create the privkeys UI, and pack it into the vbox */
static void make_privkeys_ui(GtkWidget *vbox)
{
    GtkWidget *fbox;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *frame;

    frame = gtk_frame_new(_("My private keys"));
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

    fbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(fbox), 10);
    gtk_container_add(GTK_CONTAINER(frame), fbox);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(fbox), hbox, FALSE, FALSE, 0);
    label = gtk_label_new(_("Key for account:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    ui_layout.accountmenu = pidgin_account_option_menu_new(NULL, 1,
	    G_CALLBACK(account_menu_changed_cb), NULL, NULL);
    gtk_box_pack_start(GTK_BOX(hbox), ui_layout.accountmenu, TRUE, TRUE, 0);

    /* Make sure we notice if the menu changes because an account has
     * been added or removed */
    purple_signal_connect(purple_accounts_get_handle(), "account-added",
	    ui_layout.accountmenu,
	    PURPLE_CALLBACK(account_menu_added_removed_cb), NULL);
    purple_signal_connect(purple_accounts_get_handle(), "account-removed",
	    ui_layout.accountmenu,
	    PURPLE_CALLBACK(account_menu_added_removed_cb), NULL);

    ui_layout.fprint_label = gtk_label_new("");
    gtk_label_set_selectable(GTK_LABEL(ui_layout.fprint_label), 1);
    gtk_box_pack_start(GTK_BOX(fbox), ui_layout.fprint_label,
	    FALSE, FALSE, 0);

    ui_layout.generate_button = gtk_button_new();
    gtk_signal_connect(GTK_OBJECT(ui_layout.generate_button), "clicked",
	    GTK_SIGNAL_FUNC(generate), NULL);

    label = gtk_label_new(_("Generate"));
    gtk_container_add(GTK_CONTAINER(ui_layout.generate_button), label);

    otrg_gtk_ui_update_fingerprint();

    gtk_box_pack_start(GTK_BOX(fbox), ui_layout.generate_button,
	    FALSE, FALSE, 0);
}

/* Save the global OTR settings whenever they're clicked */
static void otrsettings_save_cb(GtkButton *button, struct otrsettingsdata *os)
{
    otrg_prefs_global_save(
	    gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(os->enablebox)),
	    gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(os->automaticbox)),
	    gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(os->onlyprivatebox)),
	    gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(os->avoidloggingotrbox)));

    otrg_dialog_resensitize_all();
}

/* Save the global OTR UI options whenever they're clicked */
static void otroptions_save_cb(GtkButton *button, struct otroptionsdata *oo)
{
    otrg_gtk_ui_global_options_save(
	    gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(oo->showotrbutton)));

    otrg_dialog_resensitize_all();
}

/* Make the settings UI, and pack it into the vbox */
static void make_settings_ui(GtkWidget *vbox)
{
    GtkWidget *fbox;
    GtkWidget *frame;

    frame = gtk_frame_new(_("Default OTR Settings"));
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

    fbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(fbox), 10);
    gtk_container_add(GTK_CONTAINER(frame), fbox);

    create_otrsettings_buttons(&(ui_layout.os), fbox);

    load_otrsettings(&(ui_layout.os));

    g_signal_connect(G_OBJECT(ui_layout.os.enablebox), "clicked",
	    G_CALLBACK(otrsettings_save_cb), &(ui_layout.os));
    g_signal_connect(G_OBJECT(ui_layout.os.automaticbox), "clicked",
	    G_CALLBACK(otrsettings_save_cb), &(ui_layout.os));
    g_signal_connect(G_OBJECT(ui_layout.os.onlyprivatebox), "clicked",
	    G_CALLBACK(otrsettings_save_cb), &(ui_layout.os));
    g_signal_connect(G_OBJECT(ui_layout.os.avoidloggingotrbox), "clicked",
	    G_CALLBACK(otrsettings_save_cb), &(ui_layout.os));
}

/* Make the options UI, and pack it into the vbox */
static void make_options_ui(GtkWidget *vbox)
{
    GtkWidget *fbox;
    GtkWidget *frame;

    frame = gtk_frame_new(_("OTR UI Options"));
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

    fbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(fbox), 10);
    gtk_container_add(GTK_CONTAINER(frame), fbox);

    create_otroptions_buttons(&(ui_layout.oo), fbox);

    load_otroptions(&(ui_layout.oo));

    g_signal_connect(G_OBJECT(ui_layout.oo.showotrbutton), "clicked",
	     G_CALLBACK(otroptions_save_cb), &(ui_layout.oo));
}

/* Create the fingerprint UI, and pack it into the vbox */
static void make_fingerprints_ui(GtkWidget *vbox)
{
    GtkWidget *hbox;
    GtkWidget *table;
    GtkWidget *label;
    char *titles[5];

    titles[0] = _("Screenname");
    titles[1] = _("Status");
    titles[2] = _("Verified");
    titles[3] = _("Fingerprint");
    titles[4] = _("Account");

    ui_layout.scrollwin = gtk_scrolled_window_new(0, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ui_layout.scrollwin),
	    GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);

    ui_layout.keylist = gtk_clist_new_with_titles(5, titles);
    gtk_clist_set_column_width(GTK_CLIST(ui_layout.keylist), 0, 90);
    gtk_clist_set_column_width(GTK_CLIST(ui_layout.keylist), 1, 90);
    gtk_clist_set_column_width(GTK_CLIST(ui_layout.keylist), 2, 60);
    gtk_clist_set_column_width(GTK_CLIST(ui_layout.keylist), 3, 400);
    gtk_clist_set_column_width(GTK_CLIST(ui_layout.keylist), 4, 200);
    gtk_clist_set_selection_mode(GTK_CLIST(ui_layout.keylist),
	    GTK_SELECTION_SINGLE);
    gtk_clist_column_titles_active(GTK_CLIST(ui_layout.keylist));

    gtk_container_add(GTK_CONTAINER(ui_layout.scrollwin), ui_layout.keylist);
    gtk_box_pack_start(GTK_BOX(vbox), ui_layout.scrollwin,
	    TRUE, TRUE, 0);

    otrg_gtk_ui_update_keylist();

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    table = gtk_table_new(2, 2, TRUE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table), 20);

    gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(""), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(""), TRUE, TRUE, 0);

    ui_layout.connect_button = gtk_button_new();
    gtk_signal_connect(GTK_OBJECT(ui_layout.connect_button), "clicked",
	    GTK_SIGNAL_FUNC(connect_connection), NULL);
    label = gtk_label_new(_("Start private connection"));
    gtk_container_add(GTK_CONTAINER(ui_layout.connect_button), label);
    gtk_table_attach_defaults(GTK_TABLE(table), ui_layout.connect_button,
	    0, 1, 0, 1);

    ui_layout.disconnect_button = gtk_button_new();
    gtk_signal_connect(GTK_OBJECT(ui_layout.disconnect_button), "clicked",
	    GTK_SIGNAL_FUNC(disconnect_connection), NULL);
    label = gtk_label_new(_("End private connection"));
    gtk_container_add(GTK_CONTAINER(ui_layout.disconnect_button), label);
    gtk_table_attach_defaults(GTK_TABLE(table), ui_layout.disconnect_button,
	    0, 1, 1, 2);

    ui_layout.verify_button = gtk_button_new();
    gtk_signal_connect(GTK_OBJECT(ui_layout.verify_button), "clicked",
	    GTK_SIGNAL_FUNC(verify_fingerprint), NULL);
    label = gtk_label_new(_("Verify fingerprint"));
    gtk_container_add(GTK_CONTAINER(ui_layout.verify_button), label);
    gtk_table_attach_defaults(GTK_TABLE(table), ui_layout.verify_button,
	    1, 2, 0, 1);

    ui_layout.forget_button = gtk_button_new();
    gtk_signal_connect(GTK_OBJECT(ui_layout.forget_button), "clicked",
	    GTK_SIGNAL_FUNC(forget_fingerprint), NULL);
    label = gtk_label_new(_("Forget fingerprint"));
    gtk_container_add(GTK_CONTAINER(ui_layout.forget_button), label);
    gtk_table_attach_defaults(GTK_TABLE(table), ui_layout.forget_button,
	    1, 2, 1, 2);

    gtk_signal_connect(GTK_OBJECT(vbox), "destroy",
	    GTK_SIGNAL_FUNC(ui_destroyed), NULL);

    /* Handle selections and deselections */
    gtk_signal_connect(GTK_OBJECT(ui_layout.keylist), "select_row",
	    GTK_SIGNAL_FUNC(clist_selected), NULL);
    gtk_signal_connect(GTK_OBJECT(ui_layout.keylist), "unselect_row",
	    GTK_SIGNAL_FUNC(clist_unselected), NULL);

    /* Handle column sorting */
    gtk_signal_connect(GTK_OBJECT(ui_layout.keylist), "click-column",
	    GTK_SIGNAL_FUNC(clist_click_column), NULL);
    ui_layout.sortcol = 0;
    ui_layout.sortdir = 1;

    clist_all_unselected();
}

/* Construct the OTR UI widget */
GtkWidget* otrg_gtk_ui_make_widget(PurplePlugin *plugin)
{
    GtkWidget *vbox = gtk_vbox_new(FALSE, 5);
    GtkWidget *fingerprintbox = gtk_vbox_new(FALSE, 5);
    GtkWidget *configbox = gtk_vbox_new(FALSE, 5);
    GtkWidget *notebook = gtk_notebook_new();

    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);
    gtk_container_set_border_width(GTK_CONTAINER(fingerprintbox), 5);
    gtk_container_set_border_width(GTK_CONTAINER(configbox), 5);

    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    make_privkeys_ui(configbox);

    make_settings_ui(configbox);

    make_options_ui(configbox);

    /*
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(confwindow), vbox);
    */

    make_fingerprints_ui(fingerprintbox);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), configbox,
	    gtk_label_new(_("Config")));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), fingerprintbox,
	    gtk_label_new(_("Known fingerprints")));

    gtk_widget_show_all(vbox);

    return vbox;
}

struct cbdata {
    GtkWidget *dialog;
    PurpleBuddy *buddy;
    GtkWidget *defaultbox;
    struct otrsettingsdata os;
};

static void default_clicked_cb(GtkButton *button, struct cbdata *data)
{
    gboolean defaultset =
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->defaultbox));
    if (defaultset) {
	gtk_widget_set_sensitive(data->os.enablebox, FALSE);
	gtk_widget_set_sensitive(data->os.automaticbox, FALSE);
	gtk_widget_set_sensitive(data->os.onlyprivatebox, FALSE);
	gtk_widget_set_sensitive(data->os.avoidloggingotrbox, FALSE);
    } else {
	otrsettings_clicked_cb(button, &(data->os));
    }
}

static void load_buddyprefs(struct cbdata *data)
{
    gboolean usedefault, enabled, automatic, onlyprivate, avoidloggingotr;

    otrg_prefs_buddy_load(data->buddy, &usedefault, &enabled,
	    &automatic, &onlyprivate, &avoidloggingotr);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->defaultbox),
	    usedefault);

    if (usedefault) {
	/* Load the global defaults */
	load_otrsettings(&(data->os));
    } else {
	/* We've got buddy-specific prefs */
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(data->os.enablebox), enabled);
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(data->os.automaticbox), automatic);
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(data->os.onlyprivatebox), onlyprivate);
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(data->os.avoidloggingotrbox),
		avoidloggingotr);
    }

    default_clicked_cb(GTK_BUTTON(data->defaultbox), data);
}

static void config_buddy_destroy_cb(GtkWidget *w, struct cbdata *data)
{
    free(data);
}

static void config_buddy_clicked_cb(GtkButton *button, struct cbdata *data)
{
    gboolean enabled = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(data->os.enablebox));

    /* Apply the changes */
    otrg_prefs_buddy_save(data->buddy,
	    gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(data->defaultbox)),
		enabled,
	    gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(data->os.automaticbox)),
	    gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(data->os.onlyprivatebox)),
	    gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(data->os.avoidloggingotrbox)));

    otrg_dialog_resensitize_all();
}

static void config_buddy_response_cb(GtkDialog *dialog, gint resp,
	struct cbdata *data)
{
    gtk_widget_destroy(data->dialog);
}

static void otrg_gtk_ui_config_buddy(PurpleBuddy *buddy)
{
    GtkWidget *dialog;
    GtkWidget *label;
    char *label_text;
    char *label_markup;
    struct cbdata *data = malloc(sizeof(struct cbdata));

    if (!data) return;

    dialog = gtk_dialog_new_with_buttons(_("OTR Settings"),
	    NULL, 0,
	    GTK_STOCK_OK, GTK_RESPONSE_OK,
	    NULL);
    gtk_window_set_accept_focus(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_role(GTK_WINDOW(dialog), "otr_settings");

    gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
    gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog)->vbox), 0);
    gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), 0);

    data->dialog = dialog;
    data->buddy = buddy;

    /* Set the title */

    label_text = g_strdup_printf(_("OTR Settings for %s"),
	    purple_buddy_get_contact_alias(buddy));
    label_markup = g_strdup_printf("<span weight=\"bold\" size=\"larger\">"
	    "%s</span>", label_text);

    label = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(label), label_markup);
    g_free(label_markup);
    g_free(label_text);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label,
	    FALSE, FALSE, 5);

    /* Make the cascaded checkboxes */

    data->defaultbox = gtk_check_button_new_with_label(_("Use default "
	    "OTR settings for this buddy"));

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), data->defaultbox,
	    FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), gtk_hseparator_new(),
	    FALSE, FALSE, 5);

    create_otrsettings_buttons(&(data->os), GTK_DIALOG(dialog)->vbox);

    g_signal_connect(G_OBJECT(data->defaultbox), "clicked",
	    G_CALLBACK(default_clicked_cb), data);
    g_signal_connect(G_OBJECT(data->defaultbox), "clicked",
	    G_CALLBACK(config_buddy_clicked_cb), data);
    g_signal_connect(G_OBJECT(data->os.enablebox), "clicked",
	    G_CALLBACK(config_buddy_clicked_cb), data);
    g_signal_connect(G_OBJECT(data->os.automaticbox), "clicked",
	    G_CALLBACK(config_buddy_clicked_cb), data);
    g_signal_connect(G_OBJECT(data->os.onlyprivatebox), "clicked",
	    G_CALLBACK(config_buddy_clicked_cb), data);
    g_signal_connect(G_OBJECT(data->os.avoidloggingotrbox), "clicked",
	    G_CALLBACK(config_buddy_clicked_cb), data);

    /* Set the inital states of the buttons */
    load_buddyprefs(data);

    g_signal_connect(G_OBJECT(dialog), "destroy",
	    G_CALLBACK(config_buddy_destroy_cb), data);
    g_signal_connect(G_OBJECT(dialog), "response",
	    G_CALLBACK(config_buddy_response_cb), data);

    gtk_widget_show_all(dialog);
}

static void otr_options_cb(PurpleBlistNode *node, gpointer user_data)
{
    /* We've already checked PURPLE_BLIST_NODE_IS_BUDDY(node) */
    PurpleBuddy *buddy = (PurpleBuddy *)node;

    /* Modify the settings for this buddy */
    otrg_ui_config_buddy(buddy);
}

static void otrg_gtk_ui_buddy_menu(PurpleBuddy *buddy, GList **menu)
{
    PurpleMenuAction *act;

    act = purple_menu_action_new(_("OTR Settings"),
	(PurpleCallback)otr_options_cb, NULL, NULL);
    *menu = g_list_append(*menu, act);
}

/* Initialize the OTR UI subsystem */
static void otrg_gtk_ui_init(void)
{
    /* Nothing to do */
}

/* Deinitialize the OTR UI subsystem */
static void otrg_gtk_ui_cleanup(void)
{
    /* Nothing to do */
}

static const OtrgUiUiOps gtk_ui_ui_ops = {
    otrg_gtk_ui_init,
    otrg_gtk_ui_cleanup,
    otrg_gtk_ui_update_fingerprint,
    otrg_gtk_ui_update_keylist,
    otrg_gtk_ui_config_buddy,
    otrg_gtk_ui_buddy_menu
};

/* Get the GTK UI ops */
const OtrgUiUiOps *otrg_gtk_ui_get_ui_ops(void)
{
    return &gtk_ui_ui_ops;
}

/* vim: set tabstop=8 softtabstop=4 shiftwidth=4 noexpandtab: */
