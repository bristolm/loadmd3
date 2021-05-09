/**************************************
 *
 *  parsemd4.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Import/Export object for id software's MD4 file format
 *
 *  MD4 file format property of id software
 *
 **************************************/

#include <stdio.h>
#include <math.h>
#include "string.h"
#include "ParseMD4.h"

static char	tmp[5];
static char *id_ID = "IDP4";
static char *id_RTCW = "MDSW";

// Static values
MD4_Bone		MD4_Bone::INVALID=			MD4_Bone();
MD4_Poly		MD4_Poly::INVALID =			MD4_Poly();
MD4_Weight		MD4_Weight::INVALID =		MD4_Weight();
MD4_Vertex		MD4_Vertex::INVALID =		MD4_Vertex();
MD4_Frame		MD4_Frame::INVALID =		MD4_Frame();
MD4_Surface		MD4_Surface::INVALID =		MD4_Surface();
MD4_LOD			MD4_LOD::INVALID =			MD4_LOD();

MD4_Surface::MD4_Surface():
		Vertices(MD4_Vertex::INVALID),
		Polygons(MD4_Poly::INVALID),
		BoneReferences(0)
{
	memset(&Header,0,sizeof(Header));

	memcpy((void *)(&(Header.ID)),(void *)id_ID,4);
	Header.shader[0]= 0;
	Header.shaderIndex = 0;
	strcpy(Header.Name,"asstester");
}

void MD4_Surface::Name(char *c)
{
	strncpy(Header.Name,c,sizeof(Header.Name) -1);
}

INT32 MD4_Surface::UpdateHeaderSizes(int inset)
{
	Header.Header_Start = -inset;		// (negative number)

	Header.Vert_num				= VertexCount();
	Header.Triangle_num			= PolygonCount();
	Header.BoneReferences_num	= BoneRefCount();

	INT32 cur = sizeof(Header);
	Header.Verts_Start			= cur;

	for (int i = 0; i < Header.Vert_num; i++) {
		Vertices[i].Weight_num = Vertices[i].WeightCount();
		cur += Vertices[i].size();
	}

	Header.Triangles_Start		= cur;
	cur += (MD4_Poly::sizeofBase() * Header.Triangle_num);

	Header.BoneReferences_Start	= cur;
	cur += (sizeof(MD4_BoneRef) * Header.BoneReferences_num);

	Header.ChunkSize = cur;

	return cur;
}

int MD4_Surface::Parse(FILE *fp)
{
	INT32	cur = 0, chunksize = 0;
	int		i = 0;
	INT32	j = 0, k = 0;

	chunksize = sizeof(Header);
	i = fread((void *)&Header,chunksize,1,fp);
	cur += chunksize;

	// Vertices
	chunksize = MD4_Vertex::sizeofBase() - MD4_Weight::sizeofBase();
	for (j = 0; j < Header.Vert_num; j++) {
		// read in the main part
		cur += fread(&Vertices[j],1,chunksize,fp);

		// Then all the weights 
		for (k = 0; k < Vertices[j].Weight_num; k++) {
			cur += fread(&(Vertices[j].Weight(k)),1,MD4_Weight::sizeofBase(),fp);
		}
	}
	// cur should == Header.Triangle_Start
	// Triangles
	chunksize = MD4_Poly::sizeofBase();
	for (j = 0; j < Header.Triangle_num; j++) {
		// Read in the main part
		cur += fread(&Polygons[j],1,chunksize,fp);
	}
	// cur should == Header.BoneReferences_Start

	// Bone References
	chunksize = sizeof(MD4_BoneRef);
	for (j = 0; j < Header.BoneReferences_num; j++) {
		// Read in the main part
		cur += fread(&BoneReferences[j],1,chunksize,fp);
	}

	return cur;
}

INT32 MD4_Surface::WritetoDisk(FILE *fp)
{
	INT32	cur = 0, chunksize = 0;
	int		i = 0;
	INT32	j = 0, k = 0;

	chunksize = sizeof(Header);
	i = fwrite((void *)&Header,1,chunksize,fp);
	cur += chunksize;

	// Vertices
	static char vtxchunk[sizeof(MD4_Vertex) + (32 * sizeof(MD4_Weight))];

	chunksize = MD4_Vertex::sizeofBase() - MD4_Weight::sizeofBase();
	for (j = 0; j < Header.Vert_num; j++) {
		int vtxchunksize = 0;
		// Use a temporary buffer
		memcpy(vtxchunk + vtxchunksize,&Vertices[j],chunksize);
		vtxchunksize += chunksize;

		// Then all the weights 
		for (k = 0; k < Vertices[j].Weight_num; k++) {
			MD4_Weight& wght = Vertices[j].Weight(k);
			memcpy(vtxchunk + vtxchunksize,(void *)&wght,MD4_Weight::sizeofBase());
			vtxchunksize += MD4_Weight::sizeofBase();
		}
		cur += fwrite(vtxchunk,1,vtxchunksize,fp);
	}

	// cur should == Header.Triangle_Start
	// Triangles
	chunksize = MD4_Poly::sizeofBase();
	for (j = 0; j < Header.Triangle_num; j++) {
		// Read in the main part
		cur += fwrite(&Polygons[j],1,chunksize,fp);
	}
	// cur should == Header.BoneReferences_Start

	// Bone References
	chunksize = sizeof(MD4_BoneRef);
	for (j = 0; j < Header.BoneReferences_num; j++) {
		// Read in the main part
		cur += fwrite(&BoneReferences[j],1,chunksize,fp);
	}

	return cur;
}

INT32 MD4::UpdateHeaderSizes()
{
	Header.Frame_num		= FrameCount();
	Header.Bone_num			= Frame(0).BoneCount();
	Header.LOD_num			= LODs.Next();

	INT32 cur = sizeof(Header);
	Header.Frames_Start		= cur;

	cur += 	((MD4_Frame::sizeofBase() + (MD4_Bone::sizeofBase() * (Header.Bone_num -1)))
								 * Header.Frame_num);
	Header.LOD_Start		= cur;

	// I believe there are a set of LOD, then all the Surface blocks
	cur += (LODCount() * MD4_LOD::sizeofBase());
	for (UINT32 i = 0; i < LODCount(); i++)
	{
		MD4_LOD& lod = LODs[i];

		lod.Header.Surface_num = lod.SurfaceCount();

		// Distance from me to the beginning of my surface chunks
		lod.Header.Surfaces_Start = cur - Header.LOD_Start - (i * MD4_LOD::sizeofBase());
		
		INT32 tmp = 0;
		for (UINT32 j = 0; j < lod.SurfaceCount(); j++)
		{
			 tmp += lod.Surface(j).UpdateHeaderSizes(cur + tmp);
		}

		lod.Header.SurfacesChunkSize = tmp;
		cur += tmp;
	}

	Header.FileSize = cur;			// end of file

	return (cur);
}

char *MD4::TypeName(char *buf)
{
	if (Type() == 0)
		buf[0] = 0;
	else
	{
		*((UINT32 *)buf) = Type();
		buf[4] = 0;
	}

	return buf;
}

int MD4::isLoaded()
{
	return (strcmp(TypeName(tmp),id_ID) == 0
			|| strcmp(TypeName(tmp),id_RTCW) == 0);
}


MD4::MD4():
	Frames(MD4_Frame::INVALID),
	LODs(MD4_LOD::INVALID)
{
	FileName[0] = 0;
	memset(&Header,0,sizeof(Header));

	memcpy((void *)(&(Header.ID)),(void *)id_ID,4);
	Header.Version  = MD4_VERSION;
}

MD4::MD4(const char *filename):
	Frames(MD4_Frame::INVALID),	
	LODs(MD4_LOD::INVALID)
{
	strcpy((char *)FileName,filename);

	memset(&Header,0,sizeof(Header));

	INT32	cur = 0, nextchunk = 0;
	INT32	j = 0, k = 0;

	if ((fp = fopen(filename,"rb")) == (FILE *)NULL)
	{
		fprintf(stderr,"Can't open existing '%s'\n",filename);
		goto RETURN;
	}

	// Read in header
	nextchunk = sizeof(Header);
	cur = fread((void *)&Header,1,nextchunk,fp);
	if (!isLoaded())
		goto RETURN;

	// Now do Frames
	nextchunk = MD4_Frame::sizeofBase() - MD4_Bone::sizeofBase();
	for (j = 0; j < Header.Frame_num; j++)
	{
		// Read in the main part
		cur += fread(&Frames[j],1,nextchunk,fp);

		/// Then all the bones
		for (k = 0; k < Header.Bone_num; k++)
		{
			cur += fread(&(Frames[j].Bone(k)),1,MD4_Bone::sizeofBase(),fp);
		}
	}

	// Now do LOD
	// cur should equal  Header.LOD_Start
	nextchunk = MD4_LOD::sizeofBase();
	for (j = 0; j < Header.LOD_num; j++)
	{	// Read in the main part
		cur += fread(&(LODs[j].Header),1,MD4_LOD::sizeofBase(),fp);
	}

	// Now hit each surface in the LODs
	for (j = 0; j < Header.LOD_num; j++)
	{
		MD4_LOD &lod = LODs[j];
		for (k = 0; k < lod.Header.Surface_num; k++)
		{
			cur += LODs[j].Surface(k).Parse(fp);
		}
	}
//	fprintf(stderr,"Read in %d - expected %d\n",cur,Header.FileSize);

	UpdateHeaderSizes();

	// Make sure it has a name of some sort
	if (Header.Name[0] == 0)
	{
		for (j = strlen(filename) -2; j > 0; j--)
		{
			if (filename[j] == '/' || filename[j] == '\\')
			{
				++j;
				break;
			}
		}
		strcpy(Header.Name,filename + j);
	}

RETURN:
	if (fp) fclose(fp);
	fp = 0;
};


void MD4::WritetoDisk(const char *file)
{
	if (file == (const char *)NULL)
	{
		if (FileName[0] == 0)
			return;
		file = FileName;
	}

	if ((fp = fopen(file,"wb")) == (FILE *)NULL)
	{
		fprintf(stderr,"Can't open '%s' for write\n",file);
			return;
	}

	sprintf(Header.Name,"models/players/TEST/%s",file);

	setvbuf( fp, NULL, _IOFBF, 0x8000 ) ;

	UpdateHeaderSizes();

	// Write out header
	int		cur = 0;
	UINT32	nextchunk = sizeof(MD4_Header);
	cur = fwrite((void *)&Header,1,nextchunk,fp);
	
	// Now do Frames
	nextchunk = sizeof(stMD4_Frame) - sizeof(stMD4_Bone);
	int j = 0;
	for (j = 0; j < Header.Frame_num; j++)
	{	// just tag it so we can see where we are
		sprintf(Frames[j].Name,"Frame: %d",j);

		// Write out the main part
		cur += fwrite(&Frames[j],1,nextchunk,fp);

		/// Then all the bones
		for (int k = 0; k < Header.Bone_num; k++)
		{
			MD4_Bone& bon = Frames[j].Bone(k);
			cur += fwrite(&bon,1,sizeof(stMD4_Bone),fp);
		}
	}
	// cur should equal  Header.LOD_Start

	// Now do LOD - Create and write them out
	nextchunk = sizeof(stMD4_LOD);
	for (j = 0; j < Header.LOD_num; j++)
	{	// Read in the main part
		cur += fwrite(&(LODs[j].Header),1,sizeof(stMD4_LOD),fp);
	}

	// Now hit each surface in the LODs
	for (j = 0; j < Header.LOD_num; j++)
	{
		for (int k = 0; k < LODs[j].Header.Surface_num; k++)
		{
			cur += LODs[j].Surface(k).WritetoDisk(fp);
		}
	}

	fclose(fp);
	fp = 0;
}

