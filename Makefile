all :
	gcc -I. -I/usr/X11R6/include -L/usr/X11R6/lib/	-lGL -lGLU \
	-lglut -lm cell3d.c -o cell3d -O2

