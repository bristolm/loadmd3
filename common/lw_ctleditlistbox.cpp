/**************************************
 *
 *  lw_ctleditlistbox.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Wrapper for the control panel/
 *  frame loops list panel grouping
 *
 **************************************/

#include "lw_ctleditlistbox.h"

int		LWCTL_EditLoopList::LINES = 10;
int		LWCTL_EditLoopList::PANEL_BUFFER = 17;
int		LWCTL_EditLoopList::BUTTON_WIDTH = 20;

extern "C"  LWPanelFuncs *LW_panelFuncs;

// required by macros in lwpanel.h
static LWPanControlDesc	desc;
static LWValue ival={LWT_INTEGER},ivecval={LWT_VINT},         
		fval={LWT_FLOAT},fvecval={LWT_VFLOAT},
		sval={LWT_STRING},xpanval={LWT_XPANEL};



// Pass in the panelID to work on.
// The list of column listeners.
// The width of any input fields (buffer
LWCTL_EditLoopList::LWCTL_EditLoopList(LWPanelID panID,
									   LWCTL_Column **cols,
									   LWControl **strfields)	// Defin the width of the overall control
	:cFrameLoopList(0),
	 cFrameBtnAdd(0),cFrameBtnDel(0), cFrameBtnUpd(0),
	 cFrameStrs(strfields),
	 cFrameBtnUp(0),cFrameBtnDown(0),
	 m_panelID(panID),
	 m_columns(cols),
	 m_width(0), m_height(0), m_left(0), m_top(0),
	 m_iActiveRow(0)
{
	// Add myself to the panel
	// Figure out the widths ...
	int wid = 0;
	for (int i = 0; strfields[i]; i++)
	{
		wid += CON_W(strfields[i]) -2;
	}

	// Display area for frame loops
	cFrameLoopList = MULTILIST_CTL(LW_panelFuncs,
									panID,
									"",
									wid + PANEL_BUFFER,
									LINES,
									getValue,
									getCount,
									getWidth,
									);

	CON_SETUSERDATA(cFrameLoopList,this);
	CON_SETEVENT(cFrameLoopList,listEvent,this);

	// Setup buttons
	cFrameBtnAdd = WBUTTON_CTL(LW_panelFuncs,m_panelID ,"+", BUTTON_WIDTH);
	CON_SETEVENT(cFrameBtnAdd,buttonEvent,this);

	cFrameBtnUpd = WBUTTON_CTL(LW_panelFuncs,m_panelID ,"=", BUTTON_WIDTH);
	CON_SETEVENT(cFrameBtnUpd,buttonEvent,this);

	cFrameBtnUp = WBUTTON_CTL(LW_panelFuncs,m_panelID ,"/\\", BUTTON_WIDTH);
	CON_SETEVENT(cFrameBtnUp,buttonEvent,this);

	cFrameBtnDown = WBUTTON_CTL(LW_panelFuncs,m_panelID ,"\\/", BUTTON_WIDTH);
	CON_SETEVENT(cFrameBtnDown,buttonEvent,this);

	cFrameBtnDel = WBUTTON_CTL(LW_panelFuncs,m_panelID ,"X", BUTTON_WIDTH);
	CON_SETEVENT(cFrameBtnDel,buttonEvent,this);
}

LWCTL_EditLoopList::~LWCTL_EditLoopList()
{
	delete cFrameStrs;
}

void LWCTL_EditLoopList::moveto(int x, int y)
{
	// Arrange all my stuff
	m_left = x;
	m_top = y;

	// [user buttons ... ] [add]
	y += 2;

	MOVE_CON(cFrameBtnUpd,	x,				y);
	x += BUTTON_WIDTH;

	for (int i = 0; cFrameStrs[i]; i++)
	{
		MOVE_CON(cFrameStrs[i],	x,				y);
		x += CON_W(cFrameStrs[i]) -2;
	}

	x -= 1;
	
	MOVE_CON(cFrameBtnAdd,	x,				y);

	// Loop List below
	x = m_left;
	y += CON_H(cFrameBtnAdd);
	MOVE_CON(cFrameLoopList,		x,		y);

	// Buttons [up, down] down the side ... [delete]
	x += CON_W(cFrameLoopList);
	x += 1;

	MOVE_CON(cFrameBtnUp,	x,				y);
	y += CON_HOTH(cFrameBtnUp);
	MOVE_CON(cFrameBtnDown,	x,				y);
	y += CON_HOTH(cFrameBtnDown);

	y = CON_Y(cFrameLoopList);
	y += CON_H(cFrameLoopList);
	y -= CON_H(cFrameBtnDel);

	MOVE_CON(cFrameBtnDel,	x,				y);

	x += BUTTON_WIDTH;
	y += CON_H(cFrameBtnDel);

	// And set the other side too
	m_width = x - m_left;
	m_height = y - m_top;
}


