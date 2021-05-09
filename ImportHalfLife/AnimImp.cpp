/**************************************
 *
 *  ObjImp.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Handles the real meat of the program for importing
 *  Unreal Skeletal Animations into Layout
 *
 *  Unreal Tounament is a trademark of Epic MegaGames, Inc.
 *
 **************************************/

#include "sys_math.h"

extern "C"
{
#include "ImportUnrealSkel.h"
}
#include "lwrender.h"
#include "lwenvel.h"

#include "ParseUnrealSkl.h"

static Quaternion<float> LW_TO_UNSKEL_quat(LW_TO_UNSKEL_Coords);

LWItemID findBoneByName(LWItemID obj, char *name)
{
	LWItemInfo *itemInfo
			= (LWItemInfo *)LW_globalFuncs( LWITEMINFO_GLOBAL, GFUSE_TRANSIENT );
	LWItemID ret = itemInfo->first(LWI_BONE,obj);
	while (ret != LWITEM_NULL)
	{
		const char *c = itemInfo->name(ret);
		if (strcmp(c,name) == 0)
			break;
		ret = itemInfo->next(ret);
	}

	return ret;
}

// Convert a Rotation to LW angles
// Negate around our Conversion Quat
Vector<float> xlatRotation(FQuat& q)
{
	Matrix<float>	Mrot = (Quaternion<float>(q.X,q.Y,q.Z,q.W)).getMatrix();
	Vector<float>	Vrot = (~LW_TO_UNSKEL_Coords) * (~Mrot).getEuler();

	return Vector<float>(-Vrot[0], -Vrot[1], -Vrot[2]);
}

// Convert a Postition to LW coordinates
Vector<float> xlatPosition(FVector& v)
{
	Vector<float>   Vtmp = v;
	Vector<float>	Vpos = (~LW_TO_UNSKEL_Coords) * Vtmp;

	return Vpos;
}

void setKeyframe(LWChanGroupID group, Vector<float> *pos, Vector<float> *rot, LWTime time)
{
	if (group == (LWChanGroupID)0)
		return;

	LWChannelInfo *chanInfo
			= (LWChannelInfo *)LW_globalFuncs( LWCHANNELINFO_GLOBAL, GFUSE_TRANSIENT );

	LWEnvelopeFuncs *envFuncs
			= (LWEnvelopeFuncs *)LW_globalFuncs( LWENVELOPEFUNCS_GLOBAL, GFUSE_TRANSIENT );

	// Set some keyframe positions
	LWChannelID chan = chanInfo->nextChannel(group,0);
	while (chan != (LWChannelID)0)
	{
		const char *cname = chanInfo->channelName(chan);
		const LWEnvelopeID env = chanInfo->channelEnvelope(chan);

		if		(pos && strcmp(cname,"Position.X") == 0)
		{
			envFuncs->createKey(env,time,(double)(*pos)[0]);
		}
		else if (pos && strcmp(cname,"Position.Y") == 0)
		{
			envFuncs->createKey(env,time,(double)(*pos)[1]);
		}
		else if (pos && strcmp(cname,"Position.Z") == 0)
		{
			envFuncs->createKey(env,time,(double)(*pos)[2]);
		}
		else if (rot && strcmp(cname,"Rotation.H") == 0)
		{
			envFuncs->createKey(env,time,(double)(*rot)[1]);
		}
		else if (rot && strcmp(cname,"Rotation.P") == 0)
		{
			envFuncs->createKey(env,time,(double)(*rot)[0]);
		}
		else if (rot && strcmp(cname,"Rotation.B") == 0)
		{
			envFuncs->createKey(env,time,(double)(*rot)[2]);
		}

		// And check the next ...
		chan = chanInfo->nextChannel(group,chan);
	}
}

// Add in (create) bones based on an incoming Unreal model
void addBones(LWLayoutGeneric *local, UnrealSkeletalModel& model)
{
	LWInterfaceInfo *intInfo
			= (LWInterfaceInfo *)LW_globalFuncs( LWINTERFACEINFO_GLOBAL, GFUSE_TRANSIENT );

	LWItemInfo *itemInfo
			= (LWItemInfo *)LW_globalFuncs( LWITEMINFO_GLOBAL, GFUSE_TRANSIENT );

	LWObjectInfo *objInfo
			= (LWObjectInfo *)LW_globalFuncs( LWOBJECTINFO_GLOBAL, GFUSE_TRANSIENT );

	LWObjectFuncs *objFuncs 
			= (LWObjectFuncs *)LW_globalFuncs( LWOBJECTFUNCS_GLOBAL, GFUSE_TRANSIENT );

	LWItemID target = intInfo->selItems[0];
	const char *targetname = itemInfo->name(target);

	// Array to cache the bone info
	LWItemID *BoneIDs = new LWItemID[model.BoneCount()];

	// Start adding all our bones and orient them
	char tmp[1024];
	for (unsigned int bidx = 0; bidx < model.BoneCount(); bidx++)
	{
		// Make sure we start with the target selected
		sprintf(tmp,"SelectItem %x",target);
		LW_cmdFunc(tmp);
		
		// Add the bone
		UNSKEL_FNamedBoneBinary&	bone = model.Bone(bidx);
		
		sprintf(tmp,"AddBone %s", bone.Name);
		LW_cmdFunc(tmp);

		BoneIDs[bidx] = findBoneByName(target,bone.Name);

		// Select the bone
		sprintf(tmp,"SelectItem %x",BoneIDs[bidx]);
		LW_cmdFunc(tmp);

		// Give it a good size length
		sprintf(tmp, "BoneRestLength %d", 20);
		LW_cmdFunc(tmp);

		// Potentially do some weightmap stuff
		LWMeshInfo *mesh = objInfo->meshInfo(target,0);
		if (mesh != (LWMeshInfo *)0
				&& mesh->pntVLookup(mesh,LWVMAP_WGHT,bone.Name))
		{
			// Set the weight map
			sprintf(tmp,"BoneWeightMapName %s", bone.Name);
			LW_cmdFunc(tmp);

			// Set ONLY the weight map
			sprintf(tmp,"BoneWeightMapOnly");
			LW_cmdFunc(tmp);

			// Acivate it
			sprintf(tmp,"BoneActive");
			LW_cmdFunc(tmp);
		}
		
		// Parent it
		if (bidx > 0)
		{
			sprintf(tmp,"ParentItem %x",BoneIDs[bone.ParentIndex]);
			LW_cmdFunc(tmp);
		}

		// Set up it's rest position
		Vector<float>	Vpos = xlatPosition(bone.BonePos.Position);

		sprintf(tmp,"BoneRestPosition %f %f %f",Vpos[0] ,Vpos[1],Vpos[2]);
		LW_cmdFunc(tmp);

		// Set up its rest rotation (convert to degrees)
		Vector<float>	Vrot = xlatRotation(bone.BonePos.Orientation);

		// um ... HACK ???
		// Root needs to be negated ... for some reason ... not clear why
		if (bidx == 0)
		{
			Vrot = Vector<float>() - Vrot;
		}
		
		double PI = asin(1.0);
		sprintf(tmp,"BoneRestRotation %f %f %f",Vrot[1] * 90 / PI,
												Vrot[0] * 90 / PI,
												Vrot[2] * 90 / PI);
		LW_cmdFunc(tmp);

		// Set keyframes
		setKeyframe(itemInfo->chanGroup(BoneIDs[bidx]),
					&Vpos, &Vrot,
					intInfo->curTime);

		// Make sure we end with the target selected
		sprintf(tmp,"SelectItem %x",target);
		LW_cmdFunc(tmp);
	}
}

// Add in (create) keyframes based on an incoming Unreal model
void keyframeBones(LWLayoutGeneric *local,
				   UnrealSkeletalModel& model,
				   UNSKEL_Animation& anim )
{
	LWInterfaceInfo *intInfo
			= (LWInterfaceInfo *)LW_globalFuncs( LWINTERFACEINFO_GLOBAL, GFUSE_TRANSIENT );

	LWItemInfo *itemInfo
			= (LWItemInfo *)LW_globalFuncs( LWITEMINFO_GLOBAL, GFUSE_TRANSIENT );

	LWSceneInfo *sceneInfo
			= (LWSceneInfo *)LW_globalFuncs( LWSCENEINFO_GLOBAL, GFUSE_TRANSIENT );

	// Snag selected target
	LWItemID target = intInfo->selItems[0];

	// Array to cache the bone info
	LWItemID *BoneIDs = new LWItemID[model.BoneCount()];
	for (int bidx = 0; bidx < model.BoneCount(); bidx ++)
	{
		UNSKEL_FNamedBoneBinary&	bone = model.Bone(bidx);

		BoneIDs[bidx] = findBoneByName(target,bone.Name);
	}

	double fps = sceneInfo->framesPerSecond;
	double ikeyframe = 0;

	// For each keyframe in this animation, orient things
	int kcount = anim.KeyFrameCount() / (model.BoneCount() - anim.StartBone);
	for (int fidx = 0, kidx = 0;
			fidx < kcount;
			fidx++, ikeyframe++)
	{
		// Bone position/orientation
		for (int bidx = anim.StartBone; bidx < model.BoneCount(); kidx++, bidx ++)
		{
			UNSKEL_VQuatAnimKey&		key = anim.KeyFrame(kidx);
			UNSKEL_FNamedBoneBinary&	bone = model.Bone(bidx);

			Vector<float>	Vpos = xlatPosition(key.Position);
			Vector<float>	Vrot = xlatRotation(key.Orientation);

			// um ... HACK ???
			// Root needs to be negated ... for some reason ... not clear why
			if (bidx == 0)
			{
				Vrot = Vector<float>() - Vrot;
			}

			// Set keyframes (if the bone exists)
			if (BoneIDs[bidx] != LWITEM_NULL)
			{	
				setKeyframe(itemInfo->chanGroup(BoneIDs[bidx]),
							&Vpos, &Vrot,
							intInfo->curTime + (ikeyframe / fps));
			}
		}
	}

	delete BoneIDs;
}

// Given a loader struct, load in the object (if necessary) and add bones and stuff
int GLoad_PSK(LWLayoutGeneric *local, AnimLoader *anim)
{
	LWInterfaceInfo *intInfo
			= (LWInterfaceInfo *)LW_globalFuncs( LWINTERFACEINFO_GLOBAL, GFUSE_TRANSIENT );
	if (intInfo->selItems == 0 || intInfo->selItems[0] == LWITEM_NULL)
		return AFUNC_OK;

	UnrealSkeletalModel& model = *(UnrealSkeletalModel *)(anim->Skeleton);
	addBones(local,model);

	return AFUNC_OK;
}

int GLoad_PSA(LWLayoutGeneric *local, AnimLoader *anim)
{
	LWInterfaceInfo *intInfo
			= (LWInterfaceInfo *)LW_globalFuncs( LWINTERFACEINFO_GLOBAL, GFUSE_TRANSIENT );
	if (intInfo->selItems == 0 || intInfo->selItems[0] == LWITEM_NULL)
		return AFUNC_OK;

	UnrealSkeletalModel& model = *(UnrealSkeletalModel *)(anim->Animations);
	UNSKEL_Animation& animation = model.Animation(anim->SelectedAnimation);

	keyframeBones(local,model,animation);

	return AFUNC_OK;
}
