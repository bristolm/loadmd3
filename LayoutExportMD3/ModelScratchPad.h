#ifndef _MDSSCRATCH_H
#define _MDSSCRATCH_H

#include "lwio.h"		// load, save
#include "lwrender.h"	// For Object Info, Scene Info

extern "C"
{
#include "LayoutExportMD3.h"
}

#include "sys_math.h"
#include "ParseMD3.h"

// Object for handling the loop 'extra' data
// Structs for this plugin to use

class ModelScratchPad;

class MD3Loop
{
	friend ModelScratchPad;

public:
	int		index;

	int		LoopingFrames;
	int		FPS;
	char	Notes[256];

	MD3Loop(int idx):
		index(idx),
		LoopingFrames(0)
	{
		Notes[0] = 0;
		// get FPS from the system for default
		LWSceneInfo *sceneinfo =
			(LWSceneInfo *)LW_globalFuncs( LWSCENEINFO_GLOBAL, GFUSE_TRANSIENT );

		FPS = (int)sceneinfo->framesPerSecond;
	}
};

extern AutoArray<MD3Loop *>			SpecificLoopData;

// Model construction holding area
class ModelScratchPad
{
public:
	LWXPanelID				XPanelID;

	char					QuakeDir[256];
	char					ModelDir[256];

	char					Name[256];
	int						SaveModel;
	char					AnchorName[256];
	int						AnchorTagIdx;

	char					CfgName[256];
	int						SaveCfg;
	char					HeaderFile[256];

	MD3						Model;
	LC_FrameLoop			**Loops;

	int						ActiveLoop;

	ModelScratchPad(LC_FrameLoop **loops):
		XPanelID(0),
		SaveModel(0),AnchorTagIdx(-1),SaveCfg(0),
		Model(),
		Loops(loops),
		ActiveLoop(-1)
	{
		Name[0] = QuakeDir[0] = ModelDir[0] = AnchorName[0] = 0;
		HeaderFile[0] = 0;
		sprintf(CfgName,"animation.cfg");
	}

	ModelScratchPad& operator=( const ModelScratchPad& rhs);
	void dumpCfgFile();

	void SAVE(const LWSaveState *save);
	int LOAD(const LWLoadState *load);
};

#endif //_MDSSCRATCH_H