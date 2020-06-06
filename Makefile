# TotalJustice

EXE			= snake

SRC			= ./source

# Main source file.
SOURCES 	= main.c

SOURCES 	+= snake.c util.c

# SDL2 libs
LIBS		= `sdl2-config --libs`

# ALLEGRO libs
#LIBS		= -lallegro -lallegro_primitives -lallegro_font -lallegro_ttf

RELEASE		= -O3 -march=native -DNDEBUG

CXXFLAGS	+= -Wall -Wformat -DSDL2 #-DALLEGRO #$(RELEASE)

OBJS		= $(addsuffix .o, $(basename $(notdir $(SOURCES))))

CFLAGS		= $(CXXFLAGS)


##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------
%.o:$(SRC)/%.c
	$(CC) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(EXE)

$(EXE): $(OBJS)
	$(CC) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)

run: all
	./$(EXE)