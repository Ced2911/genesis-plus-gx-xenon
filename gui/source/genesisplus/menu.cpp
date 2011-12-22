/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * menu.cpp
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#include <xetypes.h>
//#include <ogcsys.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
//#include <wiiuse/wpad.h>

#include "libwiigui/gui.h"
#include "menu.h"
#include "demo.h"
#include "input.h"
#include "filelist.h"
#include "filebrowser.h"
#include "w_input.h"
#include <xenon_soc/xenon_power.h>
#include <debug.h>
#include <threads/threads.h>

#include "genesis_settings.h"

#define THREAD_SLEEP 100

static GuiImageData * pointer[4];
static GuiImage * bgImg = NULL;
static GuiSound * bgMusic = NULL;
static GuiWindow * mainWindow = NULL;
//static lwp_t guithread = LWP_THREAD_NULL;
static bool guiHalt = true;

//PTHREAD guithread = NULL;

static void UGUI();

GXColor ColorGrey = {104, 104, 104, 255};
GXColor ColorGrey2 = {49, 49, 49, 255};

/****************************************************************************
 * ResumeGui
 *
 * Signals the GUI thread to start, and resumes the thread. This is called
 * after finishing the removal/insertion of new elements, and after initial
 * GUI setup.
 ***************************************************************************/
static void
_ResumeGui() {
    guiHalt = false;
    //	LWP_ResumeThread (guithread);
    //    thread_resume(guithread);
    //    printf("thread_resume %d \r\n",thread_resume(guithread));
}

/****************************************************************************
 * HaltGui
 *
 * Signals the GUI thread to stop, and waits for GUI thread to stop
 * This is necessary whenever removing/inserting new elements into the GUI.
 * This eliminates the possibility that the GUI is in the middle of accessing
 * an element that is being changed.
 ***************************************************************************/
static void
_HaltGui() {
    guiHalt = true;
    // wait for thread to finish
    //while (!thread_suspend(guithread))
    //    while(guithread->SuspendCount==0){
    //        usleep(50);
    //    }
}
//
#define ResumeGui(){TR;_ResumeGui();}
#define HaltGui(){TR;_HaltGui();}

/****************************************************************************
 * WindowPrompt
 *
 * Displays a prompt window to user, with information, an error message, or
 * presenting a user with a choice
 ***************************************************************************/
int
WindowPrompt(const char *title, const char *msg, const char *btn1Label, const char *btn2Label) {
    int choice = -1;

    //    GuiWindow promptWindow(448, 288);
    GuiWindow promptWindow(640, 360);
    promptWindow.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
    promptWindow.SetPosition(0, -10);

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(xenon_button_png);
    GuiImageData btnOutlineOver(xenon_button_over_png);
    GuiTrigger trigA;

    //	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
    trigA.SetSimpleTrigger(-1, 0, PAD_BUTTON_A);

    //    GuiImageData dialogBox(dialogue_box_png);
    GuiImageData dialogBox(xenon_popup_png);
    GuiImage dialogBoxImg(&dialogBox);

    GuiText titleTxt(title, 26, ColorGrey);
    titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    titleTxt.SetPosition(0, 40);

    GuiText msgTxt(msg, 22, ColorGrey2);
    msgTxt.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
    msgTxt.SetPosition(0, -20);
    msgTxt.SetWrap(true, 600);

    GuiText btn1Txt(btn1Label, 22, ColorGrey);
    GuiImage btn1Img(&btnOutline);
    GuiImage btn1ImgOver(&btnOutlineOver);
    GuiButton btn1(btnOutline.GetWidth(), btnOutline.GetHeight());

    if (btn2Label) {
        btn1.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
        btn1.SetPosition(20, -25);
    } else {
        btn1.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
        btn1.SetPosition(0, -25);
    }

    btn1.SetLabel(&btn1Txt);
    btn1.SetImage(&btn1Img);
    btn1.SetImageOver(&btn1ImgOver);
    btn1.SetSoundOver(&btnSoundOver);
    btn1.SetTrigger(&trigA);
    btn1.SetState(STATE_SELECTED);
    btn1.SetEffectGrow();

    GuiText btn2Txt(btn2Label, 22, ColorGrey);
    GuiImage btn2Img(&btnOutline);
    GuiImage btn2ImgOver(&btnOutlineOver);
    GuiButton btn2(btnOutline.GetWidth(), btnOutline.GetHeight());
    btn2.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
    btn2.SetPosition(-20, -25);
    btn2.SetLabel(&btn2Txt);
    btn2.SetImage(&btn2Img);
    btn2.SetImageOver(&btn2ImgOver);
    btn2.SetSoundOver(&btnSoundOver);
    btn2.SetTrigger(&trigA);
    btn2.SetEffectGrow();

    promptWindow.Append(&dialogBoxImg);
    promptWindow.Append(&titleTxt);
    promptWindow.Append(&msgTxt);
    promptWindow.Append(&btn1);

    if (btn2Label)
        promptWindow.Append(&btn2);

    promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_IN, 50);
    HaltGui();
    mainWindow->SetState(STATE_DISABLED);
    mainWindow->Append(&promptWindow);
    mainWindow->ChangeFocus(&promptWindow);
    ResumeGui();

    while (choice == -1) {

        UGUI();
        usleep(THREAD_SLEEP);

        if (btn1.GetState() == STATE_CLICKED)
            choice = 1;
        else if (btn2.GetState() == STATE_CLICKED)
            choice = 0;
    }

    promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_OUT, 50);
    while (promptWindow.GetEffect() > 0) {
        UGUI();
        usleep(THREAD_SLEEP);
    }
    HaltGui();
    mainWindow->Remove(&promptWindow);
    mainWindow->SetState(STATE_DEFAULT);
    ResumeGui();
    return choice;
}

void ErrorPrompt(const char *msg) {
    WindowPrompt("Error", msg, "OK", NULL);
}

void InfoPrompt(const char *msg) {
    WindowPrompt("Information", msg, "OK", NULL);
}

/****************************************************************************
 * UpdateGUI
 *
 * Primary thread to allow GUI to respond to state changes, and draws GUI
 ***************************************************************************/

void UGUI() {
    int i;
    UpdatePads();
    mainWindow->Draw();

#ifdef HW_RVL
    for (i = 3; i >= 0; i--) // so that player 1's cursor appears on top!
    {
        if (userInput[i].wpad->ir.valid)
            Menu_DrawImg(userInput[i].wpad->ir.x - 48, userInput[i].wpad->ir.y - 48,
                96, 96, pointer[i]->GetImage(), userInput[i].wpad->ir.angle, 1, 1, 255);
        DoRumble(i);
    }
#endif
    Menu_Render();

    for (i = 0; i < 4; i++)
        mainWindow->Update(&userInput[i]);

    if (ExitRequested) {
        for (i = 0; i <= 255; i += 15) {
            mainWindow->Draw();

            Menu_DrawRectangle(0, 0, screenwidth, screenheight, (GXColor) {
                0, 0, 0, i
            }, 1);
            Menu_Render();
        }
        ExitApp();
    }
}

static void *
UpdateGUI(void *arg) {
    int i;

    TR;
    while (1) {
        //TR;
        if (guiHalt) {

            //            thread_sleep(THREAD_SLEEP);
            //            printf("thread_suspend %d \r\n",thread_suspend(guithread));
        } else {
            TR;
            UGUI();
        }
    }
    return NULL;
}

/****************************************************************************
 * InitGUIThread
 *
 * Startup GUI threads
 ***************************************************************************/
void
InitGUIThreads() {
    //    threading_init();
    //    guithread = thread_create((void*) UpdateGUI, 0, NULL, THREAD_FLAG_CREATE_SUSPENDED);
    //    thread_set_processor(guithread,2);
    //    thread_resume(guithread);
}

/****************************************************************************
 * OnScreenKeyboard
 *
 * Opens an on-screen keyboard window, with the data entered being stored
 * into the specified variable.
 ***************************************************************************/
static void OnScreenKeyboard(char * var, u16 maxlen) {
    int save = -1;

    GuiKeyboard keyboard(var, maxlen);

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(xenon_button_png);
    GuiImageData btnOutlineOver(xenon_button_over_png);
    GuiTrigger trigA;
    //	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
    trigA.SetSimpleTrigger(-1, 0, PAD_BUTTON_A);

    GuiText okBtnTxt("OK", 22, (GXColor) {
        0, 0, 0, 255
    });
    GuiImage okBtnImg(&btnOutline);
    GuiImage okBtnImgOver(&btnOutlineOver);
    GuiButton okBtn(btnOutline.GetWidth(), btnOutline.GetHeight());

    okBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
    okBtn.SetPosition(25, -25);

    okBtn.SetLabel(&okBtnTxt);
    okBtn.SetImage(&okBtnImg);
    okBtn.SetImageOver(&okBtnImgOver);
    okBtn.SetSoundOver(&btnSoundOver);
    okBtn.SetTrigger(&trigA);
    okBtn.SetEffectGrow();

    GuiText cancelBtnTxt("Cancel", 22, (GXColor) {
        0, 0, 0, 255
    });
    GuiImage cancelBtnImg(&btnOutline);
    GuiImage cancelBtnImgOver(&btnOutlineOver);
    GuiButton cancelBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    cancelBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
    cancelBtn.SetPosition(-25, -25);
    cancelBtn.SetLabel(&cancelBtnTxt);
    cancelBtn.SetImage(&cancelBtnImg);
    cancelBtn.SetImageOver(&cancelBtnImgOver);
    cancelBtn.SetSoundOver(&btnSoundOver);
    cancelBtn.SetTrigger(&trigA);
    cancelBtn.SetEffectGrow();

    keyboard.Append(&okBtn);
    keyboard.Append(&cancelBtn);

    HaltGui();
    mainWindow->SetState(STATE_DISABLED);
    mainWindow->Append(&keyboard);
    mainWindow->ChangeFocus(&keyboard);
    ResumeGui();

    while (save == -1) {
        UGUI();
        usleep(THREAD_SLEEP);

        if (okBtn.GetState() == STATE_CLICKED)
            save = 1;
        else if (cancelBtn.GetState() == STATE_CLICKED)
            save = 0;
    }

    if (save) {
        snprintf(var, maxlen, "%s", keyboard.kbtextstr);
    }

    HaltGui();
    mainWindow->Remove(&keyboard);
    mainWindow->SetState(STATE_DEFAULT);
    ResumeGui();
}


// genesis surface
extern "C" struct XenosSurface * g_pTexture;

static int _MenuInGame() {
    int menu = MENU_NONE;

    int max_saves_states_slot = 10;
    int selected_slot = 0;


    GuiText titleTxt("Genesis plus xenon - In game menu", 28, ColorGrey);
    titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
    titleTxt.SetPosition(50, 50);

    GuiTrigger trigA;
    //	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
    trigA.SetSimpleTrigger(-1, 0, PAD_BUTTON_A);

    GuiImage genesis_display(g_pTexture, screenwidth, screenheight);

    GuiImageData btnOutline(xenon_button_png);
    GuiImageData btnOutlineOver(xenon_button_over_png);

    GuiImage fileBtnImg(&btnOutline);
    GuiImage fileBtnImgOver(&btnOutlineOver);

    GuiButton saveStatesBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    GuiText SaveStateTxt("Save states", 22, ColorGrey2);
    SaveStateTxt.SetWrap(true, btnOutline.GetWidth() - 30);

    GuiImage saveBtnImg(&btnOutline);
    GuiImage saveBtnImgOver(&btnOutlineOver);

    saveStatesBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
    saveStatesBtn.SetPosition(50, 120);
    saveStatesBtn.SetLabel(&SaveStateTxt);
    saveStatesBtn.SetImage(&saveBtnImg);
    saveStatesBtn.SetImageOver(&saveBtnImgOver);
    saveStatesBtn.SetTrigger(&trigA);
    saveStatesBtn.SetEffectGrow();


    GuiButton loadStatesBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    GuiText LoadStateTxt("Load states", 22, ColorGrey2);
    LoadStateTxt.SetWrap(true, btnOutline.GetWidth() - 30);

    GuiImage loadBtnImg(&btnOutline);
    GuiImage loadBtnImgOver(&btnOutlineOver);

    loadStatesBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
    loadStatesBtn.SetPosition(50, 200);
    loadStatesBtn.SetLabel(&LoadStateTxt);
    loadStatesBtn.SetImage(&loadBtnImg);
    loadStatesBtn.SetImageOver(&loadBtnImgOver);
    loadStatesBtn.SetTrigger(&trigA);
    loadStatesBtn.SetEffectGrow();

    //    next states

    GuiButton NextStatesBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    GuiText NextStateTxt("Next states", 22, ColorGrey2);
    NextStateTxt.SetWrap(true, btnOutline.GetWidth() - 30);

    GuiImage NextBtnImg(&btnOutline);
    GuiImage NextBtnImgOver(&btnOutlineOver);

    NextStatesBtn.SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
    NextStatesBtn.SetPosition(50, 200);
    NextStatesBtn.SetLabel(&NextStateTxt);
    NextStatesBtn.SetImage(&NextBtnImg);
    NextStatesBtn.SetImageOver(&NextBtnImgOver);
    NextStatesBtn.SetTrigger(&trigA);
    NextStatesBtn.SetEffectGrow();

    //    prev states

    GuiButton PrevStatesBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    GuiText PrevStateTxt("Prev states", 22, ColorGrey2);
    PrevStateTxt.SetWrap(true, btnOutline.GetWidth() - 30);

    GuiImage PrevBtnImg(&btnOutline);
    GuiImage PrevBtnImgOver(&btnOutlineOver);

    PrevStatesBtn.SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
    PrevStatesBtn.SetPosition(50, 200);
    PrevStatesBtn.SetLabel(&PrevStateTxt);
    PrevStatesBtn.SetImage(&PrevBtnImg);
    PrevStatesBtn.SetImageOver(&PrevBtnImgOver);
    PrevStatesBtn.SetTrigger(&trigA);
    PrevStatesBtn.SetEffectGrow();


    HaltGui();

    GuiWindow w(screenwidth, screenheight);

    w.Append(&genesis_display);
    w.Append(&titleTxt);
    w.Append(&loadStatesBtn);
    w.Append(&saveStatesBtn);
    w.Append(&NextStatesBtn);
    w.Append(&PrevStatesBtn);


    mainWindow->Append(&w);
    ResumeGui();

    while (menu == MENU_NONE) {
        UGUI();
        usleep(THREAD_SLEEP);
    }

    HaltGui();
    mainWindow->Remove(&w);

    return menu;
}


static char ROMFilename[] = "test.smd";

/****************************************************************************
 * FindGameSaveNum
 *
 * Determines the save file number of the given file name
 * Returns -1 if none is found
 ***************************************************************************/
static int FindGameSaveNum(char * savefile, int device) {
    int n = -1;
    int romlen = strlen(ROMFilename);
    int savelen = strlen(savefile);

    int diff = savelen - romlen;

    if (strncmp(savefile, ROMFilename, romlen) != 0)
        return -1;

    if (savefile[romlen] == ' ') {
        if (diff == 5 && strncmp(&savefile[romlen + 1], "Auto", 4) == 0)
            n = 0; // found Auto save
        else if (diff == 2 || diff == 3)
            n = atoi(&savefile[romlen + 1]);
    }

    if (n >= 0 && n < MAX_SAVES)
        return n;
    else
        return -1;
}

static int MenuInGame() {

    int menu = MENU_NONE;
    int ret;
    int i, n, type, len, len2;
    int j = 0;
    SaveList saves;
    char filepath[1024];
    char scrfile[1024];
    char tmp[MAXJOLIET + 1];
    struct stat filestat;
    struct tm * timeinfo;

    int action = 0;

    //    int device = GCSettings.SaveMethod;
    //
    //    if (device == DEVICE_AUTO)
    //        autoSaveMethod(NOTSILENT);
    //
    //    if (!ChangeInterface(device, NOTSILENT))
    //        return MENU_GAME;

    GuiText titleTxt("Genesis plus xenon - In game menu", 28, ColorGrey);
    titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
    titleTxt.SetPosition(50, 50);

    if (action == 0)
        titleTxt.SetText("Load Game");
    else
        titleTxt.SetText("Save Game");

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiSound btnSoundClick(button_click_pcm, button_click_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(xenon_button_png);
    GuiImageData btnOutlineOver(xenon_button_over_png);
    GuiImageData btnCloseOutline(xenon_button_png);
    GuiImageData btnCloseOutlineOver(xenon_button_over_png);

    GuiTrigger trigHome;
    trigHome.SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME, 0);

    GuiTrigger trigA;
    //	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
    trigA.SetSimpleTrigger(-1, 0, PAD_BUTTON_A);
    GuiTrigger trigB;
    //	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
    trigB.SetSimpleTrigger(-1, 0, PAD_BUTTON_B);

    GuiText backBtnTxt("Go Back", 22, (GXColor) {
        0, 0, 0, 255
    });
    GuiImage backBtnImg(&btnOutline);
    GuiImage backBtnImgOver(&btnOutlineOver);
    GuiButton backBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    backBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
    backBtn.SetPosition(50, -35);
    backBtn.SetLabel(&backBtnTxt);
    backBtn.SetImage(&backBtnImg);
    backBtn.SetImageOver(&backBtnImgOver);
    backBtn.SetSoundOver(&btnSoundOver);
    backBtn.SetSoundClick(&btnSoundClick);
    backBtn.SetTrigger(&trigA);
    backBtn.SetTrigger(&trigB);
    backBtn.SetEffectGrow();

    GuiText closeBtnTxt("Close", 20, (GXColor) {
        0, 0, 0, 255
    });
    GuiImage closeBtnImg(&btnCloseOutline);
    GuiImage closeBtnImgOver(&btnCloseOutlineOver);
    GuiButton closeBtn(btnCloseOutline.GetWidth(), btnCloseOutline.GetHeight());
    closeBtn.SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
    closeBtn.SetPosition(-50, 35);
    closeBtn.SetLabel(&closeBtnTxt);
    closeBtn.SetImage(&closeBtnImg);
    closeBtn.SetImageOver(&closeBtnImgOver);
    closeBtn.SetSoundOver(&btnSoundOver);
    closeBtn.SetSoundClick(&btnSoundClick);
    closeBtn.SetTrigger(&trigA);
    closeBtn.SetTrigger(&trigB);
    closeBtn.SetTrigger(&trigHome);
    closeBtn.SetEffectGrow();

    HaltGui();
    GuiWindow w(screenwidth, screenheight);
    w.Append(&backBtn);
    w.Append(&closeBtn);
    mainWindow->Append(&w);
    mainWindow->Append(&titleTxt);
    ResumeGui();



    memset(&saves, 0, sizeof (saves));

    sprintf(browser.dir, "%s%s", "uda:/", "genesis");
    BrowseDevice("genesis","uda:/");


    // find matching files
    //    AllocSaveBuffer();

    for (i = 0; i < browser.numEntries; i++) {
        len2 = strlen(browserList[i].filename);

        if (len2 < 6 || len2 - len < 5)
            continue;

        if (strncmp(&browserList[i].filename[len2 - 4], ".srm", 4) == 0)
            type = FILE_SRAM;
        else if (strncmp(&browserList[i].filename[len2 - 4], ".gpz", 4) == 0)
            type = FILE_SNAPSHOT;
        else
            continue;

        strcpy(tmp, browserList[i].filename);
        tmp[len2 - 4] = 0;
        n = FindGameSaveNum(tmp, 0);

        if (n >= 0) {
            saves.type[j] = type;
            saves.files[saves.type[j]][n] = 1;
            strcpy(saves.filename[j], browserList[i].filename);

            if (saves.type[j] == FILE_SNAPSHOT) {
//                sprintf(scrfile, "%s%s/%s.png", pathPrefix[GCSettings.SaveMethod], GCSettings.SaveFolder, tmp);

//                memset(savebuffer, 0, SAVEBUFFERSIZE);
//                if (LoadFile(scrfile, SILENT))
//                    saves.previewImg[j] = new GuiImageData(savebuffer, 64, 48);
            }
            snprintf(filepath, 1024, "%s%s/%s", "uda:/", "genesis", saves.filename[j]);
            if (stat(filepath, &filestat) == 0) {
                timeinfo = localtime(&filestat.st_mtime);
                strftime(saves.date[j], 20, "%a %b %d", timeinfo);
                strftime(saves.time[j], 10, "%I:%M %p", timeinfo);
            }
            j++;
        }
    }

    //    FreeSaveBuffer();
    saves.length = j;

    if (saves.length == 0 && action == 0) {
        InfoPrompt("No game saves found.");
        menu = MENU_SETTINGS;
    }

    GuiSaveBrowser saveBrowser(552, 248, &saves, action);
    saveBrowser.SetPosition(0, 108);
    saveBrowser.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);

    HaltGui();
    mainWindow->Append(&saveBrowser);
    mainWindow->ChangeFocus(&saveBrowser);
    ResumeGui();


    while (menu == MENU_NONE) {
        UGUI();
        usleep(THREAD_SLEEP);

        ret = saveBrowser.GetClickedSave();

        // load or save game
        if (ret > -3) {
            int result = 0;

            if (action == 0) // load
            {
                MakeFilePath(filepath, saves.type[ret], saves.filename[ret]);
                switch (saves.type[ret]) {
                    case FILE_SRAM:
                        //                        result = LoadSRAM(filepath, NOTSILENT);
                        break;
                    case FILE_SNAPSHOT:
                        //                        result = LoadSnapshot(filepath, NOTSILENT);
                        break;
                }
                if (result)
                    menu = MENU_EXIT;
            } else // save
            {
                if (ret == -2) // new SRAM
                {
                    for (i = 1; i < 100; i++)
                        if (saves.files[FILE_SRAM][i] == 0)
                            break;

                    if (i < 100) {
                        MakeFilePath(filepath, FILE_SRAM, ROMFilename, i);
                        //                        SaveSRAM(filepath, NOTSILENT);
                        //                        menu = MENU_GAME_SAVE;
                    }
                } else if (ret == -1) // new Snapshot
                {
                    for (i = 1; i < 100; i++)
                        if (saves.files[FILE_SNAPSHOT][i] == 0)
                            break;

                    if (i < 100) {
                        MakeFilePath(filepath, FILE_SNAPSHOT, ROMFilename, i);
                        //                        SaveSnapshot(filepath, NOTSILENT);
                        //                        menu = MENU_GAME_SAVE;
                    }
                } else // overwrite SRAM/Snapshot
                {
                    MakeFilePath(filepath, saves.type[ret], saves.filename[ret]);
                    switch (saves.type[ret]) {
                        case FILE_SRAM:
                            //                            SaveSRAM(filepath, NOTSILENT);
                            break;
                        case FILE_SNAPSHOT:
                            //                            SaveSnapshot(filepath, NOTSILENT);
                            break;
                    }
                    //                    menu = MENU_GAME_SAVE;
                }
            }
        }

        if (backBtn.GetState() == STATE_CLICKED) {
            menu = MENU_SETTINGS;
        } else if (closeBtn.GetState() == STATE_CLICKED) {
            menu = MENU_EXIT;

            //            exitSound->Play();
            //            bgTopImg->SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_OUT, 15);
            //            closeBtn.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_OUT, 15);
            //            titleTxt.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_OUT, 15);
            //            backBtn.SetEffect(EFFECT_SLIDE_BOTTOM | EFFECT_SLIDE_OUT, 15);
            //            bgBottomImg->SetEffect(EFFECT_SLIDE_BOTTOM | EFFECT_SLIDE_OUT, 15);
            //            btnLogo->SetEffect(EFFECT_SLIDE_BOTTOM | EFFECT_SLIDE_OUT, 15);

            w.SetEffect(EFFECT_FADE, -15);

            usleep(350000); // wait for effects to finish
        }
    }

    HaltGui();

    for (i = 0; i < saves.length; i++)
        if (saves.previewImg[i])
            delete saves.previewImg[i];

    mainWindow->Remove(&saveBrowser);
    mainWindow->Remove(&w);
    mainWindow->Remove(&titleTxt);
    ResetBrowser();

    return menu;
}

static int __MenuInGame() {
    int menu = MENU_NONE;

    GuiText titleTxt("Genesis plus xenon - In game menu", 28, ColorGrey);
    titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
    titleTxt.SetPosition(50, 50);

    GuiTrigger trigA;
    //	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
    trigA.SetSimpleTrigger(-1, 0, PAD_BUTTON_A);

    GuiImage genesis_display(g_pTexture, screenwidth, screenheight);

    GuiImageData btnOutline(xenon_button_png);
    GuiImageData btnOutlineOver(xenon_button_over_png);

    HaltGui();

    GuiWindow w(screenwidth, screenheight);

    w.Append(&genesis_display);
    w.Append(&titleTxt);


    mainWindow->Append(&w);
    ResumeGui();

    while (menu == MENU_NONE) {
        UGUI();
        usleep(THREAD_SLEEP);
    }

    HaltGui();
    mainWindow->Remove(&w);

    return menu;
}

/****************************************************************************
 * MenuBrowseDevice
 ***************************************************************************/
static int MenuBrowseDevice() {
    char title[100];
    int i;

    ShutoffRumble();

    // populate initial directory listing
    if (BrowseDevice() <= 0) {
        int choice = WindowPrompt(
                "Error",
                "Unable to display files on selected load device.",
                "Retry",
                "Check Settings");

        if (choice)
            return MENU_BROWSE_DEVICE;
        else
            return MENU_SETTINGS;
    }

    int menu = MENU_NONE;

    sprintf(title, "Genesis Plus Xenon - Load Game");

    GuiText titleTxt(title, 28, ColorGrey);
    titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
    titleTxt.SetPosition(100, 50);

    GuiTrigger trigA;
    //	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
    trigA.SetSimpleTrigger(-1, 0, PAD_BUTTON_A);

    GuiFileBrowser fileBrowser(1080, 496);
    fileBrowser.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    fileBrowser.SetPosition(0, 100);

    GuiImageData btnOutline(xenon_button_png);
    GuiImageData btnOutlineOver(xenon_button_over_png);

    GuiText backBtnTxt("Go Back", 18, ColorGrey2);
    GuiImage backBtnImg(&btnOutline);
    GuiImage backBtnImgOver(&btnOutlineOver);
    GuiButton backBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    backBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
    backBtn.SetPosition(100, -35);
    backBtn.SetLabel(&backBtnTxt);
    backBtn.SetImage(&backBtnImg);
    backBtn.SetImageOver(&backBtnImgOver);
    backBtn.SetTrigger(&trigA);
    backBtn.SetEffectGrow();

    GuiWindow xenon_buttonWindow(screenwidth, screenheight);
    xenon_buttonWindow.Append(&backBtn);

    HaltGui();
    mainWindow->Append(&titleTxt);
    mainWindow->Append(&fileBrowser);
    mainWindow->Append(&xenon_buttonWindow);
    ResumeGui();

    while (menu == MENU_NONE) {
        UGUI();
        usleep(THREAD_SLEEP);

        // update file browser based on arrow xenon_buttons
        // set MENU_EXIT if A xenon_button pressed on a file
        for (i = 0; i < FILE_PAGESIZE; i++) {
            if (fileBrowser.fileList[i]->GetState() == STATE_CLICKED) {
                fileBrowser.fileList[i]->ResetState();
                // check corresponding browser entry
                if (browserList[browser.selIndex].isdir) {
                    if (BrowserChangeFolder()) {
                        fileBrowser.ResetState();
                        fileBrowser.fileList[0]->SetState(STATE_SELECTED);
                        fileBrowser.TriggerUpdate();
                    } else {
                        menu = MENU_BROWSE_DEVICE;
                        break;
                    }
                } else {
                    ShutoffRumble();
                    fileBrowser.ResetState();
                    //mainWindow->SetState(STATE_DISABLED);
                    extern int genesis_main(const char * root, const char * dir, const char *filename);
                    genesis_main(rootdir, browser.dir, browserList[browser.selIndex].filename);
                    menu = MENU_IN_GAME;
                    //mainWindow->SetState(STATE_DEFAULT);
                }
            }
        }
        if (backBtn.GetState() == STATE_CLICKED)
            menu = MENU_SETTINGS;
    }
    HaltGui();
    mainWindow->Remove(&titleTxt);
    mainWindow->Remove(&xenon_buttonWindow);
    mainWindow->Remove(&fileBrowser);
    return menu;
}

/****************************************************************************
 * MenuOptions
 ***************************************************************************/
static int MenuOptions() {
    int menu = MENU_NONE;
    int ret;
    int i = 0;
    bool firstRun = true;

    OptionList options;
    sprintf(options.name[MO_INPUT], "Input type");
    i++;
    sprintf(options.name[MO_DEVICE], "Device type");
    i++;
    sprintf(options.name[MO_VIDEO], "Video filter");
    i++;
    sprintf(options.name[MO_OVERSCAN], "Overscan");
    i++;
    sprintf(options.name[MO_SYSTEM], "System");
    i++;
    sprintf(options.name[MO_REGION], "Region");
    i++;
    sprintf(options.name[MO_LOCKON], "Lock-On");
    i++;
    sprintf(options.name[MO_YM2413], "YM2413");
    i++;
    sprintf(options.name[MO_SAVE], "Save");
    i++;
    options.length = i;

    GuiText titleTxt("Genesis Plus Xenon - Options", 28, ColorGrey);
    titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
    titleTxt.SetPosition(50, 50);

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(xenon_button_png);
    GuiImageData btnOutlineOver(xenon_button_over_png);

    GuiTrigger trigA;
    //	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
    trigA.SetSimpleTrigger(-1, 0, PAD_BUTTON_A);

    GuiText backBtnTxt("Go Back", 22, ColorGrey2);
    GuiImage backBtnImg(&btnOutline);
    GuiImage backBtnImgOver(&btnOutlineOver);
    GuiButton backBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    backBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
    backBtn.SetPosition(100, -35);
    backBtn.SetLabel(&backBtnTxt);
    backBtn.SetImage(&backBtnImg);
    backBtn.SetImageOver(&backBtnImgOver);
    backBtn.SetSoundOver(&btnSoundOver);
    backBtn.SetTrigger(&trigA);
    backBtn.SetEffectGrow();

    GuiOptionBrowser optionBrowser(1080, 496, &options);
    optionBrowser.SetPosition(0, 108);
    optionBrowser.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    optionBrowser.SetCol2Position(185);

    HaltGui();
    GuiWindow w(screenwidth, screenheight);
    w.Append(&backBtn);
    mainWindow->Append(&optionBrowser);
    mainWindow->Append(&w);
    mainWindow->Append(&titleTxt);
    ResumeGui();

    while (menu == MENU_NONE) {
        UGUI();
        usleep(THREAD_SLEEP);

        ret = optionBrowser.GetClickedOption();

        switch (ret) {
            case MO_INPUT:
                gensettings.input_type++;
                break;

            case MO_DEVICE:
                gensettings.device_type++;
                break;

            case MO_VIDEO:
                gensettings.video_filter++;
                break;

            case MO_OVERSCAN:
                gensettings.overscan++;
                break;

            case MO_SYSTEM:
                gensettings.system++;
                break;

            case MO_REGION:
                gensettings.region++;
                break;

            case MO_LOCKON:
                gensettings.lock_on++;
                break;
            case MO_YM2413:
                gensettings.ym2413++;
                break;

            case MO_SAVE:
                gensettings.saves++;
                break;
        }

        if (ret >= 0 || firstRun) {
            firstRun = false;

            // correct load/save methods out of bounds
            if (gensettings.input_type >= INPUT_MAX)
                gensettings.input_type = 0;
            if (gensettings.device_type >= DEVICE_MAX)
                gensettings.device_type = 0;
            if (gensettings.video_filter >= VF_MAX)
                gensettings.video_filter = 0;
            if (gensettings.overscan >= OVERSCAN_MAX)
                gensettings.overscan = 0;
            if (gensettings.system >= SYSTEM_MAX)
                gensettings.system = 0;
            if (gensettings.region >= REGIONS_MAX)
                gensettings.region = 0;
            if (gensettings.lock_on >= LOCKON_MAX)
                gensettings.lock_on = 0;
            if (gensettings.ym2413 >= YM2413_MAX)
                gensettings.ym2413 = 0;
            if (gensettings.saves >= SAVES_MAX)
                gensettings.saves = 0;

            // update strings
            getInputTypeString(gensettings.input_type, options.value[MO_INPUT]);
            getDeviceTypeString(gensettings.device_type, options.value[MO_DEVICE]);
            getVFTypeString(gensettings.video_filter, options.value[MO_VIDEO]);
            getOverscanTypeString(gensettings.overscan, options.value[MO_OVERSCAN]);
            getSystemTypeString(gensettings.system, options.value[MO_SYSTEM]);
            getRegionTypeString(gensettings.region, options.value[MO_REGION]);
            getLockOnTypeString(gensettings.lock_on, options.value[MO_LOCKON]);
            getYM2413TypeString(gensettings.ym2413, options.value[MO_YM2413]);
            getSaveTypeString(gensettings.saves, options.value[MO_SAVE]);


            // Update label
            optionBrowser.TriggerUpdate();
        }

        if (backBtn.GetState() == STATE_CLICKED) {
            //save settings
            SaveSettings(&gensettings);
            menu = MENU_SETTINGS;
        }

    }
    HaltGui();
    mainWindow->Remove(&optionBrowser);
    mainWindow->Remove(&w);
    mainWindow->Remove(&titleTxt);
    return menu;
}

static int MainMenu() {
    static int first_run = true;
    if (first_run) {
        if (LoadSettings(&gensettings) == -1)
            SaveSettings(&gensettings);
        first_run = false;
    }

    int menu = MENU_NONE;

    GuiText titleTxt("Genesis Plus Xenon", 28, ColorGrey);
    titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
    titleTxt.SetPosition(50, 50);

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(xenon_button_png);
    GuiImageData btnOutlineOver(xenon_button_over_png);
    GuiImageData btnLargeOutline(xenon_button_large_png);
    GuiImageData btnLargeOutlineOver(xenon_button_large_over_png);

    GuiTrigger trigA;
    trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
    //    trigA.SetSimpleTrigger(-1, 0, PAD_BUTTON_A);
    GuiTrigger trigHome;
    trigHome.SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME, PAD_BUTTON_LOGO);
    //    trigHome.SetButtonOnlyTrigger(-1, 0, PAD_BUTTON_LOGO);

    GuiText fileBtnTxt("Load Game", 18, ColorGrey2);
    fileBtnTxt.SetWrap(true, btnLargeOutline.GetWidth() - 30);
    GuiImage fileBtnImg(&btnLargeOutline);
    GuiImage fileBtnImgOver(&btnLargeOutlineOver);
    GuiButton fileBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
    fileBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
    fileBtn.SetPosition(50, 120);
    fileBtn.SetLabel(&fileBtnTxt);
    fileBtn.SetImage(&fileBtnImg);
    fileBtn.SetImageOver(&fileBtnImgOver);
    fileBtn.SetSoundOver(&btnSoundOver);
    fileBtn.SetTrigger(&trigA);
    fileBtn.SetEffectGrow();

    GuiText optionBtnTxt("Options", 18, ColorGrey2);
    optionBtnTxt.SetWrap(true, btnLargeOutline.GetWidth() - 30);
    GuiImage optionBtnImg(&btnLargeOutline);
    GuiImage optionBtnImgOver(&btnLargeOutlineOver);
    GuiButton optionBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
    optionBtn.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    optionBtn.SetPosition(0, 120);
    optionBtn.SetLabel(&optionBtnTxt);
    optionBtn.SetImage(&optionBtnImg);
    optionBtn.SetImageOver(&optionBtnImgOver);
    optionBtn.SetSoundOver(&btnSoundOver);
    optionBtn.SetTrigger(&trigA);
    optionBtn.SetEffectGrow();

    GuiText savingBtnTxt1("Load / Save", 18, ColorGrey2);
    savingBtnTxt1.SetWrap(true, btnLargeOutline.GetWidth() - 30);
    GuiImage savingBtnImg(&btnLargeOutline);
    GuiImage savingBtnImgOver(&btnLargeOutlineOver);
    GuiButton savingBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
    savingBtn.SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
    savingBtn.SetPosition(-50, 120);
    savingBtn.SetLabel(&savingBtnTxt1);
    savingBtn.SetImage(&savingBtnImg);
    savingBtn.SetImageOver(&savingBtnImgOver);
    savingBtn.SetSoundOver(&btnSoundOver);
    savingBtn.SetTrigger(&trigA);
    savingBtn.SetEffectGrow();

    GuiText cheatsBtnTxt("Cheats", 18, ColorGrey2);
    cheatsBtnTxt.SetWrap(true, btnLargeOutline.GetWidth() - 30);
    GuiImage cheatsBtnImg(&btnLargeOutline);
    GuiImage cheatsBtnImgOver(&btnLargeOutlineOver);
    GuiButton cheatsBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
    cheatsBtn.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    cheatsBtn.SetPosition(0, 250);
    cheatsBtn.SetLabel(&cheatsBtnTxt);
    cheatsBtn.SetImage(&cheatsBtnImg);
    cheatsBtn.SetImageOver(&cheatsBtnImgOver);
    cheatsBtn.SetSoundOver(&btnSoundOver);
    cheatsBtn.SetTrigger(&trigA);
    cheatsBtn.SetEffectGrow();

    GuiText exitBtnTxt("Exit to XELL", 18, ColorGrey2);
    GuiImage exitBtnImg(&btnOutline);
    GuiImage exitBtnImgOver(&btnOutlineOver);
    GuiButton exitBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    exitBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
    exitBtn.SetPosition(100, -35);
    exitBtn.SetLabel(&exitBtnTxt);
    exitBtn.SetImage(&exitBtnImg);
    exitBtn.SetImageOver(&exitBtnImgOver);
    exitBtn.SetSoundOver(&btnSoundOver);
    exitBtn.SetTrigger(&trigA);
    exitBtn.SetTrigger(&trigHome);
    exitBtn.SetEffectGrow();

    HaltGui();
    GuiWindow w(screenwidth, screenheight);
    w.Append(&titleTxt);
    w.Append(&fileBtn);
    w.Append(&optionBtn);
    w.Append(&savingBtn);
    // w.Append(&cheatsBtn); // unused
    w.Append(&exitBtn);

    mainWindow->Append(&w);

    ResumeGui();

    while (menu == MENU_NONE) {
        UGUI();
        usleep(THREAD_SLEEP);

        if (fileBtn.GetState() == STATE_CLICKED) {
            menu = MENU_BROWSE_DEVICE;
        } else if (optionBtn.GetState() == STATE_CLICKED) {
            menu = MENU_OPTIONS;
        } else if (savingBtn.GetState() == STATE_CLICKED) {
            menu = MENU_SAVE;
        } else if (cheatsBtn.GetState() == STATE_CLICKED) {
            menu = MENU_CHEATS;
        } else if (exitBtn.GetState() == STATE_CLICKED) {

            exitBtn.ResetState();

            int choice = WindowPrompt(
                    "Exit",
                    "Are you sure that you want to exit?",
                    "Yes",
                    "No");
            if (choice == 1) {
                menu = MENU_EXIT;
            }

        }
    }

    HaltGui();
    mainWindow->Remove(&w);
    return menu;
}

/****************************************************************************
 * MainMenu
 ***************************************************************************/
void MainMenu(int menu) {


    TR;
    int currentMenu = menu;

#ifdef HW_RVL
    pointer[0] = new GuiImageData(player1_point_png);
    pointer[1] = new GuiImageData(player2_point_png);
    pointer[2] = new GuiImageData(player3_point_png);
    pointer[3] = new GuiImageData(player4_point_png);
#endif

    GuiImageData * background = new GuiImageData(xenon_bg_png);

    mainWindow = new GuiWindow(screenwidth, screenheight);

    bgImg = new GuiImage(background);

    //    bgImg = new GuiImage(screenwidth, screenheight, (GXColor) {
    //        50, 50, 50, 255
    //    });
    //    bgImg->ColorStripe(30);
    mainWindow->Append(bgImg);

    GuiTrigger trigA;
    //	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
    trigA.SetSimpleTrigger(-1, 0, PAD_BUTTON_A);

    ResumeGui();

    bgMusic = new GuiSound(bg_music_ogg, bg_music_ogg_size, SOUND_OGG);
    bgMusic->SetVolume(50);
    bgMusic->Play(); // startup music

    while (currentMenu != MENU_EXIT) {
        switch (currentMenu) {
            case MENU_SETTINGS:
                currentMenu = MainMenu();
                break;
            case MENU_OPTIONS:
                currentMenu = MenuOptions();
                break;
            case MENU_BROWSE_DEVICE:
                currentMenu = MenuBrowseDevice();
                break;
            case MENU_IN_GAME:
                currentMenu = MenuInGame();
            default: // unrecognized menu
                currentMenu = MainMenu();
                break;
        }
    }
    ResumeGui();
    ExitRequested = 1;

    while (1) {
        UGUI();
        usleep(THREAD_SLEEP);
    }

    HaltGui();

    bgMusic->Stop();
    delete bgMusic;
    delete bgImg;
    delete mainWindow;

    delete pointer[0];
    delete pointer[1];
    delete pointer[2];
    delete pointer[3];

    mainWindow = NULL;
}

