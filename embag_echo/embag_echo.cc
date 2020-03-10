#include <iostream>

#include "lib/embag.h"

int main(int argc, char *argv[]) {
  // TODO: proper arg processing
  const std::string filename = argv[1];

  Embag reader(filename);

  std::cout << "Opening " << filename << std::endl;

  reader.open();

  for (const auto &message : reader.getView().getMessages("/CANMessageIn/vcan")) {
    message->print();
    std::cout << "----------------------------" << std::endl;
  }

  reader.close();

  std::cout << "Done." << std::endl;

  return 0;
}
