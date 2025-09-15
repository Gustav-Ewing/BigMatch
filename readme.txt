


cmake commands to compile and then run tested on linux

These commands setup up the build directory
	
  mkdir build
	cd build
	cmake ../
    ^sets current directory as the output target and the parent directory as the source

Compiling and running all executables

  Compile the project
    
    cmake --build build
  
  compiles the project into the executables Generate, Greedy, and Double which should be platform specific e.g. Greedy on linux and Greedy.exe on windows.
  The executable is put in the build folder if cmake is set up like above.

  Generate graphs
    
    build/Generate -p 10 -e 100
  
  This sets the producer count to 10 and the amount of edges per chunk to 100 and all other values to the defaults defined in generateGraphx.cxx
  These are the possible flags: -p -> producer count, -c -> consumer count, --SIZE -> size of the graph ie p+c (this is an override for testing), -e -> edges per chunk, -s -> sparsefactor ie how sparse the graph is, -g -> the gamma value to use, -b -> the beta value to use, -w -> the max weight possible for an edge.
  All flags are followed by the value that is to be set to like in the example above.

  Local Greedy algorithm
    
    build/Greedy or build/Greedy filepath
    
      where filepath is the pre 0 name of the file ie Extra/Reuters911/Reuters9110.txt becomes Extra/Reuters911/Retuers911. Probably will make this more intuitive down the line.
      Also that Extra is a folder inside BigMatch in this case so this is a relative filepath in this case.
  This is the single pass Greedy algorithm that reads the chunks extracts the neighborhoods and performs the matching in each neighborhood.

  Local Double Greedy algorithm
    
    build/Double -l 8 -pps 100
  
  Just like Generate this has some more possible flags. In the example above the l-value is set to 8 and the producers per shard is set to 100. The possible flags are: -l -> lvalue sued for neighborhoods, -pps -> max number of producer neighborhoods per shard, -pss -> producer shard size ie how many producer shards can be in memory at any time (slight missnomer), -cps and -css -> same as pps and pss but for consumer shards, -double -> forces usage of the double greedy algorithm but this is already the default, -greedy -> switches to the previous Local Greedy Algorithm but with sharding (mostly just a verification tool for the sharding). These 2 last options do not take a value as they are simple switches.
    
