/**************************************
 *
 *  parseunreal.c
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Import/Export object for Epic MegaGames, Inc.
 *  Unreal file format
 *
 *  Unreal is a trademark of Epic MegaGames, Inc.
 *
 **************************************/

#include <stdio.h>
#include <math.h>
#include "string.h"
#include "ParseUnreal.h"

static char	tmp[5];

// Static values
UNRDATA_Poly				UNRDATA_Poly::INVALID =			UNRDATA_Poly();
UNRANIM_Work_Vertex			UNRANIM_Work_Vertex::INVALID =	UNRANIM_Work_Vertex();
UNRANIM_Vertex				UNRANIM_Vertex::INVALID =		UNRANIM_Vertex();
UNRANIM_FramePoints			UNRANIM_FramePoints::INVALID =  UNRANIM_FramePoints();
UNRANIM_Work_FramePoints	UNRANIM_Work_FramePoints::INVALID =  UNRANIM_Work_FramePoints();

void UnrealModel::UpdateHeaderSizes()
{
	DataHeader.Vertex_num = WorkPoints[0].Next();
	DataHeader.Triangle_num = Polys.Next();

	AnimHeader.Frame_num = WorkPoints.Next();
	AnimHeader.Frame_size = DataHeader.Vertex_num * sizeof(UNRANIM_Vertex);
}

// Empty creation
UnrealModel::UnrealModel():
	Polys(UNRDATA_Poly::INVALID),
	WorkPoints(UNRANIM_Work_FramePoints::INVALID),
	FramePoints(UNRANIM_FramePoints::INVALID)
{
	BaseFile[0] = 0;

	memset(&DataHeader,sizeof(DataHeader),0);
	memset(&AnimHeader,sizeof(AnimHeader),0);

	UpdateHeaderSizes();
}

// Load in from file(s)
UnrealModel::UnrealModel(const char *filestub):
	Polys(UNRDATA_Poly::INVALID),
	WorkPoints(UNRANIM_Work_FramePoints::INVALID),
	FramePoints(UNRANIM_FramePoints::INVALID)
{
	strcpy(BaseFile,filestub);

	char	tmp[1024];
	UINT32	nextchunk = 0;
	FILE	*fp = (FILE *)0;
 	UINT32	i = 0, j = 0, k = 0;

	// == Data file first ==
	sprintf(tmp,"%s_d.3d",filestub);
	if ((fp = fopen(tmp,"rb")) == (FILE *)NULL)
	{
		fprintf(stderr,"Can't open Data file '%s'\n",tmp);
		goto RETURN;
	}

	// Read in header
	nextchunk = sizeof(DataHeader);
	i = fread((void *)&DataHeader,nextchunk,1,fp);

	// Load in Polygons
	nextchunk = PolyCount();
	for (j = 0; j < nextchunk;j++)
		i = fread(&Polys[j],sizeof(UNRDATA_Poly),1,fp);

	fclose(fp);

	// == Now Amim file ==
	sprintf(tmp,"%s_a.3a",filestub);
	if ((fp = fopen(tmp,"rb")) == (FILE *)NULL)
	{
		fprintf(stderr,"Can't open existing Animation file '%s'\n",tmp);
		goto RETURN;
	}

	// Read in header
	nextchunk = sizeof(AnimHeader);
	i = fread((void *)&AnimHeader,nextchunk,1,fp);

	// Load in Packed Vertex positions
	nextchunk = PointCount();
	for (k = 0; k < AnimHeader.Frame_num; k++)
		for (j = 0; j < nextchunk;j++)
			i = fread(&FramePoints[k][j],sizeof(UNRANIM_Vertex),1,fp);

//	fprintf(stderr,"Read in %d - expected %d\n",cur,Header.FileSize);

	UpdateHeaderSizes();

RETURN:
	if (fp) fclose(fp);
	fp = 0;
};

void UnrealModel::WritetoDisk(const char *filestub)
{
	if (filestub == (const char *)NULL)
	{
		if (BaseFile[0] == 0)
			return;
		strcpy(BaseFile,filestub);
	}

 
	// == Data file first ==
	FILE	*fp = (FILE *)0;
	sprintf(tmp,"%s_d.3d",filestub);
	if ((fp = fopen(tmp,"rb")) == (FILE *)NULL)
	{
		fprintf(stderr,"Can't open Data file '%s' for write\n",tmp);
		return;
	}
	setvbuf( fp, NULL, _IOFBF, 0x8000 ) ;

	// Just recheck all the header values
	UpdateHeaderSizes();

	// Write out header
	int		cur = 0;
	UINT32	nextchunk = sizeof(DataHeader);
	cur += fwrite((void *)&DataHeader,1,nextchunk,fp);

 	UINT32	i = 0, j = 0, k = 0;

	// Write out triangles
	nextchunk = PolyCount();
	for (j = 0; j < nextchunk; j++) {
		i = fwrite(&Polys[j],sizeof(UNRDATA_Poly),1,fp);
	}

	// Close, open anim file
	fclose(fp);

	// == Now Amim file ==
	sprintf(tmp,"%s_a.3a",filestub);
	if ((fp = fopen(tmp,"rb")) == (FILE *)NULL)
	{
		fprintf(stderr,"Can't open existing Animation file '%s' for write\n",tmp);
		return;
	}

	// Write out in header
	nextchunk = sizeof(AnimHeader);
	cur += fwrite((void *)&AnimHeader,1,nextchunk,fp);

	// Write out in Packed Vertex positions
	nextchunk = PointCount();
	for (k = 0; k < AnimHeader.Frame_num; k++)
	{
		for (j = 0; j < nextchunk; j++)
		{
			UNRANIM_Work_Vertex& wp = WorkPoints[k][j];
			FramePoints[k][j].packedvalue =
					PACK_UNREAL_VERTEX(wp.v[0],wp.v[1],wp.v[2]);
							
			i = fread(&FramePoints[k][j],sizeof(UNRANIM_Vertex),1,fp);
			cur += sizeof(UNRANIM_Vertex);
		}
	}

	fclose(fp);
	fp = 0;
}

