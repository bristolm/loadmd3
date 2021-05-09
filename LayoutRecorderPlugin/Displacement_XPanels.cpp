/**************************************
 *
 *  Displacement_XPanels.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Execution and manipulation of XPanel data for the Displacement plugin
 *  The displacement interface does not exist this plugin, it is 
 *  fed to this code by a specific module plugin which defines what sort of
 *  model we are creating.
 *
 *  Once this is defined, ObjectWrappers are created for the object being
 *  'displaced'.   This method of tracking points through a displacement
 *  plugin is sort of oldskool - apparently it doens't need to be done this
 *  way anymore ...
 *
 *  The DisplacementXPanelInstance object represents a single instance of a
 *  _Module's_ Displacement plugin since LayoutRecorder does not define a 
 *  Displacement plugin itself.  However, since LayoutRecorder overrides
 *  the Displacement Interface, the behavior is predictable.
 *
 **************************************/

/* Layout include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "lwdisplce.h"
#include "lwhost.h"
#include "lwsurf.h"

/* Mystuff */
extern "C"
{
#include "LayoutRecorderPlugin.h"
}

#include "Displacement_XPanels.h"
#include "ExportPanels.h"

/*
 * This simply has a drop down box listing the possible
 * models these could be added to, and a button for editing
 * that information
 */

static const char TEXT_NEW[] = "(new)";
static const char TEXT_NONE[] = "(none)";

int						Cache<DisplacementXPanelInstance>::NextUniqueIndex = 0;
AutoArray<DisplacementXPanelInstance *>
						Cache<DisplacementXPanelInstance>::All(0);

enum { ID_MODELLIST = 0x8000, ID_EDITBUTTON };

// Functions to handle the Model list
XCALL_( static int )
modelCountFn(void *inst)
{
	DisplacementXPanelInstance *pi = (DisplacementXPanelInstance *)inst;
	return ExportModelWrapper::getCountByType(pi->getUserFuncs()->modeltype) +1;
}

XCALL_( static const char * )
modelNameFn( void *inst, int idx )
{
	if (idx == 0) 
		return TEXT_NEW;

	DisplacementXPanelInstance *pi = (DisplacementXPanelInstance *)inst;
	return ExportModelWrapper::getByIndexByType(
					idx -1,
					pi->getUserFuncs()->modeltype)->getTargetDescription();
}

XCALL_( void )
eventEditButton( LWXPanelID xpan, int cid)
{
	// Grab the instance for this panel and open the selected instance
	DisplacementXPanelInstance *xpaninst = 
		(DisplacementXPanelInstance *)LW_xpanFuncs->getData(xpan,0);

	char *type = xpaninst->getUserFuncs()->modeltype;

	// Get the selected panel
	int idx = xpaninst->iListChoice;
	ExportModelWrapper *lpi = ExportModelWrapper::getByIndexByType(idx -1,type);

	// Grab the panel
	xpaninst->lockNewPanel(lpi);

	// Update my data
	xpaninst->iListChoice = lpi->getIndexByType() +1;

	// Refresh me
	xpaninst->refresh();

	// Edit
	xpaninst->edit();

	// Refresh me
	xpaninst->refresh();
}

/*
 * When the panel is detroyed, switch the value in the scratch field 
 * to an invalid one, to force us to reevaluate the index of
 * the model we are referencing the next time the panel is opened
 */
XCALL_( void )
xpanelDestroy(void *inst)
{
	DisplacementXPanelInstance *pi = (DisplacementXPanelInstance *)inst;
	pi->iListChoice = -1;
}

/*
======================================================================
xgetval()

Xpanels callback for LWXP_VIEW panels.  Returns a pointer to the data
for a given control value.
====================================================================== */
static void *xgetval( void *inst, unsigned long vid )
{
	DisplacementXPanelInstance *disp = (DisplacementXPanelInstance *)inst;
	void *v = 0;

	// Return the index that matches the ref we have
	switch ( vid ) {
      case ID_MODELLIST:
		if (disp->iListChoice < 0)
			disp->iListChoice = disp->getActiveRef() ?
					disp->getActiveRef()->getIndexByType() +1 : 0;
		v = (void *)&(disp->iListChoice);
		break;

      default:
		  ;
	}

	return v;
}


/*
======================================================================
xsetval()

Xpanels callback for LWXP_VIEW panels.  Store a value in our instance
data.
====================================================================== */

static LWXPRefreshCode xsetval( void *inst, unsigned long vid, void *value )
{
	DisplacementXPanelInstance *disp = (DisplacementXPanelInstance *)inst;
	
	// Return the index that matches the ref we have
	switch ( vid ) {
		case ID_MODELLIST:
			disp->iListChoice = *(( int * ) value );
			break;

		default:
			return LWXPRC_NONE;
	}

	return LWXPRC_DFLT;
}

#define STR_ModelList_TEXT		"Model List: "
#define STR_EditButton_TEXT		"Edit: "

// These functions handle the displacement Plugins
LWXPanelID DisplacementXPanelInstance::SetupXPanel()
{
	static LWXPanelControl xctl[] = {
		{ ID_MODELLIST,		STR_ModelList_TEXT,		"iPopChoice" },
		{ ID_EDITBUTTON,	STR_EditButton_TEXT,	"vButton" },
		{ 0 }
	};

	static LWXPanelDataDesc xdata[] = {
		{ ID_MODELLIST,		STR_ModelList_TEXT,		"integer" },
		{ ID_EDITBUTTON,	STR_EditButton_TEXT,	"integer" },
		{ 0 }
	};

	static LWXPanelHint xhint[] = {
		XpPOPFUNCS (ID_MODELLIST, modelCountFn, modelNameFn),
		XpBUTNOTIFY(ID_EDITBUTTON, eventEditButton),
		XpDESTROYNOTIFY(xpanelDestroy),
		XpEND
	};

	m_xpanelid = LW_xpanFuncs->create( LWXP_VIEW, xctl );
	if ( !m_xpanelid ) return NULL;

	LW_xpanFuncs->hint( m_xpanelid, 0, xhint );
	LW_xpanFuncs->describe( m_xpanelid, xdata, xgetval, xsetval );
	LW_xpanFuncs->viewInst( m_xpanelid, this );
	LW_xpanFuncs->setData( m_xpanelid, 0, this );
	LW_xpanFuncs->setData( m_xpanelid, ID_MODELLIST, this );
	LW_xpanFuncs->setData( m_xpanelid, ID_EDITBUTTON, this );

	return m_xpanelid;
};

char *DisplacementXPanelInstance::getDescription()
{
	static char defln[] = "No Target specified";
    static char line[256];

	if (m_lastref == 0)
		return defln;

    XCALL_INIT;

	ExportModelWrapper *tmp = (ExportModelWrapper *)m_lastref;

	sprintf(line, "%s Target: %s", 
						m_lastref->getTargetType(),
						m_lastref->getTargetDescription());

    return line;
}

// Stub in for the outside world
LWXPanelID SetupDisplacementXPanel(void *inst)
{
	DisplacementXPanelInstance *disp = (DisplacementXPanelInstance *)inst;
	return disp->SetupXPanel();
}

void DisplacementXPanelInstance::lockNewPanel(ExportModelWrapper *pan)
{
	if (pan != m_lastref)
	{
		pan->acquireRef(m_item, m_userfuncs);

		if (m_lastref != 0)
		{
			m_lastref->releaseRef(m_item);
		}

		m_lastref = pan;
	}
}

DisplacementXPanelInstance *DisplacementXPanelInstance::next(DisplacementXPanelInstance *last)
{
	int idx = last == 0 ? 0 : last->getCacheSize() +1;
	while (idx < getCacheSize())
	{
		if (getByCacheID(idx) != 0)
			return getByCacheID(idx);
		++idx;
	}

	return (DisplacementXPanelInstance *)0;
}
