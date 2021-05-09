/**************************************
 *
 *  LayoutRecorderPlugin.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Main header file for the Layout Recorder plugin
 *  This file must remain as 'C' code
 *
 **************************************/

#ifndef _LW_EXPORTQUAKE_H
#define _LW_EXPORTQUAKE_H

/* Layout include file */
#include <lwrender.h>
#include <lwserver.h>

#include <lwglobsrv.h>
#include <lwgeneric.h>
#include <lwmotion.h>
#include <lwdisplce.h>
#include <lwmeshes.h>
#include <lwsurf.h>
#include <lwmaster.h>

#include <lwxpanel.h>
#include <lwhost.h>

#include "lw_base.h"

#include "lwmrbxprt.h"

#define MRB_HIDDEN_MOTIONNAME	".MRB::MotionRecorder"
#define MRB_HIDDEN_MASTERNAME	".MRB::ExportMaster"
#define MRB_GENERIC				"MRB::ExportControl"

#define MRB_FRAMEBANK_EXTENSION	".fmb"

#define _PLUGINVERSION(a)	1.##a
#define PLUGINVERSION		(float)_PLUGINVERSION(PROG_PATCH_VER)
#define PLUGINREV			_DEF_AS_STRING(_PLUGINVERSION(PROG_PATCH_VER))

// Global values
extern GlobalFunc		*LW_globalFuncs;
extern DrawFuncs		*LW_drawFuncs;
extern LWPanelFuncs		*LW_panelFuncs;
extern LWMessageFuncs	*LW_msgsFuncs;
extern LWXPanelFuncs	*LW_xpanFuncs;
extern LWObjectFuncs	*LW_objFuncs;
extern LWSurfaceFuncs	*LW_surfFuncs;

extern LWObjectInfo		*LW_objInfo;
extern LWItemInfo		*LW_itemInfo;
extern LWBoneInfo 		*LW_boneInfo;
extern LWInterfaceInfo	*LW_intfInfo;

extern LWInstUpdate		*LW_instUpdate;

extern LWCommandFunc	 LW_cmdFunc;

// Setup functions for each plugin (just initializes their functions
extern void _Setup(GlobalFunc *global);
extern void SetupDisplacementFuncs(LWDisplacementHandler *);
extern LWXPanelID SetupDisplacementXPanel(void *inst);
extern void SetupMasterFuncs(LWMasterHandler  *local);
extern void SetupMotionFuncs(LWItemMotionHandler *local);
extern void SetupGlobalFuncs(LWGlobalService *local);
extern void ExecuteGenericFuncs(LWLayoutGeneric  *local);

extern void UpdateEntireEnvironment(void *targetwrapper);

extern int ClearingScene;

#endif // _LW_EXPORTQUAKE_H
