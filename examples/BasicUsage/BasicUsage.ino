#include <Nextion.h>

Nextion hmi(Serial1);

void setup()
{
  Serial.begin(115200);
  Serial1.begin(9600);

  hmi.changePage(0);          // Using pageId
  hmi.changePage("pageName"); // Using pageName

  hmi.refresh(1);    // Using itemId
  hmi.refresh("t3"); // Using itemName

  hmi.click(0, NextionConstants::ClickEvent::Pressed);     // Trigger click event using itemId
  hmi.click("b1", NextionConstants::ClickEvent::Released); // Trigger click event using itemName
}

void loop()
{
  hmi.update(); // Check for responses
}
