/**************************************
 *
 *  ImportMD2.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  This file must remain as 'C' code
 *
 *  Import Quake2 MD2 format files
 *
 **************************************/
#include "lwobjimp.h"
#include "lwmeshes.h"
#include "lwpanel.h"
#include "lwhost.h"
#include "lwrender.h"
#include "lwgeneric.h"

#include "lw_base.h"

#define PLUGINNAME			"MRB::MorfToFrame"

extern unsigned long LW_sysInfo;
extern GlobalFunc	*LW_globalFuncs;

typedef struct
{
	LWItemID	Object;
	char		GroupName[255];
	int			StartFrame;
} FrameInstructions;

typedef struct 
{
	int frame;
	int makemorphs;
} GUIData;

extern FrameInstructions **Get_BuildData(LWLayoutGeneric *local);
extern int BuildFrames(LWLayoutGeneric *local, FrameInstructions **data);
