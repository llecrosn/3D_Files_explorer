#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>
#include <time.h>

// ============================================================================
// 1. DÉFINITIONS ET STRUCTURES
// ============================================================================
#define MAX_FILES 1024 

typedef enum {
    MODE_FREE_LAYOUT = 0,
    MODE_SORT_NAME,
    MODE_SORT_SIZE,
    MODE_SORT_DATE
} SortMode;

typedef struct {
    Vector3 position;
    char name[64];
    bool isFolder;
    Color color;

    float baseSize;     // Sa taille normale au repos
    float currentSize;  // Sa taille animée actuelle
    float glow;         // Son intensité lumineuse (0.0 à 1.0)

    long long sizeBytes;      // Poids en octets
    time_t modTime;           // Timestamp de modification
    char modDateString[64];   // Date formatée pour l'affichage 
    bool isVisible;
} FileNode;

// ============================================================================
// 2. VARIABLES GLOBALES (Externes)
// ============================================================================
extern FileNode files[MAX_FILES];
extern int fileCount;
extern bool showHiddenFiles;
extern int heldFileIndex;
extern bool isAutoArrangeMode;
extern SortMode currentSortMode;

// --- Moteur de Recherche ---
extern bool isSearching;
extern char searchBuffer[256];
extern int searchLetterCount;

// --- Recherche Globale en arrière-plan ---
extern bool isGlobalSearching;
extern bool globalSearchDone;
extern int globalSearchCount;

// ============================================================================
// 3. GESTION DU SYSTÈME DE FICHIERS
// ============================================================================
void LoadFiles(const char *path);
void InteractWithFile(int index);

// ============================================================================
// 4. MOTEUR 3D (Rendu, Animations, Raycasting)
// ============================================================================
void DrawFiles3D(Camera *camera);
void UpdateFilesAnimations(int hoveredIndex);
int GetHoveredFileIndex(Camera *camera);


// ============================================================================
// 5. INVENTAIRE (Sac à dos : Couper / Copier / Coller)
// ============================================================================
void CutToClipboard(int index);
void CopyToClipboard(int index);
void PasteFromClipboard(Camera *camera);

// ============================================================================
// 6. SAUVEGARDE ET LAYOUT
// ============================================================================
void SaveLayout(void);
void LoadLayout(void);

// ============================================================================
// 7. INTERFACE 2D (HUD, Menus, Previews)
// ============================================================================
void DrawNearbyFileNames(Camera *camera, int screenWidth, int screenHeight);
void DrawAddressBar(int screenWidth);
void UpdateAndDrawSearchBar(int screenWidth);
void DrawClipboardHUD(int screenWidth, int screenHeight);

// Aperçu d'images / métadonnées
void ToggleImagePreview(int index);
void DrawImagePreview(int screenWidth, int screenHeight);
void UpdateSearchFilter(void);

#endif