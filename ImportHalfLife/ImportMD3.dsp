# Microsoft Developer Studio Project File - Name="ImportHalfLife" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ImportHalfLife - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ImportMD3.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ImportMD3.mak" CFG="ImportHalfLife - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ImportHalfLife - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ImportHalfLife - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ImportHalfLife - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O1 /I "..\include" /I "$(LW6_DIR)\SDK\include" /D "NDEBUG" /D D_X86_=1 /D "WIN32" /D "_CONSOLE" /D "_MBCS" /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 server.lib libcmt.lib kernel32.lib /nologo /subsystem:windows /dll /pdb:none /machine:I386 /nodefaultlib:"libcd" /nodefaultlib:"libc" /nodefaultlib:"libcmt" /out:"HalfLife.p" /implib:"Debug/ImportHalfLife.lib" /libpath:"$(LW6_DIR)\SDK\lib" /libpath:"..\lib" -export:_mod_descrip
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy
PostBuild_Cmds=copy HalfLife.p $(LW5_DIR)\Plugins\Modeler	copy HalfLife.p $(LW6_DIR)\Programs\Plugins\Input-Output	copy HalfLife.p $(DEMO_OUTPUT_DIR)
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ImportHalfLife - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\include" /I "$(LW6_DIR)\SDK\include" /D "_DEBUG" /D D_X86_=1 /D "WIN32" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 server.lib kernel32.lib libcmtd.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"libcd" /nodefaultlib:"libc" /nodefaultlib:"libcmt" /out:"HalfLife.p " /libpath:"..\lib" /libpath:"$(LW6_DIR)\SDK\lib" -export:_mod_descrip
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy
PostBuild_Cmds=copy HalfLife.p $(LW5_DIR)\Plugins\Modeler	copy HalfLife.p $(LW6_DIR)\Programs\Plugins\Input-Output	copy HalfLife.p $(DEMO_OUTPUT_DIR)
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "ImportHalfLife - Win32 Release"
# Name "ImportHalfLife - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ImportHalfLife.c
# End Source File
# Begin Source File

SOURCE=.\ObjImp.cpp
# End Source File
# Begin Source File

SOURCE=..\objHalfLife\ParseHalfLife.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ImportHalfLife.h
# End Source File
# Begin Source File

SOURCE=..\include\ParseHalfLife.h
# End Source File
# End Group
# Begin Group "Common Source"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\common\lw_base.c
# End Source File
# End Group
# End Target
# End Project
