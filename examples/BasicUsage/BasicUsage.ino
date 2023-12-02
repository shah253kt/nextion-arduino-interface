#include <NextionInterface.h>

NextionInterface hmi(Serial);
NextionComponent myComponent(0, 3, "b0");

void setup() {
  Serial.begin(115200);
  setupHmi();
}

void loop() {
  hmi.update();  // Check for responses
}

void setupHmi() {
  Serial.println();

  hmi.onPageIdUpdated = [](uint8_t pageId) {
    Serial.print(F("Page ID: "));
    Serial.println(pageId);
  };

  hmi.onNumericDataReceived = [](const NextionComponent *component, uint32_t value) {
    Serial.print(F("Numeric data received: "));
    Serial.println(value);
    Serial.print(F("Component: "));
    Serial.println(component->name());
  };

  hmi.onStringDataReceived = [](const NextionComponent *component, char* data) {
    Serial.print(F("String data received: "));
    Serial.println(data);
    Serial.print(F("Component: "));
    Serial.println(component->name());
  };

  hmi.onTouchEvent = [](uint8_t pageId, uint8_t componentId, NextionConstants::ClickEvent event) {
    Serial.print(F("Touch event received:"));
    Serial.print(F("Page ID: "));
    Serial.println(pageId);
    Serial.print(F("Component ID: "));
    Serial.println(componentId);
    Serial.print(F("Event: "));
    Serial.println(event == NextionConstants::ClickEvent::Released ? "Released" : "Pressed");
  };

  hmi.changePage(0);           // Using pageId
  hmi.changePage("pageName");  // Using pageName

  hmi.refresh(1);     // Using itemId
  hmi.refresh("t3");  // Using itemName
  hmi.refresh(myComponent);

  hmi.click(0, NextionConstants::ClickEvent::Pressed);      // Trigger click event using itemId
  hmi.click("b1", NextionConstants::ClickEvent::Released);  // Trigger click event using itemName
  hmi.click(myComponent, NextionConstants::ClickEvent::Released);

  hmi.sleep(true);
  hmi.sleep(false);

  hmi.getCurrentPageId();

  hmi.setText(myComponent, "Hello!");
  hmi.setInteger(myComponent, 123);

  hmi.getText(myComponent);
  hmi.getInteger(myComponent);

  hmi.setDate(25, 3, 1992);
  hmi.setTime(4, 17, 58);
  hmi.getDate();
  hmi.getTime();
  hmi.getDayOfTheWeek();
}
