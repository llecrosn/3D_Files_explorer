#include "context_menu.h"
#include "filesystem.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>

// --- SYSTÈME DE LISTE D'APPLICATIONS ---
typedef struct {
    char name[64];
    char exec[128];
} AppItem;

AppItem installedApps[200];
int installedAppCount = 0;
int appScrollIndex = 0;

int CompareApps(const void *a, const void *b) {
    return strcasecmp(((AppItem *)a)->name, ((AppItem *)b)->name);
}

void LoadInstalledApps(void) {
    if (installedAppCount > 0) return;

    DIR *d = opendir("/usr/share/applications");
    if (!d) return;

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (strstr(dir->d_name, ".desktop")) {
            char path[512];
            snprintf(path, sizeof(path), "/usr/share/applications/%s", dir->d_name);
            FILE *f = fopen(path, "r");
            if (f) {
                char line[256];
                bool isApp = false;
                bool noDisplay = false;
                char tempName[64] = {0};
                char tempExec[128] = {0};

                while (fgets(line, sizeof(line), f)) {
                    if (strncmp(line, "Type=Application", 16) == 0) isApp = true;
                    if (strncmp(line, "NoDisplay=true", 14) == 0) noDisplay = true;

                    if (strncmp(line, "Name=", 5) == 0 && tempName[0] == '\0') {
                        strncpy(tempName, line + 5, 63);
                        tempName[strcspn(tempName, "\n")] = 0;
                    }
                    if (strncmp(line, "Exec=", 5) == 0 && tempExec[0] == '\0') {
                        strncpy(tempExec, line + 5, 127);
                        tempExec[strcspn(tempExec, "\n")] = 0;
                        char *p = strstr(tempExec, "%");
                        if (p) *p = '\0';
                    }
                }
                fclose(f);

                if (isApp && !noDisplay && tempName[0] != '\0' && tempExec[0] != '\0') {
                    strncpy(installedApps[installedAppCount].name, tempName, 63);
                    strncpy(installedApps[installedAppCount].exec, tempExec, 127);
                    installedAppCount++;
                    if (installedAppCount >= 200) break;
                }
            }
        }
    }
    closedir(d);
    qsort(installedApps, installedAppCount, sizeof(AppItem), CompareApps);
}

// --- VARIABLES GLOBALES DU MENU ---
bool isContextMenuOpen = false;
int targetFileIndex = -1;

bool isConfirmingDelete = false;
bool isRenaming = false;
bool isCreatingFolder = false;
bool isCreatingFile = false;
bool isOpeningWith = false;
char newNameBuffer[256] = {0};
int letterCount = 0;

extern FileNode files[];

void OpenContextMenu(int fileIndex) {
    targetFileIndex = fileIndex;
    isContextMenuOpen = true;
    isCreatingFile = false;
    isConfirmingDelete = false;
    isRenaming = false;
    isCreatingFolder = false;
    isOpeningWith = false;
    memset(newNameBuffer, 0, 256);
    letterCount = 0;
    EnableCursor();
}

void CloseContextMenu(void) {
    isContextMenuOpen = false;
    isConfirmingDelete = false;
    isRenaming = false;
    isCreatingFile = false;
    isCreatingFolder = false;
    isOpeningWith = false;
    targetFileIndex = -1;
    DisableCursor();
}

void UpdateAndDrawContextMenu(int screenWidth, int screenHeight) {
    if (!isContextMenuOpen) return;

    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(COLOR_VOID, 0.6f));

    int menuWidth = 350;
    int menuHeight = isOpeningWith ? 400 : 260;
    int menuX = (screenWidth - menuWidth) / 2;
    int menuY = (screenHeight - menuHeight) / 2;

    DrawRectangle(menuX, menuY, menuWidth, menuHeight, Fade(BLACK, 0.9f));

    Color borderColor = COLOR_NEON_CYAN;
    if (isConfirmingDelete) borderColor = RED;
    if (isRenaming || isCreatingFolder || isCreatingFile || isOpeningWith) borderColor = ORANGE;
    DrawRectangleLines(menuX, menuY, menuWidth, menuHeight, borderColor);

    char *menuTitle = (targetFileIndex == -1) ? "Espace vide" : files[targetFileIndex].name;
    DrawText(menuTitle, menuX + 20, menuY + 20, 20, borderColor);
    DrawLine(menuX + 20, menuY + 45, menuX + menuWidth - 20, menuY + 45, Fade(WHITE, 0.3f));

    // --- ÉTAT 1 : LISTE DES APPLICATIONS ("Ouvrir Avec") ---
    if (isOpeningWith) {
        LoadInstalledApps(); 

        DrawText("Choisir une application :", menuX + 20, menuY + 20, 20, COLOR_NEON_CYAN);
        DrawLine(menuX + 20, menuY + 45, menuX + menuWidth - 20, menuY + 45, Fade(WHITE, 0.3f));

        appScrollIndex -= GetMouseWheelMove() * 2;
        if (appScrollIndex < 0) appScrollIndex = 0;
        if (appScrollIndex > installedAppCount - 8) appScrollIndex = installedAppCount - 8;
        if (appScrollIndex < 0) appScrollIndex = 0;

        int visibleItems = 8;
        for (int i = 0; i < visibleItems && (i + appScrollIndex) < installedAppCount; i++) {
            int actualIndex = i + appScrollIndex;
            Rectangle itemRec = {menuX + 20, menuY + 60 + (i * 40), menuWidth - 40, 35};

            bool hoverItem = CheckCollisionPointRec(GetMousePosition(), itemRec);

            DrawRectangleRec(itemRec, hoverItem ? Fade(COLOR_NEON_CYAN, 0.4f) : Fade(WHITE, 0.05f));
            DrawText(installedApps[actualIndex].name, itemRec.x + 10, itemRec.y + 10, 15, WHITE);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoverItem) {
                char cmd[512];
                snprintf(cmd, sizeof(cmd), "%s \"%s\" &", installedApps[actualIndex].exec, files[targetFileIndex].name);
                system(cmd);
                CloseContextMenu();
            }
        }

        DrawText("ÉCHAP ou Clic extérieur pour annuler", menuX + 20, menuY + 370, 10, GRAY);

        if (IsKeyPressed(KEY_ESCAPE) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(GetMousePosition(), (Rectangle){menuX, menuY, menuWidth, menuHeight}))) {
            CloseContextMenu();
        }
    }
    // --- ÉTAT 2 : SAISIE DE TEXTE (Création ou Renommage) ---
    else if (isRenaming || isCreatingFolder || isCreatingFile) {
        char *promptText = "Nouveau nom :";
        if (isCreatingFolder) promptText = "Nom du dossier :";
        if (isCreatingFile) promptText = "Nom du fichier :";

        DrawText(promptText, menuX + 20, menuY + 60, 20, WHITE);

        DrawRectangle(menuX + 20, menuY + 90, menuWidth - 40, 40, Fade(WHITE, 0.1f));
        DrawRectangleLines(menuX + 20, menuY + 90, menuWidth - 40, 40, ORANGE);
        DrawText(newNameBuffer, menuX + 30, menuY + 100, 20, WHITE);

        if (((int)(GetTime() * 2)) % 2 == 0) {
            int textW = MeasureText(newNameBuffer, 20);
            DrawText("_", menuX + 30 + textW, menuY + 100, 20, ORANGE);
        }

        DrawText("ENTRÉE pour valider | ECHAP pour annuler", menuX + 20, menuY + 180, 10, GRAY);

        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125) && (letterCount < 255)) {
                newNameBuffer[letterCount] = (char)key;
                newNameBuffer[letterCount + 1] = '\0';
                letterCount++;
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE)) {
            letterCount--;
            if (letterCount < 0) letterCount = 0;
            newNameBuffer[letterCount] = '\0';
        }

        if (IsKeyPressed(KEY_ESCAPE) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(GetMousePosition(), (Rectangle){menuX, menuY, menuWidth, menuHeight}))) {
            CloseContextMenu();
        }

        if (IsKeyPressed(KEY_ENTER) && letterCount > 0) {
            if (isCreatingFolder) {
                if (mkdir(newNameBuffer, 0777) == 0) LoadFiles(".");
            } else if (isCreatingFile) {
                FILE *f = fopen(newNameBuffer, "w");
                if (f) { fclose(f); LoadFiles("."); }
            } else if (isRenaming) {
                if (rename(files[targetFileIndex].name, newNameBuffer) == 0) LoadFiles(".");
            }
            CloseContextMenu();
        }
    }
    // --- ÉTAT 3 : DEMANDE DE CONFIRMATION DE SUPPRESSION ---
    else if (isConfirmingDelete) {
        DrawText("Voulez-vous VRAIMENT", menuX + 20, menuY + 60, 20, WHITE);
        DrawText("supprimer ce fichier ?", menuX + 20, menuY + 85, 20, WHITE);

        Rectangle btnYes = {menuX + 20, menuY + 140, 140, 40};
        Rectangle btnNo = {menuX + 190, menuY + 140, 140, 40};

        bool hoverYes = CheckCollisionPointRec(GetMousePosition(), btnYes);
        bool hoverNo = CheckCollisionPointRec(GetMousePosition(), btnNo);

        DrawRectangleRec(btnYes, hoverYes ? Fade(RED, 0.8f) : Fade(RED, 0.4f));
        DrawRectangleLinesEx(btnYes, 1, RED);
        DrawText("OUI", btnYes.x + 50, btnYes.y + 10, 20, WHITE);

        DrawRectangleRec(btnNo, hoverNo ? Fade(GRAY, 0.8f) : Fade(GRAY, 0.4f));
        DrawRectangleLinesEx(btnNo, 1, GRAY);
        DrawText("NON", btnNo.x + 50, btnNo.y + 10, 20, WHITE);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (hoverYes) {
                if (remove(files[targetFileIndex].name) == 0) LoadFiles(".");
                CloseContextMenu();
            } else if (hoverNo) {
                isConfirmingDelete = false;
            }
        }
    }
    // --- ÉTAT 4 : MENU NORMAL ---
    else {
        if (targetFileIndex != -1) {
            // UN FICHIER CIBLÉ
            int startY = menuY + 60;
            Rectangle btnOpen     = { menuX + 20, startY, menuWidth - 40, 35 };
            Rectangle btnOpenWith = { menuX + 20, startY + 45, menuWidth - 40, 35 };
            Rectangle btnRename   = { menuX + 20, startY + 90, menuWidth - 40, 35 };
            Rectangle btnDelete   = { menuX + 20, startY + 135, menuWidth - 40, 35 };
            
            bool hoverOpen     = CheckCollisionPointRec(GetMousePosition(), btnOpen);
            bool hoverOpenWith = CheckCollisionPointRec(GetMousePosition(), btnOpenWith);
            bool hoverRename   = CheckCollisionPointRec(GetMousePosition(), btnRename);
            bool hoverDelete   = CheckCollisionPointRec(GetMousePosition(), btnDelete);
            
            DrawRectangleRec(btnOpen, hoverOpen ? Fade(COLOR_NEON_GREEN, 0.5f) : Fade(COLOR_NEON_GREEN, 0.2f));
            DrawRectangleLinesEx(btnOpen, 1, COLOR_NEON_GREEN);
            DrawText("Ouvrir (Défaut)", btnOpen.x + 10, btnOpen.y + 8, 20, WHITE);

            DrawRectangleRec(btnOpenWith, hoverOpenWith ? Fade(COLOR_NEON_CYAN, 0.5f) : Fade(COLOR_NEON_CYAN, 0.2f));
            DrawRectangleLinesEx(btnOpenWith, 1, COLOR_NEON_CYAN);
            DrawText("Ouvrir avec...", btnOpenWith.x + 10, btnOpenWith.y + 8, 20, WHITE);

            DrawRectangleRec(btnRename, hoverRename ? Fade(ORANGE, 0.5f) : Fade(ORANGE, 0.2f));
            DrawRectangleLinesEx(btnRename, 1, ORANGE);
            DrawText("Renommer", btnRename.x + 10, btnRename.y + 8, 20, WHITE);

            DrawRectangleRec(btnDelete, hoverDelete ? Fade(RED, 0.5f) : Fade(RED, 0.2f));
            DrawRectangleLinesEx(btnDelete, 1, RED);
            DrawText("Supprimer", btnDelete.x + 10, btnDelete.y + 8, 20, WHITE);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (hoverOpen) {
                    char cmd[512];
                    snprintf(cmd, sizeof(cmd), "xdg-open \"%s\" &", files[targetFileIndex].name);
                    system(cmd);
                    CloseContextMenu();
                }
                else if (hoverOpenWith) isOpeningWith = true;
                else if (hoverRename) isRenaming = true;
                else if (hoverDelete) isConfirmingDelete = true;
                else if (!CheckCollisionPointRec(GetMousePosition(), (Rectangle){menuX, menuY, menuWidth, menuHeight})) CloseContextMenu();
            }
        } else {
            // LE VIDE CIBLÉ
            Rectangle btnNewFolder = {menuX + 20, menuY + 60, menuWidth - 40, 40};
            Rectangle btnNewFile = {menuX + 20, menuY + 110, menuWidth - 40, 40};
            Rectangle btnTerminal = {menuX + 20, menuY + 160, menuWidth - 40, 40};

            bool hoverNewFolder = CheckCollisionPointRec(GetMousePosition(), btnNewFolder);
            bool hoverNewFile = CheckCollisionPointRec(GetMousePosition(), btnNewFile);
            bool hoverTerminal = CheckCollisionPointRec(GetMousePosition(), btnTerminal);

            DrawRectangleRec(btnNewFolder, hoverNewFolder ? Fade(COLOR_NEON_CYAN, 0.5f) : Fade(COLOR_NEON_CYAN, 0.2f));
            DrawRectangleLinesEx(btnNewFolder, 1, COLOR_NEON_CYAN);
            DrawText("Nouveau Dossier", btnNewFolder.x + 10, btnNewFolder.y + 10, 20, WHITE);

            DrawRectangleRec(btnNewFile, hoverNewFile ? Fade(COLOR_NEON_GREEN, 0.5f) : Fade(COLOR_NEON_GREEN, 0.2f));
            DrawRectangleLinesEx(btnNewFile, 1, COLOR_NEON_GREEN);
            DrawText("Nouveau Fichier", btnNewFile.x + 10, btnNewFile.y + 10, 20, WHITE);

            DrawRectangleRec(btnTerminal, hoverTerminal ? Fade(ORANGE, 0.5f) : Fade(ORANGE, 0.2f));
            DrawRectangleLinesEx(btnTerminal, 1, ORANGE);
            DrawText(">_ Ouvrir Terminal", btnTerminal.x + 10, btnTerminal.y + 10, 20, WHITE);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (hoverNewFolder) isCreatingFolder = true;
                else if (hoverNewFile) isCreatingFile = true;
                else if (hoverTerminal) {
                    system("gnome-terminal &");
                    CloseContextMenu();
                }
                else if (!CheckCollisionPointRec(GetMousePosition(), (Rectangle){menuX, menuY, menuWidth, menuHeight})) CloseContextMenu();
            }
        }
    }
}