/**************************************
 *
 *  ImportMD2.c
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  This file must remain as 'C' code
 *
 *  Import Unreal Tournament PSA and PSK object formats
 *  ObjectLoader plugin type
 *  Generic Layout importer for Animations
 *
 *  Responds to the user choosing a .md2 file
 *
 *  Quake2 is a trademark of id software
 *
 **************************************/

/* Modeller include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "FollowTag.h"

GlobalFunc		*LW_globalFuncs = 0;



/* Main setup function for ItemMotion Handler (entry)
 *
 * Called when the plug is added to an item.
 * It seems that it is also evaluated at each frame and points as well,
 * so be aware that that might occur
 */
XCALL_(static int)
ACTIVATE (
	long version,
	GlobalFunc *global,
	LWDisplacementHandler  *local,
	void *serverData )
{
	int						retval = AFUNC_OK, i = 0;

	XCALL_INIT;
    if ( version != LWDISPLACEMENT_VERSION ) return AFUNC_BADVERSION;

	LW_globalFuncs = global;

	SetupFuncs(local);

	return AFUNC_OK;
};


/*
 * INTERFACE
 *
 * This function is called when the interface is setup
 * When the plugin is double clicked the options function is called
 *
 */

XCALL_(static int)
INTERFACE (
	long			 version,
	GlobalFunc		*global,
	LWInterface		*local,
	void			*serverData)
{

   if ( version != LWDISPLACEMENT_VERSION ) return AFUNC_BADVERSION;

	LW_globalFuncs = global;

	local->panel	= GetPanel(local->inst);
	local->options	= NULL;
	local->command	= NULL;

	return AFUNC_OK;
}

ServerRecord ServerDesc[] = {
    { LWDISPLACEMENT_HCLASS,		PLUGINNAME,	ACTIVATE},
    { LWDISPLACEMENT_ICLASS,		PLUGINNAME,	INTERFACE},
    { NULL }
};