#include "construction.h"
#include "raymath.h"
#include "config.h"
#include <string.h>
#include <math.h>

bool isBuildMode = false;
int heldFileIndex = -1;
extern SortMode currentSortMode;
extern FileNode files[];

void ToggleBuildMode(void) {
    isBuildMode = !isBuildMode;
    
    // Sécurité : si on quitte le mode construction alors qu'on porte un objet, on le lâche
    if (!isBuildMode && heldFileIndex != -1) {
        files[heldFileIndex].position.y = 1.0f;
        heldFileIndex = -1;
    }
}

void ToggleGrabFile(int hoveredIndex) {
    if (!isBuildMode) return;

    // SÉCURITÉ 1 : On bloque le Drag & Drop si on est en Tri Automatique
    if (currentSortMode != MODE_FREE_LAYOUT) {
        return; 
    }

    if (heldFileIndex != -1) {
        files[heldFileIndex].position.y = 1.0f;
        heldFileIndex = -1;
        SaveLayout(); 
    } else if (hoveredIndex != -1) {
        // SÉCURITÉ 2 : On ne déplace JAMAIS le dossier ".."
        if (strcmp(files[hoveredIndex].name, "..") != 0) {
            heldFileIndex = hoveredIndex;
        }
    }
}

void UpdateHeldFilePosition(Camera *camera) {
    if (!isBuildMode || heldFileIndex == -1) return;

    Vector3 forward = Vector3Normalize(Vector3Subtract(camera->target, camera->position));
    Vector3 targetPos = Vector3Add(camera->position, Vector3Scale(forward, 8.0f));

    // GRID SNAPPING unifié (5.0f)
    float gridSize = 5.0f;
    targetPos.x = roundf(targetPos.x / gridSize) * gridSize;
    targetPos.z = roundf(targetPos.z / gridSize) * gridSize;
    targetPos.y = 2.5f; // Lévitation

    float dt = GetFrameTime();
    float followSpeed = 12.0f;
    
    files[heldFileIndex].position.x = Lerp(files[heldFileIndex].position.x, targetPos.x, followSpeed * dt);
    files[heldFileIndex].position.y = Lerp(files[heldFileIndex].position.y, targetPos.y, followSpeed * dt);
    files[heldFileIndex].position.z = Lerp(files[heldFileIndex].position.z, targetPos.z, followSpeed * dt);
}

void DrawBuildModeHUD(int screenWidth, int screenHeight) {
    if (!isBuildMode) return;
    
    // Bandeau orange
    DrawRectangle(0, screenHeight - 90, screenWidth, 40, Fade(ORANGE, 0.2f));
    DrawLine(0, screenHeight - 90, screenWidth, screenHeight - 90, ORANGE);
    
    DrawText("[MODE CONSTRUCTION ACTIF]", 20, screenHeight - 80, 20, ORANGE);
    DrawText("B: Quitter le mode  |  E: Déplacer un cube", 350, screenHeight - 80, 20, WHITE);
}
