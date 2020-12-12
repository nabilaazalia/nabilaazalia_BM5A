#include <string.h>
// Device libraries (Arduino ESP32/ESP8266 Cores)
#include <Arduino.h>
#include <ESP8266WiFi.h>
// Custom libraries
#include <utlgbotlib.h>
#include <DHT.h>

#define DHTPIN D5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float h, t;
#define WIFI_SSID "R family"
#define WIFI_PASS "keren105"

#define MAX_CONN_FAIL 50
#define MAX_LENGTH_WIFI_SSID 31
#define MAX_LENGTH_WIFI_PASS 63

#define TLG_TOKEN "1252237161:AAEy93V99v6IYfSN_lwi4-6JrhFEjxHS2J0" //

#define DEBUG_LEVEL_UTLGBOT 0

#define PIN_RELAY 5 // ESP32 RELAYnya di pin gpio5

const char TEXT_START[] =
  "Hello, im a Bot running in an ESP microcontroller that let you turn on/off a RELAY/light.\n"
  "\n"
  "Check /help command to see how to use me.";

char buff[100];
boolean state_t, state_h;

const char TEXT_HELP[] =
  "Available Commands:\n"
  "\n"
  "/start - Show start text.\n"
  "/help - Show actual text.\n"
  "/relayon - Turn on the RELAY.\n"
  "/relayoff - Turn off the RELAY.\n"
  "/relaystatus - Show actual RELAY status.\n"
  "/statussuhu - Show actual temperature status.\n"
  "/statuskelembaban - Show actual Humidity status.\n";

void wifi_init_stat(void);
bool wifi_handle_connection(void);


// Create Bot object
uTLGBot Bot(TLG_TOKEN);

// RELAY status
uint8_t relay_status;

void setup(void)
{
  Bot.set_debug(DEBUG_LEVEL_UTLGBOT);
  Serial.begin(115200);
  digitalWrite(PIN_RELAY, HIGH);
  pinMode(PIN_RELAY, OUTPUT);
  relay_status = 1;

  wifi_init_stat();

  Serial.println("Waiting for WiFi connection.");
  while (!wifi_handle_connection())
  {
    Serial.print(".");
    delay(500);
  }
  dht.begin();
  Bot.getMe();
}

void loop()
{
  if (!wifi_handle_connection())
  {
    // Wait 100ms and check again
    delay(100);
    return;
  }

  // Check for Bot received messages
  while (Bot.getUpdates())
  {
    Serial.println("Received message:");
    Serial.println(Bot.received_msg.text);
    Serial.println(Bot.received_msg.chat.id);

    if (strncmp(Bot.received_msg.text, "/start", strlen("/start")) == 0)
    {
      Bot.sendMessage(Bot.received_msg.chat.id, TEXT_START);
    }

    else if (strncmp(Bot.received_msg.text, "/help", strlen("/help")) == 0)
    {
      Bot.sendMessage(Bot.received_msg.chat.id, TEXT_HELP);
    }

    else if (strncmp(Bot.received_msg.text, "/relayon", strlen("/relayon")) == 0)
    {
      relay_status = 0;
      Serial.println("Command /relayon received.");
      Serial.println("Turning on the Relay.");

      Bot.sendMessage(Bot.received_msg.chat.id, "Relay turned on.");
    }

    else if (strncmp(Bot.received_msg.text, "/relayoff", strlen("/relayoff")) == 0)
    {
      relay_status = 1;
      // Show command reception through Serial
      Serial.println("Command /relayoff received.");
      Serial.println("Turning off the RELAY.");

      // Send a Telegram message to notify that the RELAY has been turned off
      Bot.sendMessage(Bot.received_msg.chat.id, "RELAY turned off.");
    }

    // If /RELAYstatus command was received
    else if (strncmp(Bot.received_msg.text, "/relaystatus", strlen("/relaystatus")) == 0)
    {
      // Send a Telegram message to notify actual RELAY status
      if (relay_status)
        Bot.sendMessage(Bot.received_msg.chat.id, "The RELAY is off.");
      else
        Bot.sendMessage(Bot.received_msg.chat.id, "The RELAY is on.");
    }
    else if (strncmp(Bot.received_msg.text, "/statussuhu", strlen("/statussuhu")) == 0)
    {
      t = dht.readTemperature();
      if (isnan(h) || isnan(t)) {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
      }
      String msg = "Status Suhu :";
      msg += t;
      msg += " °C\n";

      msg.toCharArray(buff, 100);
      Bot.sendMessage(Bot.received_msg.chat.id, buff);
    }
    else if (strncmp(Bot.received_msg.text, "/statuskelembaban", strlen("/statuskelembaban")) == 0)
    {
      h = dht.readHumidity();
      if (isnan(h) || isnan(t)) {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
      }
      String msg = "Status Kelembaban :";
      msg += h;
      msg += " %Rh\n";

      msg.toCharArray(buff, 100);
      Bot.sendMessage(Bot.received_msg.chat.id, buff);
    }

    digitalWrite(PIN_RELAY, relay_status);
    // Feed the Watchdog
    yield();
  }

  h = dht.readHumidity();
  t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  if (t > 37 && state_t == 0) {
    state_t = 1;
    String msg = "Status Suhu :";
    msg += t;
    msg += " °C\n";
    msg += "Hati Hati Panas.";

    msg.toCharArray(buff, 100);
    Bot.sendMessage("556538872", buff);
  }
  else if (t <= 37) {
    state_t = 0;
  }

  if (h < 60 && state_h == 0) {
    state_h = 1;
    String msg = "Status Kelembaban :";
    msg += h;
    msg += " %Rh\n";
    msg += "Hati Hati Kering.";

    msg.toCharArray(buff, 100);
    Bot.sendMessage("556538872", buff);
  }
  else if (h >= 60) {
    state_h = 0;
  }
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.println(F("°C "));
  delay(1000);
}


// Init WiFi interface
void wifi_init_stat(void)
{
  Serial.println("Initializing TCP-IP adapter...");
  Serial.print("Wifi connecting to SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.println("TCP-IP adapter successfuly initialized.");
}


bool wifi_handle_connection(void)
{
  static bool wifi_connected = false;

  // Device is not connected
  if (WiFi.status() != WL_CONNECTED)
  {
    // Was connected
    if (wifi_connected)
    {
      Serial.println("WiFi disconnected.");
      wifi_connected = false;
    }

    return false;
  }
  // Device connected
  else
  {
    // Wasn't connected
    if (!wifi_connected)
    {
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());

      wifi_connected = true;
    }

    return true;
  }
}
