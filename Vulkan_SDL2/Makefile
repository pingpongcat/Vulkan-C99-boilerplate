all: main.o vk_mem_alloc.o
	g++ main.o vk_mem_alloc.o -o vulkan -lSDL2 -lvulkan 

main: main.c
	gcc -c main.c -g -Wall

vk_mem_alloc: vk_mem_alloc.cpp
	g++ -c vk_mem_alloc.cpp -g -Wall

shaders:
	glslc shader.vert -o vert.spv
	glslc shader.frag -o frag.spv
clear:
	rm ./*.o ./vulkan
