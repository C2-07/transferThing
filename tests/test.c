#include <stddef.h>
#include <stdio.h>
#include <string.h>

int main() {
  char name[24];

  for (size_t i = 0; i < strlen(name); i++) {
   printf("%c", name[i]);
  }
  printf("%zu", strlen(name));
  return 0;
}
