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

#include "ImportMD2.h"

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
LOAD_MD2 (long version, GlobalFunc *global, 
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

	// Load .MD2
	pExt = local->filename + strlen(local->filename) -4;
	if (strcmp(pExt,".md2") == 0 || strcmp(pExt,".MD2") == 0)
	{	// Load file
		retval = Load_MD2(local);

		// Be done
		local->done(local->data);
	}

	return retval;
};

ServerRecord ServerDesc[] = {
    { LWOBJECTIMPORT_CLASS,		PLUGINNAME,		LOAD_MD2},
    { NULL }
};