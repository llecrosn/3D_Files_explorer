#ifndef CONSTRUCTION_H
#define CONSTRUCTION_H

#include "raylib.h"
#include "filesystem.h"

// Variables d'état
extern bool isBuildMode;
extern int heldFileIndex;

// Fonctions
void ToggleBuildMode(void);
void ToggleGrabFile(int hoveredIndex);
void UpdateHeldFilePosition(Camera *camera);

// Un petit affichage spécifique pour prévenir l'utilisateur
void DrawBuildModeHUD(int screenWidth, int screenHeight);

#endif
