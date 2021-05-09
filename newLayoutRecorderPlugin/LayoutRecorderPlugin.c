/**************************************
 *
 *  LayoutRecorderPlugin.c
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Main file for the Layout Recorder plugin
 *  This file must remain as 'C' code
 *
 *  This is the entry point for the Layout Recorder plugin.  It defines names 
 *  and distributes the interface definitions to all module all plugins
 *
 *  This plugin exists as a bridge between the data created by Layout (as presented
 *  during a preview render) and any number of available export file Modules which
 *  are presented to the user as Displacement plugins.
 *
 *  Master Plugin:  [Invisible]
 *    This plugin handles all the frameloop information
 *
 *  Motion Plugin:  [Invisible]
 *    These are created dynamically by ListenChannels (currently LISTEN_BONE only)
 *
 *  Generic Plugin:
 *    This displays the master control panel - list of all current Objects
 *
 *  Global Plugin:
 *    A set of fixed functions used by modules to request data
 *
 **************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h> // for _set_sbh_threshold(0) - works around a MS crash bug

/* Mystuff */
#include "sys_base.h"
#include "LayoutRecorderPlugin.h"

#include <crtdbg.h>

GlobalFunc		*LW_globalFuncs	= 0;

DrawFuncs		*LW_drawFuncs		= 0;
LWPanelFuncs	*LW_panelFuncs		= 0;
LWMessageFuncs	*LW_msgsFuncs		= 0;
LWXPanelFuncs	*LW_xpanFuncs		= 0;
LWObjectFuncs	*LW_objFuncs		= 0;
LWSurfaceFuncs	*LW_surfFuncs		= 0;

LWObjectInfo	*LW_objInfo			= 0;
LWItemInfo		*LW_itemInfo		= 0;
LWBoneInfo		*LW_boneInfo		= 0;
LWInstUpdate	*LW_instUpdate		= 0;

LWCommandFunc   LW_cmdFunc		= (LWCommandFunc)0;

static int		Setup = 0;

void _Setup(GlobalFunc *global)
{
	int i = 0;
	if (Setup == 1) {
		return;
	}

    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) |
	_CRTDBG_LEAK_CHECK_DF);

	_set_sbh_threshold(0);
	Setup = 1;

	LW_globalFuncs = global;

	LW_msgsFuncs = (LWMessageFuncs *)LW_globalFuncs(LWMESSAGEFUNCS_GLOBAL,GFUSE_ACQUIRE );

	LW_panelFuncs = (LWPanelFuncs *)LW_globalFuncs(PANEL_SERVICES_NAME, GFUSE_ACQUIRE );
	if(!LW_panelFuncs)
	{
		(*LW_msgsFuncs->error)("Unable to activate global "PANEL_SERVICES_NAME,
						"     please add plugin lwpanels.p" );
		return;
	}
	LW_drawFuncs = LW_panelFuncs->drawFuncs;

	LW_objInfo = (LWObjectInfo *)LW_globalFuncs( LWOBJECTINFO_GLOBAL, GFUSE_ACQUIRE  );
	LW_objFuncs = (LWObjectFuncs *)LW_globalFuncs( LWOBJECTFUNCS_GLOBAL, GFUSE_ACQUIRE  );
	LW_itemInfo = (LWItemInfo *)LW_globalFuncs( LWITEMINFO_GLOBAL, GFUSE_ACQUIRE  );
	LW_surfFuncs = (LWSurfaceFuncs *)LW_globalFuncs( LWSURFACEFUNCS_GLOBAL, GFUSE_ACQUIRE  );

	LW_xpanFuncs = (LWXPanelFuncs *)LW_globalFuncs( LWXPANELFUNCS_GLOBAL, GFUSE_ACQUIRE  );
	LW_boneInfo = (LWBoneInfo *)LW_globalFuncs( LWBONEINFO_GLOBAL, GFUSE_ACQUIRE  );

	LW_cmdFunc = (LWCommandFunc)LW_globalFuncs( LWCOMMANDINTERFACE_GLOBAL, GFUSE_ACQUIRE );

	LW_instUpdate = (LWInstUpdate *)LW_globalFuncs( LWINSTUPDATE_GLOBAL, GFUSE_ACQUIRE );
}

/* 
 * HANDLER
 *
 * Called when the plug is added to an item.
 * It seems that it is also evaluated at each frame and points as well,
 * so be aware that that might occur
 */
	XCALL_(static int)
MASTERHANDLER_Activate (long version,GlobalFunc *global,
								LWMasterHandler *local,void *serverData)
{
	int						retval = AFUNC_OK, i = 0;

	XCALL_INIT;
    if ( version != LWMASTER_VERSION ) return AFUNC_BADVERSION;

	_Setup(global);

	SetupMasterFuncs(local);

	return AFUNC_OK;
};

/* Main setup function for ItemMotion Handler (entry)
 *
 * Called when the plug is added to an item.
 * It seems that it is also evaluated at each frame and points as well,
 * so be aware that that might occur
 */
	XCALL_(static int)
MOTHANDLER_Activate (long version,GlobalFunc *global,
								LWItemMotionHandler  *local,void *serverData)
{
	int						retval = AFUNC_OK, i = 0;

	XCALL_INIT;
    if ( version != LWITEMMOTION_VERSION ) return AFUNC_BADVERSION;

	_Setup(global);

	SetupMotionFuncs(local);

	return AFUNC_OK;
};

	XCALL_(static int)
GENERICHANDLER_Activate (long version,GlobalFunc *global,
								LWLayoutGeneric *local,void *serverData)
{
	int						retval = AFUNC_OK, i = 0;

	XCALL_INIT;
    if ( version != LWLAYOUTGENERIC_VERSION ) return AFUNC_BADVERSION;

	_Setup(global);

	ExecuteGenericFuncs(local);

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
MOTINTERFACE_Interface (
	long			 version,
	GlobalFunc		*global,
	LWInterface		*local,
	void			*serverData)
{

   if ( version != LWITEMMOTION_VERSION ) return AFUNC_BADVERSION;

	_Setup(global);

	local->panel	= NULL;
	local->options	= NULL;
	local->command	= NULL;

	return AFUNC_OK;
}


/*
 * GLOBAL setup function
 *
 * This function is called when the interface is setup
 * When the plugin is double clicked the options function is called
 *
 */
	XCALL_(static int)
GLOBALSERVICE_Initiate (
	long			 version,
	GlobalFunc		*global,
	void			*inst,
	void			*serverData)
{
   if ( version != LWGLOBALSERVICE_VERSION ) return AFUNC_BADVERSION;

	_Setup(global);
	SetupGlobalFuncs(inst);

	return AFUNC_OK;
}

ServerRecord ServerDesc[] = {
    { LWITEMMOTION_HCLASS,		MRB_HIDDEN_MOTIONNAME,	MOTHANDLER_Activate},
    { LWITEMMOTION_ICLASS,		MRB_HIDDEN_MOTIONNAME,	MOTINTERFACE_Interface},

    { LWMASTER_HCLASS,			MRB_HIDDEN_MASTERNAME,	MASTERHANDLER_Activate},

    { LWLAYOUTGENERIC_CLASS,	MRB_GENERIC,		GENERICHANDLER_Activate},

    { LWGLOBALSERVICE_CLASS,	LWMRBEXPORT_GLOBAL,	GLOBALSERVICE_Initiate},
	{ NULL }
};
