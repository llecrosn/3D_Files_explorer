# Bureau 3D - Explorateur de Fichiers

Un explorateur de fichiers expérimental en 3D développé en **C** avec **Raylib**. Naviguez dans votre système d'exploitation Linux comme dans un jeu vidéo à la première personne, avec une interface inspirée du design Techwear / Cyberpunk.

## Fonctionnalités Principales

* **Navigation 3D FPS :** Promenez-vous physiquement au milieu de vos fichiers et dossiers.
* **Système d'Inventaire (Sac à dos) :** Coupez, copiez et collez vos fichiers d'un dossier à l'autre via un inventaire visuel.
* **Moteur de Recherche Hybride :**
  * *Filtrage Local :* Élimination des cubes en temps réel ($O(1)$) lors de la saisie clavier.
  * *Recherche Globale Asynchrone :* Fouille complète du système (`/home`) via un sous-processus (`pthread`) sans bloquer le rendu 3D.
* **Mode Construction :** Attrapez les cubes et réorganisez physiquement votre bureau (sauvegarde persistante du layout).
* **Moteur Hautes Performances :** Intègre un système de *Frustum Culling* (élimination mathématique des objets hors champ) et de *State Caching* pour maintenir 60 FPS constants, même avec des milliers de fichiers.
* **Interface Techwear :** HUD immersif avec panneaux holographiques, effets de scanlines et affichage dynamique des métadonnées.

## Prérequis et Installation

Ce projet a été conçu et testé pour les environnements **Linux** (notamment Linux Mint/Ubuntu).

### Dépendances
* Compilateur GCC
* Make
* [Raylib 5.0+](https://www.raylib.com/) 

### Compilation
1. Clonez ce dépôt :
    ```bash
    git clone https://github.com/llecrosn/3D_Files_explorer.git
    cd 3D_Files_explorer

    ```

2. Compilez le projet :
    ```bash
    make

    ```


3. Lancez l'explorateur :
    ```bash
    make run

    ```

## Contrôles

### Navigation Générale

* **Z, Q, S, D** : Se déplacer
* **Souris** : Regarder autour de soi
* **TAB** : Basculer entre le mode Caméra (FPS) et le mode Interface (Curseur libre)
* **Clic Gauche** : Ouvrir un fichier (via l'OS) / Entrer dans un dossier
* **Clic Droit** : Ouvrir le menu contextuel
* **Espace** : Afficher le panneau des propriétés détaillées du fichier ciblé

### Raccourcis Système

* **CTRL + F** : Ouvrir la barre de recherche locale
* *Appuyez sur ENTRÉE pendant une recherche pour lancer un scan asynchrone sur tout le disque.*


* **C / X / V** : Copier / Couper / Coller le fichier ciblé
* **B** : Activer le Mode Construction (Aménagement libre)
* **E** : Saisir/Relâcher un cube en mode construction


* **H** : Afficher/Masquer les fichiers cachés
* **T** : Changer le mode de tri (Nom, Taille, Date, Disposition libre)

## Architecture Technique

Le projet est divisé en modules distincts pour assurer une maintenance et une évolutivité maximales :

* `filesystem.c` : Cœur du système (interactions OS, threads, parsing, culling).
* `main.c` : Boucle principale et gestion des états globaux.
* `explorer3d.c` : Gestion de la caméra et physique.
* `construction.c` / `context_menu.c` : Modules d'interactions spécifiques.

## Licence

Ce projet est distribué sous la licence **GNU GPLv3**. Vous êtes libre de l'utiliser, de le modifier et de le redistribuer, à condition que vos modifications restent open-source sous cette même licence.