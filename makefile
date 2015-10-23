GCC_OPTIONS=-Wall -pedantic -I include
GL_OPTIONS=-lGLEW -lGL -lglut
OPTIONS=$(GCC_OPTIONS) $(GL_OPTIONS)

part1:
	g++ $@.cpp Common/InitShader.cpp $(OPTIONS) -o $@

part2:
	g++ $@.cpp Common/InitShader.cpp $(OPTIONS) -o $@

clean:
	rm part1 part2

