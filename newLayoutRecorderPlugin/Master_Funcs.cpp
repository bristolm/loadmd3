/**************************************
 *
 *  Master_Funcs.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Layout Master Server handler plug-in.
 *
 *  Records the frame -> frame mapping (dopesheet) for a given model
 *  This allows us to only save the frames we want in the places we want
 *  This allows the frameloop information to be saved/loaded with the scene
 *
 *  One instance for all models - on save it iterates through them all
 *  and dumps out specific information
 *
 **************************************/


/* Layout include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <lwmaster.h>
#include <lwio.h>
#include <lwhost.h>
#include <lwsurf.h>

/* Mystuff */
#include "sys_base.h"

extern "C"
{
#include "LayoutRecorderPlugin.h"
}

#include "ExportPanels.h"

int ClearingScene = 0;

// Create an instance
XCALL_(static LWInstance)
CREATE(void *priv, LWItemID item, LWError *err )
{
    XCALL_INIT;

	// Reset this
	ClearingScene = 0;

	// Returns a 'bridge' object that directs points into a MD3
	// in the global space
	return (LWInstance)1;
}

// Destroy an instance - called when the plugin is removed, or
// Lightwave is shut down, or the scene is cleared
XCALL_(static void)
DESTROY(LWInstance inst)
{	// Set this so we don't do any LWInstance updates
	ClearingScene = 1;

	// Release one ref from all wrappers so these go away
	for (int midx = 0; midx < ExportModelWrapper::getCount(); midx++)
	{
		ExportModelWrapper *lpi = ExportModelWrapper::getByCacheIndex(midx);
		if (lpi == 0)
			continue;
		lpi->releaseRef(0);
	}
}

#define			_TMP_BUFF_SIZE	4096
static char		tmp[_TMP_BUFF_SIZE];

/*
  Ouput file format looks like this (FAKE means it's spacer):

{ ExportData
  VERSION
  { ModelFile
    INDEX
    ModelData FLAGS LOOPSET_INDEX FAKE
	[ ModelDataFile or MODEL_DATA ]
    StartFrames [COUNT] [[LOOP_IDX] [START_FRAME]]+
	StartFrames ...
    DisplayFlags [COUNT] [[LOOP_IDX] [FLAGS]]+
	DisplayFlags ...
  }
  { ModelFile ... }
}

 */

// Dump values for Load/Save
#define ID_XPTD  LWID_( 'X','P','T','D' )
static LWBlockIdent idmain[] = {
	ID_XPTD, "ExportData",
	0
};

#define ID_MODL  LWID_( 'M','O','D','L' )
#define ID_PLGF  LWID_( 'P','L','G','F' )
static LWBlockIdent idmodel[] = {
	ID_MODL, "Model",		// Model Wrapper
	ID_PLGF, "PluginRef",		// One per plugin type
	0
};

#define ID_STFR  LWID_( 'S','T','F','R' )
#define ID_DSPF  LWID_( 'D','S','P','F' )
#define ID_MDFI  LWID_( 'M','D','F','I' )
#define ID_LPST  LWID_( 'L','P','S','T' )

// 
static LWBlockIdent idmodelelement[] = {
	ID_MDFI, "ModelDataFile",	// Framebank file label
	ID_LPST, "LoopSet",			// List of frames
	ID_PLGF, "PluginRef",		// One per plugin type
	ID_STFR, "StartFrames",		// Start frames by loop [index][start]
	ID_DSPF, "DisplayFlags",	// Flags for each loop  [index][flags]
	0
};

// Model File bank specific tags
#define ID_MDTA  LWID_( 'M','D','T','A' )
static LWBlockIdent idmodeldata[] = {
	ID_MDTA, "MODELDATA",		// ModelData file label
	0
};

// Loop specific tags
#define ID_LDTA  LWID_( 'L','D','T','A' )
#define ID_LPTN  LWID_( 'L','P','T','N' )
static LWBlockIdent idloopelements[] = {
	ID_LDTA, "LoopData",		// many of these ...
	ID_LPTN, "LoopPattern",		// many of these ...
	0
};
/*
 * Save the Model info - splitting this out makes it
 * easier to spit it to a separate file

  MODELDATA
  { LoopSet
    LoopData LOOP_NAME LOOP_IDX LENGTH FAKE FAKE
	LoopData ...
  }
  { PluginRef
    PLUGIN_NAME
    ... PLUGIN_DATA ...
  }

 */
void ExportModelWrapper::SAVE(const LWSaveState *save)
{
	short vals[5] = {0};
	float f[1];
	f[0] = PLUGINVERSION;

	// Save my model data

	// If there are no loops, don't bother doing this
	vals[0] = m_loops.getRangeCount();
	if (vals[0] != 0)
	{
		LWSAVE_BEGIN( save, &idmodelelement[ 1 ], 0 );	// ID_LPST

		LoopRange *l = 0;
		for (int i = 0;(l = m_loops.getRange(i)) != 0; i++)
		{
		 LWSAVE_BEGIN(save, &idloopelements[ 0 ], 1 );	// ID_LDTA
		  LWSAVE_STR(save,	l->getName());

		  vals[0] = l->getCacheID();
		  vals[1] = l->getLength();
		  vals[2] = vals[3] = 0;
		  LWSAVE_I2(save, vals, 4);

		 LWSAVE_END( save );							// ID_LDTA

		 // Only save out the comlpicated patterns
		 if (l->isSimplePattern())
			 continue;

		 LWSAVE_BEGIN(save, &idloopelements[ 1 ], 1 );	// ID_LPTN
		  LWSAVE_STR(save,	l->getPattern());
		 LWSAVE_END( save );							// ID_LPTN
		}
		LWSAVE_END( save );							// ID_LPST
	}

	// Ask my plugin for info
	if (m_userinfo != 0 && m_userinst != 0)
	{
		LWSAVE_BEGIN( save, &idmodelelement[ 2 ], 0 );	// ID_PLGF
		 LWSAVE_STR(save, m_userinfo->globalcallback);

		 // Call into the plugins and spit out the data for these frames
		 m_userfuncs->save(save,m_userinst);
		LWSAVE_END( save );							// ID_PLGF
	}

	return;
}

/*
 * Dump out dopesheet data from the file
 * We use another file to handle the frames
 */
LWError ExportModelWrapper::SAVE(LWInstance inst, const LWSaveState *save)
{
    XCALL_INIT;
	short loopvals[9] = {0};
	float f[1];
	f[0] = PLUGINVERSION;

	// We'll stash references to the different callback functions here
	// so we can query for 'global' information when we are done
	AutoArray<LWMRBExportType *>	SaveInfo(0);
	AutoArray<LWMRBCallbackType *>	SaveFuncs(0);

	// For exporting the frame list 
	LWFileIOFuncs *ioFuncs
				= (LWFileIOFuncs *)LW_globalFuncs( LWFILEIOFUNCS_GLOBAL, GFUSE_TRANSIENT );

	// Start this block
	LWSAVE_BEGIN( save, &idmain[ 0 ], 0 );		// ID_XPTD
	LWSAVE_FP(save,f,1);

	for (int midx = 0; midx < ExportModelWrapper::getCount(); midx++)
	{
		ExportModelWrapper *lpi = ExportModelWrapper::getByCacheIndex(midx);

		// Stash this func in the global area so we can ca;; it later
		if (SaveInfo.Next() == 0
			 || SaveInfo.find(lpi->m_userinfo) == SaveInfo.BAD_INDEX)
		{
			SaveInfo[SaveInfo.Next()] = lpi->m_userinfo;
			SaveFuncs[SaveFuncs.Next()] = lpi->m_userfuncs;
		}

		// Write out the Data for this Model
		//  .. <Name UniqueID Length Start> +
		loopvals[0] = loopvals[1] = loopvals[2] 
					= loopvals[3] = loopvals[4] = 0;
		LWSAVE_BEGIN( save, &idmodel[ 0 ], 0 );	  // ID_MODL

		// Save out flags and scene-based model info
		loopvals[0] = lpi->getUniqueIndex();
		loopvals[1] = lpi->getFlags();

		// Get m_listref the long way to be sure it is valid
		loopvals[2] = lpi->getLoopListProvider()->getCacheID();
		LWSAVE_I2(save, loopvals, 5);

		// Dump out frame list and model info
		char *cFile = lpi->getFile();
		LWSaveState *ssModel = 0;
		if (cFile[0] != 0)
		{
			ssModel = ioFuncs->openSave(cFile,LWIO_ASCII);
		}

		// Dump the FrameBank either to file, or here - do this first
		if (ssModel)
		{
			// Dump Framebank filename for Main save file
			LWSAVE_BEGIN( save, &idmodelelement[ 0 ], 1 );	// ID_MDFI
			 LWSAVE_STR( save, cFile);
			LWSAVE_END( save );								// ID_MDFI

			// Dump frames
			LWSAVE_BEGIN( ssModel, &idmodeldata[ 0 ], 1 );	// ID_MODL
			LWSAVE_END( ssModel );							// ID_MODL
			LWSAVE_FP( ssModel,f,1);

			lpi->SAVE(ssModel);

			// Close the frame bank file
			ioFuncs->closeSave(ssModel);
		}
		else
		{
			lpi->SAVE(save);
		}

		LoopRange *rout[8] = {0};

		// Save out any data I may be caching regarding my list
		int iSaved = 0;
		int iMax = lpi->m_loops.getRangeCount() -1;
		for (int i = 0; i <= iMax; i++)
		{
			LoopRange *r = lpi->m_loops.getRange(i);
			if (r->getOffset() != 0)
				rout[iSaved++] = r;

			if (iSaved == 8 || 
				(iSaved > 0 && i == iMax))
			{	// Dump what we have [count][[index][select state]+ ]
				LWSAVE_BEGIN( save, &idmodelelement[ 3 ], 1 );	// ID_STFR
				loopvals[0] = iSaved;
				LWSAVE_I2(save, loopvals, 1);
				while (iSaved-- > 0)
				{
				 r = rout[iSaved];
				 loopvals[0] = r->getCacheID();
				 loopvals[1] = r->getOffset();
				 LWSAVE_I2(save, loopvals, 2);
				 rout[iSaved] = 0;
				}
				LWSAVE_END( save );							// ID_STFR
				iSaved = 0;
			}
		}

		// Do the same with the flags - these are local too
		LoopRangeStub *out[8] = {0};
		iSaved = 0;
		LoopList& list = lpi->getProvidedLoopList();
		iMax = list.getRangeCount() -1;
		for (i = 0; i <= iMax; i++)
		{	// Flags are always me
			LoopRange *r = list.getRange(i);
			LoopRangeStub *l = lpi->getStubByID(r->getCacheID());
			if (l->getFlags() != 0)
				out[iSaved++] = l;

			if (iSaved == 8 || 
				(iSaved > 0 && i == iMax))
			{	// Dump what we have [count][[index][select state]+ ]
				LWSAVE_BEGIN( save, &idmodelelement[ 4 ], 1 );	// ID_DSPF
				loopvals[0] = iSaved;
				LWSAVE_I2(save, loopvals, 1);
				while (iSaved-- > 0)
				{
				 l = out[iSaved];
				 loopvals[0] = l->getID();
				 loopvals[1] = l->getFlags();
				 LWSAVE_I2(save, loopvals, 2);
				 out[iSaved] = 0;
				}
				LWSAVE_END( save );							// ID_DSPF
				iSaved = 0;
			}
		}
		LWSAVE_END( save );							// ID_MODL
	}

	// Then hit the plugins with the Global list file
	for (unsigned int u = 0; u < SaveInfo.Next(); u++)
	{
		LWSAVE_BEGIN( save, &idmodel[ 1 ], 0 );	// ID_PLGF
		 LWSAVE_STR(save, SaveInfo[u]->globalcallback);

		 // Call into that plugin and spit out the rest of the data
		 SaveFuncs[u]->save(save,0);
		LWSAVE_END( save );							// ID_PLGF
	}

	LWSAVE_END( save );						// ID_XPTD

	return (NULL);
}

/*
 * Load the Frame info - from somewhere ...
 * easier to spit it to a separate file
 *
 * It is key that the loops exist before the plugin loops. because
 * we want to translate the incoming frame inforamation and change
 * their indexes
 *
 * Returns an AutoArray that defines the translation between the
 * file's frame indexes and the internal ones
 */
void ExportModelWrapper::LOAD(const LWLoadState *load)
{
	float f[1] = {0};
	LWFileIOFuncs *ioFuncs
				= (LWFileIOFuncs *)LW_globalFuncs( LWFILEIOFUNCS_GLOBAL, GFUSE_TRANSIENT );
	LWLoadState *ldModel = 0;

	LWMRBCallbackType *callback = 0;
	short vals[5] = {0};
	int i = 0;

	LWID idInner;

	while ( LWID id = LWLOAD_FIND( load, idmodelelement ))
	{
	  LoopRange *last = 0;
	  LoopRange *next = 0;
	  switch (id)
	  {
	  // Filename
	  case ID_MDFI:
		LWLOAD_STR(load,tmp,512);
		setFile(tmp);
		ldModel = ioFuncs->openLoad(tmp,LWIO_ASCII);
		LWLOAD_FIND( load, idmodeldata );	// Header
		LWLOAD_FP( ldModel, f, 1);			// Version
		LOAD(ldModel);
		ioFuncs->closeLoad(ldModel);
		break;

	  // Frame data
	  case ID_LPST:
		last = 0;
		while ( idInner = LWLOAD_FIND( load, idloopelements ))
		{
		  switch(idInner)
		  {
			case ID_LDTA:
			  LWLOAD_STR(load,tmp,MAX_LOOP_NAME);
			  LWLOAD_I2(load,vals,4);		// [index length fake fake]

			  next = new LoopRange(tmp,vals[1],vals[0]);
			  m_loops.add(next,last);
			  last = next;
			  break;

			case ID_LPTN:					
			  LWLOAD_STR(load,tmp,sizeof(tmp));	// [pattern]
			  last->setPattern(tmp);
			  
			  break;
		  }
		  LWLOAD_END(load);
		}

		break;
	  // Global plugin data - snag the name
	  case ID_PLGF:
		LWLOAD_STR(load,tmp,512);

		// Call to that plugin and build the plugin's data
		callback = (LWMRBCallbackType *)LW_globalFuncs(tmp, GFUSE_ACQUIRE );
		m_userinst = callback->load(load);
		break;

	  // Start frames by loop
	  case ID_STFR:		
		// Load in [num [id start]+ ]
		LWLOAD_I2(load,vals,1);
		i = vals[0];
		while (i-- > 0)
		{
			LWLOAD_I2(load,vals,2);
			getStubByID(vals[0])->setOffset(vals[1]);
		}
		break;

	  // Flags by loop index
	  case ID_DSPF:		
		// Load in [num [id flags]+ ]
		LWLOAD_I2(load,vals,1);
		i = vals[0];
		while (i-- > 0)
		{
			LWLOAD_I2(load,vals,2);
			getStubByID(vals[0])->setFlags(vals[1]);
		}
		break;
	  }
	  LWLOAD_END(load);
	}

	return;
}


/*
 * Read in dopesheet data from the scene
 * We use another file to handle the frames
 *
 */
LWError ExportModelWrapper::LOAD(LWInstance inst,const LWLoadState *load)
{
	float f[1] = {0};

	// Prep this function
	ExportModelWrapper *lpi = 0;
	LWMRBCallbackType *callback = 0;

	// Read in from file - model, frame lists 
	short iCount = 0;
	short i[5] = {0};

	// ID_XPTD
	LWID id = LWLOAD_FIND( load, idmain);	
	LWLOAD_FP(load,f,1);					// Version

	while ( id = LWLOAD_FIND( load, idmodel ))
	{
	  switch (id)
	  {
	  case ID_MODL:
		// New object - load the index and fake a ref
		LWLOAD_I2(load, i, 5);

		lpi = new ExportModelWrapper(i[0]);
		lpi->acquireRef(LWITEM_NULL, 0);
		lpi->setFlags((int)i[1]);
		lpi->m_listref = i[2];

		lpi->LOAD(load);
	  break;

	  case ID_PLGF:
		LWLOAD_STR(load,tmp,512);

		// Call to that plugin and build the global data for this plugin
		callback = (LWMRBCallbackType *)LW_globalFuncs(tmp, GFUSE_ACQUIRE );
		callback->load(load);
	  break;
	  }
	  LWLOAD_END( load );
	}
	LWLOAD_END( load );		// idmain

	return (NULL);
}

// copy an instance ... 
XCALL_(static LWError)
COPY(void *to, void *from)
{
    return (NULL);
}

/* Description line
 *
 * Shows up in the plug-in selection 
 */
const char *ExportModelWrapper::DESCLN(LWInstance inst)
{
    XCALL_INIT;

	// Return the number of models loaded
    static char line[256];
	sprintf(line,"%d Panels ready",ExportModelWrapper::getCount());
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

/*
 * Something happens, and it tells us here
 * If this is NOT setup, the thing will crash
 */
static LWItemID   pendingItem = LWITEM_NULL;

XCALL_(static double)
EVENT(LWInstance inst, const LWMasterAccess *lwma)
{
	return 0;
}

/*  Always world coordinates
 */
XCALL_(static unsigned int)
FLAGS(LWInstance inst)
{
    XCALL_INIT;
    return (0);
}

// Initialization
void SetupMasterFuncs(LWMasterHandler  *local)
{
	local->inst->create   = CREATE;
	local->inst->destroy  = DESTROY;
	local->inst->load     = ExportModelWrapper::LOAD;
	local->inst->save     = ExportModelWrapper::SAVE;
	local->inst->copy     = COPY;
	local->inst->descln   = ExportModelWrapper::DESCLN;

	local->item->useItems = USEITEMS;
	local->item->changeID = CHANGEID;

	local->event		  = EVENT;
	local->flags          = FLAGS;
}

