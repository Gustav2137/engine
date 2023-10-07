thc:
	g++ thc.cpp -c -o thc.o -O3
thcg:
	g++ thc.cpp -c -o thc.o -g
release: thc
	g++ engine.cpp thc.o -O3
debug: thcg
	g++ engine.cpp thc.o -g
