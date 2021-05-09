/**************************************
 *
 *  LtoM_obj.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *  
 *  Helper objects for LWO --> MD3 convresion tool
 *
 *  MD3 file format property of id software
 *
 **************************************/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "ParseMD3.h"
#include "ParseLWO.h"
#include "ParseLWO2.h"
#include "LWOtoMD3.h"

static const char			*Global_WildCard = "*";
static const char			*Global_MatchCard = "?";

int	Material_Reference::DoesNameMatch(char *in, const char *matchto)
{
	char		matchbuf[256];
	strcpy(matchbuf,matchto);

	char	*p_in = in;
	char	*p_match = matchbuf;

	// walk each string, matching and checking for the wildcard
	for (;*p_in && *p_match; p_in++, p_match++)
	{
//		fprintf(stdout,"...checking '%s'\tagainst '%s'\n",p_in, p_match);

		if (*p_match == *Global_MatchCard)
		{	// skip the next character
			continue;
		}

		if (*p_match == *Global_WildCard)
		{	// wildcard space - skip dup wildcards
			while (*p_match == *Global_WildCard)
				++p_match;

			if (*p_match == 0)				// last char was wild.  Everything matches.
				return 1;

			// walk until we find a potential match, then shell in			
			while (*(++p_in))
			{
				if (*p_match == *p_in && DoesNameMatch(p_in, p_match))
					return 1;
			}
			return 0;
		}

		// Check the absolute character value
		if (*p_in != *p_match)
			return 0;
	}

	// Allow trailing '*' to match NULL as well.
	if (*p_in == 0 && 
		(*p_match == 0 || *p_match == *Global_WildCard)	)
	{	// All characters matched
		return 1;
	}

	return 0;
}

Surf_Reference	*Material_Reference::FindSurface(char *n)
{
	Surf_Reference	*cv = 0;
	for (cv = conversion; cv; cv = cv->Next())
	{	// ... in each file that that surface should exist
		if (strcmp(cv->Name(),n) == 0)
			return cv;
	}

	return 0;
}

int Material_Reference::Validate()
{
	UINT32		ps = pcount, vs = vcount;

	RecalculateCounts();
	
	if (ps != pcount)
	{
		fprintf(stdout,"ERROR: for Input Material '%s' - found %d of %d polys\n",
						meshname, pcount, ps);
		return 0;
	}

	if (vs != vcount)
	{
		fprintf(stdout,"ERROR: for Input Material '%s' - found %d of %d points\n",
						meshname, vcount, vs);
		return 0;
	}

	return 1;
}

void Material_Reference::RecalculateCounts()
{
	Surf_Reference	*cv = (Surf_Reference *)NULL;
	LWO_ref			*rf = (LWO_ref *)NULL;

	pcount = 0;
	vcount = 0;

	for (cv = conversion; cv; cv = cv->Next())
	{	// ... in each file that that surface should exist
		for (rf = cv->ReferencedObjects(); rf; rf = rf->Next())
		{	// ... for matching names, polycounts, and pointcounts
			if (rf->ActiveObject() && 
				(strcmp(cv->Name(), rf->ReferencedName()) != 0) )
			{	// bad name
				fprintf(stdout,"ERROR: - references '%s', expected '%s'\n",
								rf->ReferencedName(), cv->Name());
				continue;
			}
			else if (!rf->Revalidate())
			{
				fprintf(stdout,"ERROR: - structure of '%s' is different from the base file\n",
								rf->ReferencedName());
				return;
			}

			pcount += rf->NumberOfApplicablePolygons();
			vcount += rf->NumberOfApplicableVertices();
		}
	}
}

int Mesh_Reference::Validate(MD3 *test)
{
	if (!Material_Reference::Validate())
		return 0;

	if (test->Mesh(Index()).PointCount() > 0 &&
		PointCount() != test->Mesh(Index()).PointCount())
	{	// Wrong number of points ...
		fprintf(stdout,"ERROR: for Mesh '%s' - found %d points, expected %d\n",
						MeshName(), PointCount(), test->Mesh(Index()).PointCount() );
		return 0;
	}
	else if (test->Mesh(Index()).PolyCount() > 0 &&
				PolyCount() !=  test->Mesh(Index()).PolyCount())
	{	// Wrong number of polygons
		fprintf(stdout,"ERROR: for Mesh '%s' - found %d polys, expected %d\n",
						MeshName(), PolyCount(), test->Mesh(Index()).PolyCount() );
		return 0;
	}


	return 1;
}

int Tag_Reference::Validate(MD3 *test)
{
	if (!Material_Reference::Validate())
		return 0;

	if (PointCount() != 3)
	{
		fprintf(stdout,"ERROR: for Tag '%s' - found %d points, expected 3\n",
							MeshName(), PointCount());
		return 0;
	}

	if (PolyCount() != 1)
	{
		fprintf(stdout,"ERROR: for Tag '%s' - found %d polys, expected 1\n",
							MeshName(), PolyCount());
		return 0;
	}

	return 1;
}

/* 
 * LWO_stub functions
 */
LWO *LWO_stub::FindFileByName(const char *name)
{
	fprintf(stdout,"Processing: '%s' ...\n",name);

	LWO	*ltmp = new LWOv1(name);

	if (!ltmp->isValid())
	{
		delete ltmp;
		ltmp = new LWOv2(name);
	}
	
	if (ltmp->isValid())
	{

		fprintf(stdout,"\t\t\tfound %-22.22s \n",
							ltmp->FileType());
		return ltmp;
	}

	delete ltmp;

	fprintf(stdout,"\t\t\tINVALID\n");

	return (LWO* )0;
}

int LWO_stub::LoadFileByFrame(UINT32 targetframe)
{
	if (ItemCount() == 0)
		return 0;
	// This might be the same file over and over ... ...
	else if (ItemCount() == 1)
	{
		if (LoadedObject() == 0)
			targetframe = 0;
	}

	if (FileName(targetframe))
	{
		LWO	*ltmp = FindFileByName(FileName(targetframe));

		if (ltmp)
		{
			SetLoadedObject(ltmp);
			return 1;
		}
		else
		{	// bad file?
			fprintf(stdout,"ERROR: Unable to load '%s'.  It will be ignored\n",
						FileName(targetframe));
		}
	}

	if (LoadedObject())
		return -1;	// leave current file
	else
		return 0;
}

void LWO_stub::PopulateBaseObject()
{
	if (baseobject)
		return;

	else if (ItemCount() == 0)		// no base & no files
		return;

	for (UINT16 j = 0;
		 j < ItemCount() && LoadFileByFrame(j) == 0;
		 j++);

	if (!baseobject)
		return;

	SetLoadedObject(0);
	return;
}

/*
 * LWO_ref functions
 * Revalidate makes sure that the Active object matches the BaseObject's
 * makeup - it doesn't have to be exact, but relatively exact - i.e. there
 * better be the same number of matching polygons and points, and each
 * polygon needs to reference the same relative points
 */
int LWO_ref::Revalidate()
{	// Sanity counts - assume new active object
	ActiveFilter = LWO_filter(this,ActiveObject());

	if (BaseFilter == ActiveFilter)
	{	// We need to see if each polygon references the same points
		try {
		for (int i = BaseFilter.PolygonCount() -1; i > -1; i--)
		{	// For each polygon in each 
			LW3D::Polygon& polyBase = BaseObject()->getPolygon(BaseFilter.OriginalPolygonIndex(i));
			LW3D::Polygon& polyActv = ActiveObject()->getPolygon(ActiveFilter.OriginalPolygonIndex(i));

			for (int j = 0; j < 2; j++)
			{
				if (BaseFilter.TranslatedVertexIndex(polyBase.pntindices[j]) !=
					ActiveFilter.TranslatedVertexIndex(polyActv.pntindices[j]))
					return 0;
			}
		}
		} catch (LWO_ref_ERROR& e) {
			fprintf(stderr,"\nFATAL: %s: '%s' (%d)\n",
				e.Message(), e.Object()->FileName, e.Index());
			return 0;
		}
	}
	else
		return 0;


	return 1;
}

/*
 * This sort of just returns if they 'might' be equal
 */
int LWO_ref::LWO_filter::operator ==(LWO_ref::LWO_filter& in)
{	// Check counts
	if (VertexCount() != in.VertexCount())
		return 0;

	if (PolygonCount() != in.PolygonCount())
		return 0;

	return 1;
}

/*
 * fill with the a list of VertexCount() values that represent
 * the new index values of each valid point in this new 'compressed' world
 */
void LWO_ref::LWO_filter::Fill(LWO *l)
{
	if (!ref || !l)
		return;

	int i = 0, num = 0;
	for (i = l->PolyCount() -1, num = 0; i > -1; i--)
	{
		LW3D::Polygon& p = l->getPolygon(i);
		if (ref->isPolyValidinObject(p,l))
		{
			xPs[i] = num;
			unxPs[num] = i;

			// Register that these points do exist
			xVs[p.pntindices[0]] = 0;
			xVs[p.pntindices[1]] = 0;
			xVs[p.pntindices[2]] = 0;

			// ... but don't do unxVs yet ...

			++num;
		}
	}

	int j = xVs.Max();
	for (i = 0, num = 0; i <= j; i++)
	{
		if (xVs[i] != 0)
			continue;

		xVs[i] = num;
		unxVs[num] = i;

		++num;
	}

	xPs.LockMax();
	unxPs.LockMax();

	xVs.LockMax();
	unxVs.LockMax();

	filled = 1;
}