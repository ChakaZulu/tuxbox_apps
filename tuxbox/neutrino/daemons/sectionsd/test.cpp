#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <set>

#include "SIutils.hpp"
#include "SIservices.hpp"
#include "SIevents.hpp"
#include "SIsections.hpp"

int main(void)
{
  FILE *file=fopen("seit1", "rb");
  if(!file) {
    puts("Kann Datei nicht oeffnen!");
    return 1;
  }
  fseek(file, 0, SEEK_END);
  long anzBytes=ftell(file);
  char *buf =new char[anzBytes];
  fseek(file, 0, SEEK_SET);
  fread(buf, anzBytes, 1, file);
  fclose(file);
  SIsection s(anzBytes, buf); // Puffer wird uebernommen
  SIsectionEIT seit(s);
  for(SIevents::iterator e=seit.events().begin(); e!=seit.events().end(); e++)
    e->dump();
}
