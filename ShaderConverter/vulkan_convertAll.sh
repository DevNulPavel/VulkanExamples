#! /usr/bin/env bash

# -q          dump reflection query database

# -c          configuration dump;
#             creates the default configuration file (redirect to a .conf file

# -g          generate debug informatio

# -H          print human readable form of SPIR-V; turns on -V


rm -rf ../resVulkan
mkdir ../resVulkan

for f in *.vert
do
	glslangValidator -V "$f" -o "../resVulkan/${f//.vert/_v.spv}"
done

for f in *.frag
do
	glslangValidator -V "$f" -o "../resVulkan/${f//.frag/_p.spv}"
done

# glslangValidator -H -V shader.vert -o shader_vert.spv
# glslangValidator -H -V shader.frag -o shader_frag.spv