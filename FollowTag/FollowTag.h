/**************************************
 *
 *  FollowTag.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  This file must remain as 'C' code
 *
 *  Header for locking an object's motion to another 
 *  object's tag polygon
 *
 **************************************/
#include "lwxpanel.h"
#include "lwdisplce.h"
#include "lwmeshes.h"

#include "lw_base.h"

#define PLUGINNAME			"MRB::TagMover"

// Tag moving structs
typedef struct
{
	LWMeshInfo	*mesh;
	LWTime		time;
	LWFrame		frame;
	int			vcounted;
	LWPntID		points[3];
	LWDVector	vectors[3];
} FrameTracker;

typedef struct
{
	LWItemID		IDs[3];		// owner, target
	LWXPanelID		xpan;
	int				lasttargetidx;
	char			lasttargetname[255];
	char			tagname[255];
	FrameTracker	tracker;
} PlugInstance;

#define OWNER_ID(data)	(data->IDs[0])
#define TARGET_ID(data)	(data->IDs[1])

// GUI functions
extern int			CountObjs  ( void *userdata );
extern int			IndexByObjID ( PlugInstance *data, LWItemID objID );
LWItemID			ObjIDByIndex ( PlugInstance *data, int idx );
extern const char	*ObjNameByIndex ( void *userdata, int idx );

extern void			FindObjectAfterLoading( PlugInstance *data );

extern GlobalFunc	*LW_globalFuncs;

extern LWXPanelID	GetPanel(LWInstance *inst);
extern void			SetupFuncs(LWDisplacementHandler *local);
