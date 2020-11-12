/** Simple ROS node to publish to the "button_press" topic
*   Author: Bernard Chen
*   ----------
*   Used by romi_emulator to substitute physical button presses
*/

#include "ros/ros.h"
#include "std_msgs/Bool.h"

#include <iostream>

int main(int argc, char **argv)
{
  // Init ROS node to emulate romi button press
  ros::init(argc, argv, "button_press_pub");
  ros::NodeHandle n;
  ros::Publisher button_pub = n.advertise<std_msgs::Bool>("button_press", 1000);

  std_msgs::Bool press;

  // Check for single button press argument
  // Send single button press and exit
  if (argc == 2) {
    std::string arg = argv[1];
    if (arg.compare("-s") || arg.compare("--single")) {
      press.data = true;

      // Wait for subscriber before publishing once
      ros::Rate poll_rate(100);
      while(button_pub.getNumSubscribers() == 0)
        poll_rate.sleep();

      button_pub.publish(press);
      exit(0);
    }
  }

  // Otherwise take user input
  while (ros::ok())
  {
    std::cout << "Press \'Enter\' to send a Kobuki button press";

    // Wait for 'Enter' key press
    std::cin.get();

    press.data = true;
    button_pub.publish(press);
  }

  return 0;
}