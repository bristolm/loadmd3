#include "lwobjimp.h"
#include "lwmeshes.h"
#include "lwpanel.h"
#include "lwhost.h"
#include "lwgeneric.h"

#include "lw_base.h"

#define PLUGINNAME_LOAD		"MRB::Import::MD3"
#define PLUGINNAME_MESH		"MRB::MeshEdit::MD3"

#define MY_VERSION			1

// Global variables
extern GlobalFunc		*LW_globalFuncs;
extern void				*LocalFuncs;
extern LWMessageFuncs	*LW_messageFuncs;
extern LWCommandFunc	LW_cmdFunc;

typedef struct
{	// User input values
	int				FrameForImport;
	int				AnchorTagIndex;
	int				ModelType;
	char			AnimCFG[1024];
} GUIData;

// Object Import function
extern int LW56_LoadinMD3();
extern int LW60_LoadinMD3();

// Meshedit function
extern int LW56_ImportMD3();
extern int LW60_ImportMD3();

// Basic name checker
int IsMD3NameOK(const char *name);

