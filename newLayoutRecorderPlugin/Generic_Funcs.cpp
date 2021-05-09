/**************************************
 *
 *  Generic_Funcs.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  The Generic plugin handles the Master Control Panel
 *  This lists which models are created, and it controls which are saved out.
 *
 **************************************/

/* Layout include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <lwmaster.h>
#include "lwhost.h"
#include "lwsurf.h"

/* Mystuff */
#include "sys_base.h"

extern "C"
{
#include "LayoutRecorderPlugin.h"
}

#include "ExportPanels.h"
#include "Displacement_XPanels.h"

LWPanelID _SingletonMasterPanel = (LWPanelID)0;

static LWControl *cSave = 0;
static LWControl *cClear = 0;
static LWControl *cList = 0;

static char cmdbuf[255];

static int	PanelWidth = 200;

static int lastClickedRow = -1;

// Single function for updating everything that needs updating
void UpdateEntireEnvironment(void *target)
{
	int i = 0;

	// Refresh all the panels
	for (i = 0; i < ExportModelWrapper::getCacheSize(); i++)
	{
		ExportModelWrapper *wrap = ExportModelWrapper::getByCacheID(i);
		if (wrap && wrap->cFrameLoopList)
		{
			if (wrap != target)
				wrap->SelectActiveEntry();
			LW_panelFuncs->draw(wrap->m_panelID,DR_REFRESH);
	
//			REDRAW_CON(wrap->cFrameLoopList);
		}
	}

	// Refresh the master panel
	if (_SingletonMasterPanel != (LWPanelID)0)
		LW_panelFuncs->draw(_SingletonMasterPanel,DR_REFRESH);


	// Refresh the XPanel instances
	for (i = 0; i < DisplacementXPanelInstance::getCacheSize(); i++)
	{
		DisplacementXPanelInstance *disp = DisplacementXPanelInstance::getByCacheID(i);
		if (disp)
		{
			disp->refresh();
		}
	}
}

// Generic panel functions
static void panelDrawEvent(LWControl *ctl, void *inst)
{
	if (lastClickedRow < 0)
	{
		GHOST_CON(cClear);
		return;
	}

	int r = 0,c = 0,i = 0;
	GET_IVEC(cList,r,c,i);

	ExportModelWrapper *ref =
				ExportModelWrapper::getByCacheIndex(r);
	if (ref == 0 || ref->getRefCount() == 0)
	{
		UNGHOST_CON(cClear);
	}
	else
	{
		GHOST_CON(cClear);
	}

	// Clear out the double clickyness
	lastClickedRow = -1;
}

// Save checked models
static void eventSave(LWControl *ctl, void *inst)
{
	// Initialize all the channels in all listening Groups
	ObjectWrapper::initializeAll();
	ExportModelWrapper *lpi = 0;

	// Grab the selected Wrapper
	int iMinFrame = 999;
	int iMaxFrame = -999;

	// Setup the Output Frame file


	// Roll through all checked wrappers
	int iLoopsChecked  = 0;
	for (int idx = 0; idx < ExportModelWrapper::getCount(); idx++)
	{
		lpi = ExportModelWrapper::getByCacheIndex(idx);
		if (lpi->getFlags() == 0)
			continue;

		// Count all the frames ...
		LC_FrameLoop **loops = lpi->startModel();
		for (int i = 0;loops && loops[i];i++)
		{
			++iLoopsChecked;
			if (loops[i]->lwframes == 0)
				continue;
			for (int j = 0; j < loops[i]->length; j++)
			{
				if (loops[i]->lwframes[j] < iMinFrame)
					iMinFrame = loops[i]->lwframes[j];
				if (loops[i]->lwframes[j] > iMaxFrame)
					iMaxFrame = loops[i]->lwframes[j];
			}
		}
	}

	LWInterfaceInfo	*intfInfo
			= (LWInterfaceInfo *)LW_globalFuncs( LWINTERFACEINFO_GLOBAL, GFUSE_TRANSIENT );
	int oldFirstFrame = intfInfo->previewStart;
	int oldLastFrame = intfInfo->previewEnd;
			
	if (iLoopsChecked > 0)
	{	// Store the current values so we can reset things ...
		sprintf(cmdbuf,"PreviewFirstFrame  %d",iMinFrame);
		LW_cmdFunc(cmdbuf);
		sprintf(cmdbuf,"PreviewLastFrame  %d",iMaxFrame);
		LW_cmdFunc(cmdbuf);
		sprintf(cmdbuf,"MakePreview");
		LW_cmdFunc(cmdbuf);
	}

//	sprintf(cmdbuf,"GoToFrame %d",i);
//	LW_cmdFunc(cmdbuf);

	// build and save all the files - one 'panel' per file

	for (idx = 0; idx < ExportModelWrapper::getCount(); idx++)
	{
		lpi = ExportModelWrapper::getByCacheIndex(idx);
		if (lpi->getFlags() == 0)
			continue;
		lpi->buildModel();
	}

	// Reset all the groups
	ObjectWrapper::resetAll();

	// Fix the view screen
	sprintf(cmdbuf,"PreviewFirstFrame  %d",oldFirstFrame);
	LW_cmdFunc(cmdbuf);
	sprintf(cmdbuf,"PreviewLastFrame  %d",oldLastFrame);
	LW_cmdFunc(cmdbuf);
}

// Clear selected model
static void eventClear(LWControl *ctl, void *inst)
{
	int r = 0,c = 0,i = 0;
	GET_IVEC(cList,r,c,i);

	ExportModelWrapper *ref =
				ExportModelWrapper::getByCacheIndex(r);
	if (ref == 0 || ref->getRefCount() > 0)
	{
		GHOST_CON(cClear);
		return;
	}

	ref->releaseRef(0);
	if (ExportModelWrapper::getCount() == 0)
		GHOST_CON(cClear);

	r = -1;
	SET_INT(cList,r);
	LW_panelFuncs->draw(_SingletonMasterPanel,DR_REFRESH);
}

// Multilist Functions
static char *ListTitles[] = {"Save","Export Model",0};
static int	ListWidths[] = {30,170,0};

static void eventList(LWControl *ctl, ExportModelWrapper *inst)
{
	int r = 0,c = 0,i = 0;
	GET_IVEC(ctl,r,c,i);

	ExportModelWrapper *ref =
				ExportModelWrapper::getByCacheIndex(r);

	// Decide what to do with that button
	if (ref->getRefCount() == 0)
	{
		UNGHOST_CON(cClear);
	}
	else 
	{
		GHOST_CON(cClear);
	}

	// only 'double click' on the name
	if (c != 0)
	{
		// Really cheesy double clicking ...
		if (r == lastClickedRow)
		{
			ref->openPanel();
		}
		lastClickedRow = r;
		return;
	}

	if (ref->getFlags())
		ref->setFlags(0);
	else
		ref->setFlags(1);

	REDRAW_CON(cList);
}

static char *listNameFnLoops( void *data, int index, int column )
{
	if (index < 0)
	{
		return ListTitles[column];
	}

	ExportModelWrapper *ref =
				ExportModelWrapper::getByCacheIndex(index);

	if (ref == 0)
	{
		return "";
	}

	if (column == 0)
	{	// Selected or not selected check
		return ref->getFlags() ?	LW_USECHECKMARK : LW_NO_CHECKMARK;
	}

	return ref->getTargetTypeAndDescription();
}

static int listCountFnLoops (void *data )
{
	ExportModelWrapper *inst = (ExportModelWrapper *)data;

	int i = ExportModelWrapper::getCount();
	if (i == 0)
	{
		GHOST_CON(cSave);
	}
	else
	{
		UNGHOST_CON(cSave);
	}

	return i;
}

static int listWidthFnLoops( void *data, int column )
{
	return ListWidths[column];
}

// Initialization
void ExecuteGenericFuncs(LWLayoutGeneric  *local)
{
	if (_SingletonMasterPanel == (LWPanelID)0)
	{
		_SingletonMasterPanel = PAN_CREATE(LW_panelFuncs,"ExportControl - v" PLUGINREV);

		// Setup button - Save
		cSave = WBUTTON_CTL(LW_panelFuncs,
								_SingletonMasterPanel ,
								"Save Checked Models",
								PanelWidth);
		CON_SETEVENT(cSave,eventSave,_SingletonMasterPanel);

		// Setup button - Clear
		cClear = WBUTTON_CTL(LW_panelFuncs,
								_SingletonMasterPanel ,
								"Clear Unreferenced Model",
								PanelWidth);
		CON_SETEVENT(cClear,eventClear,_SingletonMasterPanel);

		// List - 2 colums 'Save' and 'Name'
		cList = MULTILIST_CTL(LW_panelFuncs,
								_SingletonMasterPanel,
								"",
								PanelWidth,4,
								listNameFnLoops,
								listCountFnLoops,
								listWidthFnLoops);
		CON_SETUSERDATA(cList,_SingletonMasterPanel);
		CON_SETEVENT(cList,eventList,_SingletonMasterPanel);

		// When panel is opened
		PAN_SETDRAW(LW_panelFuncs,_SingletonMasterPanel,panelDrawEvent);

		int x = 5;
		int y = 5;
		MOVE_CON(cClear,x,y);
		y += CON_H(cClear);

		MOVE_CON(cList,x,y);
		y += CON_H(cList);

		MOVE_CON(cSave,x,y);

		listCountFnLoops(0);
	}

	LW_panelFuncs->open(_SingletonMasterPanel,PANF_FRAME);
}

//=========================================


