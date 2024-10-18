


cmake commands to compile and then run tested on linux

These commands setup up the build directory
	mkdir build
	cd build
	cmake ../				sets current directory as the output target and the parent directory as the source

Compiling and running it from the python folder with L = 10 if no number is specified it defaults to no L value
	cmake --build build		compiles the project into the executable Matching which should platform specific e.g. Matching on linux and Matching.exe on windows
	build/Matching 10		executing from the duvignau-adapt_code/python directory is advised since otherwise a lot of python code just breaks and it's a huge headache to fix it


running the code
	to run standard 2221 dataset with normal greedy naive -> build/Matching 0 0
	this causes it to default to that dataset (doesn't work for now though because it relied on old preprocess to load data)

	to run 100k with normal greedy naive -> build/Matching 0 0 "100k-8.bin"
	to run 100k with double greedy naive -> build/Matching 1 0 "100k-8.bin"
	however, the precompute files need to be managed first
	specifally make sure the opts are in bin/opts and costs in bin/costs
	then rename them to remove opt/costs i.e. "costs100k-8.bin" should be "100k-8.bin" while "opt100k.bin" should be "100k-8.bin"
	Will try to unify the binary naming at a later stage but for now it is needed to jump through these hops to run them
