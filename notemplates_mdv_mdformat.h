/*
Copyright (C) Matthew 'pagan' Baranowski & Sander 'FireStorm' van Rossen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/*
file structures data types, these are used only for disk import/exprot
runtime memory structures are in mdv_model.h
*/

#include "sys_base.h"

#ifndef _MDV_MDFORMAT_H_
#define _MDV_MDFORMAT_H_

//#pragma pack(1)     // better pack these structures!

#define IDALIASHEADER2	'2PDI'	// big endian: 'IDP2'
#define IDALIASHEADER3	'3PDI'	// big endian: 'IDP3'
#define ALIAS_VERSION2	8
#define ALIAS_VERSION3	15
#define MAX_MD3SKINNAME 64
#define MAX_MD2SKINNAME 64

// ------------------------- md3 file structures -------------------------------

typedef struct dMD3Header
{
	UINT32	ID;					//id of file, always "IDP3"
	UINT32	Version;			//i suspect this is a version number, always 15
	char	FileName[68];		//sometimes left Blank...always 64 chars
	UINT32	Frame_num;			//number of frames
	UINT32	Tag_num;			//number of 'tags'
	UINT32	Mesh_num;			//number of meshes/skins 
	UINT32	MaxSkin_num;		//unknown, always zero

	UINT32	Frame_Start;		//always equal to the length of the header
	UINT32	Tag_Start;			//starting position of tag-structures
	UINT32	Mesh_Start;			//ending position of tag-structures/starting 
											//position of mesh-structures

	UINT32	FileSize;			//size of file
} MD3_Header;

typedef struct dMD3Frame
{
	float	Mins[3];			//unknown vector, minimum size? always negative
	float	Maxs[3];			//unknown vector, probably maximum size? always positive
	float	Position[3];		//new position
	float	Scale;				//scale by this ... ?
	char	Creator[16];		//i think this is the "creator" name..						
} MD3_Frame;

/*
 * There are (Frame_num * Tag_num) tag structures.
 * for each (Frame_num)
 *	for each (Tag_num)
 *		MD3_Tag;
 */

typedef struct dMD3Tag
{
	char	Name[12];			//name of 'tag' as it's usually called in the md3 files
								//try to see it as a sub-mesh/seperate mesh-part
	char	unknown1[52];
	float	Position[3];		//relative position of tag
	float	Matrix[3][3];		//3x3 rotation matrix
} MD3_Tag;

/* Each Mesh chunk signifies the beginning of a set of structures like:
 *   Mesh chunk header
 *   for each (skin_num)
 *		MD3_Point			- name or something
 *   for each (Triangle_num)
 *		MD3_Poly			- frame triangles
 *	 for each (Vertex_num)
 *		MD3_SkinVertes		- skin vertices
 *	 for each (MeshFrame_num)
 *		for each (Vertex_num)
 *			MD3_FrameVertex		- distorted position ?
 */
typedef struct dMD3MeshChunk
{
	UINT32	ID;					//id, must be IDP3
	char	Name[68];			//name of mesh

	UINT32	MeshFrame_num;		//frames, variable
	UINT32	Skin_num;			//unknown, always 1 ...maybe "has skin" flag?
	UINT32	Vertex_num;			//number of vertices
	UINT32	Triangle_num;		//number of Triangles

	UINT32	Triangle_Start;		//starting position of Triangle data, relative to start of meshchunk
	UINT32	HeaderSize;			//size of header
	UINT32	TexVec_Start;		//starting position of texvector data, relative to start of meshchunk
	UINT32	Vertex_Start;		//starting position of vertex data, relative to start of meshchunk
	UINT32	ChunkSize;			//size of mesh
} MD3_MeshHeader;

typedef struct dMD3Skin
{
	char	Name[MAX_MD3SKINNAME];	//name of skin used by mesh
	int		fillup;
} MD3_Skin;

typedef struct dMD3Triangle
{
	UINT32	vind[3];    // triangle vertices
} MD3_Poly;

/* The simple fact here is that vertices are duplicated if the skin 
 * has been split into several pieces in one mesh.  This is unlike the 
 * MD2 format where a triangle has separate Texture and vertices indexes.
 */
typedef struct dMD3TexVec
{
	float	tex[2];	// texture coordinates
} MD3_Vert_Skin;

typedef struct dMD3Vertex
{
	INT16  v[3];						//vertex coordinate
	INT8   normal[2];					//environment mapping normal
} MD3_Vert_Frame;

// -------------------------- md2 file structures ---------------------------------------

typedef struct dMD2Header
{
	UINT32	ID;
	UINT32	Version;

	UINT32	Skin_width;
	UINT32	Skin_height;
	UINT32	Frame_size;		// byte size of each frame

	UINT32	Skin_num;
	UINT32	Vertex_num;
	UINT32	TexVec_num;		// greater than num_xyz for seams
	UINT32	Triangle_num;
	UINT32	Strips_num;		// dwords in strip/fan command list
	UINT32	Frame_num;

	UINT32	Skin_Start;		// each skin is a MAX_SKINNAME string
	UINT32	TexVec_Start;	// byte offset from start for stverts
	UINT32	Triangle_Start;	// offset for dtriangles
	UINT32	Frame_Start;	// offset for first frame
	UINT32	Strips_Start;	
	UINT32	FileSize;		// end of file
} MD2_Header;

// Each frame constists of:
//    MD3_Frame structure
//    MD2_Point_Frame structure * 

typedef struct dMD2Frame
{
	float	Scale[3];			//multiply byte verts by this
	float	Translate[3];		//then add this
	char	Creator[16];		//frame name from grabbing
} MD2_Frame;

typedef struct dMD2Vertex
{
	INT8	v[3];
	INT8	lightnormalindex;
} MD2_Vert_Frame;

typedef struct dMD2Skin
{
	char	Name[MAX_MD2SKINNAME];	//name of skin used by mesh
} MD2_Skin;

typedef struct dMD2Triangle
{
	UINT16	vertex[3];
	UINT16	texvec[3];
} MD2_Poly;

typedef struct dMD2TexVec
{
	INT16	tex[2];
} MD2_Vert_Skin;

typedef UINT32 MD2_StripItem ;

#endif // _MDV_MDFORMAT_H_