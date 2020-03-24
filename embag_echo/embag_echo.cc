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

  Embag::Bag bag_a{filename};
  Embag::Bag bag_b{"truck-201_run-15955_2020-03-12-00-06-09_luminar_107.bag"};

  for (const auto &message : bag_a.getView().getMessages(topic)) {
    std::cout << message->timestamp.secs << "." << message->timestamp.nsecs << " : " << message->topic << std::endl;
    message->print();
  }

  bag_a.close();

  return 0;
}
