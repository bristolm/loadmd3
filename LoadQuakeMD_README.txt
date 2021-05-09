MRB_QuakeMD_FileLoader

v0.5.02
Nov 07, 2000

Quake MD3 File Rebuilding utility for Lightwave

Please get in touch with me before redistributing this or posting it somewhere.

I'm going to assume that the MD3 format is copyright id software.

Latest versions of this application can be found at:
	http://users.tp.net/mbristol/LW_Md3Load.zip

Current issues:
Other than being perpetually 'in progress', things seem OK 

== History ==
0.1:  	First release.  ObjectImport loading only
0.2:  	Added MeshEdit (import) plugin
0.3:  	Added SkinMesh construction functionality
0.4:  	LW 6.0 compatilibity. LW 6.0 UV map creation. Models err... flipped properly [oops]
0.5:  	Version bump to reflect internal changes
0.5.01:	Changes for environment smoothing in the companion LWOtoMD3 convertor
0.5.02: Added scaling value to command line on companion LWOtoMD3 convertor

== Description == 

Intel - 2 Plugins:
- ObjectLoader Plug-in for Lightwave 3D
- MeshDataEdit Plug-in for Lightwave 3D - "MRB_QuakeMD_Import"

This has been tested with v5.6 but it might work on older versions too, I have not tried it
6.0 UV maps are created by default upon loading or importing.  The names are taken
from the mesh names.

== Instructions ==

There are 2 plug-ins in the included file.  People that already registered the v0.1 with
LightWave need to do it again to pick up the new one.

1:  An ObjectLoader plug-in that allows you to load .md3 files into LightWave just like
    you would a .lwo file.
	- File->Load Object
	- Change "Files of Type" to "All Types" and find the file

2:  A MeshDataEdit plug-in that can be run interactively
	- Select a polygon (one and only one polygon!) to anchor the new piece to - if you choose
	  one of the tag_ surfaces that this plug-in created, the alignment will be OK - if you 
	  choose to select another random poly, there are issues (see below)
	- Tools->Custom->MRB_QuakeMD_Import
	- Find the file to import (Path should be maintained from earlier .md3 Object loads)

The selector that pops up is identical for both plug-ins.
  "Frame" represents the frame you want to reconstruct
  "Anchor With" allows you to choose the sub-mesh which will define the orientation of
    the imported model.  When using the MeshDataEdit plug-in portion, this will be locked
    to the polygon that's selected in layout.  "None" just loads the sub-mesh in raw.
  "Build SkinMesh"
    Clicking that this button while importing a .MD3 file will construct (within
    modeler) a representation of the Quake3 model's internal skin mesh.  This mesh is 
    built on the XY plane at Z=-100 just to keep it away from the rest of the model's 
    mesh.  The 0.0 -> 1.0 UV values are converted to a -16 -> +16 range centered 
    around the 0,0,0 origin.  See below regarding what this is useful for.
  Press OK to accept, Cancel to ... well, cancel.

This loads the .md3 into modeller and labels each internal mesh and tag with
an appropriate surface name.

Once the model is loaded, you can use the Polygon Statistics window ('w') to
select portions of the model by mesh or tag name (i.e. by surface name).

LW 6.0 will make the model as one surface (based on the model name) and then
generate UV texture map levels for each surface name.  DO NOT MERGE POINTS in
6.0 - because the skinmaps will be corrupted.

== Caveat User ==

Models w/o tags structures may still be a problem - I'm not sure, but I'm just not
sure how far I trust it.  

This program is still pretty much proof-of-concept!  I hacked out a quick 
routine to parse the .md3 file and load it into modeller so I could check 
id's weapon models against my Player Model prior to compiling it into a .md3
That part works - and I thought others might find a use for it too.  

Thanks to those studs at Mental Vortex [http://q3arena.net/mentalvortex/md3view/] 
for posting a .md3 file format.  If anyone knows anything that is wrong with theirs
and would kindly like to fwd me a correct one, that would be great.  I've had no 
problems with it yet though.

I'll release the source to this once I clean it up.  It is still really embarrassing
right now since this is more of a 'overuse C++ so I get use to the syntax' project
rather than a real killer 

I'm still not sure why the file is so big.  Seems to me it's bloated somehow ...

Lightwave 5.6 Layout doesn't handle ObjectImport plug-ins, so it can't load 
.md3 files this directly.  Bummer!

== How to ... ==

ANCHOR OBJECTS TO RANDOM POLYS

Selecting a random polygon and loading in the sub-mesh will work fine, but the orientation
might not be what you expect.  Orientation of loaded portions is done based on the order
of the points in the polygon that is selected.  I'm not sure how to tell the point order of
a polygon already in Modeler, but what you can do is just re-create that poly so you
    are sure of the order, and then import the sub-mesh over it.  To get things oriented 
1     properly, you need to choose the points in the order shown at the left, with '3' being
|\     the last point chosen and also point the sub-mesh will attach to.  '1' is forward and
| \    '2' is right.  Pick them in the order 1-2-3 and press 'p' to make a new poly.  Make sure
3--2   it is selected, and then Import the new sub-mesh.

The Q3_tag.lwo file that's included with this package has been built to work as an import 
Anchor.  Load it into your other lightwave objects to allow the MeshEdit plug-in to work
as intended.

WRAP QUAKE3 TEXTURES AROUND MODELS IN LIGHTWAVE 5.6

Now, it's very imporant here not to start screwing around with the geometry other than
deleting the tag polys and maybe moving some points around.  Do not weld, merge, or
delete any points.  Do not Unify, flip, or re-surface any polygons.  Do not cut or
paste any polygons unless you effecting ALL the SkinMesh polygons, or all the
non-SkinMesh polygons.  This is extremely important - doing anything too disruptive
will screw up the point order and you'll end out with a terrible mess later inside 
Layout.

In Modeler:

1: Load and assemble the all the MD3 files that you want to wrap into Lightwave 
- be sure to check the "Build SkinMesh" button each time.

2: Use the polygon selecter ('w' key) to delete all the polygons that represent 
object tags - they will be prefixed with _tag

3: Select the SkinMesh polygons (see SkinMesh.jpg - they will be in a single plane 
at Z=-100). 'Cut'these polyons out and save the rest out as a new .lwo file.  
This will be referred to as the SKINMESH object.

4: 'Undo' to get the cut polys back, press " to swap the selected polys.

5: Then 'Cut' these and save the rest out as a new file.  This will be referred
to as the MODELMESH object.

At this point we have 2 files - one has the polys from the SKINMESH in it and the
other has the polys for the MODELMESH in it.  The idea now is that we load both 
into layout, and by using morph targets, we can apply the textures to the SKINMESH 
object and 'fold' that textured object into the shape of the MODELMESH (provided 
we didn't screw up the point order in creating the 2 separate objects).

In Layout:

1: Load the SKINMESH object and the MODELMESH into Layout.

2: On the Objects Panel's Deformations tab, set the MODELMESH as the Morph target 
of the SKINMESH.  Set the Morph Amount to 100%

3: For the MODELMESH object, on the Objects Panel underneath the Appearance Options
tab, set the Object Dissolve to 100%

4: On the Images tab, load in your texture images from your Quake3 area.  You're
going to have to unzip the model's subdirectory from the .pk3 file to get at them.

5: Now we need to tie the textures to the surfaces.  For each surface, add the
appropriate images as 32x32 Planer images mapped onto the Z plane.  This option is
found by pressing the 'T' next to the Surface color, picking 'Planar Image Map' from
the Texture type, and then the picture in the 'Texture Image' box.  (See final.jpg)

Now you're done.  In OpenGL texture preview mode, the results are quite striking
(provided the SKINMESH object is the currently selected object).  For better results,
you may want to up the Ambient light to 100% and set the diffuse level of each surface
to 100%.  There will be seams in places because of the way the texture is broken up.
A true shader would fix this, but I don't really have time for that now ... maybe later.


== Fixes in v0.5.02 ==

 + Bounding box is accurately calculated
 + Added scal parameter on convertor
 + Cleaned up creator string for each frame

== Fixes in v0.5.01 ==

 + Added implicit environmental smoothing for the companion compiler

== Fixes in v0.5 ==

 + None that I know of

== Fixes in v0.4 ==

 + Models are imported properly now - they used to be flipped.
 + LW 6.0 UV mapping is exploited properly.

== Fixes in v0.3 ==

 + MeshEdit (import) now works regardless of the active foreground layer
 + Some code restructuring so I can re-use the objects in other applications in progress
 + Added the SkinMesh creation functionality

== Fixes in v0.2 ==

 + Tags are seeded with a Yellow surface, other polys with a Gray one if one doesn't already
   exist so Messiah should no longer crash upon loading the objects.
 + Custom player models now load - id and Npherno seem to be using a differnt order for the 
   sub-lumps in the .MD3 file !
 + File with a .MD3 extension now loads along with .md3  (oops)
 + Added a MeshDataEdit plug-in as intended

== Planned changes for the future ==

 + Nothing that I can think of - I'm not sure what else it needs to do.
   I have been tinkering with a Layout shader plug-in though ...

Hope you find this useful!

Mike
ansai_@hotmail.com