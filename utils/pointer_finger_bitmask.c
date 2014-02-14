#include <stdio.h>
#include "../src/pointer_finger.h"

int main(int argc, char *argv[])
{

    int i,j;
    int width = pointer_finger.width;
    int height = pointer_finger.height;
    int bpp = pointer_finger.bytes_per_pixel;

    for(j=0;j<height;j++){
        fprintf(stdout, "\"", j);
        for(i=0;i<width;i++) {
            if( 0 < pointer_finger.pixel_data[j*width*bpp+i*bpp+3]){
                fprintf(stdout, "x");
            } else {
                fprintf(stdout, " ");
            }
        }
        fprintf(stdout, "\"\n");
    }

    return 0;
}
