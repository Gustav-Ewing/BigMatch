


cmake commands to compile and then run tested on linux

These commands setup up the build directory
	mkdir build
	cd build
	cmake ../				sets current directory as the output target and the parent directory as the source

Compiling and running it from the python folder with L = 10 if no number is specified it defaults to no L value
	cmake --build build		compiles the project into the executable Matching which should platform specific e.g. Matching on linux and Matching.exe on windows
	build/Matching 10		executing from the duvignau-adapt_code/python directory is advised since otherwise a lot of python code just breaks and it's a huge headache to fix it