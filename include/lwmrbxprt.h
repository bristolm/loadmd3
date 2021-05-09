/**************************************
 *
 *  lwmrbxprt.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Public definition of plugin functions
 *  To be used by additional module designers.
 *
 **************************************/

#include <stdio.h>
#include <string.h>

#include <lwdisplce.h>
#include <lwmeshes.h>
#include <lwpanel.h>
#include <lwsurf.h>
#include <lwxpanel.h>
#include <lwhandler.h>

#ifndef _LWMRBXPRT_H
#define _LWMRBXPRT_H

// GlobalMD3 plugin
#define LWMRBEXPORT_GLOBAL		"LWMRBExportFunctions"
#define LWMRBEXPORT_VERSION		2

// Preface all the plugins using this with this:
#define LWMRBPREFIX				"MRB::Export::"

typedef float vec_t;

typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];

typedef void *		 LC_Instance;
typedef void *		 LC_Model;

// Different types of channels to listen for
typedef enum Target_Flavors
{
	LISTEN_NULL,
	LISTEN_TAG,
	LISTEN_POLYS,
	LISTEN_BONE
} LC_FLAVOR;

// Structures that this plugin passes
typedef struct _lc_channel
{
	void			*data;		// internal data - don't touch!

	const char		*name;
	LC_FLAVOR		flavor;
	LWItemID		lwid;
	LWItemID		lwparentid;
	unsigned int	vertexcount;
	unsigned int	polygoncount;
} LC_Channel;

typedef struct _lc_objectposition
{
	vec3_t	xyz;
	vec3_t	hpb;
} LC_ObjectPosition;

typedef struct _lc_vertexoffset
{
	vec3_t	location;
} LC_VertexOffset;

typedef struct _lc_vertexposition
{
	LWPntID			lwid;
	int				index;
	vec2_t			texmap;
	float			influence;
	LC_VertexOffset	start;
} LC_VertexPosition;

typedef struct _lc_polygon
{
	LWPolID			lwid;
	LWSurfaceID		surf;
	int				index;
	int				vertex[3];
} LC_Polygon;

// defines for the Frame translation list
#define LOOP_END		    -999

// Different types of actions for a given loop
typedef enum Loop_States
{
	NO_STATE = 0,

	// Actions when passed to loopactivity
	ACTIVITY_NEW,
	ACTIVITY_DELETED,
	ACTIVITY_UPDATED,

	// States when passed to build
	STATE_ACTIVE,
	STATE_INACTIVE
} LC_LOOPSTATE;

typedef struct _lc_frameloop
{
	char				Name[256];
	int					ID;
	LC_LOOPSTATE		state;
	int					length;
	int					lwstart;
	int					*lwframes;		// Array of matching LW Frames
} LC_FrameLoop;

// Main function structure for this plugin
typedef struct st_LWMRBExportFuncs
{
/* This is my 'exposed' Activation function
 */
	int	( *get_activation )	(long version, GlobalFunc *global, LWDisplacementHandler *local);

/* This is my 'exposed' Interface function
 */
	int	( *get_interface )	(long version, GlobalFunc *global, LWInterface *local);

/* Work functions - inst is a pointer to an ObjectWrapper - passed in through buildmodel
 */
	LC_Channel			* ( *get_channel )	( LC_Instance inst, int index );

	LC_Polygon			* ( *get_polygon )			( LC_Channel *chan, int poly);
	LC_VertexPosition	* ( *get_vertex )			( LC_Channel *chan, int vtx);
	LC_VertexOffset		* ( *get_vertex_at_frame )	( LC_Channel *chan, int vtx, int frame );

	LC_ObjectPosition	* ( *get_rest_position )	( LC_Channel *chan );
	LC_ObjectPosition	* ( *get_position_at_frame )( LC_Channel *chan, int frame );
} LWMRBExportFuncs;

// Setup structure for this plugin
typedef struct st_LWMRBExportType
{
	char		*modeltype;			// Descriptive string for a given plugin (unique for type)
	char		*globalcallback;	// Global plugin callback name - see below for return struct
} LWMRBExportType;

// Structure returned by plugin named by LWMRBExportType::globalcallback member
typedef struct st_LWMRBCallbackType
{
	// Input/Output functions
	void	  *( *load )	 (	const LWLoadState *load);		// new ID values for imported frames
	void	   ( *save )	 (	const LWSaveState *save, 
								LC_Instance xpaneldata);		// null xpaneldata == global

	// Construction functions
	LWXPanelID  ( *newxpanel )	 (LC_Instance seeddata);	// Generate/populate a local struct
	char       *( *describe )	 (LC_Instance xpaneldata);	// Pass in result of newxpanel's inst data
	void        ( *loopactivity )(LC_Instance xpaneldata, LC_FrameLoop *loop);	// nothing selected: loop.index == LOOP_END

	// Building  a model - initmodel returns a new data pointer
	// that is passed to buildmodel and finishmodel as *model
	LC_Model	( *startmodel )  ( LC_Instance xpaneldata, LC_FrameLoop ***loops);
	void		( *buildmodel )  ( LC_Model model, LWInstance inst );
	void		( *finishmodel ) ( LC_Model model);
} LWMRBCallbackType;

#endif // _LWMRBXPRT_H
