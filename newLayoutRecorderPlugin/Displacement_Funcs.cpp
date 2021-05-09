/**************************************
 *
 *  Displacement_XPanels.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Records the motion of a number of objects, and feeds that data into
 *  an attached ListenChannel
 *
 *  Any Module plugin requesting to use the LayoutRecoder must
 *  interface through this.  When a module is created, it does not
 *  use it's own interface function, but instead requests an interface
 *  from LayoutRecorder by calling the Global function:
 *    (LWMRBExportFuncs *)->get_interface
 *  The below functions are then used at that plugin's interface
 *
 **************************************/

/* Layout include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <lwdisplce.h>
#include "lwhost.h"
#include "lwsurf.h"

/* Mystuff */

extern "C"
{
#include "LayoutRecorderPlugin.h"
}

#include "Displacement_XPanels.h"

//	Standard instance handler functions.

// Create an instance
XCALL_(static LWInstance)
CREATE(void *priv, LWItemID item, LWError *err )
{
    XCALL_INIT;

	// priv should be a LWMRBExportType pointer - has the build function
	// and some user stuff

	// Build a 'bridge' object that directs points into a model
	// in the global space
	DisplacementXPanelInstance *d = new DisplacementXPanelInstance(item, (LWMRBExportType *)priv);
	return (LWInstance)d;
}

// Destroy an instance - called when the plug is removed, or
// Lightwave is shut down
XCALL_(static void)
DESTROY(LWInstance inst)
{
    XCALL_INIT;

	// Need to remove the references
    if (inst)
		delete (DisplacementXPanelInstance *)inst;
}

// copy an instance ... 
XCALL_(static LWError)
COPY(void *to, void *from)
{
    XCALL_INIT;
	
    DisplacementXPanelInstance *last = (DisplacementXPanelInstance *) from;
    DisplacementXPanelInstance *next = (DisplacementXPanelInstance *) to;
    *last = *next;
	
    return (NULL);
}

// Lump values for Load/Save
#define ID_MDLX  LWID_( 'X','P','T','D' )
static LWBlockIdent idmain[] = {
	ID_MDLX, "Model",
	0
};

// Load from file - index is absolute
XCALL_(static LWError)
LOAD(LWInstance inst,const LWLoadState *load)
{
    XCALL_INIT;

	// Read the current reference index
	short n[4];
	n[0] = n[1] = n[2] = n[3] = 0;

	LWID id = LWLOAD_FIND( load, idmain);
	LWLOAD_I2(load, n, 4);

	DisplacementXPanelInstance *disp = (DisplacementXPanelInstance *)inst;
	ExportModelWrapper *lpi = ExportModelWrapper::getByCacheIndex(n[0]);

	disp->lockNewPanel(lpi);
	disp->iListChoice = -1;
	
    return (NULL);
}

// Save to file - index is absolute
XCALL_(static LWError)
SAVE(LWInstance inst,const LWSaveState *save)
{
    XCALL_INIT;

	// Dump out the current reference index
	DisplacementXPanelInstance *disp = (DisplacementXPanelInstance *)inst;

	short n[4];
	n[0] = disp->getActiveRef() ? disp->getActiveRef()->getCacheIndex() : -1;
	n[1] = n[2] = n[3] = 0;

	LWSAVE_BEGIN( save, &idmain[ 0 ], 1 );
	LWSAVE_I2(save, n, 4);
	LWSAVE_END( save  );

    return (NULL);
}

/* Init
 *
 *  Called before the first frame is rendered
 *  The mode will be either LWINIT_PREVIEW or LWINIT_RENDER. 
 *  Return error string if something bad happens, otherwise null
 */
XCALL_(static LWError)
INIT(LWInstance inst,int mode)
{
	return NULL;
}


/* NewTime
 *
 *  Called for each new frame
 */
XCALL_(static LWError)
NEWTIME(LWInstance inst, LWFrame f,LWTime t)
{
    XCALL_INIT;

	DisplacementXPanelInstance *disp = (DisplacementXPanelInstance *)inst;
	ExportModelWrapper *lpi = disp->getActiveRef();
	if (lpi)
		lpi->beginFrame(f,t);

    return (NULL);
}

/* Evaluation
 *
 * Called for each frame - pull things from the LWDisplacementAccess object
 */
XCALL_(static void)
EVALUATE(LWInstance inst, LWDisplacementAccess *da)
{
	DisplacementXPanelInstance *disp = (DisplacementXPanelInstance *)inst;
	ExportModelWrapper *lpi = disp->getActiveRef();
	if (lpi)
		lpi->processVertex(da);
}

/* Cleanup
 *
 * Called after the last frame is completed
 */
XCALL_(static void)
CLEANUP(LWInstance inst)
{
//	((SurftoMeshMap *)inst)->EndFrame();
	return;
}

/*  Always world coordinates
 */
XCALL_(static unsigned int)
FLAGS(LWInstance inst)
{
    XCALL_INIT;
    return (LWDMF_WORLD);
}

/* Description line
 *
 * Shows up in the plug-in selection 
 */
XCALL_(static const char *)
DESCLN(LWInstance inst)
{
	return ((DisplacementXPanelInstance *)inst)->getDescription();
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
void SetupDisplacementFuncs(LWDisplacementHandler *local)
{
	local->inst->create   = CREATE;
	local->inst->destroy  = DESTROY;
	local->inst->load     = LOAD;
	local->inst->save     = SAVE;
	local->inst->copy     = COPY;
	local->inst->descln   = DESCLN;

	local->item->useItems = USEITEMS;
	local->item->changeID = CHANGEID;

	local->rend->init     = INIT;
	local->rend->cleanup  = CLEANUP;
	local->rend->newTime  = NEWTIME;

	local->evaluate       = EVALUATE;
	local->flags          = FLAGS;
}


