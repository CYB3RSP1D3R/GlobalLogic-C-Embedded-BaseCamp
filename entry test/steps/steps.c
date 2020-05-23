 
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
        int steps = atoi(argv[1]);
        int stepskip = atoi(argv[2]) + 1;
        int stepscount = 0;

        if (steps % stepskip != 0) {
                steps -= steps % stepskip;
                ++stepscount;
        }

        stepscount += steps/stepskip;
        printf("%i\n", stepscount);
        return 0;
}