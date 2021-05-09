/**************************************
 *
 *  BuildFrames.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Handles the real meat of the program for
 *  importing Quake2 MD2 format files
 *
 **************************************/

#include "sys_math.h"
#include <lwdisplce.h>
#include <lwenvel.h>

extern "C"
{
#include "MorfToFrame.h"
}

#define PLUGIN_MORPHMIXER	"LW_MorphMixer"

LWCommandFunc	LW_cmdFunc;
LWItemInfo		*LW_itemInfo;
LWChannelInfo	*LW_chanInfo;
LWSceneInfo		*LW_sceneInfo;
LWEnvelopeFuncs	*LW_envFunc;

LWChanGroupID findGroup(FrameInstructions *activeInst)
{
	// First find the group for this object
	LWChanGroupID groupObj = LW_itemInfo->chanGroup(activeInst->Object);
	if (!groupObj)		return groupObj;

	// Now find the morph group we want
	LWChanGroupID groupMorf = LW_chanInfo->nextGroup(groupObj,0);
	for (;groupMorf;groupMorf = LW_chanInfo->nextGroup(groupObj,groupMorf))
	{
		char tmp[255];
		strcpy(tmp,LW_chanInfo->groupName(groupMorf));

		if (strcmp(tmp,activeInst->GroupName) == 0)
		{
			break;
		}
	}

	return groupMorf;
}

int BuildFrames(LWLayoutGeneric *local, FrameInstructions **data)
{
	LW_cmdFunc = 
		(LWCommandFunc)LW_globalFuncs( LWCOMMANDINTERFACE_GLOBAL, GFUSE_TRANSIENT );
	LW_itemInfo =
		(LWItemInfo *)LW_globalFuncs( LWITEMINFO_GLOBAL, GFUSE_TRANSIENT );
	LW_chanInfo =
		(LWChannelInfo *)LW_globalFuncs( LWCHANNELINFO_GLOBAL, GFUSE_TRANSIENT );
	LW_sceneInfo =
		(LWSceneInfo *)LW_globalFuncs( LWSCENEINFO_GLOBAL, GFUSE_TRANSIENT );
	LW_envFunc =
		(LWEnvelopeFuncs *)LW_globalFuncs( LWENVELOPEFUNCS_GLOBAL, GFUSE_TRANSIENT );
   
   // Each frame instruction:
	//   Make sure hte object has the plugin
	//   Add keyframes for the appropriate morphs
	//   Add frames before and after the target one as well, flag with 'flat' curve type

	FrameInstructions *activeInst = 0;
	for (int i = 0; activeInst = data[i];i++)
	{
		// Make sure this one is using the MorphMixer plugin
		int idx = 1;			// Base 1 index
		const char *c = 0;
		for (;(c = LW_itemInfo->server(activeInst->Object,LWDISPLACEMENT_HCLASS,idx)) != 0;idx++)
		{
			if (strcmp(c,PLUGIN_MORPHMIXER) == 0)
			{
				break;
			}
		}
		if (c == 0)
		{
			char	cmdbuf[255];
			sprintf(cmdbuf,"ApplyServer %s %s",
								LWDISPLACEMENT_HCLASS,
								PLUGIN_MORPHMIXER);
			LW_cmdFunc(cmdbuf);
		}

		// Find the group that represents the channels we want
		LWChanGroupID group = findGroup(activeInst);

		double	spf = 1.0/LW_sceneInfo->framesPerSecond;
		int		TYPE_LINEAR = 3;

		// Now add the keyframes in
		LWChannelID chan = LW_chanInfo->nextChannel(group,0);
		LWTime t = spf * activeInst->StartFrame;
		for (;chan; t += spf ,chan = LW_chanInfo->nextChannel(group,chan))
		{
			// Frame -1 and Frame +1 get keyframes where they are now
			LWEnvelopeID env = LW_chanInfo->channelEnvelope(chan);

			double before = LW_envFunc->evaluate(chan,t - spf);
			LWEnvKeyframeID keyBefore =
					LW_envFunc->createKey(env,t - spf,before);
			LW_envFunc->keySet(env,keyBefore,LWKEY_SHAPE,&TYPE_LINEAR);

			double after = LW_envFunc->evaluate(chan,t + spf);
			LWEnvKeyframeID keyAfter =
					LW_envFunc->createKey(env,t + spf,after);
			LW_envFunc->keySet(env,keyAfter,LWKEY_SHAPE,&TYPE_LINEAR);

			// Now set us at 100%
			LWEnvKeyframeID keyNow =
					LW_envFunc->createKey(env,t,1.0);
			LW_envFunc->keySet(env,keyNow,LWKEY_SHAPE,&TYPE_LINEAR);
		}		
	}
	return AFUNC_OK;
}

