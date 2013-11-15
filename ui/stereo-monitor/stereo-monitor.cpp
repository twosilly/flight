/*
 * Monitors the log file size and sends LCM messages about it
 *
 * Author: Andrew Barry, <abarry@csail.mit.edu> 2013
 *
 */

#include <iostream>


using namespace std;


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <dirent.h>


#include "../../LCM/lcmt_stereo.h"
#include "../../LCM/lcmt_stereo_monitor.h"

#include <bot_core/bot_core.h>
#include <bot_param/param_client.h>
#include <GL/gl.h>
#include <bot_lcmgl_client/lcmgl.h>

#include <bot_core/rotations.h>
#include <bot_frames/bot_frames.h>

#include "../../externals/ConciseArgs.hpp"
   
   

lcm_t * lcm;

const char *stereo_channel, *stereo_monitor_channel;

int downsample_amount = 200;
int downsample_counter = 0;

string stereo_channel_str = "stereo";
string stereo_monitor_channel_str = "stereo_monitor";

lcmt_stereo_subscription_t *stereo_sub;



void sighandler(int dum)
{
    printf("\nClosing... ");
    lcmt_stereo_unsubscribe(lcm, stereo_sub);
    lcm_destroy (lcm);

    printf("done.\n");
    
    exit(0);
}

int64_t getTimestampNow()
{
    struct timeval thisTime;
    gettimeofday(&thisTime, NULL);
    return (thisTime.tv_sec * 1000000.0) + (float)thisTime.tv_usec + 0.5;
}

void stereo_handler(const lcm_recv_buf_t *rbuf, const char* channel, const lcmt_stereo *msg, void *user)
{
    // republish every N stereo messages
    downsample_counter ++;
    
    if (downsample_counter >= downsample_amount)
    {
        downsample_counter = 0;
        
        // send a message
        lcmt_stereo_monitor monitor_msg;
        monitor_msg.timestamp = getTimestampNow();
        
        monitor_msg.frame_number = msg->frame_number;
        
        lcmt_stereo_monitor_publish(lcm, stereo_monitor_channel,
            &monitor_msg);
    }
}

int main(int argc,char** argv)
{
    

    ConciseArgs parser(argc, argv);
    parser.add(stereo_channel_str, "s", "stereo-channel",
        "LCM channel for stereo input");
    parser.add(stereo_monitor_channel_str, "m", "stereo-monitor-channel",
        "LCM channel for publishing monitoring messages");
    parser.add(downsample_amount, "n", "downsample",
        "Downsample LCM message amount");
    parser.parse();
    
    stereo_channel = stereo_channel_str.c_str();
    stereo_monitor_channel = stereo_monitor_channel_str.c_str();
    

    lcm = lcm_create ("udpm://239.255.76.67:7667?ttl=1");
    if (!lcm)
    {
        fprintf(stderr, "lcm_create failed.  Quitting.\n");
        return 1;
    }

    signal(SIGINT,sighandler);
    
    stereo_sub = lcmt_stereo_subscribe(lcm, stereo_channel, &stereo_handler, NULL);
    
    printf("Receiving: \n\tStereo: %s\n\nPublishing:\n\tMonitor: %s (downsample: %d)\n\n", stereo_channel, stereo_monitor_channel, downsample_amount);

    while (true)
    {
        // read the LCM channel
        lcm_handle (lcm);
    }

    return 0;
}
