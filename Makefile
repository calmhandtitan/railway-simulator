clean:
	rm -f *.out
	
run: clean
	g++ main.cpp -std=c++11 -pthread
