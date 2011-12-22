/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * filebrowser.cpp
 *
 * Generic file routines - reading, writing, browsing
 ***************************************************************************/

#include <xetypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <wiiuse/wpad.h>
//#include <sys/dir.h>
#include <malloc.h>

#include "filebrowser.h"
#include "menu.h"

BROWSERINFO browser;
BROWSERENTRY * browserList = NULL; // list of files/folders in browser

char rootdir[10];

/****************************************************************************
 * CleanupPath()
 * Cleans up the filepath, removing double // and replacing \ with /
 ***************************************************************************/
static void CleanupPath(char * path) {
    if (!path || path[0] == 0)
        return;

    int pathlen = strlen(path);
    int j = 0;
    for (int i = 0; i < pathlen && i < MAXPATHLEN; i++) {
        if (path[i] == '\\')
            path[i] = '/';

        if (j == 0 || !(path[j - 1] == '/' && path[i] == '/'))
            path[j++] = path[i];
    }
    path[j] = 0;
}
enum {
    FILE_SRAM,
    FILE_SNAPSHOT,
    FILE_ROM,
    FILE_CHEAT
};
bool MakeFilePath(char filepath[], int type, char * filename, int filenum) {
    char file[512];
    char folder[1024];
    char ext[4];
    char temppath[MAXPATHLEN];

    if (type == FILE_ROM) {
        // Check path length
        if ((strlen(browser.dir) + 1 + strlen(browserList[browser.selIndex].filename)) >= MAXPATHLEN) {
//            ErrorPrompt("Maximum filepath length reached!");
            filepath[0] = 0;
            return false;
        } else {
            sprintf(temppath, "%s%s", browser.dir, browserList[browser.selIndex].filename);
        }
    } else {
//        if (GCSettings.SaveMethod == DEVICE_AUTO)
//            GCSettings.SaveMethod = autoSaveMethod(SILENT);
//
//        if (GCSettings.SaveMethod == DEVICE_AUTO)
//            return false;

        switch (type) {
            case FILE_SRAM:
            case FILE_SNAPSHOT:
                sprintf(folder, "genesis");

                if (type == FILE_SRAM) sprintf(ext, "srm");
                else sprintf(ext, "frz");

                if (filenum >= -1) {
                    if (filenum == -1)
                        sprintf(file, "%s.%s", filename, ext);
                    else if (filenum == 0)
                        sprintf(file, "%s Auto.%s", filename, ext);
                    else
                        sprintf(file, "%s %i.%s", filename, filenum, ext);
                } else {
                    sprintf(file, "%s", filename);
                }
                break;
            case FILE_CHEAT:
                sprintf(folder, "genesis");
                sprintf(file, "%s.cht", "test");
                break;
        }
        sprintf(temppath, "%s%s/%s", "uda:/", folder, file);
    }
    CleanupPath(temppath); // cleanup path
    snprintf(filepath, MAXPATHLEN, "%s", temppath);
    return true;
}

/****************************************************************************
 * ResetBrowser()
 * Clears the file browser memory, and allocates one initial entry
 ***************************************************************************/
void ResetBrowser() {
    browser.numEntries = 0;
    browser.selIndex = 0;
    browser.pageIndex = 0;

    // Clear any existing values
    if (browserList != NULL) {
        free(browserList);
        browserList = NULL;
    }
    // set aside space for 1 entry
    browserList = (BROWSERENTRY *) malloc(sizeof (BROWSERENTRY));
    memset(browserList, 0, sizeof (BROWSERENTRY));
}

/****************************************************************************
 * UpdateDirName()
 * Update curent directory name for file browser
 ***************************************************************************/
int UpdateDirName() {
    int size = 0;
    char * test;
    char temp[1024];

    /* current directory doesn't change */
    if (strcmp(browserList[browser.selIndex].filename, ".") == 0) {
        return 0;
    }/* go up to parent directory */
    else if (strcmp(browserList[browser.selIndex].filename, "..") == 0) {
        /* determine last subdirectory namelength */
        sprintf(temp, "%s", browser.dir);
        test = strtok(temp, "/");
        while (test != NULL) {
            size = strlen(test);
            test = strtok(NULL, "/");
        }

        /* remove last subdirectory name */
        size = strlen(browser.dir) - size - 1;
        browser.dir[size] = 0;

        return 1;
    }/* Open a directory */
    else {
        /* test new directory namelength */
        if ((strlen(browser.dir) + 1 + strlen(browserList[browser.selIndex].filename)) < MAXPATHLEN) {
            /* update current directory name */
            sprintf(browser.dir, "%s/%s", browser.dir, browserList[browser.selIndex].filename);
            return 1;
        } else {
            return -1;
        }
    }
}

/****************************************************************************
 * FileSortCallback
 *
 * Quick sort callback to sort file entries with the following order:
 *   .
 *   ..
 *   <dirs>
 *   <files>
 ***************************************************************************/
int FileSortCallback(const void *f1, const void *f2) {
    /* Special case for implicit directories */
    if (((BROWSERENTRY *) f1)->filename[0] == '.' || ((BROWSERENTRY *) f2)->filename[0] == '.') {
        if (strcmp(((BROWSERENTRY *) f1)->filename, ".") == 0) {
            return -1;
        }
        if (strcmp(((BROWSERENTRY *) f2)->filename, ".") == 0) {
            return 1;
        }
        if (strcmp(((BROWSERENTRY *) f1)->filename, "..") == 0) {
            return -1;
        }
        if (strcmp(((BROWSERENTRY *) f2)->filename, "..") == 0) {
            return 1;
        }
    }

    /* If one is a file and one is a directory the directory is first. */
    if (((BROWSERENTRY *) f1)->isdir && !(((BROWSERENTRY *) f2)->isdir)) return -1;
    if (!(((BROWSERENTRY *) f1)->isdir) && ((BROWSERENTRY *) f2)->isdir) return 1;

    return stricmp(((BROWSERENTRY *) f1)->filename, ((BROWSERENTRY *) f2)->filename);
}

/***************************************************************************
 * Browse subdirectories
 **************************************************************************/
int
ParseDirectory() {
    DIR *dir = NULL;
    char fulldir[MAXPATHLEN];
    struct dirent *entry;

    // reset browser
    ResetBrowser();

    // open the directory
    sprintf(fulldir, "%s%s", rootdir, browser.dir); // add currentDevice to path
    dir = opendir(fulldir);

    // if we can't open the dir, try opening the root dir
    if (dir == NULL) {
        sprintf(browser.dir, "/");
        dir = opendir(rootdir);
        if (dir == NULL) {
            return -1;
        }
    }

    // index files/folders
    int entryNum = 0;

    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0)
            continue;

        BROWSERENTRY * newBrowserList = (BROWSERENTRY *) realloc(browserList, (entryNum + 1) * sizeof (BROWSERENTRY));

        if (!newBrowserList) // failed to allocate required memory
        {
            ResetBrowser();
            entryNum = -1;
            break;
        } else {
            browserList = newBrowserList;
        }
        memset(&(browserList[entryNum]), 0, sizeof (BROWSERENTRY)); // clear the new entry

        strncpy(browserList[entryNum].filename, entry->d_name, MAXJOLIET);

        if (strcmp(entry->d_name, "..") == 0) {
            sprintf(browserList[entryNum].displayname, "Up One Level");
            browserList[entryNum].isdir = 1; // flag this as a dir
        } else {
            strncpy(browserList[entryNum].displayname, entry->d_name, MAXDISPLAY); // crop name for display

            if (entry->d_type == DT_DIR)
                browserList[entryNum].isdir = 1; // flag this as a dir
        }

        printf("entry->d_name = %s\r\n",entry->d_name);

        entryNum++;
    }

    // close directory
    closedir(dir);

    // Sort the file list
    qsort(browserList, entryNum, sizeof (BROWSERENTRY), FileSortCallback);

    browser.numEntries = entryNum;
    return entryNum;
}

/****************************************************************************
 * BrowserChangeFolder
 *
 * Update current directory and set new entry list if directory has changed
 ***************************************************************************/
int BrowserChangeFolder() {
    if (!UpdateDirName())
        return -1;

    ParseDirectory();

    return browser.numEntries;
}

/****************************************************************************
 * BrowseDevice
 * Displays a list of files on the selected device
 ***************************************************************************/
int BrowseDevice() {
    sprintf(browser.dir, "/");
    sprintf(rootdir, "uda:/");
    ParseDirectory(); // Parse root directory
    return browser.numEntries;
}


/****************************************************************************
 * BrowseDevice
 * Displays a list of files on the selected device
 ***************************************************************************/
int BrowseDevice(const char * dir, const char * root) {
    sprintf(browser.dir, dir);
    sprintf(rootdir, root);
    ParseDirectory(); // Parse root directory
    return browser.numEntries;
}
