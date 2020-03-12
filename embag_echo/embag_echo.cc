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
  reader.open();

  for (const auto &message : reader.getView().getMessages("/CANMessageIn/vcan")) {
    std::cout << message->topic << " at " << message->timestamp.secs << "." << message->timestamp.nsecs << std::endl;
    // TODO: string_value make private
    std::cout << message->data->get("messages")->get(0)->get("name")->string_value << std::endl;
    std::cout << message->data->get("messages")->get(0)->get("name")->get<std::string>() << std::endl;

    // TODO: this is the target api, implement this!
    std::cout << message->data("messages")->at(0)->getValue<std::string>("name") << std::endl;

    message("messages")
    message->data("messages")
    std::cout << "----------------------------" << std::endl;
  }

  reader.close();

  std::cout << "Done." << std::endl;

  return 0;
}
