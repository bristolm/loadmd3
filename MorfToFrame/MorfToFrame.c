/**************************************
 *
 *  MorfToFrame.c
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  This file must remain as 'C' code
 *
 *  Takes a given model, and gives the user an
 *  interface by which they can choose which frames
 *  to activate when.
 *
 **************************************/

/* Modeller include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "MorfToFrame.h"

GlobalFunc		*LW_globalFuncs = 0;
LWCommandFunc	LW_cmdFunc;
unsigned long	LW_sysInfo;

/* Mystuff */
static int		Setup = 0;

void _Setup(GlobalFunc *global)
{
	int i = 0;
	if (Setup == 1) {
		return;
	}

	Setup = 1;

	LW_globalFuncs = global;
}

// version seems to be 1 for 5.6
// version seems to be 2 for 6.0

XCALL_(static int)
ACTIVATE (long version, GlobalFunc *global, 
							LWLayoutGeneric *local, void *serverData)
{
	int					retval = AFUNC_OK;
	unsigned short		lwpntIndexCounter = 0;
	XCALL_INIT;

    if ( version != LWLAYOUTGENERIC_VERSION ) return AFUNC_BADVERSION;

	_Setup(global);

	// Get the user data, build the frame data
	return BuildFrames(local,Get_BuildData(local));
};

ServerRecord ServerDesc[] = {
    { LWLAYOUTGENERIC_CLASS,		PLUGINNAME,		ACTIVATE},
    { NULL }
};