/**************************************
 *
 *  MotionHandler_Funcs.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Records the position/orientation of the NULLs and Objects
 *  that might be used by the Master Plugin
 *
 *  This plugin is 'hidden' and is created by the ListenChannel
 *  objects as needed - specifically, each LISTEN_BONE ListenChannel
 *  is attached to an instance of this motionhandler for the bone in question.
 *  The plugin and its interface are hidden, and exist as long as the
 *  ListenChannel does
 *
 **************************************/

/* Layout include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "lwdisplce.h"
#include "lwmotion.h"
#include "lwhost.h"
#include "lwsurf.h"

/* Mystuff */
#include "sys_base.h"

extern "C"
{
#include "LayoutRecorderPlugin.h"
}

#include "listenchannel.h"

/*
 * Layout Motion handler plug-in.
 *
 * Records the position/orientation of the NULLs and Objects
 * that might be used by the Master Plugin
 */

// Create an instance
XCALL_(static LWInstance)
CREATE(void *priv, LWItemID item, LWError *err )
{
    XCALL_INIT;

	// Find the channel we are attached to
	// 'item' is probably a bone - if it is, find the parent object attached to it
	const char *cMap = LW_boneInfo->weightMap(item);
	const char *cName = LW_itemInfo->name(item);

	while (LW_itemInfo->type(item) == LWI_BONE)
	{
		item = LW_itemInfo->parent(item);
		if (item == LWITEM_NULL)
			break;
	}
	if (item == LWITEM_NULL)
		return 0;
	
	ObjectWrapper *lcg = ObjectWrapper::findWrapper(item);
	if (lcg == 0)
	{
		return 0;
	}

	for (int i = lcg->ChannelCount() -1;i >= 0; i--)
	{
		ListenChannel *lc = lcg->getChannel(i);
		if (lc == 0)
			continue;

		if (lc->getFlavor() != LISTEN_BONE)
			continue;

		// Check the map name
		if (cMap != (char *)0 && strcmp(lc->getName(),cMap) == 0)
			return (LWInstance)(lcg->getChannel(i));

		// Check the bone name too ...
		if (cName != (char *)0 && strcmp(lc->getName(),cName) == 0)
			return (LWInstance)(lcg->getChannel(i));
	}

	return (LWInstance)0;

/*    lp = calloc(sizeof(struct lpdata),1);
    lp->id = LWITEM_NULL;
    lp->time = 0.0;
    lp->lagrate = .25;
    return ((LWInstance) lp);
	*/
}

// Destroy an instance - called when the plug is removed, or
// Lightwave is shut down
XCALL_(static void)
DESTROY(LWInstance inst)
{
    XCALL_INIT;	


	// Don't do anything - deleting the Displacement Handler does this
	inst = 0;
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
 * Called for each frame - pull things from the LWDisplacementAccess object
 *
 * This is necessary to determine the location of objects - the 'My Position'
 * selection
 */
XCALL_(static void)
EVALUATE(LWInstance inst, const LWItemMotionAccess  *ma)
{
	// Record orientation and position of the tag
    LWFrame		frame = ma->frame;
    LWTime		time  = ma->time;

	ListenChannel *lc = (ListenChannel *)inst;
	lc->setActiveFrame(ma->frame, ma->time);

	lc->processMotion(ma);
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
	static char defln[] = "No Channels defined";
    static char line[256];

    XCALL_INIT;

	ListenChannel *lc = (ListenChannel *)inst;

	sprintf(line, "Tracking ... %s", lc->getName());

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
