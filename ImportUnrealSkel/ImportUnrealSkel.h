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

#define PLUGINNAME			"MRB::Import::UnrealSkel"

typedef struct
{
	void		*Animations;
	void		*Skeleton;
	int			SelectedAnimation;
} AnimLoader;

// Object Import function
extern int Load_PSA(LWObjectImport *local);
extern int Load_PSK(LWObjectImport *local);

extern int GLoad_PSA(LWLayoutGeneric *local,  AnimLoader *anim);
extern int GLoad_PSK(LWLayoutGeneric *local,  AnimLoader *anim);

extern AnimLoader *Get_PSAFile(void);

extern unsigned long LW_sysInfo;

extern GlobalFunc	*LW_globalFuncs;
extern LWCommandFunc LW_cmdFunc;


