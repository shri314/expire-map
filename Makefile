all: test

a.out: test.cpp expire_map.h util.h
	g++-7 -o $@ -std=c++17 -O3 $< -pthread

s.out: test.cpp expire_map.h util.h
	g++-7 -o $@ -std=c++17 -O3 $< -pthread -g -DNDEBUG -fsanitize=thread

test: a.out s.out
	./a.out #run unit test
	./s.out #run with thread sanitizer
	@echo PASS

html: test.cpp expire_map.h util.h Makefile
	rm -rf html
	mkdir -p html
	for i in $^; do vim +TOhtml +xa $$i; mv $$i.html html; done
	for i in $^; do sed -i -e "s@\<$$i\>@<a href=\"$$i.html\">$$i</a>@g" html/*.html; done

clean:
	rm -f s.out a.out core* vgcore*
	rm -rf html
