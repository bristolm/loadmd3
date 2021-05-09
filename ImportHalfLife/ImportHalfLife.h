/**************************************
 *
 *  ImportUnrealSkel.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  This file must remain as 'C' code
 *
 *  Import Unreal Tournament PSA and PSK object formats
 *
 *  Unreal Tounament is a trademark of Epic MegaGames, Inc.
 *
 **************************************/
#include "lwobjimp.h"
#include "lwmeshes.h"
#include "lwpanel.h"
#include "lwhost.h"
#include "lwhost.h"
#include "lwgeneric.h"

#include "lw_base.h"

#define PLUGINNAME			"MRB::Import::HalfLife"

// Object Import function
extern int Load_MDL(LWObjectImport *local);

extern unsigned long LW_sysInfo;

extern GlobalFunc	*LW_globalFuncs;
extern LWCommandFunc LW_cmdFunc;


