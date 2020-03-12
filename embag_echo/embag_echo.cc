#include <iostream>

#include "lib/embag.h"

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cout << "Usage: embag_echo path/to/bagfile.bag /topic/of/interest" << std::endl;
    exit(1);
  }

  const std::string filename = argv[1];
  const std::string topic = argv[2];

  std::cout << "Opening " << filename << std::endl;

  Embag reader{filename};

  for (const auto &message : reader.getView().getMessages(topic)) {
    std::cout << message->topic << " at " << message->timestamp.secs << "." << message->timestamp.nsecs << std::endl;
  }

  reader.close();

  return 0;
}
