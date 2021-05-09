/**************************************
 *
 *  sys_base.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  base system stuff
 *
 **************************************/

#define PROG_MAIN_VER	0
#define PROG_MIN_VER	5


#define PROG_PATCH_VER	200
// .95 fixed the memory leaks
// .98 fixed the Unreal orientations
// .101 added the pattern syntax
// .102 changed some unreal stuff - names/flags
// .103 quake3 frame values (radius)
// .200 changes to hub <-> Exporter interface
//      better interface, bloatless info passing
//  added unreal import
//  fixed a lot of Math stuff

#define _DEF_TO_STRING(a)	""#a""
#define BUILD_VERSION(a,b,c)	_DEF_TO_STRING(##a.##b.##c)
#define BUILD_VERSION2(a,b)		_DEF_TO_STRING(##a.##b)
#define BUILD_VERSION1(a)		_DEF_TO_STRING(##a)
#define PROG_VERSION			BUILD_VERSION(PROG_MAIN_VER,PROG_MIN_VER,PROG_PATCH_VER)
#define _DEF_AS_STRING(a)	_DEF_TO_STRING(##a)

#ifndef _BASE_H_
#define _BASE_H_	

#define FORCE_OP		(1 << 0)
#define CURRENT_OP		(1 << 1)

#define INT32	int
#define INT16	short
#define INT8	char

#define UINT32	unsigned int
#define UINT16	unsigned short
#define UINT8	unsigned char

#define SINT32	signed int
#define SINT16	signed short
#define SINT8	signed char

#endif // _BASE_H_

#ifdef __cplusplus
#ifndef _BASE_H_CPP_
#define _BASE_H_CPP_

#include <vector>

// Definitions of classes based on structs and interfaces
#define   WRAP_STRUCT(c)    \
class c : public st##c		\
{							\
public:						\
	static c INVALID;	\
	c()	{memset(this,0,sizeof(c));}			\
	static int sizeofBase() {return sizeof(c);}

namespace MRB
{
	inline void Swap4(unsigned char *buf)
	{
		unsigned char c1 = buf[0];
		unsigned char c2 = buf[1];

		buf[0] = buf[3];
		buf[1] = buf[2];

		buf[2] = c2;
		buf[3] = c1;
	}

	inline void Swap2(unsigned char *buf)
	{
		unsigned char c1 = buf[0];

		buf[0] = buf[1];
		buf[1] = c1;
	}

	inline void Swap32(UINT32 *n)
	{
		union big32
		{
			struct
			{
				UINT8	b1, b2, b3, b4;
			} mask;
			UINT32	ui;
		} u,v;

		u.ui = *n;

		v.mask.b1 = u.mask.b4;
		v.mask.b2 = u.mask.b3;
		v.mask.b3 = u.mask.b2;
		v.mask.b4 = u.mask.b1;

		*n = v.ui;
	}

	// Personal hijacking of the 'Vector' class
	template <class T>
	class auto_vector: public std::vector<T>
	{
		T			_bad;

	public:
		auto_vector(T invalid = T())
		:std::vector<T>(),
			_bad(invalid)
		{;}

		T& operator[](size_type idx)
		{
			if (size() <= idx)
			{
				resize(idx +1);
			}

			return (*(begin() + idx));
		}

		T& at(size_type idx)
		{
			if (size() <= idx)
			{
				return _bad;
			}

			return *(begin() + idx);
		}

		int is_valid(size_type idx)
		{
			if (size() <= idx)
			{
				return 0;
			}

			return (*(begin() + idx) == _bad);
		}

		int is_valid(T& tocheck)
		{
			return (tocheck == _bad);
		}
	};
}
#endif	//_BASE_H_CPP_
#endif	//__cplusplus
