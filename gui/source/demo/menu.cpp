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

    GuiText msgTxt(msg, 22, ColorGrey);
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

    sprintf(title, "Browse Files");

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

    GuiText backBtnTxt("Go Back", 24, ColorGrey2);
    GuiImage backBtnImg(&btnOutline);
    GuiImage backBtnImgOver(&btnOutlineOver);
    GuiButton backBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    backBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
    backBtn.SetPosition(30, -35);
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
                    mainWindow->SetState(STATE_DISABLED);
                    // load file
                    printf("Launch : %s\r\n",browserList[browser.selIndex].filename);
                    mainWindow->SetState(STATE_DEFAULT);
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
 * MenuSettings
 ***************************************************************************/
static int MenuSettings() {
    int menu = MENU_NONE;

    GuiText titleTxt("Settings", 28, ColorGrey);
    titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
    titleTxt.SetPosition(50, 50);

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(xenon_button_png);
    GuiImageData btnOutlineOver(xenon_button_over_png);
    GuiImageData btnLargeOutline(xenon_button_large_png);
    GuiImageData btnLargeOutlineOver(xenon_button_large_over_png);

    GuiTrigger trigA;
    //	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
    trigA.SetSimpleTrigger(-1, 0, PAD_BUTTON_A);
    GuiTrigger trigHome;
    //	trigHome.SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME, 0);
    trigHome.SetButtonOnlyTrigger(-1, 0, 0);

    GuiText fileBtnTxt("File Browser", 22, ColorGrey2);
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

    GuiText videoBtnTxt("Video", 22, ColorGrey2);
    videoBtnTxt.SetWrap(true, btnLargeOutline.GetWidth() - 30);
    GuiImage videoBtnImg(&btnLargeOutline);
    GuiImage videoBtnImgOver(&btnLargeOutlineOver);
    GuiButton videoBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
    videoBtn.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    videoBtn.SetPosition(0, 120);
    videoBtn.SetLabel(&videoBtnTxt);
    videoBtn.SetImage(&videoBtnImg);
    videoBtn.SetImageOver(&videoBtnImgOver);
    videoBtn.SetSoundOver(&btnSoundOver);
    videoBtn.SetTrigger(&trigA);
    videoBtn.SetEffectGrow();

    GuiText savingBtnTxt1("Saving", 22, ColorGrey2);

    GuiText savingBtnTxt2("&", 18, ColorGrey2);

    GuiText savingBtnTxt3("Loading", 22, ColorGrey2);
    savingBtnTxt1.SetPosition(0, -20);
    savingBtnTxt3.SetPosition(0, +20);
    GuiImage savingBtnImg(&btnLargeOutline);
    GuiImage savingBtnImgOver(&btnLargeOutlineOver);
    GuiButton savingBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
    savingBtn.SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
    savingBtn.SetPosition(-50, 120);
    savingBtn.SetLabel(&savingBtnTxt1, 0);
    savingBtn.SetLabel(&savingBtnTxt2, 1);
    savingBtn.SetLabel(&savingBtnTxt3, 2);
    savingBtn.SetImage(&savingBtnImg);
    savingBtn.SetImageOver(&savingBtnImgOver);
    savingBtn.SetSoundOver(&btnSoundOver);
    savingBtn.SetTrigger(&trigA);
    savingBtn.SetEffectGrow();

    GuiText menuBtnTxt("Menu", 22, ColorGrey2);
    menuBtnTxt.SetWrap(true, btnLargeOutline.GetWidth() - 30);
    GuiImage menuBtnImg(&btnLargeOutline);
    GuiImage menuBtnImgOver(&btnLargeOutlineOver);
    GuiButton menuBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
    menuBtn.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    menuBtn.SetPosition(-125, 250);
    menuBtn.SetLabel(&menuBtnTxt);
    menuBtn.SetImage(&menuBtnImg);
    menuBtn.SetImageOver(&menuBtnImgOver);
    menuBtn.SetSoundOver(&btnSoundOver);
    menuBtn.SetTrigger(&trigA);
    menuBtn.SetEffectGrow();

    GuiText networkBtnTxt("Network", 22, ColorGrey2);
    networkBtnTxt.SetWrap(true, btnLargeOutline.GetWidth() - 30);
    GuiImage networkBtnImg(&btnLargeOutline);
    GuiImage networkBtnImgOver(&btnLargeOutlineOver);
    GuiButton networkBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
    networkBtn.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    networkBtn.SetPosition(125, 250);
    networkBtn.SetLabel(&networkBtnTxt);
    networkBtn.SetImage(&networkBtnImg);
    networkBtn.SetImageOver(&networkBtnImgOver);
    networkBtn.SetSoundOver(&btnSoundOver);
    networkBtn.SetTrigger(&trigA);
    networkBtn.SetEffectGrow();

    GuiText exitBtnTxt("Exit", 22, ColorGrey2);
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

    GuiText resetBtnTxt("Reset Settings", 22, ColorGrey2);
    GuiImage resetBtnImg(&btnOutline);
    GuiImage resetBtnImgOver(&btnOutlineOver);
    GuiButton resetBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    resetBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
    resetBtn.SetPosition(-100, -35);
    resetBtn.SetLabel(&resetBtnTxt);
    resetBtn.SetImage(&resetBtnImg);
    resetBtn.SetImageOver(&resetBtnImgOver);
    resetBtn.SetSoundOver(&btnSoundOver);
    resetBtn.SetTrigger(&trigA);
    resetBtn.SetEffectGrow();

    HaltGui();
    GuiWindow w(screenwidth, screenheight);
    w.Append(&titleTxt);
    w.Append(&fileBtn);
    w.Append(&videoBtn);
    w.Append(&savingBtn);
    w.Append(&menuBtn);

#ifdef HW_RVL
    w.Append(&networkBtn);
#endif

    w.Append(&exitBtn);
    w.Append(&resetBtn);

    mainWindow->Append(&w);

    ResumeGui();

    while (menu == MENU_NONE) {
        UGUI();
        usleep(THREAD_SLEEP);

        if (fileBtn.GetState() == STATE_CLICKED) {
            menu = MENU_BROWSE_DEVICE;
        } else if (videoBtn.GetState() == STATE_CLICKED) {
            menu = MENU_SETTINGS_FILE;
        } else if (savingBtn.GetState() == STATE_CLICKED) {
            menu = MENU_SETTINGS_FILE;
        } else if (menuBtn.GetState() == STATE_CLICKED) {
            menu = MENU_SETTINGS_FILE;
        } else if (networkBtn.GetState() == STATE_CLICKED) {
            menu = MENU_SETTINGS_FILE;
        } else if (exitBtn.GetState() == STATE_CLICKED) {
            menu = MENU_EXIT;
        } else if (resetBtn.GetState() == STATE_CLICKED) {
            resetBtn.ResetState();

            int choice = WindowPrompt(
                    "Reset Settings",
                    "Are you sure that you want to reset your settings?",
                    "Yes",
                    "No");
            if (choice == 1) {
                // reset settings
            }
        }
    }

    HaltGui();
    mainWindow->Remove(&w);
    return menu;
}

static int MenuTest() {
    int menu = MENU_NONE;

    GuiImageData btnLargeOutline(xenon_popup_png);

    GuiWindow promptWindow(640, 360);

    promptWindow.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);

    GuiImage fileBtnImg(&btnLargeOutline);

    fileBtnImg.SetPosition(0, 0);

    GuiText txt("Ced", 22, (GXColor) {
        0, 0, 0, 255
    });
    txt.SetPosition(100, 100);

    HaltGui();

    promptWindow.Append(&fileBtnImg);
    //    w.Append(&fileBtnImg2);
    //    w.Append(&fileBtnImg3);
    promptWindow.Append(&txt);

    mainWindow->Append(&promptWindow);

    ResumeGui();

    while (menu == MENU_NONE) {
        UGUI();
        usleep(THREAD_SLEEP);
    }

    HaltGui();
    mainWindow->Remove(&promptWindow);
    return menu;
}

/****************************************************************************
 * MenuSettingsFile
 ***************************************************************************/

static int MenuSettingsFile() {
    int menu = MENU_NONE;
    int ret;
    int i = 0;
    bool firstRun = true;
    OptionList options;
    sprintf(options.name[i++], "Load Device");
    sprintf(options.name[i++], "Save Device");
    sprintf(options.name[i++], "Folder 1");
    sprintf(options.name[i++], "Folder 2");
    sprintf(options.name[i++], "Folder 3");
    sprintf(options.name[i++], "Auto Load");
    sprintf(options.name[i++], "Auto Save");
    options.length = i;

    GuiText titleTxt("Settings - Saving & Loading", 28, ColorGrey);
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

    GuiOptionBrowser optionBrowser(552, 248, &options);
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
            case 0:
                Settings.LoadMethod++;
                break;

            case 1:
                Settings.SaveMethod++;
                break;

            case 2:
                OnScreenKeyboard(Settings.Folder1, 256);
                break;

            case 3:
                OnScreenKeyboard(Settings.Folder2, 256);
                break;

            case 4:
                OnScreenKeyboard(Settings.Folder3, 256);
                break;

            case 5:
                Settings.AutoLoad++;
                if (Settings.AutoLoad > 2)
                    Settings.AutoLoad = 0;
                break;

            case 6:
                Settings.AutoSave++;
                if (Settings.AutoSave > 3)
                    Settings.AutoSave = 0;
                break;
        }

        if (ret >= 0 || firstRun) {
            firstRun = false;

            // correct load/save methods out of bounds
            if (Settings.LoadMethod > 4)
                Settings.LoadMethod = 0;
            if (Settings.SaveMethod > 6)
                Settings.SaveMethod = 0;

            if (Settings.LoadMethod == METHOD_AUTO) sprintf(options.value[0], "Auto Detect");
            else if (Settings.LoadMethod == METHOD_SD) sprintf(options.value[0], "SD");
            else if (Settings.LoadMethod == METHOD_USB) sprintf(options.value[0], "USB");
            else if (Settings.LoadMethod == METHOD_DVD) sprintf(options.value[0], "DVD");
            else if (Settings.LoadMethod == METHOD_SMB) sprintf(options.value[0], "Network");

            if (Settings.SaveMethod == METHOD_AUTO) sprintf(options.value[1], "Auto Detect");
            else if (Settings.SaveMethod == METHOD_SD) sprintf(options.value[1], "SD");
            else if (Settings.SaveMethod == METHOD_USB) sprintf(options.value[1], "USB");
            else if (Settings.SaveMethod == METHOD_SMB) sprintf(options.value[1], "Network");
            else if (Settings.SaveMethod == METHOD_MC_SLOTA) sprintf(options.value[1], "MC Slot A");
            else if (Settings.SaveMethod == METHOD_MC_SLOTB) sprintf(options.value[1], "MC Slot B");

            snprintf(options.value[2], 256, "%s", Settings.Folder1);
            snprintf(options.value[3], 256, "%s", Settings.Folder2);
            snprintf(options.value[4], 256, "%s", Settings.Folder3);

            if (Settings.AutoLoad == 0) sprintf(options.value[5], "Off");
            else if (Settings.AutoLoad == 1) sprintf(options.value[5], "Some");
            else if (Settings.AutoLoad == 2) sprintf(options.value[5], "All");

            if (Settings.AutoSave == 0) sprintf(options.value[5], "Off");
            else if (Settings.AutoSave == 1) sprintf(options.value[6], "Some");
            else if (Settings.AutoSave == 2) sprintf(options.value[6], "All");

            optionBrowser.TriggerUpdate();
        }

        if (backBtn.GetState() == STATE_CLICKED) {
            menu = MENU_SETTINGS;
        }

    }
    HaltGui();
    mainWindow->Remove(&optionBrowser);
    mainWindow->Remove(&w);
    mainWindow->Remove(&titleTxt);
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
                currentMenu = MenuSettings();
                break;
            case MENU_SETTINGS_FILE:
                currentMenu = MenuSettingsFile();
                break;
            case MENU_BROWSE_DEVICE:
                currentMenu = MenuBrowseDevice();
                break;
            default: // unrecognized menu
                currentMenu = MenuSettings();
                break;
        }
        //        currentMenu=MenuTest();
        //        currentMenu = MenuBrowseDevice();
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

