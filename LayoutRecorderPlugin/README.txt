=====================
What this is
===============

My attempt to create a little reusable framework for object exporters.
Also an exercise in creating something that interfaces with a known entity
(in this case - lightwave)


=====================
The theory
================
A central 'hub' plugin (LayoutRecorder) takes data from Layout and caches it
and massages it during a preview playback.  Once the playbak is done. that data
is handed to a number of listening 'Module' plugins to create 3rd party objects.

This really isn't doing anything too complicated, aside from taking the
data coming from lightwave and prettifying it a bit.  Polygons and Vertices
are presented as base-0 index arrays instead of unique memory pointer IDs
=====================

=====================
LayoutRecorder
=====================
LayoutRecorder is the brains of the whole operation.  It coordinates and 
filters any data, and packages it for delivery to the module plugins.

This takes care of the basic FrameLoops (but allows a given module to 
Each object that needs to be listened to gets an ObjectWrapper object - 
this sort of collects all the information about this object during rendering.

The LayoutRecorder takes care of the middle of the processing loop, while the
module plugins provide an initial setup link (Displacement Plugin) and
also an export path (LWMRBCallbackType functions). 
=====================

=====================
Communication between plugins (lwmrbxprt.h file)
=====================
A lot of communication goes on between the LayoutRecorder plugin and it's modules.
This is done primarily through Global Plugins.  Global Plugins allow a plugin
to link a String (in the case of LayoutRecorder - defined by LWMRBEXPORT_GLOBAL)
to a static memory structure (in this case, a LWMRBExportFuncs* struct).

Each module must have a similar Global Function available.  The name of this is
fed to LayoutRecorder by the module at runtime by providing it with a pointer to a 
LWMRBExportType struct.
=====================

=====================
What Happens during setup
=====================
1)  User chooses a Module's Displacement plugin.

2)  Activate:  When Layout calls the module's Activate function, the Module stores a
	pointer to its LWMRBExportType structure in the ->inst->priv member in its 
	LWDisplacementHandler instance.  It then hands the activation off to 
	LayoutRecorder via  _get_activation (in Global_Funcs.cpp)

3)  Interface:  When Layout requests the interface function from the Module, 
	that Module passes the request to LayoutRecorder via the global fuction 
	_get_interface (in Global_Funcs.cpp)   LayoutRecorder sets up an
	XPanel for the Dispacement function, and seeds a DisplacementXPanelInstance
	with the LWMRBExportType provided above.  This links an object in LayoutRecorder
	with a Module's LWMRBExportType structure.

4)  When the User chooses a model (or a new model) from the Displacement plugin's
	XPanel, the DisplacementXPanelInstance serviceng that either takes a current
	ExportModelWrapper or creates a new one.
	Lets say it creates a new one.

5)  The new ExportModelWrapper will represent a single target Export model of
	a type defined by the Module that started all this.  That ExportModelWrapper
	is handed the LWMRBExportType structure (from the Module) and uses the 
	->globalcallback(...) member to get the static set of Generic functions for that 
	Module.  It calls in using the ->newxpanel(...) function and asks the module
	to setup some sort of value that can be used to tag that instance.  This also
	Returns the XPanelID of th embedded XPanel that will be used to define loops
	specific to that plugin.  (loopactivity(...) on the loopactivity function)

6)  ExportModelWrapper turns around and makes sure there is an ObjectWrapper defined 
	for the LightWave model in question.  Any plugin wanting motion (intead of 
	displacement) creates the requisite plugin instances.
=====================

=====================
What Happens during export
=====================
1)	Each ObjectWrapper fills itself out when the export is initiated.

2)	At each frame, Layout pushes deformations and motion changes into the
	ListenChannels assidned to those objects/bones.  That data is cached
	during the run.

3)	At completion of the preview, each ExportModelWrapper grabs a list of
	the Frames it's been asked to monitor and passes them into the Module
	that intiated this export - this begins the Export procedure.  
	the startmodel(...) function provided by the Module returns a pointer
	to the 'in-progress' data

4)	For each Object wrapper, ExportModelWrapper calls into the Module using
	buildmodel(...) - it passes in the pointer returned by startmodel(...) and 
	a means by which the plugin can investigate the data cached for it
	by LayoutRecorder

5)	Upon completion, finishmodel(...) is called - it is expected at this point that
	the Module will spit out the end file.