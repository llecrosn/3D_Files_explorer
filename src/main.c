#include "config.h"
#include "explorer3d.h"
#include "taskbar.h"
#include "filesystem.h"
#include "construction.h"
#include "context_menu.h"
#include <stdlib.h>
#include <string.h>

int main(void)
{
    // ============================================================================
    // 1. INITIALISATION DU SYSTÈME ET DE LA FENÊTRE
    // ============================================================================
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Bureau 3D - V2");
    SetExitKey(0);
    SetWindowState(FLAG_WINDOW_MAXIMIZED);
    SetTargetFPS(TARGET_FPS);

    Camera camera = {0};
    InitExplorerCamera(&camera);
    
    // Chargement initial au démarrage
    LoadFiles(getenv("HOME"));

    bool isCameraMode = true;
    int hoveredIndex = -1;
    DisableCursor();

    // ============================================================================
    // 2. BOUCLE PRINCIPALE
    // ============================================================================
    while (!WindowShouldClose())
    {
        // --- A. GESTION DES MODES GLOBAUX ---
        // On bloque le changement de mode si un menu est ouvert ou si on cherche
        if (!isContextMenuOpen && !isSearching)
        {
            if (IsKeyPressed(KEY_TAB))
            {
                isCameraMode = !isCameraMode;
                if (isCameraMode) DisableCursor();
                else EnableCursor();
            }
        }

        // --- B.(Logique et Physique) ---
        if (isCameraMode && !isContextMenuOpen)
        {
            UpdateExplorerCamera(&camera);

            if (!IsMouseOnTaskbar(SCREEN_HEIGHT))
            {
                // Gestion du viseur (désactivé si on porte un cube)
                hoveredIndex = (heldFileIndex == -1) ? GetHoveredFileIndex(&camera) : -1;

                // ACTIVATION DE LA RECHERCHE (CTRL + F)
                if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_F))
                {
                    isSearching = true;
                    searchLetterCount = 0;
                    memset(searchBuffer, 0, 256);
                }

                // --- 1. CLAVIER : Raccourcis (Désactivés si on tape du texte) ---
                if (!isSearching)
                {
                    // Raccourcis globaux
                    if (IsKeyPressed(KEY_B)) ToggleBuildMode();
                    if (IsKeyPressed(KEY_H)) { showHiddenFiles = !showHiddenFiles; LoadFiles("."); }
                    if (IsKeyPressed(KEY_T)) { currentSortMode = (currentSortMode + 1) % 4; LoadFiles("."); }

                    // Interactions selon le mode
                    if (isBuildMode)
                    {
                        if (IsKeyPressed(KEY_E)) ToggleGrabFile(hoveredIndex);
                    }
                    else
                    {
                        if (IsKeyPressed(KEY_SPACE)) ToggleImagePreview(hoveredIndex);
                        
                        // Inventaire
                        if (IsKeyPressed(KEY_C) && hoveredIndex != -1) CopyToClipboard(hoveredIndex);
                        if (IsKeyPressed(KEY_X) && hoveredIndex != -1) CutToClipboard(hoveredIndex);
                        if (IsKeyPressed(KEY_V)) PasteFromClipboard(&camera);
                    }
                }

                // --- 2. SOURIS : Interactions (Toujours actives, même en recherche !) ---
                if (!isBuildMode)
                {
                    // Clic Gauche : Ouvrir un fichier ou entrer dans un dossier
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoveredIndex != -1) {
                        bool wasFolder = files[hoveredIndex].isFolder;
                        InteractWithFile(hoveredIndex);
                        
                        // Si on vient d'entrer dans un dossier, on nettoie et on ferme la barre de recherche
                        if (wasFolder) {
                            isSearching = false;
                            searchLetterCount = 0;
                            memset(searchBuffer, 0, 256);
                        }
                    }
                    
                    // Clic Droit : Ouvrir le menu contextuel
                    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && hoveredIndex != -1) {
                        OpenContextMenu(hoveredIndex);
                    }
                }
            }
            else
            {
                hoveredIndex = -1; // Souris sur la barre des tâches = on ne vise rien
            }

            // Mise à jour de la position du cube tenu et des animations
            UpdateHeldFilePosition(&camera);
            UpdateFilesAnimations(hoveredIndex);
        }

        // ============================================================================
        // 3. RENDU (Dessin à l'écran)
        // ============================================================================
        BeginDrawing();
        ClearBackground(COLOR_VOID);

        // -- 3.1 RENDU 3D --
        BeginMode3D(camera);
        DrawTronGrid(&camera);
        DrawFiles3D(&camera);
        EndMode3D();

        // -- 3.2 RENDU 2D (Interface générale) --
        DrawTaskbar(SCREEN_WIDTH, SCREEN_HEIGHT);
        DrawAddressBar(SCREEN_WIDTH);
        UpdateAndDrawSearchBar(SCREEN_WIDTH);

        if (isCameraMode)
        {
            // HUD Holographique
            DrawNearbyFileNames(&camera, SCREEN_WIDTH, SCREEN_HEIGHT);
            DrawBuildModeHUD(SCREEN_WIDTH, SCREEN_HEIGHT);
            DrawClipboardHUD(SCREEN_WIDTH, SCREEN_HEIGHT);

            // Viseur et indicateur de mode
            DrawText("Mode: CAMERA (TAB)", 20, 40, 20, COLOR_NEON_CYAN);
            DrawCircleLines(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 5, Fade(WHITE, 0.5f));
        }
        else
        {
            DrawText("Mode: INTERFACE (TAB)", 20, 40, 20, ORANGE);
        }

        // Indicateurs systèmes (FPS et Mode de Tri)
        DrawFPS(20, 70);
        char *modeText = "MODE: LAYOUT LIBRE (T)";
        Color modeColor = ORANGE;
        
        if (currentSortMode == MODE_SORT_NAME) { modeText = "MODE: TRI NOM (T)"; modeColor = COLOR_NEON_CYAN; }
        else if (currentSortMode == MODE_SORT_SIZE) { modeText = "MODE: TRI TAILLE (T)"; modeColor = COLOR_NEON_CYAN; }
        else if (currentSortMode == MODE_SORT_DATE) { modeText = "MODE: TRI DATE (T)"; modeColor = COLOR_NEON_CYAN; }
        
        DrawText(modeText, 20, 100, 10, modeColor);

        // -- 3.3 SURCOUCHES 2D (Panneaux par-dessus l'interface) --
        DrawImagePreview(SCREEN_WIDTH, SCREEN_HEIGHT);
        UpdateAndDrawContextMenu(SCREEN_WIDTH, SCREEN_HEIGHT);

        EndDrawing();
    }

    // ============================================================================
    // 4. NETTOYAGE ET FERMETURE
    // ============================================================================
    CloseWindow();
    return 0;
}