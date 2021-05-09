/**************************************
 *
 *  DispFuncs.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Locks an object's motion to another Object's tag poly
 *  .. in theory
 *
 **************************************/

/* Layout include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <lwenvel.h>


/* Mystuff */
#include "sys_math.h"

extern "C"
{
#include "FollowTag.h"
}

/*
 * Layout Displacement handler plug-in.
 *
 * Records the position/orientation of the NULLs and Objects
 * that might be used by the Master Plugin
 */

// Create an instance
XCALL_(static LWInstance)
CREATE(void *priv, LWItemID item, LWError *err )
{
	PlugInstance *data = new PlugInstance;
	memset(data,0,sizeof(PlugInstance));

	data->IDs[0] = item;
	data->IDs[1] = LWITEM_NULL;

	return (LWInstance)data;
}

// Destroy an instance - called when the plug is removed, or
// Lightwave is shut down
XCALL_(static void)
DESTROY(LWInstance inst)
{
    XCALL_INIT;	

	delete (PlugInstance *)inst;
}

// copy an instance ... 
XCALL_(static LWError)
COPY(void *to, void *from)
{
    XCALL_INIT;
	
    return (NULL);
}

// Load from file
XCALL_(static LWError)
LOAD(LWInstance inst,const LWLoadState *ls)
{
	PlugInstance *data = (PlugInstance *)inst;
	// Load in the name of the relevant object
	LWLOAD_STR(ls,data->lasttargetname,sizeof(data->lasttargetname));
	data->lasttargetidx = -1;

	// Load in the tag surface name
	LWLOAD_STR(ls,data->tagname,sizeof(data->tagname));

    return (NULL);
}

// Save to file
XCALL_(static LWError)
SAVE(LWInstance inst,const LWSaveState *ss)
{
	PlugInstance *data = (PlugInstance *)inst;
	if (data->lasttargetidx == -1)
		FindObjectAfterLoading(data);

	// Spit out the name of the relevant object
	LWItemInfo *LW_itemInfo
		= (LWItemInfo *)LW_globalFuncs( LWITEMINFO_GLOBAL, GFUSE_TRANSIENT );
	LWSAVE_STR(ss,LW_itemInfo->name(data->IDs[1]));

	// Spit out the tag surface name
	LWSAVE_STR(ss,data->tagname);

	return (NULL);
}

/* New Time
 *
 */
XCALL_(static LWError )
NEWTIME(LWInstance inst, LWFrame frame, LWTime time)
{
	PlugInstance *data = (PlugInstance *)inst;
	if (data->lasttargetidx == -1)
		FindObjectAfterLoading(data);

	// Init a new tracking structure
	memset(&(data->tracker),0,sizeof(data->tracker));

	data->tracker.time = time;
	data->tracker.frame = frame;
	data->tracker.vcounted = -1;
	
	return (NULL);
}

// check for surface
int ScanPolys (void *inst, LWPolID pol)
{
	PlugInstance *data = (PlugInstance *)inst;
	LWMeshInfo *m = data->tracker.mesh;

	const char *tag = m->polTag(m,pol,LWPTAG_SURF);
	if (strcmp(tag,data->tagname))
		return 0;
	if (m->polSize(m,pol) < 3)
		return 0;

	// matched, grab three point ids and stop
	for (int i = 0;i < 3; i++)
	{
		data->tracker.points[i] = m->polVertex(m,pol,i);
	}
	return 3;
}


/* Evaluation
 *
 * Find the object we own, and then get the specified tag's position
 */
XCALL_(static void)
EVALUATE(LWInstance inst, LWDisplacementAccess   *da)
{
	PlugInstance *data = (PlugInstance *)inst;

	// IF we've done all 3, quit out now
	if (data->tracker.vcounted == 3)
		return;

	// If this is the first time through, find the 3 points
	// on the first poly that matches our tag name
	if (data->tracker.vcounted == -1)
	{
		data->tracker.mesh = da->info;
		da->info->scanPolys(da->info,ScanPolys,data);
		data->tracker.vcounted = 0;
	}

	// Check this point against the 3 I'm tracking.
	for (int i = 0; i < 3; i++)
	{
		if (da->point == data->tracker.points[i])
		{
			data->tracker.vectors[i][0] = da->source[0];
			data->tracker.vectors[i][1] = da->source[1];
			data->tracker.vectors[i][2] = da->source[2];
			++(data->tracker.vcounted);
			break;
		}
	}

	// If we are done, position our target
	if (data->tracker.vcounted != 3)
		return;

	// We now have 3 points, let's get vectors out of them
	Vector<double>	pos(data->tracker.vectors[2]);

	// #2 is the anchor point
	// #0 is the 'front'
	Vector<double>	Z = Vector<double>(data->tracker.vectors[0]) - pos;
	Vector<double>	X = Vector<double>(data->tracker.vectors[1]) - pos;

	// Cross to get the 3rd vector.
	Vector<double>	Y = X.cross(Z);

	// Normalize Z and Y and refigure X so they all are unit vectors
	Z = Z.normalize();
	Y = Y.normalize();
	X = Z.cross(Y).normalize();

	Vector<double>	rot = Matrix<double>(X,Y,Z).getEuler();

	// Now these need to be made into angles and stashed
	LWItemInfo *LW_itemInfo =
		(LWItemInfo *)LW_globalFuncs( LWITEMINFO_GLOBAL, GFUSE_TRANSIENT );

	if (TARGET_ID(data) == LWITEM_NULL)
		return;

	LWChannelInfo *chanInfo
			= (LWChannelInfo *)LW_globalFuncs( LWCHANNELINFO_GLOBAL, GFUSE_TRANSIENT );
	LWEnvelopeFuncs *envFuncs
			= (LWEnvelopeFuncs *)LW_globalFuncs( LWENVELOPEFUNCS_GLOBAL, GFUSE_TRANSIENT );
	LWChanGroupID group = LW_itemInfo->chanGroup(TARGET_ID(data));

	// Set some keyframe positions
	LWChannelID chan = chanInfo->nextChannel(group,0);
	while (chan != (LWChannelID)0)
	{
		const char *cname = chanInfo->channelName(chan);
		const LWEnvelopeID env = chanInfo->channelEnvelope(chan);
		double val = 0;

		// ready the next
		chan = chanInfo->nextChannel(group,chan);

		if		(strcmp(cname,"Position.X") == 0)
			val = pos[0];
		else if (strcmp(cname,"Position.Y") == 0)
			val = pos[1];
		else if (strcmp(cname,"Position.Z") == 0)
			val = pos[2];
		else if (strcmp(cname,"Rotation.H") == 0)
			val = -rot[1];
		else if (strcmp(cname,"Rotation.P") == 0)
			val = rot[0];
		else if (strcmp(cname,"Rotation.B") == 0)
			val = -rot[2];
		else
			continue;

		envFuncs->createKey(env,data->tracker.time,val);

	}
} 

/*  Always LOCAL coordinates - forces parenting
 */
XCALL_(static unsigned int)
FLAGS(LWInstance inst)
{
    XCALL_INIT;
    return 0;
}

/* Description line
 *
 * Shows up in the plug-in selection 
 */
XCALL_(static const char *)
DESCLN(LWInstance inst)
{
    static char line[256];
	PlugInstance *data = (PlugInstance *)inst;

	if (data->lasttargetidx == -1)
		FindObjectAfterLoading(data);

	LWItemInfo *LW_itemInfo
		= (LWItemInfo *)LW_globalFuncs( LWITEMINFO_GLOBAL, GFUSE_TRANSIENT );

	const char *name = LW_itemInfo->name(TARGET_ID(data));
	if (name == 0)
		name = "(unknown)";

	char *tag = data->tagname[0] ? data->tagname : "(unknown)";

	sprintf(line,"Moving %s with %s",name,tag);

    return line;
}

/* Item list
 *
 * This is a list of items (array of LWItemID terminated by LWITEM_NULL
 * or LWITEM_ALL, or just null) I care about
 */
XCALL_(static const LWItemID *)
USEITEMS(LWInstance inst)
{
	PlugInstance *data = (PlugInstance *)inst;

	return &(data->IDs[0]);
}

/* Item ID Changing
 *
 * Iteams in LW do not have static IDs - I need to update my innards when
 * they change.  The array is like - old, new, old, new ...
 */
XCALL_(static void)
CHANGEID(LWInstance inst, const LWItemID *changedids)
{
	if (changedids == 0)
		return;

	PlugInstance *data = (PlugInstance *)inst;
	if (data->lasttargetidx == -1)
		FindObjectAfterLoading(data);

	int i = 0;
	while (changedids[i] != 0)
	{
		if (changedids[i] == OWNER_ID(data))
			OWNER_ID(data) = changedids[i +1];
		if (changedids[i] == TARGET_ID(data))
			TARGET_ID(data) = changedids[i +1];

		i += 2;
	}

	return;
}

// Initialization
void SetupFuncs(LWDisplacementHandler *local)
{
	local->inst->create   = CREATE;
	local->inst->destroy  = DESTROY;
	local->inst->copy     = COPY;
	local->inst->load     = LOAD;
	local->inst->save     = SAVE;
	local->inst->descln   = DESCLN;

	local->rend->init	  = 0;
	local->rend->newTime  = NEWTIME;
	local->rend->cleanup  = 0;

	local->item->useItems = USEITEMS;
	local->item->changeID = CHANGEID;

	local->evaluate       = EVALUATE;
	local->flags          = FLAGS;
}
