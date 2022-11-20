#include <iostream>

#include <3ds.h>

int main(){
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    std::cout << "Bye" << std::endl;

    while (aptMainLoop()){
        gspWaitForVBlank();
        gfxSwapBuffers();
        hidScanInput();

        u32 kDown = hidKeysDown();
        if (kDown & KEY_START)
            break;
    }
    gfxExit();
    return 0;
}