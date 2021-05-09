/**************************************
 *
 *  MotFuncs.cpp
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

#include "lwmotion.h"

/* Mystuff */
#include "sys_base.h"

extern "C"
{
#include "FollowTag.h"
}

/*
 * Layout DIsplacement handler plug-in.
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

	data->owner = item;

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
    return (NULL);
}

// Save to file
XCALL_(static LWError)
SAVE(LWInstance inst,const LWSaveState *ss)
{
	return (NULL);
}

/* Init
 *
 *  Called befpre the first frame is rendered
 *  The mode will be either LWINIT_PREVIEW or LWINIT_RENDER. 
 *  Return error string if something bad happens, otherwise null
 */
XCALL_(static LWError)
INIT(LWInstance inst,int mode)
{
	return NULL;
}

/* Evaluation
 *
 * Find the object we own, and then get the specified tag's position
 */
XCALL_(static void)
EVALUATE(LWInstance inst, const LWItemMotionAccess  *ma)
{
} 

/*  Always world coordinates
 */
XCALL_(static unsigned int)
FLAGS(LWInstance inst)
{
    XCALL_INIT;
    return (LWIMF_AFTERIK);
}

/* Description line
 *
 * Shows up in the plug-in selection 
 */
XCALL_(static const char *)
DESCLN(LWInstance inst)
{
    static char line[256];

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
	// FIXME - we'll need to care about various things at some point
	return NULL;
}

/* Item ID Changing
 *
 * Iteams in LW do not have static IDs - I need to update my innards when
 * they change.  The array is like - old, new, old, new ...
 */
XCALL_(static void)
CHANGEID(LWInstance inst, const LWItemID *changedids)
{
	// FIXME - we'll need to care about various things at some point
	return;
}

// Initialization
void SetupMotionFuncs(LWItemMotionHandler *local)
{
	local->inst->create   = CREATE;
	local->inst->destroy  = DESTROY;
	local->inst->load     = LOAD;
	local->inst->save     = SAVE;
	local->inst->copy     = COPY;
	local->inst->descln   = DESCLN;

	local->item->useItems = USEITEMS;
	local->item->changeID = CHANGEID;

	local->evaluate       = EVALUATE;
	local->flags          = FLAGS;
}
