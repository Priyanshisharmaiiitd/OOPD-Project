target: project.cpp 
	g++ -c project.cpp
	g++ -o main project.o
	./main