/**************************************
 *
 *  Displacement_XPanels.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Definition of XPanel for the Displacement plugin
 *  for the main portion of the Layour Exporter
 *
 **************************************/

#ifndef _LW_DISPXPANEL_H
#define _LW_DISPXPANEL_H

#include <stdio.h>
#include <stdlib.h>
#include "string.h"

#include "lwrender.h"
#include "lwdisplce.h"
#include "lwmeshes.h"
#include "lwsurf.h"
#include "lwserver.h"
#include "lwxpanel.h"

#include "sys_extra.h"

#include "ExportPanels.h"

// Class to hold each instance
class DisplacementXPanelInstance:
	public Cache<DisplacementXPanelInstance>
{
	LWXPanelID			m_xpanelid;
	LWItemID			m_item;
	ExportModelWrapper *m_lastref;
	LWError				m_lastPanelError;
	LWMRBExportType	   *m_userfuncs;

public:
	int					iListChoice;

	DisplacementXPanelInstance(LWItemID item, LWMRBExportType *userfuncs):
		m_xpanelid(0),
	    m_item(item),
		m_lastref(0),
		m_lastPanelError(0),
		m_userfuncs(userfuncs),
		iListChoice(0)
	{
	}

	~DisplacementXPanelInstance()
	{
		if (m_lastref)
			m_lastref->releaseRef(m_item);
	}

	char *getDescription();
	LWXPanelID	SetupXPanel();
	ExportModelWrapper *getActiveRef()				{return m_lastref;}

	// Pass through for displacement
	void lockNewPanel(ExportModelWrapper *pan);

	// Panel functions
	void edit()
	{
		m_lastPanelError = m_lastref->openPanel();
	}

	void refresh()
	{	// Update what LW shows (if we're definately not shutting down ...)
		if (::ClearingScene == 0)
			LW_instUpdate(LWDISPLACEMENT_HCLASS,this);

		// hmmm ... apparently can't refresh if the XPanel is 'destroyed'
		if (iListChoice >= 0)
			LW_xpanFuncs->viewRefresh(m_xpanelid);
	}

	LWError getLastError()
	{
		LWError err = m_lastPanelError;
		m_lastPanelError = (LWError)0;

		return err;
	}
	LWMRBExportType	*getUserFuncs()	{return m_userfuncs;}

	static DisplacementXPanelInstance *next(DisplacementXPanelInstance *last = 0);

};

#endif //_LW_DISPXPANEL_H