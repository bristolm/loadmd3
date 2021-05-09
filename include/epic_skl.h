// **************************************************************
// Code from Vertebrate.h (c) 1999,2000 Epic MegaGames, Inc
/*=============================================================================
	
	Vertebrate.h - Binary structures for digesting and exporting skeleton, skin & animation data.

  	Copyright (c) 1999,2000 Epic MegaGames, Inc. All Rights Reserved.

	New model limits (imposed by the binary format and/or engine)
		256		materials.
		65535  'wedges' = amounts to approx 20000 triangles/vertices, depending on texturing complexity.
	A realistic upper limit, with LOD in mind, is 10000 wedges.

	Todo: 
	 Smoothing group support !
	-> Unreal's old non-lod code had structures in place for all adjacent triangles-to-a-vertex,
	   to compute the vertex normal. 
    -> LODMeshes only needs the connectivity of each triangle to its three vertices.
	-> Facet-shading as opposed to gouraud/vertex shading would be a very nice option.
	-> Normals should be exported per influence & transformed along with vertices for faster drawing/lighting pipeline in UT ?!
		 
************************************************************************/

#ifndef VERTHDR_H
#define VERTHDR_H

//#include "DynArray.h"
//#include "Win32IO.h"

#include <assert.h>
#include <stdio.h>


// Bitflags describing effects for a (classic Unreal & Unreal Tournament) mesh triangle.
enum EJSMeshTriType
{
	// Triangle types. Pick ONE AND ONLY ONE of these.
	MTT_Normal				= 0x00,	// Normal one-sided.
	MTT_NormalTwoSided      = 0x01,    // Normal but two-sided.
	MTT_Translucent			= 0x02,	// Translucent two-sided.
	MTT_Masked				= 0x03,	// Masked two-sided.
	MTT_Modulate			= 0x04,	// Modulation blended two-sided.
	MTT_Placeholder			= 0x08,	// Placeholder triangle for positioning weapon. Invisible.

	// Bit flags. Add any of these you want.
	MTT_Unlit				= 0x10,	// Full brightness, no lighting.
	MTT_Flat				= 0x20,	// Flat surface, don't do bMeshCurvy thing.
	MTT_Alpha				= 0x20,	// This material has per-pixel alpha.
	MTT_Environment			= 0x40,	// Environment mapped.
	MTT_NoSmooth			= 0x80,	// No bilinear filtering on this poly's texture.
	
};

//
// Most of these structs are mirrored in Unreal's "UnSkeletal.h" and need to stay binary compatible with Unreal's 
// skeletal data import routines.
//

// File header structure. 
struct VChunkHdr
{
	char		ChunkID[20];  // string ID of up to 19 chars (usually zero-terminated?)
	int			TypeFlag;     // Flags/reserved/Version number...
    int         DataSize;     // size per struct following;
	int         DataCount;    // number of structs/
};


// A Material
struct VMaterial
{
	char		MaterialName[64]; // Straightforward ascii array, for binary input.
	int         TextureIndex;     // multi/sub texture index 
	DWORD		PolyFlags;        // all poly's with THIS material will have this flag.
	int         AuxMaterial;      // index into another material, eg. alpha/detailtexture/shininess/whatever
	DWORD		AuxFlags;		  // reserved: auxiliary flags 
	INT			LodBias;          // material-specific lod bias
	INT			LodStyle;         // material-specific lod style

	// Necessary to determine material uniqueness
	UBOOL operator==( const VMaterial& M ) const
	{
		UBOOL Match = true;
		if (TextureIndex != M.TextureIndex) Match = false;
		if (PolyFlags != M.PolyFlags) Match = false;
		if (AuxMaterial != M.AuxMaterial) Match = false;
		if (AuxFlags != M.AuxFlags) Match = false;
		if (LodBias != M.LodBias) Match = false;
		if (LodStyle != M.LodStyle) Match = false;

		for(INT c=0; c<64; c++)
		{
			if( MaterialName[c] != M.MaterialName[c] )
			{
				Match = false;
				break;
			}
		}
		return Match;
	}	

	// Copy a name and properly zero-terminate it.
	void SetName( char* NewName)
	{
		INT c = 0;
		for(c=0; c<64; c++)
		{
			MaterialName[c]=NewName[c];
			if( MaterialName[c] == 0 ) 
				break;
		}
		// fill out
		while( c<64)
		{
			MaterialName[c] = 0;
			c++;
		}	
	}
};

struct VBitmapOrigin
{
	char RawBitmapName[MAX_PATH];
	char RawBitmapPath[MAX_PATH];
};

// A bone: an orientation, and a position, all relative to their parent.
struct VJointPos
{
	FQuat   	Orientation;  //
	FVector		Position;     //  needed or not ?

	FLOAT       Length;       //  For collision testing / debugging drawing...
	FLOAT       XSize;	      //
	FLOAT       YSize;        //
	FLOAT       ZSize;        //
};

struct FNamedBoneBinary
{
	char	   Name[64];     // ANSICHAR   Name[64];	// Bone's name
	DWORD      Flags;		 // reserved
	INT        NumChildren;  //
	INT		   ParentIndex;	 // 0/NULL if this is the root bone.  
	VJointPos  BonePos;	     //
};

struct AnimInfoBinary
{
	ANSICHAR Name[64];     // Animation's name
	ANSICHAR Group[64];    // Animation's group name	

	INT TotalBones;           // TotalBones * NumRawFrames is number of animation keys to digest.

	INT RootInclude;          // 0 none 1 included 		
	INT KeyCompressionStyle;  // Reserved: variants in tradeoffs for compression.
	INT KeyQuotum;            // Max key quotum for compression	
	FLOAT KeyReduction;       // desired 
	FLOAT TrackTime;          // explicit - can be overridden by the animation rate
	FLOAT AnimRate;           // frames per second.
	INT StartBone;            // - Reserved: for partial animations.
	INT FirstRawFrame;        //
	INT NumRawFrames;         //
};

struct VQuatAnimKey
{
	FVector		Position;           // relative to parent.
	FQuat       Orientation;        // relative to parent.
	FLOAT       Time;				// The duration until the next key (end key wraps to first...)
};


struct VBoneInfIndex // ,, ,, contains Index, number of influences per bone (+ N detail level sizers! ..)
{
	_WORD WeightIndex;
	_WORD Detail0;  // how many to process if we only handle 1 master bone per vertex.
	_WORD Detail1;  // how many to process if we're up to 2 max influences
	_WORD Detail2;  // how many to process if we're up to full 3 max influences 

};

struct VBoneInfluence // Weight and vertex number
{
	_WORD PointIndex; // 3d vertex
	_WORD BoneWeight; // 0..1 scaled influence
};

struct VRawBoneInfluence // Just weight, vertex, and Bone, sorted later.
{
	FLOAT Weight;
	INT   PointIndex;
	INT   BoneIndex;
};

//
// Points: regular FVectors (for now..)
//
struct VPoint
{	
	FVector			Point;             //  change into packed integer later IF necessary, for 3x size reduction...
};


//
// Vertex with texturing info, akin to Hoppe's 'Wedge' concept.
//
struct VVertex
{
	WORD	PointIndex;	 // Index to a point.
	FLOAT   U,V;         // Engine may choose to store these as floats, words,bytes - but raw PSK file has full floats.
	BYTE    MatIndex;    // At runtime, this one will be implied by the vertex that's pointing to us.
	BYTE    Reserved;    // Top secret.
};

//
// Textured triangle.
//
struct VTriangle
{
	WORD    WedgeIndex[3];	 // point to three vertices in the vertex list.
	BYTE    MatIndex;	     // Materials can be anything.
	BYTE    AuxMatIndex;     // Second material (eg. damage skin, shininess, detail texture / detail mesh...
	DWORD   SmoothingGroups; // 32-bit flag for smoothing groups AND Lod-bias calculation.
};

/* - unneeded for exporter - MRB 2001
//
// Physique (or other) skin.
//
struct VSkin
{
	TArray <VMaterial>			Materials; // Materials
	TArray <VPoint>				Points;    // 3D Points
	TArray <VVertex>			Wedges;  // Wedges
	TArray <VTriangle>			Faces;       // Faces
	TArray <FNamedBoneBinary>   RefBones;   // reference skeleton
	TArray <void*>				RawMaterials; // alongside Materials - to access extra data //Mtl*
	TArray <VRawBoneInfluence>	RawWeights;  // Raw bone weights..
	TArray <VBitmapOrigin>      RawBMPaths;  // Full bitmap Paths for logging

	// Brushes: UV must embody the material size...
	TArray <INT>				MaterialUSize;
	TArray <INT>				MaterialVSize;
	
	int NumBones; // Explicit number of bones (?)
};

//
// Complete Animation for a skeleton.
// Independent-sized key tracks for each part of the skeleton.
//
struct VAnimation
{
	AnimInfoBinary        AnimInfo;   //  Generic animation info as saved for the engine
	TArray <VQuatAnimKey> KeyTrack;   //  Animation track - keys * bones
};


//
// The internal animation class, which can contain various amounts of
// (mesh+reference skeleton), and (skeletal animation sequences).
//
// Knows how to save a subset of the data as a valid Unreal2
// model/skin/skeleton/animation file.
//

class VActor
{
public:

	// Some globals from the scene probe. 
	// Modifier *PhySkin;

	INT		NodeCount;
	INT     MeshCount;
	INT     PhysiqueCount;

	FLOAT	FrameTotalTicks; 
	FLOAT   FrameRate; 

	// file stuff
	char*	LogFileName;
	char*   OutFileName;

	// Multiple skins: because not all skins have to be Physique skins; and
	// we also want hierarchical actors/animation (robots etc.)

	VSkin             SkinData;

	// Single skeleton digestion (reference)
	TArray <FNamedBoneBinary>    RefSkeletonBones;
	TArray <AXNode*>		         RefSkeletonNodes;     // The node* array for each bone index....

	int	MatchNodeToSkeletonIndex(AXNode* ANode)
	{
		for (int t=0; t<RefSkeletonBones.Num(); t++)
		{
			if ( RefSkeletonNodes[t] == ANode) return t;
		}
		return -1; // no matching node found.
	}

	int IsSameVertex( VVertex &A, VVertex &B )
	{
		if (A.PointIndex != B.PointIndex) return 0;
		if (A.MatIndex != B.MatIndex) return 0;
		if (A.V != B.V) return 0;
		if (A.U != B.U) return 0;
		return 1;
	}

	// Raw animation, 'betakeys' structure
	int                   BetaNumBones;
	int                   BetaNumFrames; //#debug
	TArray<VQuatAnimKey>  BetaKeys;
	char                  BetaAnimName[64];

	// Workspace for animation - replaces Betakeys.
	VAnimation WorkAnim;

	// Array of digested Animations - all same bone count, variable frame count.
	TArray  <VAnimation> Animations; //digested;
	TArray  <VAnimation> OutAnims;   //queued for output.
	int		             AnimationBoneNumber; // Bone number consistency check..
	
	// Ctor
	VActor()
	{
		// NumFrames = 0;
		// PhySkin = NULL;
		NodeCount = 0;
		MeshCount = 0;
		PhysiqueCount = 0;
		AnimationBoneNumber = 0;

	};

	~VActor()
	{
		// Unnecessary ?
		BetaKeys.Empty();
		RefSkeletonNodes.Empty();
		RefSkeletonBones.Empty();

		// Clean up all dynamic-array-allocated ones - can't count on automatic destructor calls for multi-dimension arrays..?
		{for(INT i=0; i<Animations.Num(); i++)
		{
			Animations[i].KeyTrack.Empty();
		}}
		Animations.Empty();

		{for(INT i=0; i<OutAnims.Num(); i++)
		{
			OutAnims[i].KeyTrack.Empty();
		}}
		OutAnims.Empty();
	};


	void Cleanup()
	{
		BetaKeys.Empty();
		RefSkeletonNodes.Empty();
		RefSkeletonBones.Empty();

		// Clean up all dynamic-array-allocated ones - can't count on automatic destructor calls for multi-dimension arrays..?
		{for(INT i=0; i<Animations.Num(); i++)
		{
			Animations[i].KeyTrack.Empty();
		}}
		Animations.Empty();

		{for(INT i=0; i<OutAnims.Num(); i++)
		{
			OutAnims[i].KeyTrack.Empty();
		}}
		OutAnims.Empty();

	}

	//
	// Save the actor: Physique mesh, plus reference skeleton.
	//

	int SerializeActor(FastFileClass &OutFile)
	{
		// Header
		VChunkHdr ChunkHdr;
		_tcscpy(ChunkHdr.ChunkID,"ACTRHEAD");
		ChunkHdr.DataCount = 0;
		ChunkHdr.DataSize  = 0;
		ChunkHdr.TypeFlag  = 1999801; // ' 1 august 99' 
		OutFile.Write( &ChunkHdr, sizeof (ChunkHdr));
		////////////////////////////////////////////

		//TCHAR MessagePopup[512];
		//sprintf(MessagePopup, "Writing Skin file, 3d vertices : %i",SkinData.Points.Num());
		//PopupBox(GetActiveWindow(),MessagePopup, "Saving", MB_OK);				

		// Skin: 3D Points
		_tcscpy(ChunkHdr.ChunkID,("PNTS0000"));
		ChunkHdr.DataCount = SkinData.Points.Num();
		ChunkHdr.DataSize  = sizeof ( VPoint );
		OutFile.Write( &ChunkHdr, sizeof (ChunkHdr));

		/////////////////////////////////////////////
		for( int i=0; i < (SkinData.Points.Num()); i++ )
		{
			OutFile.Write( &SkinData.Points[i], sizeof (VPoint));
		}

		// Skin: VERTICES (wedges)
		_tcscpy(ChunkHdr.ChunkID,("VTXW0000"));
		ChunkHdr.DataCount = SkinData.Wedges.Num();
		ChunkHdr.DataSize  = sizeof (VVertex);
		OutFile.Write( &ChunkHdr, sizeof (ChunkHdr));
		//
		for( i=0; i<SkinData.Wedges.Num(); i++)
		{
			OutFile.Write( &SkinData.Wedges[i], sizeof (VVertex));
		}

		// Skin: TRIANGLES (faces)
		_tcscpy(ChunkHdr.ChunkID,("FACE0000"));
		ChunkHdr.DataCount = SkinData.Faces.Num();
		ChunkHdr.DataSize  = sizeof( VTriangle );
		OutFile.Write( &ChunkHdr, sizeof (ChunkHdr));
		//
		for( i=0; i<SkinData.Faces.Num(); i++)
		{
			OutFile.Write( &SkinData.Faces[i], sizeof (VTriangle));
		}

		// Skin: Materials
		_tcscpy(ChunkHdr.ChunkID,("MATT0000"));
		ChunkHdr.DataCount = SkinData.Materials.Num();
		ChunkHdr.DataSize  = sizeof( VMaterial );
		OutFile.Write( &ChunkHdr, sizeof (ChunkHdr));
		//
		for( i=0; i<SkinData.Materials.Num(); i++)
		{
			OutFile.Write( &SkinData.Materials[i], sizeof (VMaterial));
		}

		// Reference Skeleton: Refskeleton.TotalBones times a VBone.
		_tcscpy(ChunkHdr.ChunkID,("REFSKELT"));
		ChunkHdr.DataCount = RefSkeletonBones.Num();
		ChunkHdr.DataSize  = sizeof ( FNamedBoneBinary ) ;
		OutFile.Write( &ChunkHdr, sizeof (ChunkHdr));
		//
		for( i=0; i<RefSkeletonBones.Num(); i++)
		{
			OutFile.Write( &RefSkeletonBones[i], sizeof (FNamedBoneBinary));
		}

		// Reference Skeleton: Refskeleton.TotalBones times a VBone.
		_tcscpy(ChunkHdr.ChunkID,("RAWWEIGHTS"));
		ChunkHdr.DataCount = SkinData.RawWeights.Num(); 
		ChunkHdr.DataSize  = sizeof ( VRawBoneInfluence ) ;
		OutFile.Write( &ChunkHdr, sizeof (ChunkHdr));
			
		for( i=0; i< SkinData.RawWeights.Num(); i++)
		{
			OutFile.Write( &SkinData.RawWeights[i], sizeof (VRawBoneInfluence));
		}
		
		return OutFile.GetError();
	};


	// Save the Output animations. ( 'Reference' skeleton is just all the bone names.)
	int SerializeAnimation(FastFileClass &OutFile)
	{
		// Header :
		VChunkHdr ChunkHdr;
		_tcscpy(ChunkHdr.ChunkID,("ANIMHEAD"));
		ChunkHdr.DataCount = 0;
		ChunkHdr.DataSize  = 0;
		OutFile.Write( &ChunkHdr, sizeof (ChunkHdr));

		// Bone names (+flags) list:
		_tcscpy(ChunkHdr.ChunkID,("BONENAMES"));
		ChunkHdr.DataCount = RefSkeletonBones.Num(); 
		ChunkHdr.DataSize  = sizeof ( FNamedBoneBinary ); 
		OutFile.Write(&ChunkHdr, sizeof (ChunkHdr));
		for(int b = 0; b < RefSkeletonBones.Num(); b++)
		{
			OutFile.Write( &RefSkeletonBones[b], sizeof (FNamedBoneBinary) );
		}

		INT TotalAnimKeys = 0;
		INT TotalAnimFrames = 0;
		// Add together all frames to get the count.
		for(INT i = 0; i<OutAnims.Num(); i++)
		{
			OutAnims[i].AnimInfo.FirstRawFrame = TotalAnimKeys / RefSkeletonBones.Num();
			TotalAnimKeys   += OutAnims[i].KeyTrack.Num();
			TotalAnimFrames += OutAnims[i].AnimInfo.NumRawFrames;			
		}
	
		_tcscpy(ChunkHdr.ChunkID,("ANIMINFO"));
	    ChunkHdr.DataCount = OutAnims.Num();
		ChunkHdr.DataSize  = sizeof( AnimInfoBinary  ); // heap of angaxis/pos/length, 8 floats #debug
		OutFile.Write( &ChunkHdr, sizeof (ChunkHdr));
		for( i = 0; i<OutAnims.Num(); i++)
		{
			OutFile.Write( &OutAnims[i].AnimInfo, sizeof( AnimInfoBinary ) );
		}
		
		_tcscpy(ChunkHdr.ChunkID,("ANIMKEYS"));
	    ChunkHdr.DataCount = TotalAnimKeys;            // RefSkeletonBones.Num() * BetaNumFrames; 
		ChunkHdr.DataSize  = sizeof( VQuatAnimKey );   // Heap of angaxis/pos/length, 8 floats #debug
		OutFile.Write( &ChunkHdr, sizeof (ChunkHdr));

		// Save out all in our 'digested' array.
		for( INT a = 0; a<OutAnims.Num(); a++ )
		{	
			// Raw keys chunk....
			for( INT i=0; i<OutAnims[a].KeyTrack.Num(); i++) 
			{
				OutFile.Write( &OutAnims[a].KeyTrack[i], sizeof ( VQuatAnimKey ) );
			}
		}

		return 1;
	};

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

		WorkAnim.KeyTrack.Empty(); // #debug explicit zeroing - kludgy => used to guarantee zero initialization.
		//WorkAnim.KeyTrack.ArrayNum = 0;  
		//WorkAnim.KeyTrack.ArrayMax = 0;  
		//WorkAnim.KeyTrack.Data = NULL;   

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

			Animations[ThisIndex].KeyTrack.Empty();

			for(INT t=0; t< BetaKeys.Num(); t++)
			{
				Animations[ThisIndex].KeyTrack.AddItem( BetaKeys[t] );
			}
			// get name
			_tcscpy( Animations[ThisIndex].AnimInfo.Name, BetaAnimName );
			// get group name
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

	// Write out a brush to the file. Everything's in SkinData - ignore any weights etc.
	int WriteBrush(FastFileClass &OutFile, INT DoSmooth, INT OneTexture )
	{		
		//for(INT m=0; m<SkinData.Materials.Num(); m++)
		//	PopupBox("MATERIAL OUTPUT SIZES: [%i] %i %i",m,SkinData.MaterialUSize[m],SkinData.MaterialVSize[m]);


		OutFile.Print("Begin PolyList\r\n");
		// Write all faces.
		for(INT i=0; i<SkinData.Faces.Num(); i++)
		{
			
			FVector Base;
			FVector Normal;
			FVector TextureU;
			FVector TextureV;
			INT PointIdx[3];
			FVector Vertex[3];
			FLOAT U[3],V[3];

			INT TexIndex = SkinData.Faces[i].MatIndex;
			if ( OneTexture ) TexIndex = 0;

			for(INT v=0; v<3; v++)
			{
				PointIdx[v]= SkinData.Wedges[ SkinData.Faces[i].WedgeIndex[v] ].PointIndex;
				Vertex[v] = SkinData.Points[ PointIdx[v]].Point;
				U[v] = SkinData.Wedges[ SkinData.Faces[i].WedgeIndex[v] ].U;
				V[v] = SkinData.Wedges[ SkinData.Faces[i].WedgeIndex[v] ].V;
			}

			// Compute Unreal-style texture coordinate systems:
			//FLOAT MaterialWidth  = 1024; //SkinData.MaterialUSize[TexIndex]; //256.0f
			//FLOAT MaterialHeight = -1024; //SkinData.MaterialVSize[TexIndex]; //256.0f

			FLOAT MaterialWidth  = SkinData.MaterialUSize[TexIndex]; 
			FLOAT MaterialHeight = -SkinData.MaterialVSize[TexIndex]; 			

			FTexCoordsToVectors
			(
				Vertex[0], FVector( U[0],V[0],0.0f) * FVector( MaterialWidth, MaterialHeight, 1),
				Vertex[1], FVector( U[1],V[1],0.0f) * FVector( MaterialWidth, MaterialHeight, 1),
				Vertex[2], FVector( U[2],V[2],0.0f) * FVector( MaterialWidth, MaterialHeight, 1),
				&Base, &TextureU, &TextureV 
			);

			// Need to flip the one texture vector ?
			TextureV *= -1;
			// Pre-flip ???
			FVector Flip(-1,1,1);
			Vertex[0] *= Flip;
			Vertex[1] *= Flip;
			Vertex[2] *= Flip;			
			Base *= Flip;
			TextureU *= Flip;
			TextureV *= Flip;

			// Maya: need to flip everything 'upright' -Y vs Z?
			
			// Write face
			OutFile.Print("	Begin Polygon");

			

			OutFile.Print("	Texture=%s", SkinData.Materials[TexIndex].MaterialName );
			if( DoSmooth )
				OutFile.Print(" Smooth=%u", SkinData.Faces[i].SmoothingGroups);

			OutFile.Print(" Link=%u", i);
			OutFile.Print("\r\n");

			OutFile.Print("		Origin   %+013.6f,%+013.6f,%+013.6f\r\n", Base.X, Base.Y, Base.Z );			
			OutFile.Print("		TextureU %+013.6f,%+013.6f,%+013.6f\r\n", TextureU.X, TextureU.Y, TextureU.Z );
			OutFile.Print("		TextureV %+013.6f,%+013.6f,%+013.6f\r\n", TextureV.X, TextureV.Y, TextureV.Z );
			for( v=0; v<3; v++ )
			OutFile.Print("		Vertex   %+013.6f,%+013.6f,%+013.6f\r\n", Vertex[v].X, Vertex[v].Y, Vertex[v].Z );
			OutFile.Print("	End Polygon\r\n");
		}		
		OutFile.Print("End PolyList\r\n");

		return 0;
	}
*/

// T3D export code: see UnEdExp.cpp 
/*
UBOOL UPolysExporterT3D::ExportText( UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn )
{
	guard(UPolysExporterT3D::ExportText);
	UPolys* Polys = CastChecked<UPolys>( Object );

	Ar.Logf( TEXT("%sBegin PolyList\r\n"), appSpc(TextIndent) );
	for( INT i=0; i<Polys->Element.Num(); i++ )
	{
		FPoly* Poly = &Polys->Element(i);
		TCHAR TempStr[256];

		// Start of polygon plus group/item name if applicable.
		Ar.Logf( TEXT("%s   Begin Polygon"), appSpc(TextIndent) );
		if( Poly->ItemName != NAME_None )
			Ar.Logf( TEXT(" Item=%s"), *Poly->ItemName );
		if( Poly->Texture )
			Ar.Logf( TEXT(" Texture=%s"), Poly->Texture->GetPathName() );
		if( Poly->PolyFlags != 0 )
			Ar.Logf( TEXT(" Flags=%i"), Poly->PolyFlags );
		if( Poly->iLink != INDEX_NONE )
			Ar.Logf( TEXT(" Link=%i"), Poly->iLink );
		if( Poly->SmoothingMask != 0 )
			Ar.Logf( TEXT(" Smooth=%u"), Poly->SmoothingMask );
		Ar.Logf( TEXT("\r\n") );

		// All coordinates.
		Ar.Logf( TEXT("%s      Origin   %s\r\n"), appSpc(TextIndent), SetFVECTOR(TempStr,&Poly->Base) );
		Ar.Logf( TEXT("%s      Normal   %s\r\n"), appSpc(TextIndent), SetFVECTOR(TempStr,&Poly->Normal) );
		if( Poly->PanU!=0 || Poly->PanV!=0 )
			Ar.Logf( TEXT("%s      Pan      U=%i V=%i\r\n"), appSpc(TextIndent), Poly->PanU, Poly->PanV );
		Ar.Logf( TEXT("%s      TextureU %s\r\n"), appSpc(TextIndent), SetFVECTOR(TempStr,&Poly->TextureU) );
		Ar.Logf( TEXT("%s      TextureV %s\r\n"), appSpc(TextIndent), SetFVECTOR(TempStr,&Poly->TextureV) );
		for( INT j=0; j<Poly->NumVertices; j++ )
			Ar.Logf( TEXT("%s      Vertex   %s\r\n"), appSpc(TextIndent), SetFVECTOR(TempStr,&Poly->Vertex[j]) );
		Ar.Logf( TEXT("%s   End Polygon\r\n"), appSpc(TextIndent) );
	}
	Ar.Logf( TEXT("%sEnd PolyList\r\n"), appSpc(TextIndent) );


	//
	// Output a vector.
	//
	EDITOR_API TCHAR* SetFVECTOR( TCHAR* Dest, const FVector* FVector )
	{
		guard(SetFVECTOR);
		appSprintf( Dest, TEXT("%+013.6f,%+013.6f,%+013.6f"), FVector->X, FVector->Y, FVector->Z );
		return Dest;
		unguard;
	}


	return 1;
	unguard;
}









  void Logf(char* LogString, ... )
	{
		char TempStr[4096];
		appGetVarArgs(TempStr,4096,LogString);

		if( LStream )
		{
			fprintf(LStream,TempStr);
		}
	}




};
*/

#endif






