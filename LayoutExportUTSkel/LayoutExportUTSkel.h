/**************************************
 *
 *  LayoutExportUTSkel.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  This file must remain as 'C' code
 *
 *  Main header file for the Unreal Skeletal export module that
 *  interfaces with my LayoutRecorder plugin
 *
 *  Unreal Tounament is a trademark of Epic MegaGames, Inc.
 *
 **************************************/

#ifndef _LW_LAYOUTEXPORTMD3_H
#define _LW_LAYOUTEXPORTMD3_H

#include <lwserver.h>
#include <lwhost.h>
#include <lwsurf.h>
#include <lwglobsrv.h>
#include <lwmrbxprt.h>

#define PLUGINNAME			LWMRBPREFIX"UnrealSkel"
#define GLOBALPLUGINNAME	LWMRBPREFIX"UnrealSkelGlobal"

#define _PLUGINVERSION(a)		1.##a
#define PLUGINVERSION		(float)_PLUGINVERSION(PROG_PATCH_VER)

extern GlobalFunc		*LW_globalFuncs;
extern LWMRBExportFuncs	*LW_xprtFuncs;
extern LWMessageFuncs	*LW_msgsFuncs;
extern LWXPanelFuncs	*LW_xpanFuncs;

extern LWMRBExportType	 *getFunc();
extern LWMRBCallbackType *getCallback();

#endif //_LW_LAYOUTEXPORTMD3_H
