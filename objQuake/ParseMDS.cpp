/**************************************
 *
 *  parsemds.cpp
 *  Copyright (c) 2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Import/Export object for id software's MDS file format
 *
 *  MDS file format property of id software
 *  MDS is from the Return to Castle Wolfenstien release
 *
 **************************************/

#include <stdio.h>
#include <math.h>
#include "string.h"
#include "ParseMDS.h"

Matrix<float>	LW_TO_MDS_Coords(Vector<float>(0,-1,0),
								 Vector<float>(0,0,1),
								 Vector<float>(1,0,0));


static char	tmp[5];
static char *id_RTCW = "MDSW";

// Static values
MDS_Bone		MDS_Bone::INVALID=			MDS_Bone();
MDS_BoneFrame	MDS_BoneFrame::INVALID=		MDS_BoneFrame();
MDS_Poly		MDS_Poly::INVALID =			MDS_Poly();
MDS_Weight		MDS_Weight::INVALID =		MDS_Weight();
MDS_Vertex		MDS_Vertex::INVALID =		MDS_Vertex();
MDS_Frame		MDS_Frame::INVALID =		MDS_Frame();
MDS_Surface		MDS_Surface::INVALID =		MDS_Surface();
MDS_LOD			MDS_LOD::INVALID =			MDS_LOD();
MDS_Tag			MDS_Tag::INVALID =			MDS_Tag();

MDS_Surface::MDS_Surface():
		Vertices(MDS_Vertex::INVALID),
		Polygons(MDS_Poly::INVALID),
		BoneReferences(0)
{
	memset(&Header,0,sizeof(Header));

	memcpy((void *)(&(Header.ID)),(void *)id_RTCW,4);
	Header.shader[0]= 0;
	Header.shaderIndex = 0;
	strcpy(Header.Name,"asstester");
}

void MDS_Surface::Name(char *c)
{
	strncpy(Header.Name,c,sizeof(Header.Name) -1);
}

INT32 MDS_Surface::UpdateHeaderSizes(int inset)
{
	Header.Header_start = -inset;		// (negative number)

	Header.Vert_num				= VertexCount();
	Header.Triangle_num			= PolygonCount();
	Header.BoneReferences_num	= BoneRefCount();

	INT32 cur = sizeof(Header);
	Header.Vert_start			= cur;

	for (int i = 0; i < Header.Vert_num; i++) {
		Vertices[i].Weight_num = Vertices[i].WeightCount();
		cur += Vertices[i].size();
	}

	Header.Triangles_start		= cur;
	cur += (MDS_Poly::sizeofBase() * Header.Triangle_num);

	Header.BoneReferences_start	= cur;
	cur += (sizeof(MDS_BoneRef) * Header.BoneReferences_num);

	Header.ChunkSize = cur;

	return cur;
}

int MDS_Surface::Parse(FILE *fp)
{
	INT32	cur = 0, chunksize = 0;
	int		i = 0;
	INT32	j = 0, k = 0;
	long curpos = ftell(fp);

	chunksize = sizeof(Header);
	i = fread((void *)&Header,chunksize,1,fp);
	cur += chunksize;

	// Vertices
	fseek(fp,curpos + Header.Vert_start,SEEK_SET);
	chunksize = MDS_Vertex::sizeofBase();
	for (j = 0; j < Header.Vert_num; j++) {
		// read in the main part
		cur += fread(&Vertices[j],1,chunksize,fp);

		// Then all the weights 
		for (k = 0; k < Vertices[j].Weight_num; k++) {
			cur += fread(&(Vertices[j].Weight(k)),1,MDS_Weight::sizeofBase(),fp);
		}
	}

	// cur should == Header.Triangle_Start
	// Triangles
	fseek(fp,curpos + Header.Triangles_start,SEEK_SET);
	chunksize = MDS_Poly::sizeofBase();
	for (j = 0; j < Header.Triangle_num; j++) {
		// Read in the main part
		cur += fread(&Polygons[j],1,chunksize,fp);
	}
	// cur should == Header.BoneReferences_Start

	// Bone References
	fseek(fp,curpos + Header.BoneReferences_start,SEEK_SET);
	chunksize = sizeof(MDS_BoneRef);
	for (j = 0; j < Header.BoneReferences_num; j++) {
		// Read in the main part
		cur += fread(&BoneReferences[j],1,chunksize,fp);
	}

	// and bump it past our chunk
	fseek(fp,curpos + Header.ChunkSize,SEEK_SET);

	return cur;
}

INT32 MDS_Surface::WritetoDisk(FILE *fp)
{
	INT32	cur = 0, chunksize = 0;
	int		i = 0;
	INT32	j = 0, k = 0;

	chunksize = sizeof(Header);
	i = fwrite((void *)&Header,1,chunksize,fp);
	cur += chunksize;

	// Vertices
	static char vtxchunk[sizeof(MDS_Vertex) + (32 * sizeof(MDS_Weight))];

	chunksize = MDS_Vertex::sizeofBase() - MDS_Weight::sizeofBase();
	for (j = 0; j < Header.Vert_num; j++) {
		int vtxchunksize = 0;
		// Use a temporary buffer
		memcpy(vtxchunk + vtxchunksize,&Vertices[j],chunksize);
		vtxchunksize += chunksize;

		// Then all the weights 
		for (k = 0; k < Vertices[j].Weight_num; k++) {
			MDS_Weight& wght = Vertices[j].Weight(k);
			memcpy(vtxchunk + vtxchunksize,(void *)&wght,MDS_Weight::sizeofBase());
			vtxchunksize += MDS_Weight::sizeofBase();
		}
		cur += fwrite(vtxchunk,1,vtxchunksize,fp);
	}

	// cur should == Header.Triangle_Start
	// Triangles
	chunksize = MDS_Poly::sizeofBase();
	for (j = 0; j < Header.Triangle_num; j++) {
		// Read in the main part
		cur += fwrite(&Polygons[j],1,chunksize,fp);
	}
	// cur should == Header.BoneReferences_Start

	// Bone References
	chunksize = sizeof(MDS_BoneRef);
	for (j = 0; j < Header.BoneReferences_num; j++) {
		// Read in the main part
		cur += fwrite(&BoneReferences[j],1,chunksize,fp);
	}

	return cur;
}

INT32 MDS::UpdateHeaderSizes()
{
	Header.Frame_num		= FrameCount();
	Header.Bone_num			= Frame(0).BoneCount();
	Header.LOD_num			= LODs.Next();

	INT32 cur = sizeof(Header);
	Header.Frames_start		= cur;

	cur += 	((MDS_Frame::sizeofBase() + (MDS_Bone::sizeofBase() * (Header.Bone_num -1)))
								 * Header.Frame_num);
//	Header.LOD_Start		= cur;

	// I believe there are a set of LOD, then all the Surface blocks
	cur += (LODCount() * MDS_LOD::sizeofBase());
	for (UINT32 i = 0; i < LODCount(); i++)
	{
		MDS_LOD& lod = LODs[i];

		lod.Header.Surface_num = lod.SurfaceCount();

		// Distance from me to the beginning of my surface chunks
//		lod.Header.Surfaces_Start = cur - Header.LOD_Start - (i * MDS_LOD::sizeofBase());
		
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

char *MDS::TypeName(char *buf)
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

int MDS::isLoaded()
{
	return (strcmp(TypeName(tmp),id_RTCW) == 0);
}


MDS::MDS():
	Frames(MDS_Frame::INVALID),
	LODs(MDS_LOD::INVALID),
	Bones(MDS_Bone::INVALID),
	Tags(MDS_Tag::INVALID)
{
	FileName[0] = 0;
	memset(&Header,0,sizeof(Header));

	memcpy((void *)(&(Header.ID)),(void *)id_RTCW,4);
	Header.Version  = MDS_VERSION;
}

MDS::MDS(const char *filename):
	Frames(MDS_Frame::INVALID),	
	LODs(MDS_LOD::INVALID),
	Bones(MDS_Bone::INVALID),
	Tags(MDS_Tag::INVALID)
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

	// Snag the bones
	cur = fseek(fp,Header.Bone_start,SEEK_SET);
	nextchunk = MDS_Bone::sizeofBase();
	for (j = 0; j < Header.Bone_num; j++)
	{
		cur += fread(&Bones[j],1,nextchunk,fp);
	}

	// Now do Frames
	cur = fseek(fp,Header.Frames_start,SEEK_SET);
	nextchunk = MDS_Frame::sizeofBase();
	for (j = 0; j < Header.Frame_num; j++)
	{
		cur += fread(&Frames[j],1,nextchunk,fp);

		/// Then all the bones
		for (k = 0; k < Header.Bone_num; k++)
		{
			stMDS_BoneFrame boneframe;
			cur += fread(&boneframe,1,MDS_BoneFrame ::sizeofBase(),fp);

			Frames[j].Bone(k) = MDS_BoneFrame(boneframe);
		}
	}

	// Now do meshes
	cur = fseek(fp,Header.LOD_start,SEEK_SET);
	nextchunk = MDS_Surface::sizeofBase();
	for (j = 0; j < Header.LOD_num; j++)
	{
		for (k = 0; k < Header.Surface_num; k++)
		{
			LODs[j].Surface(k).Parse(fp);
		}
	}

	// Now there is some sort of tag reference tacked on the end
	cur = fseek(fp,Header.Tag_start,SEEK_SET);
	nextchunk = MDS_Tag::sizeofBase();
	for (j = 0; j < Header.Tag_num; j++)
	{
		cur += fread(&(Tags[j]),1,nextchunk,fp);
	}

	// Now do LOD
	// cur should equal  Header.LOD_Start
//	nextchunk = MDS_LOD::sizeofBase();
//	for (j = 0; j < Header.LOD_num; j++)
//	{	// Read in the main part
//		cur += fread(&(LODs[j].Header),1,MDS_LOD::sizeofBase(),fp);
//	}

	// Now hit each surface in the LODs
//	for (j = 0; j < Header.LOD_num; j++)
//	{
//		MDS_LOD &lod = LODs[j];
//		for (k = 0; k < lod.Header.Surface_num; k++)
//		{
//			cur += LODs[j].Surface(k).Parse(fp);
//		}
//	}
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


void MDS::WritetoDisk(const char *file)
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
	UINT32	nextchunk = sizeof(MDS_Header);
	cur = fwrite((void *)&Header,1,nextchunk,fp);
	
	// Now do Frames
	nextchunk = sizeof(stMDS_Frame) - sizeof(stMDS_Bone);
	int j = 0;
	for (j = 0; j < Header.Frame_num; j++)
	{	// just tag it so we can see where we are
//		sprintf(Frames[j].Name,"Frame: %d",j);

		// Write out the main part
		cur += fwrite(&Frames[j],1,nextchunk,fp);

		/// Then all the bones
		for (int k = 0; k < Header.Bone_num; k++)
		{
			MDS_BoneFrame& bon = Frames[j].Bone(k);
			cur += fwrite(&bon,1,sizeof(stMDS_Bone),fp);
		}
	}
	// cur should equal  Header.LOD_Start

	// Now do LOD - Create and write them out
	nextchunk = sizeof(stMDS_LOD);
//	for (j = 0; j < Header.LOD_num; j++)
//	{	// Read in the main part
//		cur += fwrite(&(LODs[j].Header),1,sizeof(stMDS_LOD),fp);
//	}

	// Now hit each surface in the LODs
//	for (j = 0; j < Header.LOD_num; j++)
//	{
//		for (int k = 0; k < LODs[j].Header.Surface_num; k++)
//		{
//			cur += LODs[j].Surface(k).WritetoDisk(fp);
//		}
//	}

	fclose(fp);
	fp = 0;
}

