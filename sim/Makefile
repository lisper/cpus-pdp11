#
#

all: pi dis

isn.h: isn.txt maketables.pl
	./maketables.pl >isn.h

pi: pi.c isn.c isn.h
	cc -o pi pi.c isn.c

dis: dis.c isn.c isn.h
	cc -o dis dis.c isn.c
