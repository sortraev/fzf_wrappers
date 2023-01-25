#include <stdio.h>
int main() {
  char *l[] = {
    "#include <stdio.h>",
    "int main() {",
    "  char *l[] = {",
    "    ",
    "  };",
    "  for (int i = 0; i < 3; i++)",
    "    (void)(printf(l[i])|putchar(10));",
    "  for (int i = 0; i < sizeof(l) / 8; i++)",
    "    (void)(printf(l[3])|putchar(34)|printf(l[i])|putchar(34)|putchar(44)|putchar(10));",
    "  for (int i = 4; i < sizeof(l) / 8; i++)",
    "    (void)(printf(l[i])|putchar(10));",
    "}",
  };
  for (int i = 0; i < 3; i++)
    (void)(printf(l[i])|putchar(10));
  for (int i = 0; i < sizeof(l) / 8; i++)
    (void)(printf(l[3])|putchar(34)|printf(l[i])|putchar(34)|putchar(44)|putchar(10));
  for (int i = 4; i < sizeof(l) / 8; i++)
    (void)(printf(l[i])|putchar(10));
}
