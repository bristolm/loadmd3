/**************************************
 *
 *  LayoutExportUTSkel.c
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  This file must remain as 'C' code
 *
 *  Main file for the Unreal Tournament Skeletal export module that
 *  interfaces with my LayoutRecorder plugin
 *
 *  Defines a Displacement plugin (which hands it's Activate and
 *  Interface functions over to LayoutRecorder) and a Global
 *  as is required for LayoutRecorder interfacing
 *
 *  Unreal Tounament is a trademark of Epic MegaGames, Inc.
 **************************************/

/* Layout include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h> // for _set_sbh_threshold(0) - works around a MS crash bug
#include <crtdbg.h>

/* Mystuff */
#include "LayoutExportUTSkel.h"

#include "lwdisplce.h"

GlobalFunc			*LW_globalFuncs	= 0;
LWMessageFuncs		*LW_msgsFuncs = 0;
LWXPanelFuncs		*LW_xpanFuncs = 0;
LWMRBExportFuncs	*LW_xprtFuncs = 0;

static int		Setup = 0;

void _Setup(GlobalFunc *global)
{
	int i = 0;
	if (Setup == 1) {
		return;
	}

    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) |_CRTDBG_LEAK_CHECK_DF);

	_set_sbh_threshold(0);
	Setup = 1;

	LW_globalFuncs = global;
	LW_msgsFuncs = (LWMessageFuncs *)LW_globalFuncs(LWMESSAGEFUNCS_GLOBAL,GFUSE_ACQUIRE );
	LW_xpanFuncs = (LWXPanelFuncs *)LW_globalFuncs( LWXPANELFUNCS_GLOBAL, GFUSE_ACQUIRE  );
	LW_xprtFuncs = (LWMRBExportFuncs*)LW_globalFuncs(LWMRBEXPORT_GLOBAL,GFUSE_ACQUIRE );
	if(!LW_xprtFuncs)
	{
		(*LW_msgsFuncs->error)("Unable to activate global "LWMRBEXPORT_GLOBAL,
						"     please add plugin MRB_Xhub.p" );
		return;
	}
}

	XCALL_(static int)
DISPHANDLER_Activate (long version,GlobalFunc *global,
								LWDisplacementHandler *local,void *serverData)
{
	int						retval = AFUNC_OK, i = 0;

	XCALL_INIT;
    if ( version != LWDISPLACEMENT_VERSION ) return AFUNC_BADVERSION;

	_Setup(global);
	if (LW_xprtFuncs == 0)
		return AFUNC_BADAPP;

	local->inst->priv = getFunc();
	return (int)LW_xprtFuncs->get_activation(version,global,local);
};
	
	XCALL_(static int)
DISPINTERFACE_Interface (
	long			 version,
	GlobalFunc		*global,
	LWInterface		*local,
	void			*serverData)
{

   if ( version != LWINTERFACE_VERSION ) return AFUNC_BADVERSION;

	_Setup(global);
	if (LW_xprtFuncs == 0)
		return AFUNC_BADAPP;

	return (int)LW_xprtFuncs->get_interface(version,global,local);
}

	XCALL_(static int)
GLOBALSERVICE_Initiate (
	long			 version,
	GlobalFunc		*global,
	void			*inst,
	void			*serverData)
{
   if ( version != LWGLOBALSERVICE_VERSION ) return AFUNC_BADVERSION;

	_Setup(global);
	if (LW_xprtFuncs == 0)
		return AFUNC_BADAPP;
	
	((LWGlobalService *)inst)->data = getCallback();

	return AFUNC_OK;
}

ServerRecord ServerDesc[] = {
    { LWDISPLACEMENT_HCLASS,	PLUGINNAME,			DISPHANDLER_Activate},
    { LWDISPLACEMENT_ICLASS,	PLUGINNAME,			DISPINTERFACE_Interface},

    { LWGLOBALSERVICE_CLASS,	GLOBALPLUGINNAME,	GLOBALSERVICE_Initiate},

	{ NULL }
};
