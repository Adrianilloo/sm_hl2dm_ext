/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC. All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation. You must obey the GNU General Public License in
 * all respects for all other code used. Additionally, AlliedModders LLC grants
 * this exception to all derivative works. AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_

/**
 * @file smsdk_config.h
 * @brief Contains macros for configuring basic extension information
 */

/* Basic information exposed publicly */
#define SMEXT_CONF_NAME        "Half-Life 2: Deathmatch"
#define SMEXT_CONF_DESCRIPTION "SourceMod extension with corrections and enhancements for/under HL2:DM"
#define SMEXT_CONF_VERSION     "1.1"
#define SMEXT_CONF_AUTHOR      "AdRiAnIlloO"
#define SMEXT_CONF_URL         "https://github.com/Adrianilloo/sm_ext_hl2dm"
#define SMEXT_CONF_LOGTAG      "HL2:DM Ext"
#define SMEXT_CONF_LICENSE     "GPL"
#define SMEXT_CONF_DATESTRING  __DATE__

/** 
 * @brief Exposes plugin's main interface
 */
#define SMEXT_LINK(name) SDKExtension *g_pExtensionIface = name;

/**
 * @brief Sets whether or not this plugin required Metamod.
 * NOTE: Uncomment to enable, comment to disable.
 */
#define SMEXT_CONF_METAMOD

#define SMEXT_ENABLE_PLAYERHELPERS
#define SMEXT_ENABLE_GAMEHELPERS
#define SMEXT_ENABLE_TIMERSYS
#define SMEXT_ENABLE_USERMSGS
#define SMEXT_ENABLE_TRANSLATOR

#endif // _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
