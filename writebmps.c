#include "inc/oscar.h"
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "leanXtools.h"
#include "leanXip.h"

int main(const int argc, const char * argv[])
{
	void *hFramework;
	
	static uint8 colorPic[3 * OSC_CAM_MAX_IMAGE_WIDTH * OSC_CAM_MAX_IMAGE_HEIGHT];
	
	struct OSC_PICTURE pic;
	
	int i=0;	
	int retval;
	char filename[100];
	
	OscCreate(&hFramework);

	OscBmpCreate(hFramework);
	
	/* setup Picture to write to file */
	pic.width = OSC_CAM_MAX_IMAGE_WIDTH;
	pic.height = OSC_CAM_MAX_IMAGE_HEIGHT;
	pic.type = OSC_PICTURE_GREYSCALE;
	pic.data = colorPic;

	while(1) {
		retval=fread(colorPic, pic.width*pic.height*3, 1, stdin);
		if (retval >0) {
			sprintf(filename, "movie%06i.bmp", i++);
			OscBmpWrite(&pic, filename);
			printf("Wrote file %s\n",filename);
		}
	}
	
	/* Destroy modules */
	OscBmpDestroy(hFramework);
	
	/* Destroy framework */
	OscDestroy(hFramework);
	
	return 0;
}
