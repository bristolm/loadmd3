/**************************************
 *
 *  id_md2.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Wrapper of id software's MD2 file format
 *
 *  MD2 file format property of id software
 *
 **************************************/

#include "sys_math.h"
#include "sys_extra.h"

#ifndef _ID_MD2_H_
#define _ID_MD2_H_

#define ID_HEADER_MD2	'2PDI'	// big endian: 'IDP2'
#define MAX_MD2SKINNAME 64
#define MAX_MD2TRIANGLES  4096

typedef struct
{
	UINT32	ID;				// ID_HEADER_MD2
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
} MD2Header;

// Each frame constists of:
//    MD3_Frame structure
//    MD2_Point_Frame structure * 
typedef struct
{
	float	Scale[3];		//multiply byte verts by this
	float	Translate[3];	//then add this
	char	Creator[16];	//frame name from grabbing
} MD2Frame;

typedef struct
{
	UINT8	v[3];
	UINT8	lightnormalindex;
} MD2Vert_Frame;
typedef MD2Vert_Frame	MD2Point_Frame;

typedef struct
{
	char	Name[MAX_MD2SKINNAME];	//name of skin used by mesh
} MD2Skin;

typedef struct
{
	UINT16	vertex[3];
	UINT16	texvec[3];
} MD2Poly;

typedef struct
{
	INT16	tex[2];
} MD2Vert_Skin;

typedef MD2Vert_Skin	MD2Point_Skin;

typedef UINT32 MD2StripItem ;

#endif // _ID_MD2_H_

