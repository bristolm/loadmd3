/**************************************
 *
 *  parsehalflife.cpp
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
 **************************************/

#include <stdio.h>
#include <stdlib.h>

#include "ParseHalfLife.h"
#include "string.h"
#include "math.h"

extern Matrix<float>	LW_TO_HALFLIFE_Coords(	Vector<float>(0,-1,0),
												Vector<float>(0,0,1),
												Vector<float>(1,0,0));

// Static Definitions
HALFLIFE_Vertex			HALFLIFE_Vertex::INVALID	= HALFLIFE_Vertex();
HALFLIFE_Model			HALFLIFE_Model::INVALID		= HALFLIFE_Model();
HALFLIFE_BodyParts		HALFLIFE_BodyParts::INVALID = HALFLIFE_BodyParts();

// Create an empty one
HalfLifeModel::HalfLifeModel():
BodyParts(HALFLIFE_BodyParts::INVALID)
{
	FileName[0] = 0;
}

// Load from file(s)
HalfLifeModel::HalfLifeModel(const char *filename):
BodyParts(HALFLIFE_BodyParts::INVALID)
{
	// Then load in what we were asked to load
	Parse(filename);

	strcpy(FileName,filename);
}

//
// Load in form a file
//
int HalfLifeModel::Parse(const char *filename)
{
	char tmp[1024];
	FILE *fp = 0;

	strcpy(tmp,filename);
	char *pExt = tmp + strlen(tmp) - 4;

	if (strcmp(pExt,".MDL") && strcmp(pExt,".mdl"))
	{
		fprintf(stderr,"I don't understand this file '%s'\n",tmp);
		return 0;
	}

	// start loading
	memset(&Header,0,sizeof(Header));

	UINT32	cur = 0, nextchunk = 0;
	UINT32	j = 0, k = 0;
	int		i = 0;

	if ((fp = fopen(filename,"rb")) == (FILE *)NULL)
	{
		fprintf(stderr,"Can't open existing '%s'\n",filename);
		return 0;
	}

	// Read in header
	nextchunk = sizeof(Header);
	i = fread((void *)&Header,nextchunk,1,fp);



	return 1;
}
