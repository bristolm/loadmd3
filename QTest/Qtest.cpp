/*
 * Just a test program for my math stuff
 */

#include "GLquat.h"
#include "sys_math.h"
#define TO_RAD   (acos(-1) / 180)

int main (int argc, char *argv[])
{
	Vector<double>	Veuler(	atof(argv[1]) * TO_RAD,
							atof(argv[2]) * TO_RAD,
							atof(argv[3]) * TO_RAD);
	std::cout << "  ================= "<< std::endl;

/*
	Vector<double> f(1,1,0);
	std::cout << f << std::endl;
	std::cout << std::endl;

	Matrix<double> m(	Vector<double>(1,2,3),
						Vector<double>(4,5,6),
						Vector<double>(7,8,9)
					);
	std::cout << m << std::endl;
	std::cout << std::endl;

	Matrix<double> mINV = ~m;
	std::cout << mINV << std::endl;
	std::cout << std::endl;

	Matrix<double>	Xform(	Vector<double>(0,-1,0),
							Vector<double>(0,0,1),
							Vector<double>(1,0,0));
	Vector<double> fTest = Xform * f;
	std::cout << fTest << std::endl;
	std::cout << std::endl;

	fTest = ~Xform * f;

	std::cout << fTest << std::endl;
	std::cout << std::endl;

	// Euler angles to Quaternion
	std::cout << "Start w/ Euler: " << std::endl;
	std::cout << "         " << Veuler << std::endl;
	std::cout << "         " << Veuler / TO_RAD << std::endl;

	// Make a quat
	std::cout << "Quat from Euler vector ----------------" << std::endl;
	Quaternion<double> Qeuler1 = Quaternion<double>(Veuler);
	std::cout << ">Quat :  " << Qeuler1 << std::endl;

	// Compare
	GL_QUAT test;
	gluEulerToQuat_EXT(Veuler[0],Veuler[1],Veuler[2],&test);
	Quaternion<double> Qtest = Quaternion<double>(test.x,test.y,test.z,test.w);

	std::cout << " == ??:  " << Qtest << std::endl;

	// ... And back
	std::cout << " Matrx:  " << Qeuler1.getMatrix() << std::endl;
	std::cout << "  Back:  " << Qeuler1.getMatrix().getEuler() / TO_RAD  << std::endl;
	std::cout << " Back2:  " << Qeuler1.getEuler() / TO_RAD  << std::endl;
	std::cout << std::endl;

	// Make the quat differently (need a normalized vector)
	std::cout << "3 Quats muliplied (from Euler angles) -" << std::endl;

	// Multiplying by Heading (z), Pitch (y) , Roll (x)
	Quaternion<double> Qeuler2 = (
						Quaternion<double>(0.0, 0.0, sin(Veuler[2] / 2.0),
											cos(Veuler[2] / 2.0))
					*	Quaternion<double>(	0.0, sin(Veuler[1] / 2.0), 0.0,
											cos(Veuler[1] / 2.0))
					*	Quaternion<double>(	sin(Veuler[0] / 2.0), 0.0, 0.0,
											cos(Veuler[0] / 2.0))
											);
	std::cout << ">Quat :  " << Qeuler2 << std::endl;

	// And back
	std::cout << " Matrx:  " << Qeuler2.getMatrix() << std::endl;
	std::cout << "  Back:  " << Qeuler2.getMatrix().getEuler() / TO_RAD  << std::endl;
	std::cout << " Back2:  " << Qeuler2.getEuler() / TO_RAD  << std::endl;
	std::cout << std::endl;

	std::cout << "---------------------------------------" << std::endl;
*/

	// Comes in as LW coordinates hpb == yxz
	Matrix<float>	LW_TO_UNSKEL_Coords(	Vector<float>(0,-1,0),
											Vector<float>(0,0,1),
											Vector<float>(1,0,0));
	// Flip and orient properly
	Vector<float>	v_lw(Veuler[1],Veuler[0],Veuler[2]);
	Vector<float>	v_unskel(v_lw[2],-v_lw[0],v_lw[1]);
//	v_unskel = LW_TO_UNSKEL_Coords * v_unskel;


	// To 3 Quats and multiply Heading *  Pitch * Bearing in Unreal
	// Multiplying by Heading (z), Pitch (y) , Roll (x)
//	Quaternion<float> q_unskel = Quaternion<float>(m_unskel);

	Quaternion<float> Qx = Quaternion<float>(	sin(v_unskel[0] / 2.0), 0.0, 0.0,
												cos(v_unskel[0] / 2.0));
	Quaternion<float> Qy = Quaternion<float>(	0.0, sin(v_unskel[1] / 2.0), 0.0,
												cos(v_unskel[1] / 2.0));
	Quaternion<float> Qz = Quaternion<float>(	0.0, 0.0, sin(v_unskel[2] / 2.0),
												cos(v_unskel[2] / 2.0));

	Quaternion<float> Q = (Qx * Qy * Qz);

	// Multiply in order - Z,Y,X
	std::cout << "LW test: " << Veuler / TO_RAD << std::endl;
	std::cout << "         " << v_lw / TO_RAD << std::endl;
	std::cout << "        (" << v_unskel / TO_RAD << ")" << std::endl;
	std::cout << ">Quat :  " << Q << std::endl;

	// Try what I think is the unreal angles
	std::cout << ">USkel:  " << Q.getMatrix().getEuler() / TO_RAD << std::endl;

	// And back out to LW
	Matrix<float>	Mrot = Q.getMatrix();
	Vector<float>	Vrot = (~LW_TO_UNSKEL_Coords) * (~Mrot).getEuler();

	v_lw = Vector<float>(-Vrot[1], -Vrot[0], -Vrot[2]);
	std::cout << "LW HPB:  " << v_lw / TO_RAD << std::endl;

	return 0;
}