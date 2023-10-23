#include <Nextion.h>

Nextion hmi(Serial1);
NextionComponent myComponent(3, "b0");

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);

  while(!Serial);

  hmi.onPageNumberUpdated = [](uint8_t pageNumber) {
    Serial.print(F("Page number: "));
    Serial.println(pageNumber);
  };

  hmi.onNumericDataReceived = [](uint32_t value) {
    Serial.print(F("Numeric data received: "));
    Serial.println(value);
  };

  hmi.onStringDataReceived = [](char* data) {
    Serial.print(F("String data received: "));
    Serial.println(data);
  };

  hmi.onTouchEvent = [](uint8_t pageNumber, uint8_t componentId, NextionConstants::ClickEvent event) {
    Serial.print(F("Touch event received:"));
    Serial.print(F("Page number: "));
    Serial.println(pageNumber);
    Serial.print(F("Component ID: "));
    Serial.println(componentId);
    Serial.print(F("Event: "));
    Serial.println(event == NextionConstants::ClickEvent::Released ? "Released" : "Pressed");
  };

  hmi.changePage(0);           // Using pageId
  hmi.changePage("pageName");  // Using pageName
  hmi.changePage(myComponent);

  hmi.refresh(1);     // Using itemId
  hmi.refresh("t3");  // Using itemName
  hmi.refresh(myComponent);

  hmi.click(0, NextionConstants::ClickEvent::Pressed);      // Trigger click event using itemId
  hmi.click("b1", NextionConstants::ClickEvent::Released);  // Trigger click event using itemName
  hmi.click(myComponent, NextionConstants::ClickEvent::Released);

  hmi.sleep(true);
  hmi.sleep(false);

  hmi.getCurrentPageNumber();

  hmi.setText("t3", "Hello!");
  hmi.setInteger("sys0", 123);

  hmi.setText(myComponent, "Hello!");
  hmi.setInteger(myComponent, 123);

  hmi.getText("t3");
  hmi.getInteger("sys0");

  hmi.getText(myComponent);
  hmi.getInteger(myComponent);
}

void loop() {
  hmi.update();  // Check for responses
}
