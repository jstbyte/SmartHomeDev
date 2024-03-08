#include <Helper.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <IRutils.h>
#include <IRrecv.h>
#include "SinricPro.h"
#include <SinricProFanUS.h>
#include "SinricProSwitch.h"
#include <TaskScheduler.h>

#define WIFI_SSID "xxxxxxx"
#define WIFI_PASS "xxxxxxx"
#define APP_KEY "xxxxxxxxx"
#define APP_SECRET "xxxxxx"

// Devices Identity;
#define SWITCH_0 "xxxxxxxx"
#define SWITCH_1 "xxxxxxxx"
#define CLNG_FAN "xxxxxxxx"

constexpr const uint8_t switch_pins[] = {5, 4, 14, 12};
decode_results ir_result;
Scheduler scheduler;
IRrecv irrecv(13);

bool onSwitchPowerState(const String &deviceId, bool &state)
{
  if (deviceId == SWITCH_0)
  {
    digitalWrite(switch_pins[0], state ? LOW : HIGH);
    return true;
  }
  if (deviceId == SWITCH_1)
  {
    digitalWrite(switch_pins[1], state ? LOW : HIGH);
    return true;
  }
  if (deviceId == CLNG_FAN)
  {
    digitalWrite(switch_pins[2], state ? LOW : HIGH);
    return true;
  }

  return false;
}

bool manualSwitchPowerState(uint8_t pin, bool state)
{
  String devId; // Store Device ID;
  switch (pin)
  {
  case switch_pins[0]:
    devId = SWITCH_0;
    break;
  case switch_pins[1]:
    devId = SWITCH_1;
    break;
  // case switch_pins[2]:
  //   devId = SWITCH_2;
  //   break;
  default:
    digitalWrite(pin, state);
    return false;
  }

  digitalWrite(pin, state); // ASAP;
  SinricProSwitch &sw = SinricPro[devId];
  return sw.sendPowerStateEvent(!state, "IR_REMOTE_PRESSED");
}

void handleIRemote(uint64_t value)
{
  uint8_t pin; // From IR Remote;
  switch (value)
  {
  case IR_POWER:
    manualSwitchPowerState(switch_pins[0], HIGH);
    manualSwitchPowerState(switch_pins[1], HIGH);
    manualSwitchPowerState(switch_pins[2], HIGH);
    manualSwitchPowerState(switch_pins[3], HIGH);
    return;
  case IR_MUTE:
    manualSwitchPowerState(switch_pins[0], LOW);
    manualSwitchPowerState(switch_pins[1], LOW);
    manualSwitchPowerState(switch_pins[2], LOW);
    manualSwitchPowerState(switch_pins[3], LOW);
    return;
  case IR_1:
    pin = switch_pins[0];
    break;
  case IR_2:
    pin = switch_pins[1];
    break;
  case IR_3:
    pin = switch_pins[2];
    digitalWrite(switch_pins[3], HIGH);
    break;
  case IR_MODE:
    pin = switch_pins[3];
    digitalWrite(pin, !digitalRead(pin));
    return;
  // case IR_4:
  //   pin = switch_pins[3];
  //   break;
  default:
    return;
  }

  bool state = digitalRead(pin);
  manualSwitchPowerState(pin, !state);
}

void onConnected()
{
  Serial.printf("Connected to SinricPro\r\n");
  analogWrite(LED_BUILTIN, 254);
  ledTask.disable();
}

void onDisconnected()
{
  Serial.printf("Disconnected from SinricPro\r\n");
  ledTask.enable();
}

bool onFanRangeValue(const String &deviceId, int &speed)
{
  if (speed == 1)
  {
    digitalWrite(switch_pins[3], LOW);
  }
  else
  {
    digitalWrite(switch_pins[3], HIGH);
  }

  return true;
}

void setup()
{
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.setHostname("JSHome"); // Device Name;

  // Switch Pins Initialization;
  pinMode(switch_pins[0], OUTPUT);
  digitalWrite(switch_pins[0], HIGH);
  pinMode(switch_pins[1], OUTPUT);
  digitalWrite(switch_pins[1], HIGH);
  pinMode(switch_pins[2], OUTPUT);
  digitalWrite(switch_pins[2], HIGH);
  pinMode(switch_pins[3], OUTPUT);
  digitalWrite(switch_pins[3], HIGH);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  ledTask.set(5, TASK_FOREVER, &ledBlink);
  scheduler.addTask(ledTask);
  ledTask.enableDelayed();

  irrecv.enableIRIn();
  SinricProSwitch &sw_1 = SinricPro[SWITCH_0];
  sw_1.onPowerState(onSwitchPowerState);
  SinricProSwitch &sw_2 = SinricPro[SWITCH_1];
  sw_2.onPowerState(onSwitchPowerState);

  SinricProFanUS &fan = SinricPro[CLNG_FAN];
  fan.onAdjustRangeValue(onFanRangeValue);
  fan.onPowerState(onSwitchPowerState);
  fan.onRangeValue(onFanRangeValue);

  SinricPro.onConnected(onConnected);
  SinricPro.onDisconnected(onDisconnected);
  SinricPro.begin(APP_KEY, APP_SECRET);
}

void loop()
{
  SinricPro.handle();
  scheduler.execute();
  if (irrecv.decode(&ir_result))
  {
    handleIRemote(ir_result.value);
    irrecv.resume();
  }
}