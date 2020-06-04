#include "includes.h"
#include "snake.h"

#define STRMATCH(x,y) ((strcmp(x,y) == 0))

int main(int argc, char *argv[])
{
    fprintf(stdout, "hello world\n");
    
    /// handle cmd args.
    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            /// load save
            if (STRMATCH("--loadsave", argv[i]))
            {
                /// check that we have a string after load.
                if (i + 1 != argc)
                {
                    ++i;
                    fprintf(stdout, "loading save: %s\n", argv[i]);
                }
                else
                {
                    fprintf(stderr, "no arg given after using %s\n", argv[i]);
                }
            }

            else if (STRMATCH("--norend", argv[i]))
            {

            }
        }
    }

    snake_play();
    
    return 0;
}