#
# MPSSE and BITBANG code Makefile for MacOS X
# Hiroki Mori 2010/05/04
#

FT_DXX_LIB=../FT_D2XX_Lib.1.2.2/D2XX/bin/10.5-10.7
FT_DXX_INC=../FT_D2XX_Lib.1.2.2/D2XX/Samples

OBJ = mpsse bitbang jtagtest pracc wrt54g spireadid hjtag jtagscan exlda exkey

all: $(OBJ)

mpsse: main.c libftd2xx.dylib
	gcc -o mpsse main.c -I$(FT_DXX_INC) -L. -lftd2xx 

bitbang: bitbang.c libftd2xx.dylib
	gcc -o bitbang bitbang.c -I$(FT_DXX_INC) -L. -lftd2xx 

exlda: exlda.c libftd2xx.dylib
	gcc -o exlda exlda.c -I$(FT_DXX_INC) -L. -lftd2xx 

exkey: exkey.c libftd2xx.dylib
	gcc -o exkey exkey.c -I$(FT_DXX_INC) -L. -lftd2xx 

jtagtest: jtagtest.c libftd2xx.dylib
	gcc -o jtagtest jtagtest.c -DFT2232 -I$(FT_DXX_INC) -L. -lftd2xx 

jtagscan: jtagscan.c libftd2xx.dylib
	gcc -o jtagscan jtagscan.c -DFTBITBANG -I$(FT_DXX_INC) -L. -lftd2xx 

hjtag: hjtag.c libftd2xx.dylib
	gcc -o hjtag hjtag.c -DFT2232 -I$(FT_DXX_INC) -L. -lftd2xx 

pracc: pracc.c libftd2xx.dylib
	gcc -o pracc pracc.c -DFT2232 -I$(FT_DXX_INC) -L. -lftd2xx 

wrt54g: wrt54g.c libftd2xx.dylib
	gcc -o wrt54g wrt54g.c -DFT2232 -I$(FT_DXX_INC) -L. -lftd2xx 

spireadid: spireadid.c libftd2xx.dylib
	gcc -o spireadid spireadid.c -I$(FT_DXX_INC) -L. -lftd2xx 

libftd2xx.dylib:
	cp $(FT_DXX_LIB)/libftd2xx.*.dylib libftd2xx.dylib
	install_name_tool -id @executable_path/libftd2xx.dylib libftd2xx.dylib

clean:
	rm -rf libftd2xx.dylib $(OBJ)
