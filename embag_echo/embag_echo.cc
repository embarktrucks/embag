#include <iostream>

#include "lib/embag.h"

int main(int argc, char *argv[]) {
  // TODO: proper arg processing
  const std::string filename = argv[1];

  std::cout << "Opening " << filename << std::endl;

  Embag reader(filename);
  reader.open();

  for (const auto &message : reader.getView().getMessages("/CANMessageIn/vcan")) {
    std::cout << message->topic << " at " << message->timestamp.secs << "." << message->timestamp.nsecs << std::endl;
    message->value->print();
    std::cout << "----------------------------" << std::endl;
  }

  reader.close();

  std::cout << "Done." << std::endl;

  return 0;
}
