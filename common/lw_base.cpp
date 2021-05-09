#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lw_base.h"
#include "lwobjimp.h"
#include "sys_base.h"

extern Vector<float>	i(0,-1,0);
extern Vector<float>	j(0,0,1);
extern Vector<float>	k(1,0,0);

extern Matrix<float>	LW_TO_MD3_Coords(i,j,k);