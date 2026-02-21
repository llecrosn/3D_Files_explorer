#ifndef EXPLORER3D_H
#define EXPLORER3D_H

#include "raylib.h"

// Initialise la caméra avec les bonnes coordonnées de départ
void InitExplorerCamera(Camera *camera);

// Gère les déplacements (Z,Q,S,D et Souris)
void UpdateExplorerCamera(Camera *camera);

// Dessine le décor (la grille infinie)
void DrawTronGrid(Camera *camera);

#endif // EXPLORER3D_H
