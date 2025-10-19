/**
 * @file progress_bar.c
 * @brief Functions for displaying a progress bar.
 *
 * This file implements the logic for rendering a progress bar in the console
 * to visualize the file transfer progress.
 */

#include "progress_bar.h"
#include "colors.h"
#include <stdio.h>    // For printf(), fflush()
#include <stdlib.h>   // For exit(), EXIT_FAILURE
#include <sys/time.h> // For struct timeval (not used in this file)
#include <unistd.h>   // For usleep()

#define BYTES_TO_MB(bytes) ((bytes) / (1024.0f * 1024.0f))
#define BAR_WIDTH 40

/**
 * @brief Updates and displays the progress bar.
 *
 * This function calculates the progress percentage and displays a bar
 * in the console, along with the percentage and the amount of data transferred.
 *
 * @param done The amount of data transferred so far.
 * @param total The total size of the file.
 */
void progressBarUpdate(int done, int total) {
  // Calculate the progress ratio.
  float ratio = (float)done / total;
  // Calculate the number of filled segments in the progress bar.
  int filled = (int)(ratio * BAR_WIDTH);
  // Calculate the percentage.
  float percent = ratio * 100.0f;

  // Determine the color of the progress bar based on the percentage.
  const char *barColor = percent < 33   ? COLOR_RED
                         : percent < 66 ? COLOR_YELLOW
                                        : COLOR_GREEN;

  // Print the progress bar.
  printf("\r%sProgress:%s [", COLOR_BRIGHT_WHITE, COLOR_END);

  for (int i = 0; i < BAR_WIDTH; i++) {
    if (i < filled)
      printf("%s=%s", barColor, COLOR_END);
    else
      printf("%s·%s", COLOR_BRIGHT_BLACK, COLOR_END);
  }

  // Print the percentage and the amount of data transferred.
  printf("] %s%6.2f%%%s (%.2f / %.2f MB)", COLOR_BRIGHT_CYAN, percent,
         COLOR_END, BYTES_TO_MB(done), BYTES_TO_MB(total));

  // Flush the output to ensure the progress bar is displayed immediately.
  fflush(stdout);

  // If the transfer is complete, clear the line and print a completion message.
  if (done >= total) {
    printf("\r\033[K"); // Clear the line
    printf("%s[✔]%s %sTransfer Complete!%s (%.2f MB)\n", COLOR_BRIGHT_GREEN,
           COLOR_END, COLOR_BRIGHT_WHITE, COLOR_END, BYTES_TO_MB(total));
  }
}
