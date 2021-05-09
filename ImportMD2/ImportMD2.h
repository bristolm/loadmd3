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
#include "lwhost.h"
#include "lwgeneric.h"

#include "lw_base.h"

#define PLUGINNAME			"MRB::Import::MD2"

// Object Import function
extern int Load_MD2(LWObjectImport *local);

extern unsigned long LW_sysInfo;

extern GlobalFunc	*LW_globalFuncs;
extern LWCommandFunc LW_cmdFunc;

typedef struct 
{
	int frame;
	int makemorphs;
} GUIData;

extern GUIData *Get_FrameNum(int range);
