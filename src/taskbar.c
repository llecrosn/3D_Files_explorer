#include "taskbar.h"
#include "config.h"

// On fixe la hauteur de la barre
#define TASKBAR_HEIGHT 50

bool IsMouseOnTaskbar(int screenHeight) {
    // Si la position Y de la souris est dans les 50 derniers pixels du bas
    return GetMouseY() >= (screenHeight - TASKBAR_HEIGHT);
}

void DrawTaskbar(int screenWidth, int screenHeight) {
    int taskbarY = screenHeight - TASKBAR_HEIGHT;

    // 1. Le fond de la barre (Léger effet de transparence)
    DrawRectangle(0, taskbarY, screenWidth, TASKBAR_HEIGHT, Fade(COLOR_VOID, 0.9f));
    // Une belle ligne lumineuse pour séparer la 3D de la 2D
    DrawLine(0, taskbarY, screenWidth, taskbarY, COLOR_NEON_CYAN);

    // 2. Le bouton SYS (Menu Démarrer)
    Rectangle sysBtn = { 0, taskbarY, 80, TASKBAR_HEIGHT };
    
    // On vérifie si la souris survole le bouton
    bool isHovered = CheckCollisionPointRec(GetMousePosition(), sysBtn);
    Color btnColor = isHovered ? WHITE : COLOR_NEON_CYAN;

    // Dessin du bouton
    DrawRectangleLinesEx(sysBtn, 2, btnColor);
    DrawText("SYS", sysBtn.x + 20, sysBtn.y + 15, 20, btnColor);

    // 3. Gestion du clic (Uniquement sur ce bouton pour l'instant)
    if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        // TraceLog écrit proprement dans la console sans bloquer le jeu
        TraceLog(LOG_INFO, "UI: Bouton SYS cliqué !");
    }
}
