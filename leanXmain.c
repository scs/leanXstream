/*	
	leanXstream, a streaming video server for the LeanXcam.
	Copyright (C) 2009 Reto Baettig
	
	This library is free software; you can redistribute it and/or modify it
	under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation; either version 2.1 of the License, or (at
	your option) any later version.
	
	This library is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
	General Public License for more details.
	
	You should have received a copy of the GNU Lesser General Public License
	along with this library; if not, write to the Free Software Foundation,
	Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "oscar.h"
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "leanXtools.h"
#include "leanXip.h"

#define REG_AEC_AGC_ENABLE 0xaf
#define CAM_REG_RESERVED_0x20 0x20
#define CAM_REG_CHIP_CONTROL 0x07


/*! @brief The system state and buffers of this application. */
struct SYSTEM {
	void *hFileNameReader;

	int32 shutterWidth; /* Microseconds */
	uint8 frameBuffer1[OSC_CAM_MAX_IMAGE_WIDTH * OSC_CAM_MAX_IMAGE_HEIGHT]; 
	uint8 frameBuffer2[OSC_CAM_MAX_IMAGE_WIDTH * OSC_CAM_MAX_IMAGE_HEIGHT];
	uint8 doubleBufferIDs[2]; /* The frame buffer IDs of the frame
				   * buffers creating a double buffer. */
} sys;

/*********************************************************************//*!
 * @brief Initialize framework and system parameters
 *
 * @param s Pointer to the system state 
 *//*********************************************************************/
void initSystem(struct SYSTEM *s)
{
  /******* Create the framework **********/
  OscCreate(	&OscModule_log,
		&OscModule_bmp,
		&OscModule_cam,
		&OscModule_vis,
		&OscModule_gpio,
		&OscModule_jpg,
		&OscModule_frd);

	OscLogSetConsoleLogLevel(WARN);
	OscLogSetFileLogLevel(WARN);
	
	#if defined(OSC_HOST)
		/* Setup file name reader (for host compiled version); read constant image */
		OscFrdCreateConstantReader(&s->hFileNameReader, "imgCapture.bmp");
		OscCamSetFileNameReader(s->hFileNameReader);
	#endif
	
	/* Configure camera */
	OscCamPresetRegs();
	/* Set AGC and AEC */
        OscCamSetRegisterValue(REG_AEC_AGC_ENABLE, 0x3);
        /* Turn on continuous capture for this application. */
        OscCamSetRegisterValue(CAM_REG_CHIP_CONTROL, 0x388);
        /* Set the undocumented reserved almighty Micron register to the
           "optimal" value. */
        OscCamSetRegisterValue(CAM_REG_RESERVED_0x20, 0x3d5);

	OscCamSetAreaOfInterest(0, 0, OSC_CAM_MAX_IMAGE_WIDTH, OSC_CAM_MAX_IMAGE_HEIGHT);
	OscCamSetupPerspective(OSC_CAM_PERSPECTIVE_180DEG_ROTATE);

	OscCamSetFrameBuffer(0, OSC_CAM_MAX_IMAGE_WIDTH * OSC_CAM_MAX_IMAGE_HEIGHT, s->frameBuffer1, TRUE); 
	OscCamSetFrameBuffer(1, OSC_CAM_MAX_IMAGE_WIDTH * OSC_CAM_MAX_IMAGE_HEIGHT, s->frameBuffer2, TRUE); 

	s->doubleBufferIDs[0] = 0;
	s->doubleBufferIDs[1] = 1;
	OscCamCreateMultiBuffer(2, s->doubleBufferIDs); 

	OscCamSetupCapture(OSC_CAM_MULTI_BUFFER); 
} /* initSystem */

/*********************************************************************//*!
 * @brief Shut down system and close framework
 *
 * @param s Pointer to the system state 
 *//*********************************************************************/
void cleanupSystem(struct SYSTEM *s)
{
	/* Destroy framework */
	OscDestroy();
} /* cleanupSysteim */

/*********************************************************************//*!
 * @brief  The main program
 * 
 * Opens the camera and reads pictures as fast as possible
 * Makes a debayering of the image
 * Writes the debayered image to a buffer which can be read by
 * TCP clients on Port 8111. Several concurrent clients are allowed.
 * The simplest streaming video client looks like this:
 * 
 * nc 192.168.1.10 8111 | mplayer - -demuxer rawvideo -rawvideo w=376:h=240:format=bgr24:fps=100
 * 
 * Writes every 10th picture to a .jpg file in the Web Server Directory
 *//*********************************************************************/
int main(const int argc, const char * argv[])
{
	struct OSC_PICTURE rawPic;
	int j=0;
	
	initSystem(&sys);

	ip_start_server();
	
	/* setup variables */
	rawPic.width = OSC_CAM_MAX_IMAGE_WIDTH;
	rawPic.height = OSC_CAM_MAX_IMAGE_HEIGHT;
	rawPic.type = 0;
	
	#if defined(OSC_TARGET)
		/* Take a picture, first time slower ;-) */
		usleep(10000); OscGpioTriggerImage(); usleep(10000);
		OscLog(DEBUG,"Triggered CAM ");
	#endif

	while(true) {

		OscCamReadPicture(OSC_CAM_MULTI_BUFFER, (void *) &rawPic.data, 0, 0);
	
		/* Take a picture */
		usleep(2000);
		OscCamSetupCapture(OSC_CAM_MULTI_BUFFER); 

		#if defined(OSC_TARGET)
			OscGpioTriggerImage();
		#else
			usleep(10000);
		#endif
	
		if (ip_send_all((char *)rawPic.data, rawPic.width*rawPic.height*
			OSC_PICTURE_TYPE_COLOR_DEPTH(rawPic.type)/8))
			OscLog(ALERT, "Sent picture\n");	

		ip_do_work();

		OscLog(DEBUG, "Got and debayered %d0 pix\n",j++);
		
	}
	ip_stop_server();

	cleanupSystem(&sys);

	return 0;
} /* main */
