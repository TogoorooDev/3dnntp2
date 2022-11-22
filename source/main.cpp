#include <iostream>

#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <3ds.h>

#include "nntp.hpp"

using std::cout;
using std::endl;
using std::string;

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

void error_out(){
    for(u8 i = 10; i >= 1; i--){
        printf("%d ", i);
        cout << std::flush;
        sleep(1);
    }
}

int main(){
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    cout << "3dnntp2 v0.1" << endl;
    cout << "Connecting to the server" << endl;

    u32 *SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

    socInit(SOC_buffer, SOC_BUFFERSIZE);

    nntp::nntpcon con = nntp::init("46.165.242.75", 119);
    if (con.err != NNTPERR_OK){
        printf("Connection Failure\n Code: %d\n", con.err);
        sleep(10);
        //error_out();
        exit(0); 
    }

    cout << "Getting group data" << endl;
    nntp::nntpgroups groupdata = nntp::get_groups(con);
    if (groupdata.err != NNTPERR_OK){
        printf("Failure\n, Error Code: %d\n", groupdata.err);

        error_out();
        exit(0);
    }

    cout << "Groups gathered successfully\nDeath Grips is online" << endl;

    while (aptMainLoop()){
        gspWaitForVBlank();
        gfxSwapBuffers();
        hidScanInput();

        u32 kDown = hidKeysDown();
        if (kDown & KEY_START)
            break;

        if (kDown & KEY_A){
            printf("Searching for alt groups\n");
            nntp::nntpgroups newgroups = nntp::find_groups("alt", 0, groupdata);
            for (string group : newgroups.groups){
               printf("%s\n", group.c_str());
            }
            printf("Groups have been listed\n");
        }
    }

    socExit();
    gfxExit();
    return 0;
}