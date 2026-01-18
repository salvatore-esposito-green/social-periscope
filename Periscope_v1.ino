#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <GxEPD2_3C.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <QRCodeGFX.h>

// --- HARDWARE CONFIGURATION ---
#define BUZZER_PIN 3
#define LED_PIN    8
#define WIFI_CHANNEL 1 

// E-Paper Display Pins (SPI)
#define EPD_CS      7
#define EPD_DC      1
#define EPD_RST     2
#define EPD_BUSY    10

// --- DATA STRUCTURES ---
typedef struct struct_message {
    char name[20];
    char surname[20];
    char role[20];
    char company[20];
    char qrLink[50];
} struct_message;

struct_message myProfile = {"Salvatore", "Esposito", "Developer", "Codemotion", "https://github.com/salvatore-esposito-green"};
struct_message receivedProfile;
struct_message myData;

// --- DISPLAY & GRAPHICS OBJECTS ---
GxEPD2_3C<GxEPD2_290_C90c, GxEPD2_290_C90c::HEIGHT> display(GxEPD2_290_C90c(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
U8G2_FOR_ADAFRUIT_GFX u8g2;
QRCodeGFX qrcode(display);

// --- GLOBAL STATE VARIABLES ---
bool needRefresh = true;    // Triggers E-ink update
bool inContact = false;     // True if a peer is "locked on"
float smoothRSSI = -100.0;  // Filtered signal strength to prevent jitter
unsigned long lastRecvTime = 0;
unsigned long lastBeepTime = 0;
unsigned long lastSendTime = 0;
unsigned long beepOffTime = 0;
uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// --- AUDIO/VISUAL FEEDBACK ---
void startBeep(int freq) {
    ledcWriteTone(BUZZER_PIN, freq);
    digitalWrite(LED_PIN, LOW); // LED ON (assuming Active Low)
}

void stopBeep() {
    ledcWriteTone(BUZZER_PIN, 0);
    digitalWrite(LED_PIN, HIGH); // LED OFF
}

// --- GRAPHICS DRAWING FUNCTIONS ---

// Default screen showing my own info
void drawMyProfile() {
    u8g2.setBackgroundColor(GxEPD_WHITE);
    u8g2.setForegroundColor(GxEPD_BLACK);
    u8g2.setFont(u8g2_font_helvB18_tf);
    u8g2.setCursor(5, 25); u8g2.print(myProfile.name);
    u8g2.setCursor(5, 50); u8g2.print(myProfile.surname);
    
    u8g2.setFont(u8g2_font_luBIS10_tf);
    u8g2.setCursor(5, 85); u8g2.print(myProfile.role);
    
    u8g2.setForegroundColor(GxEPD_RED);
    u8g2.setFont(u8g2_font_helvB18_tf);
    u8g2.setCursor(5, 110); u8g2.print(myProfile.company);
    
    qrcode.generateData(myProfile.qrLink);
    qrcode.setScale(3);
    qrcode.draw(display.width() - qrcode.getSideLength(), 0);
    
    u8g2.setForegroundColor(GxEPD_BLACK);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(100, 125); u8g2.print("MODE: PERISCOPE...");
}

// Screen shown when a nearby peer is detected
void drawTargetAcquired() {
    u8g2.setBackgroundColor(GxEPD_WHITE);
    u8g2.setForegroundColor(GxEPD_BLACK);
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.setCursor(70, 15); u8g2.print("!!! TARGET ACQUIRED !!!");
    
    display.drawRect(0, 20, display.width(), 2, GxEPD_BLACK);
    
    u8g2.setFont(u8g2_font_helvB14_tf);
    u8g2.setCursor(10, 50); u8g2.print(receivedProfile.name);
    u8g2.setCursor(10, 70); u8g2.print(receivedProfile.surname);
    
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.setCursor(10, 95); u8g2.print(receivedProfile.role);
    
    u8g2.setForegroundColor(GxEPD_RED);
    u8g2.setCursor(10, 110); u8g2.print(receivedProfile.company);
    
    qrcode.generateData(receivedProfile.qrLink);
    qrcode.setScale(3);
    int qrSize = qrcode.getSideLength();
    qrcode.draw(display.width() - qrSize - 5, display.height() - qrSize - 5);
}

void drawContent() {
    if (inContact) drawTargetAcquired();
    else drawMyProfile();
}

// Full refresh handler for E-Ink (Slow process)
void refreshDisplay() {
    display.init(115200, true, 50, false);
    display.setRotation(3);
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        drawContent();
    } while (display.nextPage());
    display.hibernate(); // Put display to sleep to save power/prevent damage
    needRefresh = false;
}

// --- ESP-NOW CALLBACK ---
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    lastRecvTime = millis();
    if (recv_info->rx_ctrl != nullptr) {
        int currentRSSI = recv_info->rx_ctrl->rssi;
        // EMA Filter: 70% old value, 30% new value to smooth out signal spikes
        if (smoothRSSI < -99) smoothRSSI = currentRSSI;
        else smoothRSSI = (smoothRSSI * 0.7) + (currentRSSI * 0.3);
        
        memcpy(&receivedProfile, data, sizeof(receivedProfile));
    }
}

void setup() {
    Serial.begin(115200);
    
    // PWM Setup for Buzzer
    ledcAttach(BUZZER_PIN, 2000, 8);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    // WiFi & ESP-NOW Setup
    WiFi.mode(WIFI_STA);
    esp_wifi_start();
    esp_wifi_set_max_tx_power(78); // Max power for range
    esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
    esp_now_init();
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    
    // Register Broadcast Peer
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddr, 6);
    peerInfo.channel = 0;
    esp_now_add_peer(&peerInfo);
    
    memcpy(&myData, &myProfile, sizeof(myData));

    // Display Init
    display.init(115200, true, 50, false);
    u8g2.begin(display);
    refreshDisplay();
    
    // Startup Sound
    startBeep(2000); delay(300); stopBeep();
}

void loop() {
    unsigned long now = millis();

    // Handle asynchronous buzzer shutoff
    if (now >= beepOffTime) stopBeep();

    // Broadcast my data every 300ms
    if (now - lastSendTime >= 300) {
        lastSendTime = now;
        esp_now_send(NULL, (uint8_t *) &myData, sizeof(myData));
    }

    // Timeout: If no data received for 2.5s, person has walked away
    if (now - lastRecvTime > 2500) {
        smoothRSSI = -100;
        if (inContact) { 
            inContact = false; 
            needRefresh = true; 
        }
    }

    int currentRSSI = (int)smoothRSSI;

    // RADAR LOGIC
    if (currentRSSI > -95) { // Potential target detected
        // Map RSSI to beep frequency (Distance logic)
        int clampedRSSI = constrain(currentRSSI, -95, -80);
        int interval = map(clampedRSSI, -95, -80, 2000, 50); // Faster beeps when closer

        if (now - lastBeepTime >= interval) {
            lastBeepTime = now;
            int pitch = map(clampedRSSI, -95, -80, 800, 3500); // Higher pitch when closer
            startBeep(pitch);
            beepOffTime = now + 20; // Short 20ms click/beep
        }

        // LOCK ON LOGIC: Target is very close
        if (currentRSSI > -80 && !inContact) {
            inContact = true;
            
            // Audible Alarm Phase (5 seconds)
            unsigned long waitStart = millis();
            while(millis() - waitStart < 5000) {
                if (millis() - lastBeepTime >= 50) {
                    lastBeepTime = millis();
                    startBeep(3500);
                    delay(15);
                    stopBeep();
                }
                yield(); // Prevent watchdog triggers
            }
            needRefresh = true; 
        }
    }

    // Execute E-Ink refresh if state changed
    if (needRefresh) {
        refreshDisplay();
        if (inContact) {
            // Once a target is captured, pause for 10s to let the user read/scan
            delay(10000); 
            lastBeepTime = millis();
            lastRecvTime = millis();
        }
    }
    yield();
}