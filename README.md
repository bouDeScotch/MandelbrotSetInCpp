## Presentation

Ce projet est une implémentation du [Mandelbrot Set](https://fr.wikipedia.org/wiki/Ensemble_de_Mandelbrot) en C++ avec la bibliothèque [SDL2](https://www.libsdl.org/).
On peut zoomer sur l'ensemble de Mandelbrot en cliquant et en selectionnant une zone de l'écran avec la souris.

## Installation

### Prérequis

Assurez-vous d'avoir installé les bibliothèques suivantes :

- [SDL2](https://www.libsdl.org/download-2.0.php)

### Instructions

1. Clonez le dépôt :

   ```bash
   gh repo clone boudescotch/MandelbrotSetInCpp
   ```

2. Compiler le projet :

   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

3. Exécutez le programme :

   ```bash
   ./MandelbrotSet
   ```

