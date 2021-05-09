/**************************************
 *
 *  parsemd3.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Import/Export object for id software's MD3 file format
 *
 *  MD3 file format property of id software
 *
 **************************************/

#include <stdio.h>
#include <math.h>
#include "string.h"
#include "ParseMD3.h"

static char	tmp[5];
static char *id_ID = "IDP3";

Matrix<float>	LW_TO_MD3_Coords(Vector<float>(0,-1,0),
								 Vector<float>(0,0,1),
								 Vector<float>(1,0,0));

// Static values
MD3_Tag			MD3_Tag::INVALID  =			MD3_Tag();
MD3_Frame		MD3_Frame::INVALID=			MD3_Frame();
MD3_Skin		MD3_Skin::INVALID =			MD3_Skin();
MD3_Poly		MD3_Poly::INVALID =			MD3_Poly();
MD3_Point_Frame	MD3_Point_Frame::INVALID =	MD3_Point_Frame();
MD3_FramePoints	MD3_FramePoints::INVALID =	MD3_FramePoints();
MD3_FrameTags	MD3_FrameTags::INVALID =	MD3_FrameTags();
MD3_Vert_Skin	MD3_Vert_Skin::INVALID =	MD3_Vert_Skin();
MD3_Mesh		MD3_Mesh::INVALID =			MD3_Mesh();

#define _VEC_ENV_CONV			(255 / 2 / 3.14159)
void MD3_Point_Frame::setNormal(Vector<float> &v)
{	
	normal[0] = (char)(64 - atan(v[2] / sqrt((v[0] * v[0]) + (v[1] * v[1]))) * _VEC_ENV_CONV);
	normal[1] = (char)(atan(v[1] / v[0]) * _VEC_ENV_CONV);
	if (v[0] < 0)
		normal[1] += (char)128;
}
#undef _VEC_ENV_CONV

// These points should be the 'native' object's coordinates, NOT MD3
void Prep_PointsforTag::FillTag(MD3_Tag *t)
{
	double	d;
	Vector<float>	v(pos[2]);
	int i;

	// First point is the anchor point - convert it to the MD3 space
	t->Position = LW_TO_MD3_Coords * v;
//	RotateByMatrix(pos[2],LW_TO_MD3_Coords,t->Position);

//	t->Position[0] = (float)(pos[2][0]);
//	t->Position[1] = (float)(pos[2][1]);
//	t->Position[2] = (float)(pos[2][2]);

// --- FILL MATRIX ---
// 1.  Fill the X direction
	// 2nd point defines the direction of the first vector (front)
	for (i = 0; i < 3; i++)
		v[i] = (float)(pos[0][i] - pos[2][i])/TAG_POLY_LEN;

	// ...Normalize it
	d = sqrt((v[0] * v[0]) + 
			 (v[1] * v[1]) +
			 (v[2] * v[2]) );

	for (i = 0; i < 3; i++)
		v[i] /= (float)d;

	//Then convert that to MD3 coordinate system
	t->Rotation[0] = LW_TO_MD3_Coords * v;
//	RotateByMatrix(v,LW_TO_MD3_Coords,t->Rotation[0]);

// 2.  Fill the Z direction (normal is up anyways ...)
	v[0] = (float)(nrml[0]);
	v[1] = (float)(nrml[1]);
	v[2] = (float)(nrml[2]);

	//Then convert that to MD3 coordinate system
	t->Rotation[2] = LW_TO_MD3_Coords * v;
//	RotateByMatrix(v,LW_TO_MD3_Coords,t->Rotation[2]);

// 3.  Fill the Y direction
	// Find the resulting vector (up X front == left)
	t->Rotation[1] = t->Rotation[2].cross(t->Rotation[0]);
//	CrossProduct(t->Rotation[2], t->Rotation[0], t->Rotation[1]);
}

void Prep_PointsforTag::HackFillTag(MD3_Tag *t)
{
	double	d;

	// First point is the anchor point
	t->Position[0] = (float)(pos[2][0]);
	t->Position[1] = (float)(pos[2][1]);
	t->Position[2] = (float)(pos[2][2]);

	// 2nd point defines the direction of the first vector (front)
	t->Rotation[2][0] = (float)(pos[0][0] - pos[2][0])/TAG_POLY_LEN;
	t->Rotation[2][1] = (float)(pos[0][1] - pos[2][1])/TAG_POLY_LEN;
	t->Rotation[2][2] = (float)(pos[0][2] - pos[2][2])/TAG_POLY_LEN;

	// ...Normalize it
	d = sqrt((t->Rotation[2][0] * t->Rotation[2][0]) + 
			 (t->Rotation[2][1] * t->Rotation[2][1]) +
			 (t->Rotation[2][2] * t->Rotation[2][2]) );

	t->Rotation[2][0] /= (float)d;
	t->Rotation[2][1] /= (float)d;
	t->Rotation[2][2] /= (float)d;
	
	// Store upwards direction (top)
	t->Rotation[1][0] = (float)(nrml[0]);
	t->Rotation[1][1] = (float)(nrml[1]);
	t->Rotation[1][2] = (float)(nrml[2]);

	// Find the resulting vector (right = front X top)
	t->Rotation[1] = t->Rotation[2].cross(t->Rotation[0]);
//	CrossProduct(t->Rotation[1], t->Rotation[2],t->Rotation[0]);
}

// See above note.  Makes life a little more difficult
// --- THIS IS WRONG ---
void Prep_PointsforTag::CalculateNormal()
{
	Vector<double> a, b, c;
	int i = 0;

	a[0] = pos[0][2] - pos[2][2];
	a[1] = pos[0][0] - pos[2][0];
	a[2] = pos[0][1] - pos[2][1];

	b[0] = pos[1][2] - pos[2][2];
	b[1] = pos[1][0] - pos[2][0];
	b[2] = pos[1][1] - pos[2][1];

	c = a.cross(b);

//	CrossProduct(a, b, c);

	double d = sqrt((c[0] * c[0]) + 
					(c[1] * c[1]) +
					(c[2] * c[2]) );

	for (i = 0; i < 3; i ++)
		nrml[i] = c[i] / d;
}

/*efine CHECK_FILE_POS(f,i,x)											\
	if (i != x) {														\
		fprintf(stderr,"--Cursor invalid: %d != expected %d\n",			\
					i, x);												\
		fseek(fp,(long)x,SEEK_SET);  }
*/
// MD3_Mesh functions

MD3_Mesh::MD3_Mesh(const char *name) :
	  Polys(MD3_Poly::INVALID),
	  Skins(MD3_Skin::INVALID),
	  FramePoints(MD3_FramePoints::INVALID),
	  SkinPoints(MD3_Vert_Skin::INVALID)
{
	memset(&Header,0,sizeof(Header));
	memcpy((void *)(&(Header.ID)),(void *)id_ID,4);
	if (name != NULL) {
		strcpy(Header.Name,name);
	}

	//Header.MeshFrame_num = fcount;
	//Header.Triangle_num = pcount;
	//Header.Skin_num = scount;
	//Header.Vertex_num = vcount;
	// fill sizes
}

void MD3_Mesh::UpdateName(const char *c)
{
	strcpy(Header.Name,c);
}

UINT32 MD3_Mesh::UpdateHeaderSizes()
{

	Header.HeaderSize		= sizeof(Header);

	Header.MeshFrame_num	= FrameCount();
	Header.Skin_num			= SkinCount();
	Header.Vertex_num		= PointCount();			//number of vertices
	Header.Triangle_num		= PolyCount();

	Header.Triangle_Start	= Header.HeaderSize		+ (SkinCount()  * sizeof(MD3_Skin));
	Header.TexVec_Start		= Header.Triangle_Start + (PolyCount()  * sizeof(MD3_Poly));
	Header.Vertex_Start		= Header.TexVec_Start	+ (PointCount() * sizeof(MD3_Vert_Skin));
	Header.ChunkSize		= Header.Vertex_Start	+ (PointCount() * FrameCount() * sizeof(MD3_Point_Frame));

	return (Header.ChunkSize);
}

void MD3_Mesh::AddSkin(const char *c)
{
	MD3_Skin s;
	strcpy(s.Name,c);

	Skins[Skins.Next()] = s;
}

int MD3_Mesh::Parse(FILE *fp)
{
	UINT32	count = 0, chunksize = 0;
	int		i = 0, partsdone = 0;
	UINT32	j = 0, k = 0;

	chunksize = sizeof(MD3_MeshHeader);
	i = fread((void *)&Header,chunksize,1,fp);
	count += chunksize;

	while (count < Header.ChunkSize)
	{
		if (count == Header.Triangle_Start)
		{	// Read in Polygon data
			chunksize = Header.Triangle_num;
			for (j = 0; j < chunksize; j++) {
				i = fread(&Polys[j] ,sizeof(MD3_Poly),1,fp);
			}
			count += (chunksize * sizeof(MD3_Poly));
		}
		else if (count == Header.TexVec_Start)
		{	// Read in skin verts - this is ONE big chunk
			chunksize = Header.Vertex_num;
			for (j = 0; j < chunksize; j++) {
				i = fread(&SkinPoints[j] ,sizeof(MD3_Vert_Skin),1,fp);
			}
			count += (chunksize * sizeof(MD3_Point_Frame));
		}
		else if (count == Header.Vertex_Start)
		{	// Read in frame data
			// - This is really [FrameCount()][PointCount()]
			chunksize = Header.Vertex_num * Header.MeshFrame_num;
			for (j = 0; j < Header.MeshFrame_num; j++) {
				for (k = 0; k < Header.Vertex_num; k++) {
					i = fread(&FramePoints[j][k],sizeof(MD3_Point_Frame),1,fp);
				}
			}
			count += (chunksize * sizeof(MD3_Point_Frame));
		}
		else
		{	// Read in skin data
			chunksize = Header.Skin_num; 
			if (chunksize > 0)
			{
				for (j = 0; j < chunksize; j++) {
					i = fread(&Skins[j] ,sizeof(MD3_Skin),1,fp);
				}
				count += (chunksize * sizeof(MD3_Skin));
			}

		}
		if (++partsdone > 5)
			break;
	}
	return count;
}

void MD3_Mesh::WritetoDisk(FILE *fp)
{
	int		i = 0;
	UINT32	j = 0, k = 0;

	// Write out header
	UINT32 	chunksize = sizeof(MD3_MeshHeader);
	i = fwrite((void *)&Header,1,chunksize,fp);

	// Write out skin data
	chunksize = SkinCount(); 
	for (j =0; j < chunksize; j++) {
		i = fwrite(&Skins[j],sizeof(MD3_Skin),1,fp);
	}

	// Write out polys
	chunksize = PolyCount();
	for (j =0; j < chunksize; j++) {
		i = fwrite(&Polys[j],sizeof(MD3_Poly),1,fp);
	}

	// Write out skin verts
	chunksize = SkinPointCount();
	for (j = 0; j < chunksize; j++) {
		i = fwrite(&SkinPoints[j],sizeof(MD3_Vert_Skin),1,fp);
	}

	// Write out frame data
	chunksize = PointCount() * FrameCount();
	for (j =0; j < FrameCount(); j++) {
		for (k =0; k < PointCount(); k++) {
			i = fwrite(&FramePoints[j][k],sizeof(MD3_Point_Frame),1,fp);
		}
	}
}

// MD3 functions
UINT32 MD3::PointCount()
{
	int		i = 0;
	UINT32	pCnt = 0;

	for (i = MeshCount() -1;i >= 0;i--)
		pCnt += Meshes[i].PointCount();

	return pCnt;
}

UINT32 MD3::PolyCount()
{
	int		i = 0;
	UINT32	pCnt = 0;

	for (i = MeshCount() -1;i >= 0;i--)
		pCnt += Meshes[i].PolyCount();

	return pCnt;
}

char *MD3::TypeName(char *buf)
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

int MD3::isLoaded()
{
	return (strcmp(TypeName(tmp),id_ID) == 0);
}

void MD3::UpdateName(const char *c)
{
	strcpy(Header.Name,c);
	strcpy(FileName,c);
}

UINT32 MD3::UpdateHeaderSizes()
{
	Header.Frame_Start	= sizeof(Header);

	Header.Frame_num = FrameCount();
	Header.Tag_num = TagCount();
	Header.Mesh_num = MeshCount();
	Header.MaxSkin_num	= 0;

	Header.Tag_Start	= Header.Frame_Start	+ (FrameCount() * sizeof(MD3_Frame));
	Header.Mesh_Start	= Header.Tag_Start		+ (TagCount() * FrameCount() * sizeof(MD3_Tag));
	Header.FileSize		= Header.Mesh_Start;

	UINT32 i = 0;
	for (i = 0; i < MeshCount(); i ++)
	{
		Header.FileSize += Mesh(i).UpdateHeaderSizes();
	}

	return (Header.FileSize);
}

MD3::MD3():
		Frames(MD3_Frame::INVALID),
		Meshes(MD3_Mesh::INVALID),
		Tags(MD3_FrameTags::INVALID)
{
	FileName[0] = 0;
	memset(&Header,0,sizeof(Header));

	memcpy((void *)(&(Header.ID)),(void *)id_ID,4);
	Header.Version  = 15;
}

MD3::MD3(const char *filename):
		Frames(MD3_Frame::INVALID),
		Meshes(MD3_Mesh::INVALID),
		Tags(MD3_FrameTags::INVALID)
{
	strcpy((char *)FileName,filename);

	memset(&Header,0,sizeof(Header));

	UINT32	cur = 0, nextchunk = 0;
	UINT32	j = 0, k = 0;
	int		i = 0;

	if ((fp = fopen(filename,"rb")) == (FILE *)NULL)
	{
		fprintf(stderr,"Can't open existing '%s'\n",filename);
		goto RETURN;
	}

	// Read in header
	nextchunk = sizeof(MD3_Header);
	i = fread((void *)&Header,nextchunk,1,fp);
	if (!isLoaded())
		goto RETURN;

	// Now do Frames
	nextchunk = Header.Frame_num;
	for (j = 0; j < nextchunk; j++) {
		i = fread(&Frames[j],sizeof(MD3_Frame),1,fp);
	}

	// Now do Tags
	nextchunk = Header.Frame_num * Header.Tag_num;
	if (nextchunk > 0)
	{
		for (j = 0; j < Header.Frame_num; j++) {
			for (k = 0; k < Header.Tag_num; k++) {
				i = fread(&Tags[j][k] ,sizeof(MD3_Tag),1,fp);
			}
		}
	}
	cur = Header.Mesh_Start;

	// Now do Meshes
	nextchunk = Header.Mesh_num;
	for (j = 0; j < nextchunk; j++) {
		cur += Meshes[j].Parse(fp);
	}

//	fprintf(stderr,"Read in %d - expected %d\n",cur,Header.FileSize);

	UpdateHeaderSizes();

	// Don't worry about the name ...
/*
	if (Header.Name[0] == 0)
	{
		for (j = strlen(filename) -2; j > 0; j--) {
			if (filename[j] == '/' || filename[j] == '\\') {
				++j;
				break;
			}
		}
		strcpy(Header.Name,filename + j);
	}
*/
RETURN:
	if (fp) fclose(fp);
	fp = 0;
};

void MD3::WritetoDisk(const char *file)
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

	setvbuf( fp, NULL, _IOFBF, 0x8000 ) ;

	// Make sure all the meshes have at least 1 skin
	UINT32	j = 0, k = 0;
	for (j = 0; j < MeshCount(); j++)
	{
		if (Meshes[j].SkinCount() == 0)
		{
			MD3_Skin& sk = Meshes[j].Skin(0);
			sk.Name[0] = 0;
		}
	}

	// Find the frame extent values
	for (unsigned int fridx = 0; fridx < Mesh(0).FrameCount(); fridx++)
	{
		double	mins[3] = {999,999,999};
		double	maxs[3] = {-999,-999,-999};

		for (unsigned int m = 0; m < MeshCount(); m++)
		{
			MD3_Mesh& mesh = Mesh(m);

			for (unsigned int v = 0; v < PointCount(); v++)
			{
				MD3_Point_Frame &vert = mesh.PointsAtFrame(fridx)[v];
				for (int d = 0; d < 3; d ++)
				{
					if (vert.v[d] < mins[d])
						mins[d] = vert.v[d];
					if (vert.v[d] > maxs[d])
						maxs[d] = vert.v[d];
				}
			}
		}

		// Store frame data
		MD3_Frame&	frm = Frame(fridx);

		frm.Radius = 0;
		for (int j = 0; j < 3; j++)
		{
			frm.Maxs[j] = maxs[j] / 64;
			frm.Mins[j] = mins[j] / 64;
			frm.Position[j] = 0;

			// Accumulate the Radius value
			if (frm.Maxs[j] > (-frm.Mins[j]))
			{
				frm.Radius += (frm.Maxs[j] * frm.Maxs[j]);
			}
			else
			{
				frm.Radius += (frm.Mins[j] * frm.Mins[j]);
			}
		}
		frm.Radius = sqrt(frm.Radius);
	}

	// Just recheck all the header values
	UpdateHeaderSizes();

	// Write out header
	int		i = 0;
	UINT32	nextchunk = sizeof(MD3_Header);
	i = fwrite((void *)&Header,1,nextchunk,fp);

	// Now do Frames
	nextchunk = FrameCount();
	for (j = 0; j < nextchunk; j++ ){
		i = fwrite(&Frames[j],sizeof(MD3_Frame),1,fp);
	}

	// Now do Tags
	nextchunk = FrameCount() * TagCount();
	if (nextchunk > 0)
	{
		for (j = 0; j < FrameCount(); j++) {
			for (k = 0; k < TagCount(); k++) {
				i = fwrite(&Tags[j][k] ,sizeof(MD3_Tag),1,fp);
			}
		}
	}

	// Now do Meshes
	nextchunk = MeshCount();
	for (j = 0; j < nextchunk; j++)
		Meshes[j].WritetoDisk(fp);

	fclose(fp);
	fp = 0;
}

