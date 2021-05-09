/**************************************
 *
 *  parseunrealskel.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Import/Export object for Epic MegaGames, Inc.
 *  Unreal Tounament Skeletal file format
 *
 *  Unreal Tounament is a trademark of Epic MegaGames, Inc.
 *
 **************************************/

#include <stdio.h>
#include <math.h>
#include "string.h"
#include "ParseUnrealSkl.h"

// Code based on Vertebrate.h file (c) Epic MegaGames
//	Copyright (c) 1999,2000 Epic MegaGames, Inc. All Rights Reserved.

Matrix<float>	LW_TO_UNSKEL_Coords(	Vector<float>(0,-1,0),
										Vector<float>(0,0,1),
										Vector<float>(1,0,0));

// Static values
UNSKEL_VPoint				UNSKEL_VPoint::INVALID =				UNSKEL_VPoint();
UNSKEL_VVertex				UNSKEL_VVertex::INVALID =				UNSKEL_VVertex();
UNSKEL_VTriangle			UNSKEL_VTriangle::INVALID =				UNSKEL_VTriangle();
UNSKEL_VMaterial			UNSKEL_VMaterial::INVALID =				UNSKEL_VMaterial();
UNSKEL_VRawBoneInfluence	UNSKEL_VRawBoneInfluence::INVALID =		UNSKEL_VRawBoneInfluence();
UNSKEL_FNamedBoneBinary		UNSKEL_FNamedBoneBinary::INVALID =		UNSKEL_FNamedBoneBinary();
UNSKEL_VQuatAnimKey			UNSKEL_VQuatAnimKey::INVALID =			UNSKEL_VQuatAnimKey();
UNSKEL_Animation			UNSKEL_Animation::INVALID =				UNSKEL_Animation();

// Create an empty one
UnrealSkeletalModel::UnrealSkeletalModel():
	Points(UNSKEL_VPoint::INVALID),
	Wedges(UNSKEL_VVertex::INVALID),
	Triangles(UNSKEL_VTriangle::INVALID),
	Materials(UNSKEL_VMaterial::INVALID),
	Weights(UNSKEL_VRawBoneInfluence::INVALID),
	Bones(UNSKEL_FNamedBoneBinary::INVALID),
	Animations(UNSKEL_Animation::INVALID)
{
	BaseFile[0] = 0;
}

// Load from file(s)
	UnrealSkeletalModel::UnrealSkeletalModel(const char *filename):
	Points(UNSKEL_VPoint::INVALID),
	Wedges(UNSKEL_VVertex::INVALID),
	Triangles(UNSKEL_VTriangle::INVALID),
	Materials(UNSKEL_VMaterial::INVALID),
	Weights(UNSKEL_VRawBoneInfluence::INVALID),
	Bones(UNSKEL_FNamedBoneBinary::INVALID),
	Animations(UNSKEL_Animation::INVALID)
{	// First get the stub name
	for (int i = 0; filename[i]; i++)
	{
		if (filename[i] == '.')
		{	// End with the name
			BaseFile[i] = 0;
			break;
		}
		BaseFile[i] = filename[i];
	}

	// Then load in what we were acked to load
	Parse(filename);
}

//
// Load in form a file (first is basic)
//
int UnrealSkeletalModel::Parse(const char *filename)
{
	char tmp[1024];
	strcpy(tmp,filename);
	
	char *c = &(tmp[0]);
	while (*c)
		c++;

	// If this filename doesn't have an extenzion, look for both
	if (*(c -4) != '.')
	{
		sprintf(c,".psk");
		ParseSkeleton(tmp);

		sprintf(c,".psa");
		ParseAnimation(tmp);
	}
	else if (*(c -1) == 'K' || *(c -1) == 'k')
	{	// Load skeleton, then look for animation
		ParseSkeleton(tmp);

		sprintf(c -4,".psa");
		ParseAnimation(tmp);
		*(c - 4) = 0;
	}
	else if (*(c -1) == 'A' || *(c -1) == 'a')
	{	// Load animation, then look for skeleton
		ParseAnimation(tmp);

		sprintf(c -4,".psk");
		ParseSkeleton(tmp);
		*(c - 4) = 0;
	}
	else
	{
		fprintf(stderr,"I don't understand this file '%s'\n",tmp);
		return 0;
	}

	strcpy(BaseFile,tmp);

	return 1;
}

int UnrealSkeletalModel::ParseSkeleton(const char *filename)
{
	FILE *fp = fopen(filename,"rb");
	if (fp == (FILE *)NULL)
	{
		fprintf(stderr,"Can't open skeleton file '%s'\n",filename);
		return 0;
	}

	int i = 0;

	// Header
	UNSKEL_VChunkHdr ChunkHdr;
	fread( (void *)&ChunkHdr, sizeof (ChunkHdr),1,fp);
	////////////////////////////////////////////

	//TCHAR MessagePopup[512];
	//sprintf(MessagePopup, "Writing Skin file, 3d vertices : %i",SkinData.Points.Num());
	//PopupBox(GetActiveWindow(),MessagePopup, "Saving", MB_OK);				

	// Skin: 3D Points
	fread( (void *)&ChunkHdr, sizeof (ChunkHdr),1,fp);
	for( i=0; i < ChunkHdr.DataCount; i++ )
	{
		fread( &Points[i], ChunkHdr.DataSize ,1,fp);
	}

	// Skin: VERTICES (wedges)
	fread( (void *)&ChunkHdr, sizeof (ChunkHdr),1,fp);
	for( i=0; i<ChunkHdr.DataCount; i++)
	{
		fread( &Wedges[i], ChunkHdr.DataSize ,1,fp);
	}

	// Skin: TRIANGLES (faces)
	fread( (void *)&ChunkHdr, sizeof (ChunkHdr),1,fp);
	for( i=0; i<ChunkHdr.DataCount; i++)
	{
		fread( &Triangles[i], ChunkHdr.DataSize ,1,fp);
	}

	// Skin: Materials
	fread( (void *)&ChunkHdr, sizeof (ChunkHdr),1,fp);
	for( i=0; i<ChunkHdr.DataCount; i++)
	{
		fread( &Materials[i], ChunkHdr.DataSize,1,fp);
	}

	// Reference Skeleton: Refskeleton.TotalBones times a VBone.
	fread( (void *)&ChunkHdr, sizeof (ChunkHdr),1,fp);
	//
	for( i=0; i<ChunkHdr.DataCount; i++)
	{
		// Bones are in both animatio nand skeletal - if we're loading
		// both, let's have a qiuck check at how close things are
		UNSKEL_FNamedBoneBinary	&bone = Bones[i];
		if (bone.Name[0] != 0)
		{
			UNSKEL_FNamedBoneBinary tmpBone;
			fread((void *)&tmpBone, ChunkHdr.DataSize ,1,fp);
			if (strcmp(tmpBone.Name,bone.Name))
			{
				fprintf(stderr,"Animated Bone #%d '%s' != loaded Skeleton bone '%s'\n",
							i,bone.Name,tmpBone.Name);
			}
		}
		else
		{	// Nothing there, read away
			fread( &Bones[i], ChunkHdr.DataSize ,1,fp);
		}
	}

	// Reference Weights
	fread( (void *)&ChunkHdr, sizeof (ChunkHdr),1,fp);		
	for( i=0; i< ChunkHdr.DataCount; i++)
	{
		fread( &Weights[i], ChunkHdr.DataSize ,1,fp);
	}

	fclose(fp);
	return 1;
}

//
// Save the .psk file
//
void UnrealSkeletalModel::WriteSkeletontoDisk(const char *filebase)
{
	FILE	*fp = (FILE *)0;
	char tmp[1024];
	sprintf(tmp,"%s.psk",filebase);

	int i = 0;

	// Open file
	if ((fp = fopen(tmp,"wb")) == (FILE *)NULL)
	{
		fprintf(stderr,"Can't open Data file '%s'\n",tmp);
		return;
	}

	// Header
	UNSKEL_VChunkHdr ChunkHdr;
	sprintf(ChunkHdr.ChunkID,"ACTRHEAD");
	ChunkHdr.DataCount = 0;
	ChunkHdr.DataSize  = 0;
	ChunkHdr.TypeFlag  = 1999801; // ' 1 august 99' 
	fwrite( &ChunkHdr, sizeof (ChunkHdr),1,fp);
	////////////////////////////////////////////

	//TCHAR MessagePopup[512];
	//sprintf(MessagePopup, "Writing Skin file, 3d vertices : %i",SkinData.Points.Num());
	//PopupBox(GetActiveWindow(),MessagePopup, "Saving", MB_OK);				

	// Skin: 3D Points
	sprintf(ChunkHdr.ChunkID,"PNTS0000");
	ChunkHdr.DataCount = Points.Next();
	ChunkHdr.DataSize  = sizeof ( UNSKEL_VPoint );
	fwrite( &ChunkHdr, sizeof (ChunkHdr),1,fp);

	/////////////////////////////////////////////
	for( i=0; i < ChunkHdr.DataCount; i++ )
	{
		fwrite( &Points[i], ChunkHdr.DataSize ,1,fp);
	}

	// Skin: VERTICES (wedges)
	sprintf(ChunkHdr.ChunkID,"VTXW0000");
	ChunkHdr.DataCount = Wedges.Next();
	ChunkHdr.DataSize  = sizeof (UNSKEL_VVertex);
	fwrite( &ChunkHdr, sizeof (ChunkHdr),1,fp);
	//
	for( i=0; i<ChunkHdr.DataCount; i++)
	{
		fwrite( &Wedges[i], ChunkHdr.DataSize ,1,fp);
	}

	// Skin: TRIANGLES (faces)
	sprintf(ChunkHdr.ChunkID,"FACE0000");
	ChunkHdr.DataCount = Triangles.Next();
	ChunkHdr.DataSize  = sizeof( UNSKEL_VTriangle );
	fwrite( &ChunkHdr, sizeof (ChunkHdr),1,fp);
	//
	for( i=0; i<ChunkHdr.DataCount; i++)
	{
		fwrite( &Triangles[i], ChunkHdr.DataSize ,1,fp);
	}

	// Skin: Materials
	sprintf(ChunkHdr.ChunkID,"MATT0000");
	ChunkHdr.DataCount = Materials.Next();
	ChunkHdr.DataSize  = sizeof( UNSKEL_VMaterial );
	fwrite( &ChunkHdr, sizeof (ChunkHdr),1,fp);
	//
	for( i=0; i<ChunkHdr.DataCount; i++)
	{
		fwrite( &Materials[i], ChunkHdr.DataSize,1,fp);
	}

	// Reference Skeleton: Refskeleton.TotalBones times a VBone.
	sprintf(ChunkHdr.ChunkID,"REFSKELT");
	ChunkHdr.DataCount = Bones.Next();
	ChunkHdr.DataSize  = sizeof ( UNSKEL_FNamedBoneBinary ) ;
	fwrite( &ChunkHdr, sizeof (ChunkHdr),1,fp);
	//
	for( i=0; i<ChunkHdr.DataCount; i++)
	{
		fwrite( &Bones[i], ChunkHdr.DataSize,1,fp);
	}

	// Reference Skeleton: Refskeleton.TotalBones times a VBone.
	sprintf(ChunkHdr.ChunkID,"RAWWEIGHTS");
	ChunkHdr.DataCount = Weights.Next(); 
	ChunkHdr.DataSize  = sizeof ( VRawBoneInfluence ) ;
	fwrite( &ChunkHdr, sizeof (ChunkHdr),1,fp);
		
	for( i=0; i< ChunkHdr.DataCount; i++)
	{
		fwrite( &Weights[i], ChunkHdr.DataSize ,1,fp);
	}

	fclose(fp);
}

// Load the animation
int UnrealSkeletalModel::ParseAnimation(const char *filename)
{
	int i = 0;
	FILE *fp = fopen(filename,"rb");
	if (fp == (FILE *)NULL)
	{
		fprintf(stderr,"Can't open animation file '%s'\n",filename);
		return 0;
	}

	// Header :
	UNSKEL_VChunkHdr ChunkHdr;
	fread( (void *)&ChunkHdr, sizeof (ChunkHdr),1,fp);

	// Bone names (+flags) list:
	fread( (void *)&ChunkHdr, sizeof (ChunkHdr),1,fp);
	for( i=0; i< ChunkHdr.DataCount; i++)
	{
		// Bones are in both animatio nand skeletal - if we're loading
		// both, let's have a qiuck check at how close things are
		UNSKEL_FNamedBoneBinary	&bone = Bones[i];
		if (bone.Name[0] != 0)
		{
			UNSKEL_FNamedBoneBinary tmpBone;
			fread((void *)&tmpBone, ChunkHdr.DataSize ,1,fp);
			if (strcmp(tmpBone.Name,bone.Name))
			{
				fprintf(stderr,"Skeleton Bone #%d '%s' != loaded Animated bone '%s'\n",
							i,bone.Name,tmpBone.Name);
			}
		}
		else
		{	// Nothing there, read away
			fread( &Bones[i], ChunkHdr.DataSize ,1,fp);
		}
	}

	// Animation Info
	fread( (void *)&ChunkHdr, sizeof (ChunkHdr),1,fp);
	for( i=0; i< ChunkHdr.DataCount; i++)
	{
		fread( &Animations[i], ChunkHdr.DataSize ,1,fp);
	}

	// Key Frames
	fread( (void *)&ChunkHdr, sizeof (ChunkHdr),1,fp);
	for( unsigned int a = 0; a<Animations.Next(); a++ )
	{	
		UNSKEL_Animation& anim = Animations[a];
		int num_keys = (anim.NumRawFrames * anim.TotalBones);
		// Raw keys chunk....
		for( INT i=0; i<num_keys; i++) 
		{
			fread( &anim.KeyFrames[i], ChunkHdr.DataSize ,1,fp);
		}
	}

	fclose(fp);
	return 1;
}

// Save the Animations. - .psa file
void UnrealSkeletalModel::WriteAnimationtoDisk(const char *filebase)
{
	FILE	*fp = (FILE *)0;
	char tmp[1024];
	sprintf(tmp,"%s.psa",filebase);

	int i = 0;
	unsigned int u = 0;

	// Open file
	if ((fp = fopen(tmp,"wb")) == (FILE *)NULL)
	{
		fprintf(stderr,"Can't open Data file '%s'\n",tmp);
		return;
	}

	// Header :
	UNSKEL_VChunkHdr ChunkHdr;
	sprintf(ChunkHdr.ChunkID,("ANIMHEAD"));
	ChunkHdr.DataCount = 0;
	ChunkHdr.DataSize  = 0;
	fwrite( &ChunkHdr, sizeof (ChunkHdr),1,fp);

	// Bone names (+flags) list:
	sprintf(ChunkHdr.ChunkID,("BONENAMES"));
	ChunkHdr.DataCount = Bones.Next(); 
	ChunkHdr.DataSize  = sizeof ( UNSKEL_FNamedBoneBinary ); 
	fwrite( &ChunkHdr, sizeof (ChunkHdr),1,fp);
	for( i=0; i< ChunkHdr.DataCount; i++)
	{
		fwrite( &Bones[i], ChunkHdr.DataSize ,1,fp);
	}
	
	// Add together all frames to get the counts.
	INT TotalAnimKeys = 0;
	INT TotalAnimFrames = 0;
	for( u = 0; u<Animations.Next(); u++)
	{
		Animations[u].FirstRawFrame = TotalAnimKeys / Bones.Next();
		TotalAnimKeys   += Animations[u].KeyFrames.Next();
		TotalAnimFrames += Animations[u].NumRawFrames;			
	}

	sprintf(ChunkHdr.ChunkID,("ANIMINFO"));
	ChunkHdr.DataCount = Animations.Next();
	ChunkHdr.DataSize  = sizeof( UNSKEL_AnimInfoBinary  ); // heap of angaxis/pos/length, 8 floats #debug
	fwrite( &ChunkHdr, sizeof (ChunkHdr),1,fp);
	for( i=0; i< ChunkHdr.DataCount; i++)
	{
		fwrite( &Animations[i], ChunkHdr.DataSize ,1,fp);
	}

	sprintf(ChunkHdr.ChunkID,("ANIMKEYS"));
	ChunkHdr.DataCount = TotalAnimKeys;					// RefSkeletonBones.Num() * BetaNumFrames; 
	ChunkHdr.DataSize  = sizeof( UNSKEL_VQuatAnimKey ); // Heap of angaxis/pos/length, 8 floats #debug
	fwrite( &ChunkHdr, sizeof (ChunkHdr),1,fp);

	// Save out all in our 'digested' array.
	for( u = 0; u<Animations.Next(); u++ )
	{	
		UNSKEL_Animation& anim = Animations[u];
		int num_keys = (anim.NumRawFrames * anim.TotalBones);

		// Raw keys chunk.... total = (frames * bones)
		for( i=0; i<num_keys; i++) 
		{
			fwrite( &anim.KeyFrames[i], ChunkHdr.DataSize ,1,fp);
		}
	}

	fclose(fp);
};

void UnrealSkeletalModel::WritetoDisk(const char *filebase)
{
	WriteAnimationtoDisk(filebase);
	WriteSkeletontoDisk(filebase);
}

/*
// Save the Output animations. ( 'Reference' skeleton is just all the bone names.)
int LoadAnimation(FastFileClass &InFile)
{
	//
	// Animation layout:  
	//
	// name        variable										type
	//
	// ANIMHEAD
	// BONENAMES   RefSkeletonBones								FNamedBoneBinary
	// ANIMINFO    OutAnims										AnimInfoBinary
	// ANIMKEYS    OutAnims[0-.KeyTrack.Num()].KeyTrack[i]....  VQuatAnimKey
	//

	// Animation header.
	VChunkHdr ChunkHdr;
	// Output error message if not found.
	INT ReadBytes = InFile.Read(&ChunkHdr,sizeof(ChunkHdr));	

	// Bones
	InFile.Read(&ChunkHdr,sizeof(ChunkHdr));
	RefSkeletonBones.Empty();		
	RefSkeletonBones.Add( ChunkHdr.DataCount ); 
	for( INT i=0; i<RefSkeletonBones.Num(); i++ )
	{
		InFile.Read( &RefSkeletonBones[i], sizeof(FNamedBoneBinary) );			
	}		
	// Animation info
	InFile.Read(&ChunkHdr,sizeof(ChunkHdr));

	// Proper cleanup: de-linking of tracks! -> because they're 'an array of arrays' and our 
	// dynamic array stuff is dumb and doesn't call individual element-destructors, which would usually carry this responsibility.
	for( i=0; i<OutAnims.Num();i++)
	{
		OutAnims[i].KeyTrack.Empty(); // Empty should really unlink any data !
	}

	OutAnims.Empty();
	OutAnims.AddZeroed( ChunkHdr.DataCount ); // Zeroed... necessary, the KeyTrack is a dynamic array.

	for( i = 0; i<OutAnims.Num(); i++)
	{	
		InFile.Read( &OutAnims[i].AnimInfo, sizeof(AnimInfoBinary));
	}		

	// Key tracks.
	InFile.Read(&ChunkHdr,sizeof(ChunkHdr));
	// verify if total matches read keys...
	INT TotalKeys = ChunkHdr.DataCount;
	INT ReadKeys = 0;

	//PopupBox(" Start loading Keytracks, number: %i OutAnims: %i ", ChunkHdr.DataCount , OutAnims.Num() );
	for( i = 0; i<OutAnims.Num(); i++)
	{	
		INT TrackKeys = OutAnims[i].AnimInfo.NumRawFrames * OutAnims[i].AnimInfo.TotalBones;
		OutAnims[i].KeyTrack.Empty();
		OutAnims[i].KeyTrack.Add(TrackKeys);    // Crash on 'add' -> addzeroed ?????
		InFile.Read( &(OutAnims[i].KeyTrack[0]), TrackKeys * sizeof(VQuatAnimKey) );

		ReadKeys += TrackKeys;
	}

	if( OutAnims.Num() &&  (AnimationBoneNumber > 0) && (AnimationBoneNumber != OutAnims[0].AnimInfo.TotalBones ) )
		PopupBox(" ERROR !! Loaded animation bone number [%i] \n inconsistent with digested bone number [%i] ",OutAnims[0].AnimInfo.TotalBones,AnimationBoneNumber);

	return 1;
}

	// Add a current 'betakeys?' animation to our TempActor's in-memory repertoire.
	// #DEBUG  - make this bone-aware instead of just copying the bulk data !!!!
	int RecordAnimation()
	{		
		// TArray<VQuatAnimKey>  BetaKeys;		
		WorkAnim.AnimInfo.FirstRawFrame = 0; // Fixed up at write time
		WorkAnim.AnimInfo.NumRawFrames =  BetaNumFrames; // BetaKeys.Num() 
		WorkAnim.AnimInfo.StartBone = 0; // 
		WorkAnim.AnimInfo.TotalBones = BetaNumBones; // OurBoneTotal;
		WorkAnim.AnimInfo.TrackTime = BetaNumFrames; // FrameRate;
		WorkAnim.AnimInfo.AnimRate =  FrameRate; // BetaNumFrames;
		WorkAnim.AnimInfo.RootInclude = 0;
		WorkAnim.AnimInfo.KeyReduction = 1.0;
		WorkAnim.AnimInfo.KeyQuotum = BetaKeys.Num(); // NumFrames * BetaNumBones; //Set to full size...
		WorkAnim.AnimInfo.KeyCompressionStyle = 0;

		WorkAnim.KeyTrack.Empty(); 

		if( BetaNumFrames && BetaNumBones ) 
		{
			INT ThisIndex = Animations.Num();

			// Very necessary - see if we're adding inconsistent skeletons to the (raw) frame memory !
			if( ThisIndex==0)
			{
				AnimationBoneNumber = BetaNumBones;
			}
			else if ( AnimationBoneNumber != BetaNumBones )
			{				
				PopupBox("ERROR !! Inconsistent number of bones detected: %i instead of %i",BetaNumBones,AnimationBoneNumber );
				return 0;
			}

			Animations.AddItem( WorkAnim );			
			Animations[ThisIndex].KeyTrack.AddZeroed( BetaKeys.Num() );

			for(INT t=0; t< BetaKeys.Num(); t++)
			{
				Animations[ThisIndex].KeyTrack[t] = BetaKeys[t];
			}
			// Get name.
			_tcscpy( Animations[ThisIndex].AnimInfo.Name, BetaAnimName );
			// Get group name.
			_tcscpy( Animations[ThisIndex].AnimInfo.Group, ("None") );
			
			INT TotalKeys = Animations[ThisIndex].KeyTrack.Num();					
			BetaNumFrames = 0;
			BetaKeys.Empty();
			return TotalKeys;
		}
		else 
		{
			BetaNumFrames = 0;
			BetaKeys.Empty();
			return 0;
		}
	}
*/
