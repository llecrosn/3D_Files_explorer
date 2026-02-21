#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "filesystem.h"
#include "config.h"
#include <dirent.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/stat.h>
#include <math.h>
#include <pthread.h>

// ============================================================================
// 1. VARIABLES GLOBALES (États du système)
// ============================================================================

// --- Fichiers et Dossiers ---
FileNode files[MAX_FILES];
int fileCount = 0;
char currentDirectoryPath[1024] = {0};

// --- Options d'affichage ---
bool showHiddenFiles = false; 
SortMode currentSortMode = MODE_FREE_LAYOUT;

// --- Inventaire / Sac à dos ---
#define MAX_CLIPBOARD 100
int clipboardCount = 0;
bool isClipboardCopyMode = false;
char clipboardPaths[MAX_CLIPBOARD][1024] = {0}; 
char clipboardNames[MAX_CLIPBOARD][64] = {0};   

// --- Interface 2D (Aperçu) ---
bool isPreviewOpen = false;
int previewIndex = -1;

// --- Moteur de Recherche ---
bool isSearching = false;
char searchBuffer[256] = {0};
int searchLetterCount = 0;

// --- Recherche Globale ---
bool isGlobalSearching = false;
bool globalSearchDone = false;
int globalSearchCount = 0;
#define MAX_SEARCH_RESULTS 100
char globalSearchResults[MAX_SEARCH_RESULTS][1024] = {0};
char globalSearchQuery[256] = {0};

// ============================================================================
// 2. FONCTIONS DE GESTION DES FICHIERS (Cœur du système)
// ============================================================================

int CompareFiles(const void *a, const void *b) {
    FileNode *f1 = (FileNode *)a;
    FileNode *f2 = (FileNode *)b;

    if (strcmp(f1->name, "..") == 0) return -1;
    if (strcmp(f2->name, "..") == 0) return 1;

    if (f1->isFolder && !f2->isFolder) return -1;
    if (!f1->isFolder && f2->isFolder) return 1;

    if (currentSortMode == MODE_SORT_SIZE) {
        if (f1->sizeBytes < f2->sizeBytes) return 1;
        if (f1->sizeBytes > f2->sizeBytes) return -1;
    } 
    else if (currentSortMode == MODE_SORT_DATE) {
        if (f1->modTime < f2->modTime) return 1;
        if (f1->modTime > f2->modTime) return -1;
    }

    return strcasecmp(f1->name, f2->name);
}

void LoadFiles(const char *path) {
    DIR *d = opendir(path);
    if (!d) return;

    chdir(path);
    if (getcwd(currentDirectoryPath, sizeof(currentDirectoryPath)) == NULL) {
        strcpy(currentDirectoryPath, "Chemin inconnu");
    }

    fileCount = 0;
    struct dirent *dir;

    while ((dir = readdir(d)) != NULL) {
        if (fileCount >= MAX_FILES - 1) {
            TraceLog(LOG_WARNING, "LIMITE ATTEINTE : Trop de fichiers dans ce dossier !");
            break; 
        }

        if (strcmp(dir->d_name, ".") == 0) continue;
        if (!showHiddenFiles && dir->d_name[0] == '.' && strcmp(dir->d_name, "..") != 0) continue;
        
        strncpy(files[fileCount].name, dir->d_name, 63);

        struct stat st;
        if (stat(dir->d_name, &st) == 0) {
            files[fileCount].isFolder = S_ISDIR(st.st_mode);
            files[fileCount].sizeBytes = st.st_size; 
            files[fileCount].modTime = st.st_mtime;  

            struct tm *tm_info = localtime(&st.st_mtime);
            strftime(files[fileCount].modDateString, 64, "%d/%m/%Y %H:%M", tm_info);
        } else {
            files[fileCount].isFolder = (dir->d_type == DT_DIR);
            files[fileCount].sizeBytes = 0;
            strcpy(files[fileCount].modDateString, "Inconnu");
        }

        files[fileCount].color = files[fileCount].isFolder ? COLOR_NEON_CYAN : COLOR_NEON_GREEN;
        fileCount++;
    }
    closedir(d);

    qsort(files, fileCount, sizeof(FileNode), CompareFiles);

    int cols = 10;
    float spacing = 5.0f; 

    for (int i = 0; i < fileCount; i++) {
        int row = i / cols;
        int col = i % cols;

        files[i].position = (Vector3){ (col - cols / 2) * spacing, 1.0f, (row - 2) * spacing };
        files[i].baseSize = files[i].isFolder ? 2.0f : 1.5f;
        files[i].currentSize = files[i].baseSize;
        files[i].glow = 0.0f;
        files[i].isVisible = true;
    }

    if (currentSortMode == MODE_FREE_LAYOUT) {
        LoadLayout();
    }
}

void InteractWithFile(int index) {
    if (index < 0 || index >= fileCount) return;

    if (files[index].isFolder) {
        if (chdir(files[index].name) == 0) {
            LoadFiles(".");
        }
    } else {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "xdg-open \"%s\" &", files[index].name);
        system(cmd);
    }
}

// --- Fonction récursive de fouille ---
void RecursiveSearch(const char *dirPath, const char *query) {
    if (globalSearchCount >= MAX_SEARCH_RESULTS) return; // Sécurité anti-dépassement

    DIR *d = opendir(dirPath);
    if (!d) return;

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;
        
        // OPTIMISATION : On ignore les dossiers cachés Linux (.cache, .config...)
        if (dir->d_name[0] == '.') continue;

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, dir->d_name);

        if (strcasestr(dir->d_name, query) != NULL) {
            if (globalSearchCount < MAX_SEARCH_RESULTS) {
                strncpy(globalSearchResults[globalSearchCount], fullPath, 1023);
                globalSearchCount++;
            }
        }

        if (dir->d_type == DT_DIR) {
            RecursiveSearch(fullPath, query);
        }
    }
    closedir(d);
}

void* GlobalSearchThread(void* arg) {
    (void)arg; // Paramètre inutilisé
    
    // On commence la recherche depuis le home de l'utilisateur
    char *home = getenv("HOME"); 
    RecursiveSearch(home, globalSearchQuery);
    
    isGlobalSearching = false;
    globalSearchDone = true;
    
    return NULL;
}


// ============================================================================
// 3. FONCTIONS DE RENDU 3D ET ANIMATIONS
// ============================================================================

void UpdateFilesAnimations(int hoveredIndex) {
    float dt = GetFrameTime();
    float animSpeed = 15.0f; 

    for (int i = 0; i < fileCount; i++) {
        float targetSize = files[i].baseSize;
        float targetGlow = 0.0f;

        if (i == hoveredIndex) {
            targetSize = files[i].baseSize * 1.25f; 
            targetGlow = 1.0f;                      
        }

        files[i].currentSize = Lerp(files[i].currentSize, targetSize, animSpeed * dt);
        files[i].glow = Lerp(files[i].glow, targetGlow, animSpeed * dt);
    }
}

void DrawFiles3D(Camera *camera) {
    // On calcule le vecteur direction de notre regard
    Vector3 camForward = Vector3Normalize(Vector3Subtract(camera->target, camera->position));

    for (int i = 0; i < fileCount; i++) {
        if (!files[i].isVisible) continue;

        // --- OPTIMISATION CULLING (Élimination) ---
        // On calcule la distance au carré
        float distSqr = Vector3DistanceSqr(camera->position, files[i].position);
        
        // CULLING 1 : La Distance (On ne dessine pas ce qui est trop loin : au-delà de 100 unités)
        // 100 * 100 = 10000
        if (distSqr > 10000.0f) continue;

        // CULLING 2 : Le Champ de vision (Dot Product)
        // On ignore les objets très proches pour éviter qu'ils disparaissent sur les bords de l'écran
        if (distSqr > 10.0f) {
            // Vecteur entre la caméra et le cube
            Vector3 toCube = Vector3Normalize(Vector3Subtract(files[i].position, camera->position));
            
            // Le Produit Scalaire (1 = on le regarde, 0 = sur le côté, -1 = dans notre dos)
            float dot = Vector3DotProduct(camForward, toCube);
            
            // Si le résultat est inférieur à -0.2, le cube est hors de notre champ de vision direct
            if (dot < -0.2f) continue;
        }

        //on dessine ce qui rest
        float s = files[i].currentSize;
        Color currentColor = ColorLerp(files[i].color, WHITE, files[i].glow);
        
        bool inClipboard = false;
        for (int c = 0; c < clipboardCount; c++) {
            if (strcmp(files[i].name, clipboardNames[c]) == 0) {
                inClipboard = true;
                break;
            }
        }

        float alpha = inClipboard ? 0.1f : (0.4f + (files[i].glow * 0.4f));

        if (files[i].isFolder) {
            DrawCube(files[i].position, s, s, s, Fade(currentColor, alpha));
            DrawCubeWires(files[i].position, s, s, s, currentColor);
        } else {
            DrawCube(files[i].position, s, 0.2f, s, Fade(currentColor, alpha));
            DrawCubeWires(files[i].position, s, 0.2f, s, currentColor);
            
            Vector3 topCenter = { files[i].position.x, files[i].position.y + 1.0f, files[i].position.z };
            DrawLine3D(files[i].position, topCenter, Fade(currentColor, alpha + 0.2f));
        }
    }
}

int GetHoveredFileIndex(Camera *camera) {
    int hovered = -1;
    float minDistance = 10000.0f;

    Vector3 direction = Vector3Normalize(Vector3Subtract(camera->target, camera->position));
    Ray ray = {camera->position, direction};

    for (int i = 0; i < fileCount; i++) {
        if (!files[i].isVisible) continue;

        float distSqr = Vector3DistanceSqr(camera->position, files[i].position);
        if (distSqr > 3600.0f) continue;

        float s = files[i].isFolder ? 2.0f : 1.5f;
        float halfSize = s / 2.0f;
        BoundingBox box = {
            (Vector3){files[i].position.x - halfSize, files[i].position.y - halfSize, files[i].position.z - halfSize},
            (Vector3){files[i].position.x + halfSize, files[i].position.y + halfSize, files[i].position.z + halfSize}
        };

        RayCollision collision = GetRayCollisionBox(ray, box);
        if (collision.hit && collision.distance < minDistance) {
            minDistance = collision.distance;
            hovered = i;
        }
    }
    return hovered;
}


// ============================================================================
// 4. INTERFACE 2D (HUD, Barre d'adresse, Aperçu)
// ============================================================================

void DrawNearbyFileNames(Camera *camera, int screenWidth, int screenHeight) {
    Vector2 screenCenter = {screenWidth / 2.0f, screenHeight / 2.0f};
    Vector3 camForward = Vector3Normalize(Vector3Subtract(camera->target, camera->position));

    for (int i = 0; i < fileCount; i++) {
        if (!files[i].isVisible) continue;

        // --- Limite de distance 3D physique ---
        float maxDistance3D = 40.0f; // Distance max d'affichage (environ 8 cubes de profondeur)
        float dist3D = Vector3Distance(camera->position, files[i].position);
        
        // Si le cube est trop loin, on ignore son texte
        if (dist3D > maxDistance3D) continue;

        Vector3 toFile = Vector3Normalize(Vector3Subtract(files[i].position, camera->position));
        if (Vector3DotProduct(camForward, toFile) > 0.0f) {
            Vector3 textPos3D = files[i].position;
            textPos3D.y += 1.0f; 
            Vector2 screenPos = GetWorldToScreen(textPos3D, *camera);

            float dist2D = Vector2Distance(screenCenter, screenPos);
            float maxRevealDistance = 250.0f; 

            if (dist2D < maxRevealDistance) {
                // --- Double calcul d'opacité (Alpha) ---
                // Plus on s'éloigne du centre de l'écran, plus ça disparaît
                float alpha2D = 1.0f - (dist2D / maxRevealDistance);
                // Plus le cube est loin physiquement, plus ça disparaît
                float alpha3D = 1.0f - (dist3D / maxDistance3D);
                
                // On multiplie les deux : il faut être PRÈS et BIEN VISER pour voir le texte net !
                float finalAlpha = alpha2D * alpha3D*2;

                DrawLine(screenPos.x, screenPos.y, screenPos.x + 20, screenPos.y - 20, Fade(COLOR_NEON_CYAN, finalAlpha * 0.5f));
                DrawText(files[i].name, screenPos.x + 25, screenPos.y - 30, 20, Fade(WHITE, finalAlpha));
            }
        }
    }
}

void DrawAddressBar(int screenWidth) {
    DrawRectangle(0, 0, screenWidth, 30, Fade(BLACK, 0.8f));
    DrawLine(0, 30, screenWidth, 30, COLOR_NEON_CYAN);
    DrawText(TextFormat(">_ %s", currentDirectoryPath), 15, 8, 15, COLOR_NEON_CYAN);
}

void FormatSize(long long bytes, char *buffer) {
    if (bytes < 1024) sprintf(buffer, "%lld Octets", bytes);
    else if (bytes < 1024 * 1024) sprintf(buffer, "%.2f Ko", (float)bytes / 1024.0f);
    else if (bytes < 1024 * 1024 * 1024) sprintf(buffer, "%.2f Mo", (float)bytes / (1024.0f * 1024.0f));
    else sprintf(buffer, "%.2f Go", (float)bytes / (1024.0f * 1024.0f * 1024.0f));
}

void ToggleImagePreview(int index) {
    if (isPreviewOpen) {
        isPreviewOpen = false;
        previewIndex = -1;
    } else if (index != -1) {
        isPreviewOpen = true;
        previewIndex = index;
    }
}

void DrawImagePreview(int screenWidth, int screenHeight) {
    if (!isPreviewOpen || previewIndex == -1) return;

    FileNode *file = &files[previewIndex];
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));

    int nameLen = strlen(file->name);
    int maxCharsPerLine = 30; 
    int extraLines = (nameLen > 0) ? (nameLen - 1) / maxCharsPerLine : 0;

    int panelW = 400;
    int panelH = 250 + (extraLines * 25);
    int panelX = (screenWidth - panelW) / 2;
    int panelY = (screenHeight - panelH) / 2;

    DrawRectangle(panelX, panelY, panelW, panelH, Fade(COLOR_VOID, 0.9f));
    DrawRectangleLines(panelX, panelY, panelW, panelH, file->color);
    
    DrawRectangle(panelX, panelY, panelW, 40, Fade(file->color, 0.3f));
    DrawText("MÉTADONNÉES DU SYSTÈME", panelX + 15, panelY + 12, 10, WHITE);
    
    DrawText("Nom :", panelX + 20, panelY + 60, 20, GRAY);
    
    char lineBuffer[64];
    for (int i = 0; i <= extraLines; i++) {
        int charsToCopy = nameLen - (i * maxCharsPerLine);
        if (charsToCopy > maxCharsPerLine) charsToCopy = maxCharsPerLine;
        
        strncpy(lineBuffer, file->name + (i * maxCharsPerLine), charsToCopy);
        lineBuffer[charsToCopy] = '\0';
        
        DrawText(lineBuffer, panelX + 80, panelY + 60 + (i * 25), 20, WHITE);
    }

    int offsetY = extraLines * 25;
    DrawText(TextFormat("Type : %s", file->isFolder ? "Dossier Système" : "Fichier de données"), panelX + 20, panelY + 100 + offsetY, 20, GRAY);
    
    char sizeStr[64];
    FormatSize(file->sizeBytes, sizeStr);
    DrawText(TextFormat("Taille : %s", file->isFolder ? "--" : sizeStr), panelX + 20, panelY + 140 + offsetY, 20, GRAY);
    
    DrawText(TextFormat("Modifié le : %s", file->modDateString), panelX + 20, panelY + 180 + offsetY, 20, GRAY);

    DrawText("Appuyez sur ESPACE pour fermer", panelX + (panelW / 2) - 100, panelY + panelH - 30, 10, Fade(WHITE, 0.5f));
}

void UpdateAndDrawSearchBar(int screenWidth) {
    if (!isSearching) return;

    // --- CAPTURE DU CLAVIER ---
    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= 32) && (key <= 125) && (searchLetterCount < 255)) {
            searchBuffer[searchLetterCount] = (char)key;
            searchBuffer[searchLetterCount + 1] = '\0';
            searchLetterCount++;
            UpdateSearchFilter();
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        searchLetterCount--;
        if (searchLetterCount < 0) searchLetterCount = 0;
        searchBuffer[searchLetterCount] = '\0';
        UpdateSearchFilter();
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        isSearching = false;
        globalSearchDone = false;
        searchLetterCount = 0;
        memset(searchBuffer, 0, 256);
        UpdateSearchFilter();
    }

    if (IsKeyPressed(KEY_ENTER) && searchLetterCount > 0 && !isGlobalSearching) {
        isGlobalSearching = true;
        globalSearchDone = false;
        globalSearchCount = 0;
        strcpy(globalSearchQuery, searchBuffer);

        // Lancement du sous-processus
        pthread_t threadId;
        pthread_create(&threadId, NULL, GlobalSearchThread, NULL);
        pthread_detach(threadId);
    }

    // --- AFFICHAGE DE LA BARRE ---
    int barW = 600;
    int barH = 50;
    int barX = (screenWidth - barW) / 2;
    int barY = 40; 

    DrawRectangle(barX, barY, barW, barH, Fade(BLACK, 0.9f));
    DrawRectangleLines(barX, barY, barW, barH, COLOR_NEON_CYAN);
    
    DrawText("Recherche :", barX + 15, barY + 15, 20, GRAY);
    DrawText(searchBuffer, barX + 140, barY + 15, 20, WHITE);

    if (((int)(GetTime() * 2)) % 2 == 0) {
        int textW = MeasureText(searchBuffer, 20);
        DrawText("_", barX + 140 + textW, barY + 15, 20, COLOR_NEON_CYAN);
    }
    
    DrawText("ECHAP: Annuler | ENTRÉE: Chercher dans tout le système (HOME)", barX, barY + 60, 10, GRAY);

    // --- AFFICHAGE DES RÉSULTATS (PENDANT ET APRÈS) ---
    if (isGlobalSearching) {
        // Animation de chargement
        int dots = ((int)(GetTime() * 3)) % 4;
        char* loading = (dots == 0) ? "" : (dots == 1) ? "." : (dots == 2) ? ".." : "...";
        DrawText(TextFormat("Analyse du disque en cours%s", loading), barX, barY + 80, 20, ORANGE);
    } 
    else if (globalSearchDone) {
        int resultBoxH = 30 + (globalSearchCount > 10 ? 10 : globalSearchCount) * 25;
        if (globalSearchCount == 0) resultBoxH = 50;

        DrawRectangle(barX, barY + 80, barW, resultBoxH, Fade(BLACK, 0.95f));
        DrawRectangleLines(barX, barY + 80, barW, resultBoxH, COLOR_NEON_GREEN);
        
        if (globalSearchCount == 0) {
            DrawText("Aucun résultat trouvé.", barX + 15, barY + 95, 15, RED);
        } else {
            DrawText(TextFormat("%d résultat(s) (Cliquez pour y aller) :", globalSearchCount), barX + 15, barY + 90, 15, COLOR_NEON_GREEN);
            
            // On limite l'affichage à 10 résultats pour ne pas déborder de l'écran
            int displayCount = globalSearchCount > 10 ? 10 : globalSearchCount;
            
            for (int i = 0; i < displayCount; i++) {
                Rectangle itemRec = { barX + 10, barY + 115 + (i * 25), barW - 20, 20 };
                bool hover = CheckCollisionPointRec(GetMousePosition(), itemRec);
                
                if (hover) DrawRectangleRec(itemRec, Fade(COLOR_NEON_CYAN, 0.3f));

                char* displayPath = globalSearchResults[i];
                if (strlen(displayPath) > 65) displayPath += strlen(displayPath) - 65; // Tronquer le début si trop long
                
                DrawText(TextFormat("...%s", displayPath), itemRec.x + 5, itemRec.y + 5, 10, WHITE);

                // --- TÉLÉPORTATION ---
                if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    char folderPath[1024];
                    strcpy(folderPath, globalSearchResults[i]);
                    
                    // On cherche le dernier '/' pour isoler le dossier contenant le fichier
                    char *lastSlash = strrchr(folderPath, '/');
                    if (lastSlash) {
                        *lastSlash = '\0'; 
                        if (chdir(folderPath) == 0) {
                            LoadFiles("."); // On recharge la 3D dans ce nouveau dossier
                            isSearching = false; 
                            globalSearchDone = false;
                        }
                    }
                }
            }
        }
    }
}

// --- Calculateur de recherche optimisé ---
void UpdateSearchFilter(void) {
    if (!isSearching || searchLetterCount == 0) {
        // Si on ne cherche rien, tout redevient visible
        for (int i = 0; i < fileCount; i++) files[i].isVisible = true;
        return;
    }

    // Sinon, on calcule qui a le droit d'être affiché
    for (int i = 0; i < fileCount; i++) {
        if (strcasestr(files[i].name, searchBuffer) != NULL) {
            files[i].isVisible = true;
        } else {
            files[i].isVisible = false;
        }
    }
}

// ============================================================================
// 5. INVENTAIRE ET SAUVEGARDE (Gestion de la mémoire)
// ============================================================================

void SaveLayout(void) {
    FILE *f = fopen(".bureau_layout", "w");
    if (!f) return;

    for (int i = 0; i < fileCount; i++) {
        fprintf(f, "%f %f %f %s\n", files[i].position.x, files[i].position.y, files[i].position.z, files[i].name);
    }
    fclose(f);
}

void LoadLayout(void) {
    FILE *f = fopen(".bureau_layout", "r");
    if (!f) return;

    char name[256];
    float x, y, z;

    while (fscanf(f, "%f %f %f %[^\n]", &x, &y, &z, name) == 4) {
        for (int i = 0; i < fileCount; i++) {
            if (strcmp(files[i].name, name) == 0) {
                files[i].position = (Vector3){x, y, z};
                break;
            }
        }
    }
    fclose(f);
}

void CutToClipboard(int index) {
    if (index < 0 || index >= fileCount) return;
    if (strcmp(files[index].name, "..") == 0) return; 

    isClipboardCopyMode = false;

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) return;

    char fullPath[2048];
    snprintf(fullPath, sizeof(fullPath), "%s/%s", cwd, files[index].name);

    for (int i = 0; i < clipboardCount; i++) {
        if (strcmp(clipboardPaths[i], fullPath) == 0) {
            for (int j = i; j < clipboardCount - 1; j++) {
                strcpy(clipboardPaths[j], clipboardPaths[j + 1]);
                strcpy(clipboardNames[j], clipboardNames[j + 1]);
            }
            clipboardCount--;
            return;
        }
    }

    if (clipboardCount < MAX_CLIPBOARD) {
        strcpy(clipboardPaths[clipboardCount], fullPath);
        strncpy(clipboardNames[clipboardCount], files[index].name, 63);
        clipboardCount++;
    }
}

void CopyToClipboard(int index) {
    if (index < 0 || index >= fileCount) return;
    if (strcmp(files[index].name, "..") == 0) return; 

    isClipboardCopyMode = true;

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) return;

    char fullPath[2048];
    snprintf(fullPath, sizeof(fullPath), "%s/%s", cwd, files[index].name);

    for (int i = 0; i < clipboardCount; i++) {
        if (strcmp(clipboardPaths[i], fullPath) == 0) {
            for (int j = i; j < clipboardCount - 1; j++) {
                strcpy(clipboardPaths[j], clipboardPaths[j + 1]);
                strcpy(clipboardNames[j], clipboardNames[j + 1]);
            }
            clipboardCount--;
            return;
        }
    }

    if (clipboardCount < MAX_CLIPBOARD) {
        strcpy(clipboardPaths[clipboardCount], fullPath);
        strncpy(clipboardNames[clipboardCount], files[index].name, 63);
        clipboardCount++;
    }
}

void PasteFromClipboard(Camera *camera) {
    if (clipboardCount == 0) return;

    bool success = false;
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera->target, camera->position));
    Vector3 baseDropPos = Vector3Add(camera->position, Vector3Scale(forward, 8.0f));

    int countToPlace = clipboardCount;
    char tempNames[MAX_CLIPBOARD][64];

    for (int i = 0; i < clipboardCount; i++) {
        if (isClipboardCopyMode) {
            // MODE COPIER : commande système 'cp'
            char destName[256];
            strcpy(destName, clipboardNames[i]);
            
            //vérification si le fichier existe déjà dans le dossier
            if (access(destName, F_OK) == 0) {
                snprintf(destName, sizeof(destName), "%s_copie", clipboardNames[i]);
            }
            
            char cmd[2048];
            // cp -r permet de copier aussi bien les fichiers que les dossiers pleins
            snprintf(cmd, sizeof(cmd), "cp -r \"%s\" \"%s\"", clipboardPaths[i], destName);
            if (system(cmd) == 0) {
                success = true;
                strcpy(tempNames[i], destName); // On garde le nouveau nom pour le placement 3D
            }
        } else {
            // MODE COUPER : On déplace
            if (rename(clipboardPaths[i], clipboardNames[i]) == 0) {
                success = true;
                strcpy(tempNames[i], clipboardNames[i]);
            }
        }
    }

    if (success) {
        if (!isClipboardCopyMode) {
            clipboardCount = 0; 
        }
        clipboardCount = 0;
        LoadFiles(".");     

        if (currentSortMode == MODE_FREE_LAYOUT) {
            for (int i = 0; i < countToPlace; i++) {
                for (int j = 0; j < fileCount; j++) {
                    if (strcmp(files[j].name, tempNames[i]) == 0) {
                        files[j].position.x = roundf((baseDropPos.x + (i * 5.0f)) / 5.0f) * 5.0f;
                        files[j].position.y = 1.0f;
                        files[j].position.z = roundf(baseDropPos.z / 5.0f) * 5.0f;
                        break;
                    }
                }
            }
            SaveLayout(); 
        }
    }
}

void DrawClipboardHUD(int screenWidth, int screenHeight) {
    (void)screenHeight;
    if (clipboardCount == 0) return;
    
    int displayCount = clipboardCount > 4 ? 4 : clipboardCount;
    int boxWidth = 250;
    int boxHeight = 40 + (displayCount * 20) + (clipboardCount > 4 ? 20 : 0);
    int boxX = screenWidth - boxWidth - 20;
    int boxY = 50; 

    Color modeColor = isClipboardCopyMode ? COLOR_NEON_GREEN : ORANGE;

    DrawRectangle(boxX, boxY, boxWidth, boxHeight, Fade(BLACK, 0.8f));
    DrawRectangleLines(boxX, boxY, boxWidth, boxHeight, modeColor);
    
    DrawText(TextFormat("Sac à dos (%s): %d", isClipboardCopyMode ? "Copie" : "Déplacement", clipboardCount), boxX + 10, boxY + 10, 15, modeColor);
    
    for (int i = 0; i < displayCount; i++) {
        DrawText(TextFormat("- %s", clipboardNames[clipboardCount - 1 - i]), boxX + 15, boxY + 35 + (i * 20), 10, WHITE);
    }
    
    if (clipboardCount > 4) {
        DrawText("...", boxX + 15, boxY + 35 + (displayCount * 20), 10, GRAY);
    }
}