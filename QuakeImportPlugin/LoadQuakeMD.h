typedef struct
{	// Global variables
	void			*Globals;
	void			*Funcs;
	void			*Message;
	LWPanelFuncs	*Panel;

	// User input values
	int				FrameForImport;
	int				AnchorTagIndex;
	int				LoadSkinVerts;
	float			ScaleForSkinMesh;
} BuildData;

extern BuildData	*CurrentData;

// Object Import function
extern int LW56_LoadinMD3(BuildData *myData);
extern int LW60_LoadinMD3(BuildData *myData);

// Meshedit function
extern int LW56_ImportMD3(BuildData *myData);
extern int LW60_ImportMD3(BuildData *myData);

// Basic name checker
int IsMD3NameOK(const char *name);

