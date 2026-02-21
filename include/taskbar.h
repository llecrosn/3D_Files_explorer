#ifndef TASKBAR_H
#define TASKBAR_H

#include "raylib.h"
#include <stdbool.h>

// Permet au main.c de savoir si la souris survole la barre
bool IsMouseOnTaskbar(int screenHeight);

// Dessine la barre et gère les clics sur ses boutons
void DrawTaskbar(int screenWidth, int screenHeight);

#endif
