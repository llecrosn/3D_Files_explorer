#ifndef CONTEXT_MENU_H
#define CONTEXT_MENU_H

#include "raylib.h"
#include <stdbool.h>


// Variables globales d'état du menu
extern bool isContextMenuOpen;
extern int targetFileIndex;
// État de confirmation pour les actions dangereuses
extern bool isConfirmingDelete;
// --- Variables pour le renommage ---
extern bool isRenaming;
extern bool isCreatingFolder;
extern bool isCreatingFile;
extern char newNameBuffer[256];
extern int letterCount;

// Fonctions
void OpenContextMenu(int fileIndex);
void CloseContextMenu(void);

// Gère l'affichage et les clics sur les boutons du menu
void UpdateAndDrawContextMenu(int screenWidth, int screenHeight);

#endif
