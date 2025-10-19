/**
 * @file test.c
 * @brief A simple test program for the progress bar.
 *
 * This file contains a main function that demonstrates the usage of the
 * progressBar function by simulating a progress increase.
 */

#include "colors.h"
#include <stdio.h>      // For printf(), fflush()
#include <stdlib.h>     // For exit(), EXIT_FAILURE
#include <unistd.h>     // For usleep()

/**
 * @brief Displays a progress bar in the console.
 *
 * This function takes a progress value (0-100) and displays a simple
 * progress bar in the console.
 *
 * @param progress The progress value (0-100).
 */
void progessBar(int progress) {
  const int BAR_SIZE = 50;
  // Print the progress bar with colors.
  printf("\r%sProgress:   %s%s|%s", COLOR_YELLOW, COLOR_END,
         progress == 0 ? COLOR_RED : COLOR_GREEN, COLOR_END);
  for (int i = 0; i < BAR_SIZE; i++) {
    if (i < progress / 2) {
      printf("%s█%s", COLOR_GREEN, COLOR_END);
    } else {
      printf("%s_%s", COLOR_RED, COLOR_END);
    }
  }
  printf("%s|%s %s- %d%%%s ", progress == 100 ? COLOR_GREEN : COLOR_RED,
         COLOR_END, COLOR_BLUE, progress, COLOR_END);
  // Flush the output to ensure the progress bar is displayed immediately.
  fflush(stdout);
}

/**
 * @brief Main function to test the progress bar.
 *
 * This function calls the progessBar function in a loop to simulate
 * a progress increase from 0 to 100.
 *
 * @return 0 on success.
 */
int main() {
  for (int i = 0; i <= 100; i++) {
    progessBar(i);
    usleep(100000); // Sleep for 100ms to simulate work.
  }
  printf("\n");
  return 0;
}
