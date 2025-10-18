#include "progress_bar.h"
#include "colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void progessBar(int progress) {
  const int BAR_SIZE = 50;
  printf("\r%sPROGRESS   -   %s%s[%s", COLOR_YELLOW, COLOR_END,
         progress == 0 ? COLOR_RED : COLOR_GREEN, COLOR_END);
  for (int i = 0; i < BAR_SIZE; i++) {
    if (i < progress / 2) {
      printf("%s=%s", COLOR_GREEN, COLOR_END);
    } else {
      printf("%s_%s", COLOR_RED, COLOR_END);
    }
  }
  printf("%s]%s %s- %d%%%s ", progress == 100 ? COLOR_GREEN : COLOR_RED,
         COLOR_END, COLOR_BLUE, progress, COLOR_END);
  fflush(stdout);
}
