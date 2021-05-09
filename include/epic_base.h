#ifndef _EPIC_BASE_H_
#define _EPIC_BASE_H_

// **************************************************************
// Code from UnSkeletal.h (c) 1999,2000 Epic MegaGames, Inc
/*=============================================================================

	UnSkeletal.h: General ActorXporter support functions, misc math classes ripped-out from Unreal.

    Copyright (c) 1999,2000 Epic MegaGames, Inc. All Rights Reserved.

=============================================================================*/
#define DELTA			(0.00001)

// Unsigned base types.
typedef unsigned char		BYTE;		// 8-bit  unsigned.
typedef unsigned short		_WORD;		// 16-bit unsigned.
typedef unsigned long		DWORD;		// 32-bit unsigned.
//typedef unsigned __int64	QWORD;		// 64-bit unsigned.

// Signed base types.
typedef	signed char			SBYTE;		// 8-bit  signed.
typedef signed short		SWORD;		// 16-bit signed.
typedef signed int  		INT;		// 32-bit signed.
//typedef signed __int64		SQWORD;		// 64-bit signed.

// Character types.
typedef char			    ANSICHAR;	// An ANSI character.
typedef unsigned short      UNICHAR;	// A unicode character.

// Other base types.
typedef signed int			UBOOL;		// Boolean 0 (false) or 1 (true).
typedef float				FLOAT;		// 32-bit IEEE floating point.
typedef double				DOUBLE;		// 64-bit IEEE double.
typedef unsigned long       SIZE_T;     // Corresponds to C SIZE_T.


/*-----------------------------------------------------------------------------
	FQuat.          
-----------------------------------------------------------------------------*/

//
// floating point quaternion.
//
class FQuat 
{
	public:
	// Variables.
	FLOAT X,Y,Z,W;

	// Constructors.
	FQuat()
	{}

	FQuat( FLOAT InX, FLOAT InY, FLOAT InZ, FLOAT InA )
	:	X(InX), Y(InY), Z(InZ), W(InA)
	{}


	// Binary operators.
	FQuat operator+( const FQuat& Q ) const
	{
		return FQuat( X + Q.X, Y + Q.Y, Z + Q.Z, W + Q.W );
	}

	FQuat operator-( const FQuat& Q ) const
	{
		return FQuat( X - Q.X, Y - Q.Y, Z - Q.Z, W - Q.W );
	}

	FQuat operator*( const FQuat& Q ) const
	{
		return FQuat( 
			X*Q.X - Y*Q.Y - Z*Q.Z - W*Q.W, 
			X*Q.Y + Y*Q.X + Z*Q.W - W*Q.Z, 
			X*Q.Z - Y*Q.W + Z*Q.X + W*Q.Y, 
			X*Q.W + Y*Q.Z - Z*Q.Y + W*Q.X
			);
	}

	FQuat operator*( const FLOAT& Scale ) const
	{
		return FQuat( Scale*X, Scale*Y, Scale*Z, Scale*W);			
	}
	
	// Unary operators.
	FQuat operator-() const
	{
		return FQuat( X, Y, Z, -W );
	}

    // Misc operators
	UBOOL operator!=( const FQuat& Q ) const
	{
		return X!=Q.X || Y!=Q.Y || Z!=Q.Z || W!=Q.W;
	}
	
	UBOOL Normalize()
	{
		// 
		FLOAT SquareSum = (FLOAT)(X*X+Y*Y+Z*Z+W*W);
		if( SquareSum >= DELTA )
		{
			FLOAT Scale = 1.0f/(FLOAT)sqrt(SquareSum);
			X *= Scale; 
			Y *= Scale; 
			Z *= Scale;
			W *= Scale;
			return true;
		}
		else 
		{	
			X = 0.0f;
			Y = 0.0f;
			Z = 0.1f;
			W = 0.0f;
			return false;
		}
	}

	/*
    // Angle scaling.
	FQuat operator*( FLOAT Scale, const FQuat& Q )
	{
		return FQuat( Q.X, Q.Y, Q.Z, Q.W * Scale );
	}
	*/

	// friends
	// friend FAngAxis	FQuatToFAngAxis(const FQuat& Q);
	// friend void SlerpQuat(const FQuat& Q1, const FQuat& Q2, FLOAT slerp, FQuat& Result)		
	// friend FQuat	BlendQuatWith(const FQuat& Q1, FQuat& Q2, FLOAT Blend)
	// friend  FLOAT FQuatDot(const FQuat&1, FQuat& Q2);
};

#endif // _EPIC_BASE_H_