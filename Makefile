ALLEGRO_VERSION=5.0.10
MINGW_VERSION=4.7.0
FOLDER=D:\Courses\00_SistemaInformacao\2020.2\DCC203-PDSI\TP

FOLDER_NAME=\allegro-$(ALLEGRO_VERSION)-mingw-$(MINGW_VERSION)
PATH_ALLEGRO=$(FOLDER)$(FOLDER_NAME)
LIB_ALLEGRO=\lib\liballegro-$(ALLEGRO_VERSION)-monolith-mt.a
INCLUDE_ALLEGRO=\include

all: combat.exe

combat.exe: combat.o
	gcc -o combat.exe combat.o $(PATH_ALLEGRO)$(LIB_ALLEGRO)

combat.o: combat.c
	gcc -I $(PATH_ALLEGRO)$(INCLUDE_ALLEGRO) -c combat.c	
	
	
clean:
	del combat.o
	del combat.exe