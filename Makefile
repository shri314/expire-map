all: test

a.out: test.cpp expire_map.h util.h
	g++-7 -o $@ -std=c++17 -O3 test.cpp -pthread

s.out: test.cpp expire_map.h util.h
	g++-7 -o $@ -std=c++17 -O3 test.cpp -pthread -g -DNDEBUG -fsanitize=thread

test: a.out s.out
	./a.out #run unit test
	./s.out #run with thread sanitizer
	@echo PASS

clean:
	rm -f s.out a.out core* vgcore*
