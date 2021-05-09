/**************************************
 *
 *  lw_ctl_base.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Definintions for interfaces used for
 *  my new extension controls
 *
 **************************************/
#ifndef _LW_CTL_BASE_H
#define _LW_CTL_BASE_H

// Used for 'column' data.  Really just an interface
class LWCTL_Column
{
public:
	virtual float	width( void )		= 0;	// returns 0.0 .. 1.0 of total
	virtual char	*value( int row )	= 0;	// -1 == title
	virtual void		event( int row )= 0;	// -1 == title
};


#endif // _LW_CTL_BASE_H