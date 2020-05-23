#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv)
{
        int spans_number = atof(argv[1]);
        int distance = atof(argv[2]);

        int total_length = (distance*(spans_number + 1) * spans_number)/2;
        printf("%i\n", total_length);
        
        return 0;
}