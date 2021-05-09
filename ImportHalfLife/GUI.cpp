/**************************************
 *
 *  GUI.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Just gets info from the user for loading associated format
 *
 *  Unreal Tounament is a trademark of Epic MegaGames, Inc.
 *
 **************************************/

#include "sys_math.h"

extern "C"
{
#include "ImportUnrealSkel.h"
}
#include "lwxpanel.h"
#include "lwrender.h"
#include "lwenvel.h"

#include "ParseUnrealSkl.h"
enum { ID_AFILE = 0x8000, ID_SFILE, ID_ANIM };

#define STR_SFile_TEXT		"Bone info "
#define STR_AFile_TEXT		"Keyframes: "
#define STR_Anim_TEXT		"Animation: "

// Functions to handle the Animation list
XCALL_( static int )
animationCountFn(void *inst)
{
	AnimLoader *anim = (AnimLoader *)inst;

	if (anim == 0 || anim->Animations == 0)
		return 0;

	UnrealSkeletalModel *model = (UnrealSkeletalModel *)(anim->Animations);
	return model->AnimationCount();
}

XCALL_( static const char * )
animationNameFn( void *inst, int idx )
{
	AnimLoader *anim = (AnimLoader *)inst;

	if (anim == 0 || anim->Animations == 0
		|| idx < 0)
		return 0;

	UnrealSkeletalModel *model = (UnrealSkeletalModel *)(anim->Animations);

	UNSKEL_Animation& a = model->Animation(idx);
	return a.Name;
}

static void *xgetval( void *inst, unsigned long vid )
{
	char tmp[1024];
	sprintf (tmp,"");

	// Find the index for the selected loop
	AnimLoader *anim = (AnimLoader *)inst;
	void *v = (void *)&tmp;

	// Return the index that matches the ref we have
	switch ( vid ) {
      case ID_AFILE:
		if (anim->Animations)
		{
			strcpy(tmp,((UnrealSkeletalModel *)anim->Animations)->getName());
		}
		break;

      case ID_SFILE:
		if (anim->Skeleton)
		{
			strcpy(tmp,((UnrealSkeletalModel *)anim->Skeleton)->getName());
		}
		break;

	  case ID_ANIM:
		v = (void *)&anim->SelectedAnimation;
		break;

      default:
		  ;
	}

	return v;
}

static LWXPRefreshCode xsetval( void *inst, unsigned long vid, void *value )
{
	// Use the active loop
	AnimLoader *anim = (AnimLoader *)inst;
	char *c = (char *)value;
	
	switch ( vid ) {
      case ID_AFILE:
		if (anim->Animations != 0)
		{
			if (strcmp(c,((UnrealSkeletalModel *)anim->Animations)->getName()) == 0)
				break;
			delete anim->Animations;
			anim->Animations = 0;
		}
		if (c != 0 && c[0] != 0)
		{
			anim->Animations = new UnrealSkeletalModel(c);
		}
		break;

      case ID_SFILE:
		if (anim->Skeleton != 0)
		{
			if (strcmp(c,((UnrealSkeletalModel *)anim->Skeleton)->getName()) == 0)
				break;
			delete anim->Skeleton;
			anim->Skeleton = 0;
			anim->SelectedAnimation = 0;
		}
		if (c != 0 && c[0] != 0)
		{
			anim->Skeleton = new UnrealSkeletalModel(c);
		}
		break;

	  case ID_ANIM:
		anim->SelectedAnimation = *(int *)value;
		break;

	  default:
			return LWXPRC_NONE;
	}
	return LWXPRC_DFLT;
}

// Open a panel with a filename requestor on it, and fire away
AnimLoader *Get_PSAFile()
{
	LWXPanelFuncs *xpanFuncs = 
			(LWXPanelFuncs *)LW_globalFuncs( LWXPANELFUNCS_GLOBAL, GFUSE_TRANSIENT  );

	static LWXPanelControl xctl[] = {
		{ ID_SFILE,		STR_SFile_TEXT,	"sFileName" },
		{ ID_AFILE,		STR_AFile_TEXT,	"sFileName" },
		{ ID_ANIM,		STR_Anim_TEXT,	"iPopChoice" },
		{ 0 }
	};

	static LWXPanelDataDesc xdata[] = {
		{ ID_SFILE,		STR_SFile_TEXT,	"string" },
		{ ID_AFILE,		STR_AFile_TEXT,	"string" },
		{ ID_ANIM,		STR_Anim_TEXT,	"integer" },
		{ 0 }
	};

	static LWXPanelHint xhint[] = {
		XpPOPFUNCS (ID_ANIM, animationCountFn, animationNameFn),
		XpLINK (ID_AFILE, ID_ANIM),
		XpEND
	};

	LWXPanelID xpanid = xpanFuncs->create( LWXP_VIEW, xctl );
	if ( !xpanid ) return 0;

	AnimLoader *anim = new AnimLoader;
	anim->Animations = 0;
	anim->Skeleton = 0;
	anim->SelectedAnimation = 0;

	// Init the form
	xpanFuncs->hint( xpanid, 0, xhint );
	xpanFuncs->describe( xpanid, xdata, xgetval, xsetval );
	xpanFuncs->viewInst( xpanid, anim );
	xpanFuncs->setData( xpanid, 0, anim );
	xpanFuncs->setData( xpanid, ID_ANIM, anim );

	// Open modally
	xpanFuncs->post(xpanid);

	xpanFuncs->destroy(xpanid);

	return anim;
}
