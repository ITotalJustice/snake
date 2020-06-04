# TotalJustice

EXE			= snake

SRC			= ./source

# Main source file.
SOURCES 	= main.c

SOURCES 	+= snake.c

LIBS		= `sdl2-config --libs`

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