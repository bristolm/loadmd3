#ifndef _MDSSCRATCH_H
#define _MDSSCRATCH_H

#include "lwio.h"		// load, save
#include "lwrender.h"	// For Object Info, Scene Info

extern "C"
{
#include "LayoutExportMD2.h"
}

#include "sys_math.h"
#include "ParseMD2.h"

// Object for handling the loop 'extra' data
// Structs for this plugin to use

class ModelScratchPad;

// Model construction holding area
class ModelScratchPad
{
public:
	LWXPanelID				XPanelID;

	char					QuakeDir[256];
	char					ModelDir[256];

	char					ModelName[256];
	int						SaveModel;
	int						SkinWidth, SkinHeight;

	char					HeaderFile[1024];
	int						SaveHeader;

	MD2						Model;
	LC_FrameLoop			**Loops;

	int						ActiveLoop;

	ModelScratchPad(LC_FrameLoop **loops):
		XPanelID(0),
		SaveModel(0),SkinWidth(256),SkinHeight(256),
		SaveHeader(0),
		Model(),
		Loops(loops),
		ActiveLoop(-1)
	{
		HeaderFile[0] = ModelName[0] = QuakeDir[0] = ModelDir[0] = 0;
	}

	ModelScratchPad& operator=( const ModelScratchPad& rhs);

	void dumpHeaderFile();

	void SAVE(const LWSaveState *save);
	int LOAD(const LWLoadState *load);
};

#endif //_MDSSCRATCH_H