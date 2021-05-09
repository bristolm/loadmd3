Lightwave -> MD3 file compiler utility.

version 0.05.02
November 07, 2000

Michael Bristol - ansai_@hotmail.com

Thanks to BlazeQ for helping me test this out!

NOTE that the configuration file format is now totally different than
the version 0.04 format

======= IMPORTANT!!! ======= 
  Make backup copies of EVERYTHING you work with.  This will overwrite .MD3 files
  w/o thinking twice about it.

  .. OK, sometimes it will merely update them - but beware!
======= IMPORTANT!!! ======= 

== DESCRIPTION ==
Basically, this takes a series of .lwo object files and builds a .MD3 out of them.
The main idea is that a set of Lightwave object surfaces or UV maps are mapped onto a 
Quake3 mesh.  If the object is a Lightwave v6.0 object file, the UV coordinates are
preserved.

This can transparently handle both Lightwave v5.6 (i.e. LWOB) and v6.0 (i.e. LWOB2) 
object files.

Anything with a detail polygon will probably kill it.  ... and that seam fitter plugin in 
6.0b seems a little scary too.

== EXECUTION==
This is a command-line utility.  Syntax is as follows:
	LWOtoMD3 <Input cfg file> [+u] [+s scale amount]

The parameters are as follows:
	<Input cfg file>	- the name of the configuration file that explains how
				you wish to build your MD3 file.

	[+u]			- optional update flag - if this is set, the application will
				try to load in an existing .MD3 file and augment it.  This is
				useful with Lightwave 5.6 files where only wish to add in some
				extra frames of animation w/o destroying a an existing skinmap

	[+s scale amount]	- value to scale the objects by.  Rather than rerender, you can
				tweak the size of your MD3 from the command line.

The program is run like this;
	LWOtoMD3 configfile.cfg

== CONFIGURATION FILE ==
Basic structure of a configuration file is as follows (note that all these values are typically
not necessary, but they are shown here solely for descriptive purposes):

// Comment text
md3
{
	name	[NAME]		// MD3 output file name
	filedir	[DIR]		// directory where LWO files are located

	mesh
	{
		name 	 [NAME]			// MD3 internal mesh name
		images	 [IMAGE]<,IMAGE>+	// list of images to shove into it
		surfaces [SURF] <, SURF>+	// list of LW surfaces that make up this mesh
		txuv	 <TXUV> <, TXUV>+		// list of LW 6.0 TXUV maps to make up this mesh
		noupdate
	}
	tag
	{
		name 	 [NAME]		// tag name
		surfaces [SURF]		// surface that defines this tag poly

	}
	filelist
	{
		base	[FILENAME] 			// single file to use as a base
		single	[FILENAME]			// single file to load
		range	[BASE_FILE]  [START_FRAME]  [TOTAL_FRAMES]	// list of files
	}
}

== EXPLANATION OF TERMS IN THE CONFIGURATION FILE ==
First off - The { } brackets are important.
... And you NEED spaces between the tokens.
... And you NEED to put things on different lines.
Anything in the line past a comment token (//) is ignored.

--> md3
This just defines the beginning of the definition.  Nothing too spectacular

--> filedir
This can be used over and over again - only one copy is placed in memory, so if you 
want to get files from elsewhere, just rename it.  This was meant to save on long, 
fully qualified filenames.

--> tag
tag maps a tagname to a particular Lightwave surface.  Only the first polygon
of the surface will be used for the tag position.  The application reads the tag 
in a specific way, it's best you use the Q3_tag.lwo file that came with this 
distribution to designate a tag.  Point the long end 'forward' for the tag, and place
the surface pointing up.  The orientation isn't imperative as long as it is 
consistant, but if you want to swap youre heads with other heads on other models,
you should follow the above guidelines.

--> mesh
mesh maps a mesh to a list of Lightwave surfaces.  This is a way of collapsing a number
of individual meshes down into a single lightwave mesh.  The mesh and tag tokens say what 
portions of each file counts.  Once a mesh or tag is defined, it is valid for any 
filelist unless something else overrides it.  The mesh and tag lists can be repeated - 
the rule is that each subsequent one OVERRIDES a previous definition if there is a naming 
conflict.

--> surfaces 
this lists the surfaces that will be combined to define this mesh.  Surface smoothing will
not cross the surface boundaries.  Omitting a list implies that you want to use a surface 
with the same name as the mesh or tag.  Some wildcard support exists for surface names -
'*' is a variable length wildcard and '?' is a single character wildcard.

--> txuv
txuv maps a list of lightwave 6.0 UV maps to a mesh.  This is useful if you are using
Lightwave 6.0 files.  Using this implicitly tells the compiler to overwrite the current
UV maps inside any MD3 file that is being updated (using the +u flag on the command line)
Wildcard tokens apply here just as with the surfaces tag.  You can think of this as a
glorified switch - if you are using LW 6.0 files, you need only to provide this
token in order to get it to accept the skinmaps in the LW 6.0 files provided that you've
used the 'surfaces' token to definewhich surfaces go into which mesh.

--> noupdate
This overrides the txuv token's ability to define UV maps inside the MD3 file.

A word about 'txuv' and 'surfaces':
Typically you use one or the other.  If you use them both, some ... interesting things
can happen.  If you provide a list of surfaces, and a NULL txuv token, skinmaps WILL 
be generated based on the surfaces, but all the coordinates will be set to 0.  If a NULL 
txuv is given, and the surface list is also omitted, this implies that you want to map UV 
coordinates based on a TXUV map with the same name as the mesh.

--> filelist
This defines the beginning of a new list of files.  All the various file lists end out
being accumulated into one single MD3 file.  The first file from each list makes up the
structure of the MD3 at frame 0; the second file from each list makes up the structure of
the MD3 at frame 1, and so on.  If the frame counts are not identical, warnings will be
spat out to the console, but compilation will continue.  Missing objects will be replaced
by the last good one.  If you are updating a MD3 (+u flag) then any missing files will
be ignored, and the old frames will remain.

--> base
This defines a file that is used to to setup the polygon/point relationships as well as
any potential UVmap.  For example - you could have a sinly LW 6.0 file defining the UV
maps, but the rest of the files could be LW 5.6 files (for whatever reason ..)

--> single
This is simply a single file.

--> range
This defines a ragne of files.  File ranges use a simple base template to find a 
series of similarly named files - both SaveMorf and Project:Messiah are capable of
writing out file sequences with this format.  They work like this:

   NAME_0000    10     40

This would load 40 files in - named NAME_0010.lwo through NAME_0049.lwo

The trick here is to get the file sequences out of Layout.  Using the Savemorf 
plugin will do it and anyone using Project:Messiah is set - just save out a Morph 
Sequence.

==================================

Example: Building a new model

md3
{
	name	 	upper.md3
	filedir		C:\Render\

	// Mesh definitions
	mesh
	{ // Pull all UVmaps matching "torso*" and add to the u_torso mesh
		name	u_torso
		images  models\players\eyegore\u_torso.tga
		txuv	torso*
	}
	mesh
	{ // Map the 'upper_arm' and 'lower_arm' UVmaps to the u_armsmesh
		name	u_arms
		image	models\players\eyegore\u_arms.tga
		txuv	upper_arm, lower_arm
	} 
	mesh
	{ // Map the 'gloves' UVmap to the u_gloves mesh
		name	u_gloves
		image	models\players\eyegore\u_gloves.tga
		txuv	gloves
	}
	// Tag definitions
	tag
	{ // Pull out the tag_head surface for the tag_head tag
		name	tag_head
	}
	tag
	{ // Pull out the tag_weapon surface for the tag_head tag
		name	tag_weapon
	}
	filelist
	{ // Apply the previous definitions to these files
		base	upper
		range	upperdead_0000	0	60
		range	upper_0000	2	15		// walk
		range	upper_0000	0	2		// idle
		range	upper_0000	17	13		// swim
		range	upper_0000	30	10		// run
	}

	// Static tag definitions
	tag
	{ // Put a single tag_torso at the base of the torso
		name	 tag_head
		surfaces *		// use a '*' to override the previous meshes and tags

	}
	filelist
	{ // We only need 1 file because it is not moving
		single	Q3_tag
	}
}

What happens with that?

Well, we've got a series of files - 
	upperdead_0000, upperdead_0001, ... upperdead_0059
and	upper_0000, upper_0001 ... upper_0039 
that are animated.  We've decide to re-order them during the build.

We've got a single file for an anchor tag that will be used for each frame - that way
things are constant.

The mesh u_torso is built of UVmaps matching torso*

==================================

Differences between 'surfaces' and 'txuv':

mesh
{ // Use polygons serviced by LW 6.0 TxUV maps A and B to make up mesh TEST
	name		TEST
	txuv		A, B
}

mesh
{ // Use polygons in surfaces A and B for mesh TEST, but no skinmaps
	name		TEST
	surfaces	A, B
}

mesh
{ // Use polygons in surfaces A and B and their TXUV values for mesh TEST
	name		TEST
	surfaces	A, B
	txuv
}

mesh
{ // Use polygons and skinmap values in TXUV TEST for mesh TEST
	name		TEST
	txuv
}


==================================

Example: Updating an old model

Here I'm going to reposition the skeleton model (skel01.md3) that comes with Quake3.
This is a good technique for Lightwave 5.6 because I don't disrupt the skinmap - I
simply remake the structure.

1) Load the .MD3 into Lightwave (if 5.6 - choose no skin).
2) Move things around - don't ADD anything, and only delete the tag polys.
3) Save out the newly positioned one as a .lwo file (skel_mid.lwo)
4) Copy the old .md3 to the new name (skel_mid.md3)
5) Make a config file -  mine is called skeleton.cfg looks like:

md3
{
	name skel_sid.md3
	mesh
	{
		name skel01
		images models/mapobjects/skel/skel.tga
		surfaces skel01
		noupdate
	}
	filelist
	{
		single skel_sid
	}
}

6) Run LWOtoMD3 skeleton.cfg +u
 - This updates the images and the Vertex positions, does not touch the Skin UV maps.  It
   basically allows you to re-position any model using Lightwave 5.6  This also allows you
   to use another application to do the skinning for any model - as long as you 'recycle' the
   model for animation before you re-build.

==================================






