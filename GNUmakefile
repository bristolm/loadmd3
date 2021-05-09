# top level makefile

include GNUMaster

.PHONY: libs clean LWOtoMD3
VALUE = nothing

libs:
	cd objQuake ; make
	cd objUnreal ; make
	cd objLW ; make

clean:
	-cd objQuake ; make clean
	-cd objUnreal ; make clean
	-cd objLW ; make clean
	-cd LWOtoMD3 ; make clean

LWOtoMD3:
	cd LWOtoMD3 ; make


