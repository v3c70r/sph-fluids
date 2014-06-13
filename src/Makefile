#includes
VSL_INC=-I./VSL/VSL/include
INCLUDES=$(VSL_INC)
#libs
OPENGL_LIBS=-lGL -lGLU -lglut -lGLEW
IL_LIBS=-lIL
TINYXML_LIBS = -ltinyxml
ASSIMP_LIBS = `pkg-config --libs assimp`
LIBS=$(OPENGL_LIBS) $(IL_LIBS) $(TINYXML_LIBS) $(ASSIMP_LIBS)

CFLAGS= -std=c++11 -g -Wall -DNDEBUG 
CC = g++

SOURCE_FILES=\
			 ./sph.cpp\
			 ./glWater.cpp\
			 ./VSL/VSL/source/vsLogLib.cpp\
			 ./VSL/VSL/source/vsProfileLib.cpp\
			 ./VSL/VSL/source/vsMathLib.cpp\
			 ./VSL/VSL/source/vsResModelLib.cpp\
			 ./VSL/VSL/source/vsGLInfoLib.cpp\
			 ./VSL/VSL/source/vsFontLib.cpp\
			 ./VSL/VSL/source/vsShaderLib.cpp\
			 ./VSL/VSL/source/vsResourceLib.cpp\

ALL_OBJECTS=\
	$(patsubst %.cpp,%.o,$(SOURCE_FILES))

DEPENDENCY_FILES=\
	$(patsubst %.o,%.d,$(ALL_OBJECTS))


all: ../bin/sph

%.o: %.cpp Makefile
	@# Make dependecy file
	$(CXX) -MM -MT $@ -MF $(patsubst %.cpp,%.d,$<) $(CFLAGS) $(INCLUDES) $<
	@# Compile
	$(CXX) $(CFLAGS) -c -o $@ $< $(INCLUDES)

-include $(DEPENDENCY_FILES)

../bin/sph: $(ALL_OBJECTS)	
	$(CXX) $(LDFLAGS)  -o $@ $(ALL_OBJECTS) $(LIBS)

.PHONY: clean
clean:
	rm -r ./VSL/VSL/source/*.o && rm ./*.o && rm -r ./VSL/VSL/source/*.d && rm ./*.d


