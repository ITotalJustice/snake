# TotalJustice

EXE			= snake

SRC			= ./source

# Main source file.
SOURCES 	= main.c util.c

SOURCES 	+= snake.c snake_poll.c snake_update.c snake_render.c snake_util.c

# SDL2 libs
#CXXFLAGS	+=	-DSDL2
#LIBS		+= `sdl2-config --static-libs`
#LIBS		+=	/usr/lib/x86_64-linux-gnu/libSDL2.a -Wl,--no-undefined -lm -ldl \
				-lasound -lm -ldl -lpthread -lpulse-simple -lpulse -lX11 -lXext -lXcursor \
				-lXinerama -lXi -lXrandr -lXss -lXxf86vm -lwayland-egl -lwayland-client \
				-lwayland-cursor -lxkbcommon -lpthread -lrt

# ALLEGRO libs
CXXFLAGS	+=	-DALLEGRO
LIBS		+= -lallegro -lallegro_primitives -lallegro_font -lallegro_ttf
#LIBS		+= `pkg-config --libs --static allegro-static-5 \
				allegro_primitives-static-5 allegro_font-static-5 allegro_ttf-static-5`

#RELEASE		= -O3 -march=native -DNDEBUG

CXXFLAGS	+= -Wall -Wformat $(RELEASE)

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
	`strip -s $(EXE)`

clean:
	rm -f $(EXE) $(OBJS)

run: all
	./$(EXE)