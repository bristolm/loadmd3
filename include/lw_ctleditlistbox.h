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
#ifndef _LW_CTLEDITLISTBOX_H
#define _LW_CTLEDITLISTBOX_H

#include "lwpanel.h"
#include "lw_base.h"
#include "lw_ctl_base.h"

class LWCTL_EditLoopList
{
	static int	LINES;
	static int	PANEL_BUFFER;
	static int	BUTTON_WIDTH;
private:
	// Pointers to the controls grouped within me
	LWControl		*cFrameLoopList;
	LWControl		*cFrameBtnAdd,*cFrameBtnDel, *cFrameBtnUpd;
	LWControl		**cFrameStrs;
	LWControl		*cFrameBtnUp,*cFrameBtnDown;

	// Parent panel I'm attached to
	LWPanelID		m_panelID;

	// List of value feeders, null terminated - 1 per column
	LWCTL_Column	**m_columns;

	// Overall dimensions - determined by the input fields
	int				m_width, m_height, m_left, m_top;
	
	// Active row
	int				m_iActiveRow;

	// static callback functions for the list control
	static int getCount( void *data )
	{
		return ((LWCTL_EditLoopList *)data)->count();
	}
	static int getWidth( void *data, int index )
	{
		return ((LWCTL_EditLoopList *)data)->width(index);
	}
	static char *getValue( void *data, int index, int column )
	{
		return ((LWCTL_EditLoopList *)data)->value(index,column);
	}
	static void listEvent(LWControl *ctl, void *data)
	{
		int r = 0,c = 0,i = 0;
		GET_IVEC(ctl,r,c,i);
		
		((LWCTL_EditLoopList *)data)->event(r,c);
	}

	// static callback function for the buttons
	static void buttonEvent(LWControl *button, void *data)
	{
		((LWCTL_EditLoopList *)data)->event(button);
	}
	void event(LWControl *button)
	{
		if (button == cFrameBtnAdd)
		{
			add(m_iActiveRow);
		}
		else if (button == cFrameBtnDel)
		{
			remove(m_iActiveRow);
		}
		else if (button == cFrameBtnUpd)
		{	
			update(m_iActiveRow);
		}
		else if (button == cFrameBtnUp)
		{
			if (m_iActiveRow > 1)
			{
				event(m_iActiveRow, m_iActiveRow -1);
			}
		}
		else if (button == cFrameBtnDown)
		{
			if (m_iActiveRow < count() +1)
			{
				event(m_iActiveRow, m_iActiveRow +1);
			}
		}
	}

protected:
//// CALLBACKs for table ///////////
	// how many rows
	virtual int count( void )			= 0;

	// Column width
	virtual int width( int col )
	{
		return (int)(m_columns[col]->width() * m_width);
	}

	// Value in the cell at col,row
	virtual char *value( int row, int col )
	{
		return (m_columns[col]->value(row));
	}

	// Someone clicked on the table
	virtual void event( int row, int col )
	{
		m_columns[col]->event(row);
	}

//// CALLBACKs from buttons ///////////
	virtual void add(int afterrow)			= 0;
	virtual void remove(int row)			= 0;
	virtual void update(int row)			= 0;
	virtual void swap(int& row_a, int row_b)= 0;

public:
	LWCTL_EditLoopList(LWPanelID panID,
						LWCTL_Column **cols,
						LWControl **strfields);
	virtual ~LWCTL_EditLoopList();

	// Local functions
	void moveto(int x, int y);
	int width()			{return m_width;}
	int height()		{return m_height;}
	int top()			{return m_top;}
	int left()			{return m_left;}

	int getActiveRow()		{return m_iActiveRow;}
};

#endif _LW_CTLEDITLISTBOX_H