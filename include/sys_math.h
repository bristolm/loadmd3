/**************************************
 *
 *  sys_math.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  math objects - Vector, Matrix, Quaternion
 *
 *  All the stuff barbie hates is from the Matrix and
 *  Quaternion FAQ - any search engine will give you a
 *  number of places to find it
 *
 **************************************/

#ifndef _SYS_MATH_H_
#define _SYS_MATH_H_

#include "sys_base.h"
#include "math.h"

#include <iostream>

namespace MRB
{
	// ----------------------------
	// CONSTANTS
	// ----------------------------
	static const int WAY_TOO_SMALL = 0.000001;

	// ----------------------------
	// Vector template class
	// ----------------------------
	template <typename T>
	class Vector
	{
	protected:
		T v[3];
	public:
		Vector(T i, T j, T k)
		{
			v[0] = i;
			v[1] = j;
			v[2] = k;
		}
		Vector(T in)
		{
			v[0] = v[1] = v[2] = in;
		}
		Vector(T *in)
		{
			v[0] = in[0];
			v[1] = in[1];
			v[2] = in[2];
		}
		Vector(void)
		{
			v[0] = v[1] = v[2] = 0;
		}
		Vector(const Vector<T>& rhs)
		{
			v[0] = rhs.v[0];
			v[1] = rhs.v[1];
			v[2] = rhs.v[2];
		}

		inline T	X()		{return v[0];}
		inline T	Y()		{return v[1];}
		inline T	Z()		{return v[2];}

		// Return a value
		T& operator [] (unsigned int idx )
		{	// TTT - should be error checking
			return v[idx];
		}

		// Fill an ara of T with our values
		void fillarray(T *in)
		{
			in[0] = v[0];
			in[1] = v[1];
			in[2] = v[2];
		}

		// Compare 2 vectors
		int operator == ( const Vector<T>& rhs) const
		{
			return (v[0] == rhs.v[0]
					&& v[1] == rhs.v[1]
					&& v[2] == rhs.v[2]);
		}

		// Add me to another one
		Vector<T> & operator += (const Vector<T> & rhs )
		{
			if (this != &rhs)
			{
				v[0] += rhs.v[0];
				v[1] += rhs.v[1];
				v[2] += rhs.v[2];
			}

			return *this;
		}

		// Add 2 vectors
		Vector<T> operator + (const Vector<T> & rhs )
		{
			return Vector<T>(v[0] + rhs.v[0],
							 v[1] + rhs.v[1],
							 v[2] + rhs.v[2]);
		}

		// Subtract 2 vectors
		Vector<T> operator - (const Vector<T> & rhs )
		{
			return Vector<T>(v[0] - rhs.v[0],
							 v[1] - rhs.v[1],
							 v[2] - rhs.v[2]);
		}

		// Multiply by a constant value
		Vector<T> operator * (const T rhs )
		{
			Vector<T>	a;

			a[0] = v[0] * rhs;
			a[1] = v[1] * rhs;
			a[2] = v[2] * rhs;

			return a;
		}

		// Multiply myself by a constant value
		Vector<T> operator *= (const T rhs )
		{
			v[0] *= rhs;
			v[1] *= rhs;
			v[2] *= rhs;

			return *this;
		}

		// Divide by a constant value
		Vector<T> operator / (const T rhs )
		{
			Vector<T>	a;

			a[0] = v[0] / rhs;
			a[1] = v[1] / rhs;
			a[2] = v[2] / rhs;

			return a;
		}

		// Divide myself by a constant value
		Vector<T> operator /= (const T rhs )
		{
			v[0] /= rhs;
			v[1] /= rhs;
			v[2] /= rhs;

			return *this;
		}

		// Set equal to a item of the same type
		Vector<T> & operator = (const Vector<T> & rhs )
		{
			if (this != &rhs)
			{
				v[0] = rhs.v[0];
				v[1] = rhs.v[1];
				v[2] = rhs.v[2];
			}

			return *this;
		}

		// Set equal to a constant value (like 0)
		Vector<T> & operator = (const T rhs )
		{
			v[0] = (T)rhs;
			v[1] = (T)rhs;
			v[2] = (T)rhs;

			return *this;
		}

		// Cross 2 vectors
		Vector<T> cross( Vector<T> & rhs )
		{
			Vector<T> res;
			res[0] = v[1]*rhs[2] - v[2]*rhs[1];
			res[1] = v[2]*rhs[0] - v[0]*rhs[2];
			res[2] = v[0]*rhs[1] - v[1]*rhs[0];

			return res.clean();
		}

		// Normalize
		Vector<T> normalize()
		{
			T m = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
			if (fabs(m) <= WAY_TOO_SMALL)
				return *this;
		
			m = (T)(1.0 / sqrt(m));

			Vector<T> res(	v[0] * m,
							v[1] * m,
							v[2] * m);

			return res.clean();
		}

		// clean
		Vector<T> clean()
		{
			Vector<T> res;

			res[0] = (fabs(v[0]) <= WAY_TOO_SMALL) ? 0 : v[0];
			res[1] = (fabs(v[1]) <= WAY_TOO_SMALL) ? 0 : v[1];
			res[2] = (fabs(v[2]) <= WAY_TOO_SMALL) ? 0 : v[2];

			return res;
		}

		// Spit out the Vector
		friend std::ostream& operator<< (std::ostream& os, const Vector<T>& out)
		{
			os << "[ " << out.v[0] << " , " << out.v[1] << " , " << out.v[2] << " ]";
			return os;
		}
	};

	// ----------------------------
	// Matrix template class
	// ----------------------------
	template <typename T>
	class Matrix :
		public Vector< Vector<T> >
	{
	public:
		Matrix():
		  Vector< Vector<T> >()
		{;}

		Matrix(Vector<T> i,Vector<T> j,Vector<T> k):
		  Vector< Vector<T> > (i,j,k)
		{;}

		// Treat this as a vector of Euler angles to multiply together in order
		Matrix(Vector<T> euler) :
		  Vector< Vector<T> >()
		{
			double A = cos(euler.X());
			double B = sin(euler.X());
			double C = cos(euler.Y());
			double D = sin(euler.Y());
			double E = cos(euler.Z());
			double F = sin(euler.Z());

			double AD = A * D;
			double BD = B * D;

			v[0][0] =  C * E;
			v[1][0] = -C * F;
			v[2][0] =  D;

			v[0][1] = -(BD * E) + (A * F);
			v[1][1] = -(BD * F) + (A * E);
			v[2][1] = - B  * C;

			v[0][2] = -(AD * E) + (B * F);
			v[1][2] =  (AD * F) + (B * E);
			v[2][2] =   A  * C;

			for(int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 3; j++)
				{
					if (v[i][j] < (T)0.0001 && v[i][j] > -(T)0.0001)
						v[i][j] = (T)0;
				}
			}
		}

		  // Add 2 Matrices
		Matrix<T> operator + (const Matrix<T> & rhs )
		{
			return Matrix<T>(v[0] + rhs.v[0],
							 v[1] + rhs.v[1],
							 v[2] + rhs.v[2]);
		}

		// Subtract 2 Matrices
		Matrix<T> operator - (const Matrix<T> & rhs )
		{
			return Matrix<T>(v[0] - rhs.v[0],
							 v[1] - rhs.v[1],
							 v[2] - rhs.v[2]);
		}

		// Multiply 2 Matrices
		Matrix<T> operator * ( Matrix<T> & rhs )
		{
			Matrix<T> ret;
			for(int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 3; j++)
				{
					for (int m = 0; m < 3; m++)
					{
						ret[i][j] += (v[i][m] * rhs.v[m][j]);
					}
				}
			}
			return ret;
		}

		// Rotate Vector by myself
		Vector<T> rotate( Vector<T>& in)
		{
			Vector<T> result;
			for (int i = 0; i < 3; i ++)
			{	// Rotate  rotation matrix
				result[i] = (in[0] * v[0][i] +
							 in[1] * v[1][i] + 
							 in[2] * v[2][i]		);
			}
			return result;
		}

		// Return Transposed Matrix
		Matrix<T> operator ~ (void)
		{
			Matrix<T> res;
			for (int i = 0; i < 3; i ++)
			{
				for (int j = 0; j < 3; j++)
				{
					res.v[i][j] = v[j][i];
				}
			}

			return res;
		}

		// Rotate Vector by myself
		Vector<T> operator * ( Vector<T> & in )
		{
			Vector<T> result;
			for (int i = 0; i < 3; i ++)
			{	// Rotate as if we are a rotation matrix
				result[i] = (in[0] * v[0][i] +
							 in[1] * v[1][i] + 
							 in[2] * v[2][i]		);
			}
			return result;
		}

		// Treating my 3 vectors as a plane, get the normal vector
		Vector<T> getNormal()			
		{
			Vector<T> a, b, c;

			for (int z = 0; z < 3;z++)
			{
				a[z] = v[1][z] - v[0][z];
				b[z] = v[2][z] - v[0][z];
			}

			// Cross product
			c = a.cross(b);

			T len = (T)sqrt(	(c[0] * c[0]) + 
								(c[1] * c[1]) +
								(c[2] * c[2]) );

			return Vector<T>((c[0]/len), (c[1]/len) ,(c[2]/len));
		}

		// Try and clean up some rounding errors
		Matrix<T> clean()
		{
			Matrix<T> res;
			for (int i = 0; i < 3; i ++)
			{
				for (int j = 0; j < 3; j++)
				{
					if (fabs(((T)((int)v[i][j])) - v[i][j]) < 0.0001)
					{
						res[i][j] = (T)((int)v[i][j]);
					}
					else
					{
						res[i][j] = v[i][j];					
					}
				}
			}
			return res;
		}

		// Return euler angles
		Vector<T> getEuler()
		{
			Vector<double> ret;
			ret[1] = -asin( v[2][0] );
			double tmp =	cos(ret[1]);

			if (fabs( tmp ) > 0.00005)
			{
				ret[0]  = atan2( (-v[2].Y() / tmp), ( v[2].Z() / tmp) );
				ret[2]  = atan2( (-v[1].X() / tmp), ( v[0].X() / tmp) );
			}
			else
			{	// Gimbal lock
				ret[0] = 0;
				ret[2] = atan2( v[0].Y(), v[1].Y());
			}

			tmp = acos(-1) *2;
			for (int i = 0; i < 3; i++)
			{
				if (ret[i] < 0)
					ret[i] += tmp;
				if (ret[i] >= tmp)
					ret[i] -= tmp;
			}
			
			return Vector<T> ((T)ret[0],(T)ret[1],(T)ret[2]);
		}
	};

	// ----------------------------
	// Quaternion template class
	// ----------------------------
	template <typename T>
	class Quaternion
	{
		T		v[4];

	public:
		Quaternion()
		{
			v[0] = (T)0;
			v[1] = (T)0;
			v[2] = (T)0;
			v[3] = (T)0;
		}

		// Create from 4 values
		Quaternion(T x, T y, T z, T w)
		{
			v[0] = x;
			v[1] = y;
			v[2] = z;
			v[3] = w;
		}

		// Create from a directional vector, and an angle
		Quaternion(Vector<T> vec, T w)
		{
			v[0] = vec.X();
			v[1] = vec.Y();
			v[2] = vec.Z();
			v[3] = w;
		}

		inline T	X()		{return v[0];}
		inline T	Y()		{return v[1];}
		inline T	Z()		{return v[2];}
		inline T	W()		{return v[3];}

		// Treat this as a Euler angle vector
		Quaternion(Vector<T> euler)
		{
			Vector<double> Vcos(cos(euler.X()/2),
								cos(euler.Y()/2),
								cos(euler.Z()/2));

			Vector<double> Vsin(sin(euler.X()/2),
								sin(euler.Y()/2),
								sin(euler.Z()/2));

			for (int i = 0; i < 4; i ++)
			{
				double tmpa = 1.0;
				double tmpb = 1.0;
				for (int j = 0; j < 3; j++)
				{
					if (i == j)
					{
						tmpa *= Vsin[j];
						tmpb *= Vcos[j];
					}
					else
					{
						tmpa *= Vcos[j];
						tmpb *= Vsin[j];
					}
				}

				if (i & 0x1)
				{
					v[i] = tmpa + tmpb;
				}
				else
				{
					v[i] = tmpa - tmpb;
				}
			}

			normalize();
		}

		// Treat this as a Rotation matrix
		Quaternion(Matrix<T> rot)
		{
			T t = rot[0].X() + rot[1].Y() +	rot[2].Z();

			if (t > (T)0.00000)
			{	// W is the dominant one
				T s = 0.5 / sqrt(t + 1.0);
				v[0] = (rot[1].Z() - rot[2].Y()) * s;
				v[1] = (rot[2].X() - rot[0].Z()) * s;
				v[2] = (rot[0].Y() - rot[1].X()) * s;
				v[3] = 0.25 / s;
			}
			else if (rot[0].X() > rot[1].Y() && rot[0].X() > rot[2].Z())
			{	// X is dominant
				T s = sqrt(1.0 + rot[0].X() - rot[1].Y() - rot[2].Z()) * 2.0;;
				v[0] = 0.5 / s;
				v[1] = (rot[0].Y() + rot[1].X()) / s;
				v[2] = (rot[2].X() + rot[0].Z()) / s;
				v[3] = (rot[1].Z() + rot[2].Y()) / s;
			}
			else if (rot[1].Y() > rot[2].Z())
			{	// Y is dominant
				T s = sqrt(1.0 + rot[1].Y() - rot[0].X() - rot[2].Z()) * 2.0;
				v[0] = (rot[0].Y() + rot[1].X()) / s;
				v[1] = 0.5 / s;
				v[2] = (rot[1].Z() + rot[2].Y()) / s;
				v[3] = (rot[2].X() + rot[0].Z()) / s;
			}
			else
			{	// Z is dominant
				T s = sqrt(1.0 + rot[2].Z() - rot[0].X() - rot[1].Y()) * 2.0;
				v[0] = (rot[2].X() + rot[0].Z()) / s;
				v[1] = (rot[1].Z() + rot[2].Y()) / s;
				v[2] = 0.5 / s;
				v[3] = (rot[0].Y() + rot[1].X()) / s;
			}

			normalize();
		}


		// Multiplying 2 Quaternions
		/*
		res->x = q1->w * q2->x + q1->x * q2->w + q1->y * q2->z - q1->z * q2->y;
		res->y = q1->w * q2->y + q1->y * q2->w + q1->z * q2->x - q1->x * q2->z;
		res->z = q1->w * q2->z + q1->z * q2->w + q1->x * q2->y - q1->y * q2->x;
		res->w = q1->w * q2->w - q1->x * q2->x - q1->y * q2->y - q1->z * q2->z;
		*/
		Quaternion<T> operator * (const Quaternion<T> & rhs )
		{
			return Quaternion<T>(
				(v[3]*rhs.v[0]) + (v[0]*rhs.v[3]) +  (v[1]*rhs.v[2]) - (v[2]*rhs.v[1]),
				(v[3]*rhs.v[1]) - (v[0]*rhs.v[2]) +  (v[1]*rhs.v[3]) + (v[2]*rhs.v[0]),
				(v[3]*rhs.v[2]) + (v[0]*rhs.v[1]) -  (v[1]*rhs.v[0]) + (v[2]*rhs.v[3]),

				(v[3]*rhs.v[3]) - (v[0]*rhs.v[0]) -  (v[1]*rhs.v[1]) - (v[2]*rhs.v[2])
			);
		}

		T& operator [] (unsigned int idx )
		{	// TTT - should be error checking
			return v[idx];
		}

		// Set equal to a item of the same type
		const Quaternion<T> & operator = (const Quaternion<T> & rhs )
		{
			if (this != &rhs)
			{
				v[0] = rhs.v[0];
				v[1] = rhs.v[1];
				v[2] = rhs.v[2];
				v[3] = rhs.v[3];
			}

			return *this;
		}

		// Return a Rotation Matrix designation
		Matrix<T> getMatrix()
		{
			normalize(); 

			T wx	= W() * X();
			T wy	= W() * Y();
			T wz	= W() * Z();

			T xx	= X() * X();
			T xy	= X() * Y();
			T xz	= X() * Z();
			
			T yy	= Y() * Y();
			T yz	= Y() * Z();
			
			T zz	= Z() * Z();

			return Matrix<T> (
				Vector<T>(1 -2 * ( yy + zz ),  -2 * ( xy - wz ),   2 * ( xz + wy )),
				Vector<T>(	-2 * ( xy + wz ),1 -2 * ( xx + zz ),  -2 * ( yz - wx )),
				Vector<T>(	 2 * ( xz - wy ),  -2 * ( yz + wx ),1 -2 * ( xx + yy ))
				).clean();
		}

		// Try and convert to euler angles ...
		Vector<T> getEuler()
		{
			Vector<T> ret;


			return ret;
		}

		// Normalize
		Quaternion<T> normalize()
		{
			T m = v[0]*v[0] + v[1]*v[1] + v[2]*v[2] + v[3]*v[3];
			if (m == 0.0)
				return *this;

			m = (T)(1.0 / sqrt(m));

			for (int i = 0; i < 4; i++)
			{
				v[i] *= m;
				if (fabs(v[i]) < 0.00001)
					v[i] = 0.0;
			}

			return Quaternion<T> (v[0]*m,v[1]*m,v[2]*m,v[3]*m);
		}

		friend std::ostream& operator<< (std::ostream& os, const Quaternion<T>& out)
		{
			os << "[ (" << out.v[0] << "	" << out.v[1];
			os << " , " << out.v[2] << ") , " << out.v[3] << " ]";
			return os;
		}

		// Return reversed Quat
		Quaternion<T> operator ~ (void)
		{
			Quaternion<T> res(-v[0],-v[1],-v[2],v[3]);

			return res;
		}
	};
}

#endif	//_SYS_MATH_H_
