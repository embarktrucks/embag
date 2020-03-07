#include <iostream>

#include "lib/embag.h"

int main(int argc, char *argv[]) {
  // TODO: arg processing
  const std::string filename = argv[1];

  Embag reader(filename);

  std::cout << "Opening " << filename << std::endl;

  reader.open();

  reader.readRecords();

  //reader.printAllMsgs();

  for (const auto &message : reader.getView().getMessages("/CANMessageIn/vcan")) {
    reader.printMsg(message);
    std::cout << "----------------------------" << std::endl;
  }

  reader.close();

  std::cout << "Done." << std::endl;

  return 0;
}
