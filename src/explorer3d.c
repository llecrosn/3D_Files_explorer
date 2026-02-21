#include "explorer3d.h"
#include "config.h"
#include <math.h>

void InitExplorerCamera(Camera *camera) {
    camera->position = (Vector3){ 0.0f, 6.0f, 10.0f };
    camera->target = (Vector3){ 0.0f, 2.0f, 0.0f };
    camera->up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera->fovy = 65.0f;
    camera->projection = CAMERA_PERSPECTIVE;
}

void UpdateExplorerCamera(Camera *camera) {
    // Vitesses de déplacement
    float speed = IsKeyDown(KEY_LEFT_SHIFT) ? 2.5f : 0.6f;
    Vector3 move = {0};

    // Contrôles ZQSD (ou WASD selon le clavier)
    if (IsKeyDown(KEY_W)) move.x += 1;
    if (IsKeyDown(KEY_S)) move.x -= 1;
    if (IsKeyDown(KEY_D)) move.y += 1;
    if (IsKeyDown(KEY_A)) move.y -= 1;

    // Rotation de la caméra avec la souris
    Vector2 delta = GetMouseDelta();
    float sensitivity = 0.15f;
    Vector3 rot = { delta.x * sensitivity, delta.y * sensitivity, 0.0f };

    // UpdateCameraPro est parfait pour un mouvement de type "First Person"
    UpdateCameraPro(camera, (Vector3){move.x * speed, move.y * speed, 0}, rot, 0.0f);
}

void DrawTronGrid(Camera *camera) {
    // Calcul pour centrer la grille sur la caméra (illusion d'infini)
    int cx = (int)(camera->position.x / 5.0f) * 5;
    int cz = (int)(camera->position.z / 5.0f) * 5;

    BeginBlendMode(BLEND_ADDITIVE);
    
    for (int i = -250; i <= 250; i += 5) {
        // Distance pour l'effet de fondu (fade) au loin
        float dx = (cx + i) - camera->position.x;
        float dz = (cz + i) - camera->position.z;
        
        float alphaX = 1.0f - (fabs(dx) / 200.0f);
        float alphaZ = 1.0f - (fabs(dz) / 200.0f);
        
        if (alphaX < 0) alphaX = 0;
        if (alphaZ < 0) alphaZ = 0;

        // Lignes Z
        DrawLine3D((Vector3){cx + i, -2.0f, cz - 250}, 
                   (Vector3){cx + i, -2.0f, cz + 250}, 
                   Fade(COLOR_NEON_CYAN, alphaZ * 0.15f));
                   
        // Lignes X
        DrawLine3D((Vector3){cx - 250, -2.0f, cz + i}, 
                   (Vector3){cx + 250, -2.0f, cz + i}, 
                   Fade(COLOR_NEON_CYAN, alphaX * 0.15f));
    }
    
    EndBlendMode();
}
