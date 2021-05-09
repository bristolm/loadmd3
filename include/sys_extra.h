/**************************************
 *
 *  sys_extra.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  additional system objects -
 *  AutoArray, Cache template objects
 *
 **************************************/

#ifndef _SYS_EXTRA_H_
#define _SYS_EXTRA_H_

// Template function for initializing arrays of objects
template <typename T>
T **InitArray(int num)
{
	if (num == 0) {
		return (T **)0;
	}

	T **tmp = new T *[num];
	while (num > 0) {
		tmp[--num] = new T();
	}

	return tmp;
}

/* Expanding array 
 *    arraysize		represents the number of currently available slots
 *    arraylimit	represents the largest number of slots we want to hold
 *    maxvalue		represents the highest _used_ index
 *
 * Here's the deal - the array portion is never resized, new chunks are
 * simply allocated as needed (so the positioning is ALWAYS constant).  
 * The actual meat of the array is reference counted, and NOT duplicated
 * when the object is copied.
 */

#define _AUTO_ARRAY_BLOCK_BITS			5
#define _AUTO_ARRAY_BLOCK_SIZE			(1 << _AUTO_ARRAY_BLOCK_BITS)
#define _AUTO_ARRAY_BLOCK_SLOTMASK		(_AUTO_ARRAY_BLOCK_SIZE  -1)
#define _AUTO_ARRAY_BLOCK_STEPSITE		1

// Returns the block (row) that this values is in
#define _GET_BLOCK(idx)					((idx) >> _AUTO_ARRAY_BLOCK_BITS)

// Returns the slot that this value is in
#define _GET_SLOT(idx)					((idx) & _AUTO_ARRAY_BLOCK_SLOTMASK)

template <typename T>
class AutoArray
{
public:
	const unsigned int		BAD_INDEX;
private:
	T					badvalue;
	unsigned int		arraysize;
	unsigned int		arraylimit;
	unsigned int		nextidx;
	T**					array;		// Array of blocks of 64 - we NEVER move the values

public:
	AutoArray(T bad, unsigned int limit = (unsigned int)(-1)):
		BAD_INDEX((unsigned int)-1),
		badvalue(bad),			// '0' values
		arraysize(0),			// Current number allocated
		arraylimit(limit),		// Maximum number allowed by the user
		nextidx(0),				// Next slot
		array(0)
	{
	}

	AutoArray(const AutoArray& rhs):
		BAD_INDEX((unsigned int)-1),
		badvalue(rhs.badvalue),			// '0' values
		arraysize(rhs.arraysize),		// Current number allocated
		arraylimit(rhs.arraylimit),		// Maximum number allowed by the user
		nextidx(rhs.nextidx)			// Next slot
	{
		// Build a copy of the array
		if (rhs.arraysize == 0 || rhs.array == 0)
		{
			array = 0;
			return;
		}

		int block = _GET_BLOCK(rhs.arraysize -1);
		array = new T*[block +1];
		for (int i = 0; i <= block; i++)
		{
			if (rhs.array[i] == 0)
			{
				array[i] = 0;
				continue;
			}

			array[i] = new T[_AUTO_ARRAY_BLOCK_SIZE];
			for (int j = 0; j < _AUTO_ARRAY_BLOCK_SIZE; j++)
			{
				array[i][j] = rhs.array[i][j];
			}
		}
	}

	~AutoArray()
	{
		if (array == (T**) 0)
			return;

		unsigned int i = _GET_BLOCK(arraysize -1) +1;
		while (i > 0)
		{
			if (array[--i])
			{
				delete [] array[i];
				array[i] = 0;
			}
		}
		delete [] array;
		array = 0;
	}

	AutoArray& operator=( const AutoArray& rhs)
	{
		if (this != &rhs)
		{
			badvalue = rhs.badvalue;
			arraysize = rhs.arraysize;
			arraylimit = rhs.arraylimit;
			nextidx = rhs.nextidx;

			if (rhs.arraysize == 0)
			{
				array = 0;
				return *this;
			}

			// Build a copy of the array
			int block = _GET_BLOCK(arraysize -1) +1;

			array = new T*[block];
			for (int i = 0; i < block; i++)
			{
				if (rhs.array[i] == 0)
				{
					array[i] = 0;
					continue;
				}

				array[i] = new T[_AUTO_ARRAY_BLOCK_SIZE];
				for (int j = 0; j < _AUTO_ARRAY_BLOCK_SIZE; j++)
				{
					array[i][j] = rhs.array[i][j];
				}
			}
		}

		return *this;
	}

	// Return a single member by index
	T& operator [] (unsigned int idx)
	{	// TTT - should be error checking
		if (idx > arraylimit)
			return badvalue;

		int block = _GET_BLOCK(idx);
		int slot = _GET_SLOT(idx);

		if (idx >= arraysize)
		{	// Add another block of slots
			unsigned int oldblockcount = 0;
			unsigned int newblockcount = 0;
			if (arraysize != 0)
			{
				oldblockcount = _GET_BLOCK(arraysize -1);
			}
			newblockcount = block;

			T** tmp = new T*[newblockcount +1];
			unsigned int i = 0;
			if (array != 0)
			{
				for (;i <= oldblockcount;i++)
					tmp[i] = array[i];
				delete array;
				array = 0;
			}
			for (;i <= newblockcount;i++)
				tmp[i] = 0;

			array = tmp;
			arraysize = (newblockcount +1) << _AUTO_ARRAY_BLOCK_BITS;
		}

		if (idx >= nextidx)
			nextidx = idx +1;

		// See if we need to fill a new block
		if (array[block] == 0)
		{
			array[block] = new T[_AUTO_ARRAY_BLOCK_SIZE];
			for (unsigned int i = 0;i < _AUTO_ARRAY_BLOCK_SIZE;i++)
				array[block][i] = badvalue;
		}

		return array[block][slot];
	}

	// Manipulation functions
	void LockMax(unsigned int limit = 0)
	{
		arraylimit = limit == 0 ? 
			(nextidx > 0 ? nextidx -1 : 0 ) : limit;
	}

	// Return the largest value
	unsigned int Max()
	{
		return nextidx > 0 ? nextidx -1 : 0;
	}

	// Return the next value
	unsigned int Next()
	{
		return nextidx;
	}

	// Return the number of values that are not 'invalid'
	unsigned int Count()
	{
		if (array == (T**) 0)
			return 0;

		unsigned int ret = 0;
		unsigned int block = _GET_BLOCK(arraysize -1) +1;
		while (block-- > 0)
		{
			if (array[block] == 0)
				continue;
			for (unsigned int i = 0; i < _AUTO_ARRAY_BLOCK_SIZE; i++)
				if (!(array[block][i] == badvalue))
					++ret;
		};
		return ret;
	}

	// Reset all items to the bad value
	void Clear()
	{
		if (array != 0)
		{
			unsigned int block = _GET_BLOCK(arraysize -1) +1;
			while (block-- > 0)
			{
				if (array[block] == 0)
					continue;
				for (unsigned int idx = 0; idx < _AUTO_ARRAY_BLOCK_SIZE; idx++)
				{	// Don't delete the bad ones
	//				if (memcmp(&array[idx],&badvalue, sizeof(T)) == 0)
	//					continue;
	//				delete array[idx];
					array[block][idx] = badvalue;
				}
			};
		}
		nextidx = 0;
	}

	// Flush out the bad ones and reduce maxidx
	void Reduce()
	{
		int curidx = 0;
		int idx = 0;
		unsigned int max = Next();
		int block = _GET_BLOCK(max);
		int slot = _GET_SLOT(max);
		for (int i = 0; i <= block; i++)
		{
			int iRowMax = (i == block) ? slot : _AUTO_ARRAY_BLOCK_SIZE;
			for (int j = 0; j < iRowMax; j++, idx++)
			{
				if (array[i][j] == badvalue)
					continue;

				if (curidx != idx)
				{
					array[_GET_BLOCK(curidx)][_GET_SLOT(curidx)] = array[i][j];
					array[i][j] = badvalue;
				}
				++curidx;
			}
		}

		// If we deleted them all, clean up
		if (curidx == 0)
		{
			Clear(); 
		}
		else
		{
			nextidx = curidx;
		}
	}

	// Find the index whose entry equals a provided alue
	unsigned int find(T& target, unsigned int start = 0)
	{
		unsigned int last = Next();
		for (;start < last; start++)
		{
			int block = _GET_BLOCK(start);
			int slot = _GET_SLOT(start);
			if (array[block] == 0)
				continue;
			if (array[block][slot] == target)
				return start;
		}

		return BAD_INDEX;
	}

};

// TTT - casting will not work properly with all items
template <typename T>
class ChainItem
{
#ifdef WIN32
	friend ChainItem <T>;
#endif

	T	*next;					// next in chain
	T	*prev;					// previous in chain

	int	index;

	inline void reIndex()
	{
		T*	h = Head();
		int	idx = 0;
		for (;h;h = h->Next())
			h->index = idx++;
	}

	inline void _SetNext(T *t)
	{ next = t; reIndex(); }

	inline void _SetPrevious(T *t)
	{ prev = t;	reIndex(); }


public:
	ChainItem() : next(0), prev(0), index(0)
	{;}

	~ChainItem()
	{	// Remove item from any chain at deletion
		Remove();
	}

	inline int	Index(void)
	{ return (static_cast<T *>(this) == 0) ? -1 : index; }

	inline int Count(void)
	{
		T *t = static_cast<T *>(this);
		if (t == 0)
			return 0;

		int i = 1;

		while (t = t->Next())
			i++;

		return i;
	} 

	// Walk forward
	inline T *Next(void)			// return next item in chain
	{ return (static_cast<T *>(this) == 0) ? 0 : next; }

	inline T *Head(void)
	{
		if (static_cast<T *>(this) == 0) return 0;
		if (prev)
			return (prev->Head());
		else
			return static_cast<T *>(this);
	}

	// Walk backward
	inline T *Previous(void)		// return previous item in chain
	{  return (static_cast<T *>(this) == 0) ? 0 :  prev; }

	inline T *Tail(void)
	{
		if (static_cast<T *>(this) == 0) return 0;

		if (next)
			return (next->Tail());
		else
			return static_cast<T *>(this);
	}

	// Remove items
	inline T *Remove(void)
	{
		if (static_cast<T *>(this) == 0) return 0;
		if (prev)
			prev->_SetNext(next);

		if (next)
			next->_SetPrevious(prev);

		prev = next = 0;

		reIndex();

		return static_cast<T *>(this);
	}

	inline T *BreakNext()			// Make me a tail, return new head of other chain
	{
		if (static_cast<T *>(this) == 0) return 0;
		T *n = prev;

		if (n)
			n->_SetPrevious(0);

		reIndex();

		return n;
	}

	inline T *BreakPrevious()		// Make me a head, return new tail of other chain
	{
		if (static_cast<T *>(this) == 0) return 0;
		T *p = prev;

		if (p)
			p->_SetNext(0);

		prev = 0;

		reIndex();

		return p;
	}

	// Add items
	inline T *InsertPrevious(T *t)
	{
		if (static_cast<T *>(this) == 0) return 0;
		if (prev == t)
			return t;
		else
		{
			_SetPrevious(t->Tail());
			Previous()->_SetNext(static_cast<T *>(this));
		}

		reIndex();
		return t;
	}

	inline T *InsertNext(T *t)		// set next item in chain
	{
		if (static_cast<T *>(this) == 0) return 0;
		if (next == t)				// same one we already have
			return t;
		else
		{
			_SetNext(t->Head());
			Next()->_SetPrevious(static_cast<T *>(this));
		}

		reIndex();
		return t;
	}

	inline T *InsertHead(T *t)
	{
		if (static_cast<T *>(this) == 0) return 0;
		Head()->InsertPrevious(t);
		reIndex();
		return (t->Head());
	}

	inline T *InsertTail(T *t)
	{
		if (static_cast<T *>(this) == 0) return 0;
		Tail()->InsertNext(t);
		reIndex();
		return (t->Tail());
	}
};

template <typename T>
class Block : public AutoArray<T>
{
public:
	Block() :
	  AutoArray<T>(T::INVALID)
	{;}

	static Block<T>	INVALID;
};

template <typename T>
class Cache
{
	int		m_uniqueid;

	static	AutoArray<T *> All;
	static	int NextUniqueIndex;

protected:
	Cache()
	{
		m_uniqueid = NextUniqueIndex++;
		All[m_uniqueid] = reinterpret_cast<T *>(this);
	}

	Cache(int seedidx)
	{
		m_uniqueid = seedidx;

		if (m_uniqueid < 0)
			return;

		All[m_uniqueid] = reinterpret_cast<T *>(this);
		if (m_uniqueid >= NextUniqueIndex)
			NextUniqueIndex = m_uniqueid +1;
	}

	~Cache()
	{
		if (m_uniqueid >= 0)
			All[m_uniqueid] = 0;
	}

	void resetCacheID(int idx)
	{
		if (m_uniqueid >= 0)
			All[m_uniqueid] = 0;
		
		All[idx] = reinterpret_cast<T *>(this);;
		m_uniqueid = idx;
	}

public:
	// Values per cache item
	int			 getCacheID()			{return m_uniqueid;}
	int			 getCacheIndex()
	{
		int iCount = 0;
		for (int i = 0; i < NextUniqueIndex; i++)
		{
			if (All[i])
			{
				++iCount;
				if (All[i] == this)
					return iCount -1;
			}
		}
		return -1;
	}

	// How many things are there ?
	static int	 getCacheSize()			{return NextUniqueIndex;}
	static int	 getCount()
	{
		int iCount = 0;
		for (int i = 0; i < NextUniqueIndex; i++)
		{
			if (All[i])
				++iCount;
		}
		return iCount;
	}

	// Get back a particular one
	static T	*getByCacheID(int idx)	{return idx < 0 ? 0 : All[idx];}
	static T	*getByCacheIndex(int idx)
	{
		if (idx < 0)
			return 0;

		for (int i = 0; i < NextUniqueIndex; i++)
		{
			if (All[i] && idx-- == 0)
				return All[i];
		}

		return 0;
	}

	int operator == ( const Cache<T>& rhs) const
	{
		return m_uniqueid == rhs.m_uniqueid;
	}

};

#endif // _SYS_EXTRA_H_
