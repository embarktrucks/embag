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

  Embag::Bag bag_a{"/home/jason/em/embag/truck-201_run-15955_2020-03-12-00-06-09_slim_107.bag"};
  Embag::Bag bag_b{"/home/jason/em/embag/truck-201_run-15955_2020-03-12-00-06-09_luminar_107.bag"};

  Embag::View view{};

  // FIXME
  //view.addBags({bag_a, bag_b});
  view.addBag(bag_a);
  view.addBag(bag_b);

  for (const auto &message : view.getMessages({"/debug/planner", "/luminar_pointcloud"})) {
    std::cout << message->timestamp.secs << "." << message->timestamp.nsecs << " : " << message->topic << std::endl;
    //message->print();
  }

  bag_a.close();

  return 0;
}
