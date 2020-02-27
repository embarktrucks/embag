#include <iostream>

#include "lib/embag.h"

int main(int argc, char *argv[]) {
  // TODO: arg processing
  const std::string filename = argv[1];

  Embag reader(filename);

  std::cout << "Opening " << filename << std::endl;
  reader.open();

  reader.readRecords();

  reader.printMsgs();

  reader.close();

  std::cout << "Done." << std::endl;

  return 0;
}