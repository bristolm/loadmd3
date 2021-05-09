/**************************************
 *
 *  parsehalflife.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Wrapper of Valve's Halflife file format
 *  code based on mdlviewer/studio_utils.cpp from Halflife SDK
 *
 *  HalfLife is a trademark of Valve LLC.
 *	This product contains software technology licensed from Id 
 *	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
 *	All Rights Reserved.
 *
 *  Model file is constructed as a header pointing to groups of various things
 *
 *  bodypartindex --> array of mstudiobodyparts_t
 *	[   per model:
 *		array of boneindicies for each vertex
 *		array of 'normal' bone indices (re: this model's surface)
 *		array of vertices		(vec3_t each ...)
 *		array of normals		(vec3_t each ...)
 *		array of mstudiomesh_t
 *		triangle fans - gl commands
 *
 **************************************/

#include "sys_extra.h"
#include "sys_math.h"

// Setup so the next header file loads ok
typedef		float			vec3_t[3];
typedef		unsigned int	byte;

#include "valve_mdl.h"

extern Matrix<float>	LW_TO_HALFLIFE_Coords;

typedef		studiohdr_t				HALFLIFE_Header;
typedef		mstudiobone_t			stHALFLIFE_Bone;
typedef		mstudiobonecontroller_t	stHALFLIFE_BoneController;
typedef		mstudiobbox_t			stHALFLIFE_BBox;
typedef		mstudioseqdesc_t		stHALFLIFE_Sequence;
typedef		mstudiotexture_t		stHALFLIFE_Texture;

typedef		mstudiobodyparts_t		stHALFLIFE_BodyParts;
typedef		mstudiomodel_t			stHALFLIFE_Model;

typedef struct
{
	Vector<float>		pos;
} stHALFLIFE_Vertex;

typedef struct
{
	Vector<float>	vertidx;
	Vector<float>	normal;

	float			u,v;
} stHALFLIFE_Triangle;

WRAP_STRUCT(HALFLIFE_Bone)
};
WRAP_STRUCT(HALFLIFE_BoneController)
};
WRAP_STRUCT(HALFLIFE_BBox)
};
WRAP_STRUCT(HALFLIFE_Sequence)
};
WRAP_STRUCT(HALFLIFE_Texture)
};

// Model/Mesh structures
WRAP_STRUCT(HALFLIFE_Vertex)
};

WRAP_STRUCT(HALFLIFE_Model)
};

class HALFLIFE_BodyParts : public stHALFLIFE_BodyParts
{
public:
	AutoArray<HALFLIFE_Model>		Models;

	HALFLIFE_BodyParts() :
	  Models(HALFLIFE_Model::INVALID)
	{
		name[0];
	}

	inline UINT32 ModelCount()
	{	return Models.Next();		}
	HALFLIFE_Model&	Model(UINT32 kidx)
	{	return (Models[kidx]);		}

	static HALFLIFE_BodyParts INVALID;
};


// And the main guy ...
class HalfLifeModel
{
private:
	char	FileName[512];

	HALFLIFE_Header		Header;

	AutoArray<HALFLIFE_BodyParts>	BodyParts;

public:
	HalfLifeModel();						// Empty creation
	HalfLifeModel(const char *filename);	// populate from file

	// Query functions
	inline char *getFilename()
	{return FileName;}

	inline UINT32 BodyPartCount()
	{return BodyParts.Next();}

	// Entity retrieval
	inline HALFLIFE_BodyParts&		BodyPart(UINT32 pidx)
	{	return (BodyParts[pidx]);		}

	// Output
	void WritetoDisk(const char *basefile = 0);

	// Input
	int Parse(const char *filename);
};
