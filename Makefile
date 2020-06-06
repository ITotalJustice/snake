# TotalJustice

EXE			= snake

SRC			= ./source

# Main source file.
SOURCES 	= main.c util.c

SOURCES 	+= snake.c snake_poll.c snake_update.c snake_render.c snake_util.c

#-DSDL2 #-DALLEGRO

# SDL2 libs
LIBS		+= `sdl2-config --libs`

# ALLEGRO libs
LIBS		+= -lallegro -lallegro_primitives -lallegro_font -lallegro_ttf

RELEASE		= -O3 -march=native -DNDEBUG

CXXFLAGS	+= -Wall -Wformat #$(RELEASE)

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