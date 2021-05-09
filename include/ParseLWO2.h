/**************************************
 *
 *  parselwo2.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Wrapper of NewTek's LWO2 (LW 6+) file format
 *
 *  LWO2 file format property of NewTek, Inc.
 *
 **************************************/


#ifndef  _PARSE_LWO2_H_
#define  _PARSE_LWO2_H_

#include "sys_math.h"
#include "ParseLWOcommon.h"

namespace LW3D
{
	static const char *LW_CHNK_FACE	= "FACE";

	UINT32 ParseVIndex(UINT16 **p);

	class  LWOv2 : public LWO
	{
		const char *fileID()
		{
			return "LWO2";
		}
		const char *fileType()
		{
			return "Lightwave Object [v2]";
		}

		void ParseVerts(IFF::Chunk& b);
		void ParsePolys(IFF::Chunk& b);
		void ParseVmap(IFF::Chunk& b);
		void ParseSurf(IFF::Chunk& b);
		void ParseTag(IFF::Chunk& b);
		void ParsePTag(IFF::Chunk& b);
		void ParsePVmap(IFF::Chunk& b);

	public:
		LWOv2(const char *filename);

		UV& VertexUVmap(UINT32 vertidx);

		void ProcessNextChunkfromFile(IFF::Chunk& b);
	};
}

#endif // _PARSE_LWO2_H_
