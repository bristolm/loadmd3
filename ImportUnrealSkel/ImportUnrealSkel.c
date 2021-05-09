/**************************************
 *
 *  ImportUnrealSkel.c
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  This file must remain as 'C' code
 *
 *  Import Unreal Tournament PSA and PSK object formats
 *  ObjectLoader plugin type
 *  Generic Layout importer for Animations
 *
 *  Responds to the user choosing a .psk or psa file
 *
 *  Unreal Tounament is a trademark of Epic MegaGames, Inc.
 *
 **************************************/

/* Modeller include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ImportUnrealSkel.h"

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

	LW_sysInfo = ( unsigned long )LW_globalFuncs( LWSYSTEMID_GLOBAL, GFUSE_TRANSIENT );	
	LW_cmdFunc = (LWCommandFunc)LW_globalFuncs( LWCOMMANDINTERFACE_GLOBAL, GFUSE_ACQUIRE );

}

// version seems to be 1 for 5.6
// version seems to be 2 for 6.0

XCALL_(static int)
LOAD_UnrealSkeletal (long version, GlobalFunc *global, 
							LWObjectImport *local, void *serverData)
{
	int				retval = AFUNC_OK;
	unsigned short	lwpntIndexCounter = 0;

	const char		*pExt = (char *)NULL;	
	LWDirInfoFunc 	*DirInfo = (LWDirInfoFunc  *)global(LWDIRINFOFUNC_GLOBAL,GFUSE_TRANSIENT);

	XCALL_INIT;

    if ( version != LWOBJECTIMPORT_VERSION ) return AFUNC_BADVERSION;

	// Preload values
	local->result = LWOBJIM_NOREC;

	_Setup(global);

	// Set path ...
	StoreLastPath(DirInfo("Objects"));

	// Load either .psk or .psa (as requested ...)
	pExt = local->filename + strlen(local->filename) -4;
	if (strcmp(pExt,".psk") == 0 || strcmp(pExt,".PSK") == 0)
	{	// Load skeelton file
		retval = Load_PSK(local);
		// Be done
		local->done(local->data);
	}
	else if (strcmp(pExt,".psa") == 0 || strcmp(pExt,".PSA") == 0)
	{	// Load Animation file
		retval = Load_PSA(local);
		// Be done
		local->done(local->data);
	}


	return retval;
};

// Animation import for Layout
XCALL_(static int)
GENERIC_UnrealSkeletal (long version,GlobalFunc *global,
								LWLayoutGeneric *local, void *serverData)
{
	int			retval = AFUNC_OK;
	AnimLoader *file = 0;

	XCALL_INIT;
    if ( version != LWLAYOUTGENERIC_VERSION ) return AFUNC_BADVERSION;

	_Setup(global);

	// .psa only
	file = Get_PSAFile();
	if (file->Skeleton != 0)
		retval = GLoad_PSK(local,file);

	if (file->Animations != 0)
		retval = GLoad_PSA(local,file);

	return retval;
};


ServerRecord ServerDesc[] = {
    { LWOBJECTIMPORT_CLASS,		PLUGINNAME"OK",		LOAD_UnrealSkeletal},
    { LWLAYOUTGENERIC_CLASS,	PLUGINNAME,		GENERIC_UnrealSkeletal},
    { NULL }
};