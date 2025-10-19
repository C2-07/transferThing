#include "progress_bar.h"
#include "colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BYTES_TO_MB(bytes) ((bytes) / (1024.0f * 1024.0f))
#define BAR_WIDTH 40


void progressBarUpdate(int done, int total) {
  float ratio = (float)done / total;
  int filled = (int)(ratio * BAR_WIDTH);
  float percent = ratio * 100.0f;

  const char *barColor = percent < 33   ? COLOR_RED
                         : percent < 66 ? COLOR_YELLOW
                                        : COLOR_GREEN;

  printf("\r%sProgress:%s [", COLOR_BRIGHT_WHITE, COLOR_END);

  for (int i = 0; i < BAR_WIDTH; i++) {
    if (i < filled)
      printf("%s=%s", barColor, COLOR_END);
    else
      printf("%s·%s", COLOR_BRIGHT_BLACK, COLOR_END);
  }
  printf("%s]%s %s- %d%%%s ", progress == 100 ? COLOR_GREEN : COLOR_RED,
         COLOR_END, COLOR_BLUE, progress, COLOR_END);
  fflush(stdout);
}
