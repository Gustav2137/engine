thc:
	g++ thc.cpp -c -o thc.o -O3
thcg:
	g++ thc.cpp -c -o thc.o -g
r: thc
	g++ zad7.cpp thc.o -o zad7 -std=c++20 -O3
d: thcg
	g++ zad7.cpp thc.o -o zad7 -std=c++20 -g
b: thc
	g++ better.cpp thc.o -o better -std=c++20 -O3
db: thcg
	g++ better.cpp thc.o -o better -std=c++20 -g
m: thc
	g++ multi.cpp thc.o -o multi -std=c++20 -O3
mb: thcg
	g++ multi.cpp thc.o -o multi -std=c++20 -g
