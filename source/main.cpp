#include "App.h"
#include <cstdio>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <unistd.h>

#define ENABLE_NXLINK
#ifndef ENABLE_NXLINK
#define TRACE(fmt,...) ((void)0)
#else
#include <unistd.h>
#define TRACE(fmt,...) printf("%s: " fmt "\n", __PRETTY_FUNCTION__, ## __VA_ARGS__)

static int s_nxlinkSock = -1;

static void initNxLink()
{
    if (R_FAILED(socketInitializeDefault()))
        return;

    s_nxlinkSock = nxlinkStdio();
    if (s_nxlinkSock >= 0)
        TRACE("printf output now goes to nxlink server");
    else
        socketExit();
}

static void deinitNxLink()
{
    if (s_nxlinkSock >= 0)
    {
        close(s_nxlinkSock);
        socketExit();
        s_nxlinkSock = -1;
    }
}

extern "C" void userAppInit()
{
    initNxLink();
}

extern "C" void userAppExit()
{
    deinitNxLink();
}

#endif

int main(int argc, char *argv[])
{
    printf("Hello world!\n");
    Result rc = romfsInit();
    if (R_FAILED(rc))
    {
        printf("romfsInit() failed: 0x%x\n", rc);
        return EXIT_FAILURE;
    }

    // FILE* f = fopen("/switch/models/dingus.txt", "r");
    // //Print the file
    // if (f)
    // {
    //     char line[256];
    //     while (fgets(line, sizeof(line), f))
    //     {
    //         printf("%s", line);
    //     }
    //     fclose(f);
    // } else {
    //     printf("Failed to open /models/cat/cat.obj for reading\n");
    // }


    // Create the app
    App app;
    if (!app.init())
    {
        printf("App initialization failed\n");
        return EXIT_FAILURE;
    }

    app.run();
    app.shutdown();

    printf("Exiting...\n");
    socketExit();

    return EXIT_SUCCESS;
}