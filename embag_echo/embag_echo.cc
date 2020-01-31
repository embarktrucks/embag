#include <iostream>

#include "lib/embag.h"

int main(int argc, char *argv[]) {
  const std::string filename = argv[1];

  Embag reader(filename);

  std::cout << "Opening " << filename << std::endl;
  reader.open();

  reader.read_records();

  reader.close();

  return 0;
}