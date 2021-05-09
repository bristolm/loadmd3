#include "sys_math.h"

#ifndef  _LWOTOMD3_H_
#define  _LWOTOMD3_H_

using namespace LW3D;

#define IS_TAG_PREFIX(s)  (strncmp(s,"tag_",4) == 0)
#define MAX_NAMELEN		256

class CharBuff
{
	char		name[MAX_NAMELEN];					// Name of input file

	inline char *_name(const char *n)
	{
		if (n && n[0])
		{
			strcpy(name,n);
			return name;
		}
		else
		{
			name[0] = 0;
			return 0;
		}
	}
public:
	CharBuff()
	{
		name[0] = 0;
	}

	~CharBuff()
	{
		name[0] = 0;
	}

	inline char *Name()								// Get name
	{	return name; }

	inline char *HardSetName(const char *n)			// Unconditionally set name
	{	return (_name(n));	}

	inline char *SoftSetName(const char *n)			// If name's set, don't set it
	{
		if (!n)
			return 0;

		if (name[0])
			return 0;
		else
			return _name(n);
	}

	inline void ClearName()							// Clear name
	{	_name(0);	}
};

#define CHARBUFFARRAY_INC		10

class CharBuffArray
{
	CharBuff		*items;				// Array of size 'itemcount'
	int				itemsize;			// number of items we can hold
	int				itemcount;			// number of items we expect

public:
	inline CharBuff *StoreItem(const char *name, int f = -1)
	{
		if (f > itemcount)
			return 0;
		else if (f < 0)
		{	// add to NEXT empty one
			int i = 0;
			for (i = 0;i < itemcount;i++)
			{
				if (!items[i].SoftSetName(name))
					continue;

				return &(items[i]);
			}

			// out of space ...
			CharBuff *cb = new CharBuff[itemsize = itemcount + CHARBUFFARRAY_INC];
			if (items)
			{
				memcpy(cb,items,sizeof(CharBuff) * itemcount);
				delete [] items;
			}

			items = cb;
			itemcount++;

			items[i].SoftSetName(name);

			return &(items[i]);;
		}
		else
		{	// ???
			return &(items[f]);
		}
	}

	inline const char *GetItem(int f)
	{
		if (f < itemcount)
			return items[f].Name();
		else
			return 0;
	}

	CharBuffArray(int f)
	{
		if ((itemcount = itemsize = f) > 0)
			items = new CharBuff[itemsize];
		else
			items = 0;
	}

	CharBuffArray(const char *name)
	{
		itemcount = itemsize = 1;
		if (name && name[0])
		{
			items = new CharBuff[itemsize];
			items[0].HardSetName(name);
		}
		else
			items = 0;
	}

	inline UINT32	ItemCount()
	{return itemcount;}
};

/*
 * LWO_stub 'stubs' out a particular Lightwave object instance.  It basically
 * allows many-to-one sort of relationships - many materials to one LW file.
 * It also is in charge of mapping the vertext and polygonrelationships between the
 * 2 files
 */
class LWO_stub :	public CharBuffArray,
					public ChainItem<LWO_stub>
{
	LWO		*lwo;					// Currently loaded lightwave object file
	LWO		*baseobject;			// Permanently loaded lightwave object file - seeds
													// polygon/point relations ships and UV maps

	inline void SetLoadedObject(LWO *l)
	{
		if (lwo && baseobject != lwo)
			delete lwo;

		lwo = l;

		// seed baseobject if it hasn't been already
		if (!baseobject)
			baseobject = l;
	}

	LWO *FindFileByName(const char *n);
	void PopulateBaseObject();

public:
	LWO_stub(int f):CharBuffArray(f),
		lwo(0),
		baseobject(0)
	{;}

	LWO_stub(const char *name):CharBuffArray(name),
		lwo(0),
		baseobject(0)
	{;}

	inline LWO	*LoadedObject()
	{ return lwo;	}
	inline LWO	*BaseObject()
	{
		if (baseobject == (LWO *)0)
			PopulateBaseObject();

		return baseobject;
	};


	inline CharBuff *StoreFileName(const char *name)
	{
		return StoreItem(name,-1);
	}

	inline const char *FileName(int f = -1)
	{
		if (f < 0)
			return (BaseObject()->Name());
		else
			return GetItem(f);
	}

	int LoadFileByFrame(UINT32 targetframenum);
	inline void Unload()
	{ SetLoadedObject(0); }
	inline int SetBaseObject(char *n)
	{
		if (n == (char *)NULL ||
			n[0] == 0)
			return 0;

		LWO *l = FindFileByName(n);
		if (l == (LWO *)NULL)
			return 0;

		if (baseobject && baseobject != lwo)
			delete baseobject;

		baseobject = l;
		return 1;
	}
};

class LWO_ref_ERROR
{
	UINT32		idx;
	const char	*msg;
	LWO	*lwo;	

public:
	LWO_ref_ERROR(LWO *obj,const char *c, UINT32 index):
	  idx(index), msg(c), lwo(obj)
	{};

	const char *Message()	{ return msg; }
	UINT32 Index()			{ return idx; }
	LWO	*Object()	{ return lwo; }
};

/*
 * LWO_ref object
 *
 * There can be many meshes in one .MD3 file			
 * Each mesh can be composed of one or many unique surface names	(Mesh_Reference)
 *   Each surface name can exist in one or many objects				(Surf_Reference)
 *
 * This object works alot like a filter - it is meant to take the object(s) in the 
 * LWO_stub and map [0,1,2 ... n] array to the [4,6,9,12,...] indexes in the real
 * object
 */
class LWO_ref : public ChainItem<LWO_ref>
{
protected:
	class LWO_filter
	{
		LWO_ref			*ref;		// for polygon validation purposes

		AutoArray<UINT32>		xVs;		// Map of translated point index by original point index
		AutoArray<UINT32>		xPs;		// Map of translated poly index by original poly index

		AutoArray<UINT32>		unxVs;		// Map of original point index by translated object point index
		AutoArray<UINT32>		unxPs;		// Map of original poly index by translated object poly index

		void Fill(LWO *l);

		int						filled;

	public:
		LWO_filter(LWO_ref *r = 0, LWO *l = 0):
		  ref(r),
		  xVs(INVALID_VERTEX_IDX),
		  xPs(INVALID_POLYGON_IDX),
		  unxVs(INVALID_VERTEX_IDX),
		  unxPs(INVALID_POLYGON_IDX),
		  filled(0)
		  {
			if (!l)
				return;

			Fill(l);
		  }

		  int isValid() { return filled;}

		int operator ==(LWO_filter&);

		UINT32 VertexCount()  { return unxVs.Count();} 
		UINT32 PolygonCount() { return unxPs.Count();}

		UINT32 OriginalPolygonIndex(UINT32 refidx)
		{
			if (refidx >= PolygonCount())
				throw(LWO_ref_ERROR(ref->BaseObject(),"polygon exceeds count",refidx));

			return (unxPs[refidx]);
		};

		UINT32 TranslatedVertexIndex(UINT32 lwoidx, int badok = 0)
		{
//			if (lwoidx >= VertexCount())
//				throw(LWO_ref_ERROR(ref->BaseObject(),"vertex exceeds count in base object",lwoidx));

			UINT32 i = xVs[lwoidx];
			if (i == INVALID_VERTEX_IDX && !badok)
				throw(LWO_ref_ERROR(ref->BaseObject(),"vertex not found in base object",lwoidx));
			return i;
		}
	};

	LWO_stub	*lwostub;			// lwo_stub that has reference to this surface
	LWO_filter  BaseFilter;
	LWO_filter  ActiveFilter;

	LWO_ref(LWO_stub *lstub):
	  lwostub(lstub)
	  {;}

	virtual ~LWO_ref() {;}

	void Populate()
	{
		if (!BaseFilter.isValid())
			BaseFilter = LWO_filter(this,BaseObject());

		ActiveFilter = LWO_filter(this,ActiveObject());
	}

public:
	inline LWO	*BaseObject()
	{	return lwostub->BaseObject(); }

	inline LWO	*ActiveObject()
	{	return lwostub->LoadedObject()
						? lwostub->LoadedObject()
						: lwostub->BaseObject(); }

	inline UINT32 NumberOfApplicableVertices()
	{ return BaseFilter.VertexCount(); }
	UINT32 NumberOfApplicablePolygons(int force = 0)
	{ return BaseFilter.PolygonCount(); }

	inline UINT32 OriginalPolygonIndex(UINT32 idx)
	{
		if (ActiveFilter.PolygonCount() == 0)
			return BaseFilter.OriginalPolygonIndex(idx);
		return ActiveFilter.OriginalPolygonIndex(idx);
	}

	inline UINT32 TranslatedVertexIndex(UINT32 idx, int badok = 0)
	{
		if (ActiveFilter.PolygonCount() == 0)
			return BaseFilter.TranslatedVertexIndex(idx,badok);
		return ActiveFilter.TranslatedVertexIndex(idx,badok);
	}

	int Revalidate();

	// VIRTUAL
	virtual int isPolyValidinObject(Polygon& p,  LWO *l)	= 0;
	virtual const char *ReferencedName()								= 0;
};

class LWO_ref_bysurface : public LWO_ref
{
	INT16		surfaceidx;		// index value for this surface in the the referenced lwostub object

public:
	LWO_ref_bysurface(LWO_stub *lstub, LWO *lwo, INT16 idx):
		LWO_ref(lstub),
		surfaceidx(idx)
		{ Populate(); }

	inline int isPolyValidinObject(Polygon& p,  LWO *l)
	{
		return l->getSurface(surfaceidx).getPolygonIndex(p.getIndex());
	}

	inline const char *ReferencedName()
	{	return (lwostub->BaseObject()
					? lwostub->BaseObject()->getSurface(surfaceidx).name
					: "<Not loaded>");}
};

class LWO_ref_bytxuv : public LWO_ref
{
	INT16		uvidx;		// index value for this surface in the the referenced lwostub object

public:
	LWO_ref_bytxuv(LWO_stub *lstub, LWO *lwo, INT16 idx):
		LWO_ref(lstub),
		uvidx(idx)
		{ Populate(); }

	inline int isPolyValidinObject(Polygon& p,  LWO *l)
	{
		return l->getVMap(uvidx).getPolygonIndex(p.getIndex());
	}

	inline const char *ReferencedName()
	{	return (lwostub->BaseObject()
					? lwostub->BaseObject()->getVMap(uvidx).name
					: "<Not loaded>");}
};

class Surf_Reference : public ChainItem<Surf_Reference>
{

	const char		*surfacename;	// name for this surface
	LWO_ref			*lworef;		// (eventually) chain of pointers to references that further define this surface
public:

	Surf_Reference(const char *n):
	  surfacename(n),lworef(0)
	  {;}
	virtual ~Surf_Reference() {;}

	inline LWO_ref *AddLwoRef(LWO_ref	*l)
	{
		if (lworef)
			lworef->InsertTail(l);
		else
			lworef = l;

		return l;
	}

	inline const char		*Name()
	{ return surfacename; }

	inline LWO_ref			*ReferencedObjects()
	{ return lworef; }
};

class Material_Reference : public ChainItem<Material_Reference>
{
	char			meshname[256];	// output meshname
	Surf_Reference	*conversion;	// surfaces to fill this mesh
	UINT32			vcount;			// Total number of vertices in contributing objects
	UINT32			pcount;			// Total number of polys in the contributing objects

	CharBuffArray	surfaces;		// Surface alias list

protected:
	int				DoesNameMatch(char *in, const char *matchto);

public:
	Material_Reference(const char *n):
	  conversion(0),
	  vcount(0),
	  pcount(0),
	  surfaces(0)
	{
		strcpy((char *)&(meshname[0]),n);
	}
	virtual ~Material_Reference() {;}

	inline Surf_Reference	*AddSurface(Surf_Reference *s)
	{
		if (!conversion)
			conversion = s;
		else
			conversion->InsertTail(s);

		// Eventually this should return the cumulative counts of all sub-objects
		if (s->ReferencedObjects())
			RecalculateCounts();

		return s;
	}

	inline CharBuff *StoreMaterialAlias(const char *name)
	{
		return surfaces.StoreItem(name,-1);
	}
	inline int FindSurfaceMatch(char *n)
	{
		int i = surfaces.ItemCount();
		while (i-- > 0)
		{
			if (DoesNameMatch(n,surfaces.GetItem(i)) == 1)
				return 1;
		}
		return 0;
	}

	void			RecalculateCounts();

	Surf_Reference *FindSurface(char *n);

	inline const char *MeshName()
	{ return meshname; }
	inline Surf_Reference *ConversionList()
	{ return conversion; }
	inline UINT32 PointCount()
	{ return vcount; }
	inline UINT32 PolyCount()
	{ return pcount; }

	inline int SurfaceCount()
	{ return conversion->Count(); }

	virtual const char *Label()		= 0;
	virtual int Validate(MD3 *m)	= 0;

	int Validate(void);

	inline virtual void unFlagForMappingSkins()	{;}
	inline virtual void FlagForMappingSkins()	{;}
	inline virtual int FlaggedForMappingSkins()	{ return 0; }

	virtual void AddImage(const char *)	= 0;
	virtual const char *GetImage(int )	= 0;

	inline virtual int FindSkinMapMatch(char *n)		{ return 0;}
	inline virtual void StoreSkinMapAlias(const char *c)				{;}
	inline virtual const char *GetSkinMapName(int i)	{ return (const char *)0; }
};

class Mesh_Reference: public Material_Reference
{
	CharBuffArray	images;
	CharBuffArray	txuv;
	int				domapskins;

public:
	Mesh_Reference(const char *n) :Material_Reference(n),
		images(0),
		txuv(0),
		domapskins(0)
	  {;}
	virtual ~Mesh_Reference() {;}

	inline const char *Label()
	{ return "Mesh";}
	int Validate(MD3 *test);
	inline void AddImage(const char *c)
	{ images.StoreItem(c); }
	inline  const char *GetImage(int i)
	{ return images.GetItem(i); }

	inline void FlagForMappingSkins()
	{ domapskins = 1; }
	inline void unFlagForMappingSkins()
	{ domapskins = 0; }
	inline int FlaggedForMappingSkins()
	{ return domapskins != 0; }

	inline virtual int FindSkinMapMatch(char *n)
	{
		int i = txuv.ItemCount();
		while (i-- > 0)
		{
			if (DoesNameMatch(n,txuv.GetItem(i)) == 1)
				return 1;
		}
		return 0;
	}
	inline void StoreSkinMapAlias(const char *c)
	{ txuv.StoreItem(c); }
	inline const char *GetSkinMapName(int i)
	{ return txuv.GetItem(i); }
};

class Tag_Reference :	public Material_Reference
{
public:
	Tag_Reference(const char *n) :Material_Reference(n)
	  {;}
	
	inline const char *Label()
	{ return "Tag";}
	int Validate(MD3 *test);
	inline void AddImage(const char *)	{ ; }
	inline const char *GetImage(int )	{ return 0; }
};

class CfgData
{
	char				*MD3_Name;
	LWO_stub			*LWOfiles;
	Material_Reference	*SurfMeshes;
	Material_Reference	*TagMeshes;
public:
	CfgData() :
	  MD3_Name(0),
	  LWOfiles(0),
	  SurfMeshes(0),
	  TagMeshes(0)
	{;}

	inline char *Name(char *in = 0)
	{ if (in) MD3_Name = in; return MD3_Name; }
	inline LWO_stub *FileLists(LWO_stub *in = 0)
	{ if (in) LWOfiles = in; return LWOfiles; }
	inline Material_Reference *Meshes(Material_Reference *in = 0)
	{ if (in) SurfMeshes = in; return SurfMeshes; }
	inline Material_Reference *Tags(Material_Reference *in = 0)
	{ if (in) TagMeshes = in; return TagMeshes; }

};

extern int ProcessCfgFile(char *cfgfile, CfgData *);

#endif // _LWOTOMD3_H_
