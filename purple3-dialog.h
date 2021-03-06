/*
 *  Off-the-Record Messaging plugin for libpurple 3
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
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __OTRG_PURPLE3_DIALOG_H__
#define __OTRG_PURPLE3_DIALOG_H__

PurpleE2eeState *
otrg_purple3_trustlevel_to_e2eestate(TrustLevel level);

const gchar*
otrg_purple3_account_full_name(const char *protocol, const char *account);

/* Get the libpurple 3 dialog UI ops */
const OtrgDialogUiOps *otrg_purple3_dialog_get_ui_ops(void);

#endif

/* vim: set tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab: */
