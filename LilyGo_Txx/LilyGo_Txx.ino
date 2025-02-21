// The first time you need to define the board model and version

// #define T4_V12
#define T4_V13
// #define T10_V14
// #define T10_V18
// #define T10_V20

#if defined (T10_V18)
#include "T10_V18.h"
#elif defined(T10_V14)
#include "T10_V14.h"
#elif defined(T4_V12)
#include "T4_V12.h"
#elif defined(T4_V13)
#include "T4_V13.h"
#elif defined(T10_V20)
#include "T10_V20.h"
#else
#error "T4_V13"
#endif

#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include <Ticker.h>
#include <Button2.h>
#include <SD.h>

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

#ifdef ENABLE_MPU9250
#include "sensor.h"
extern MPU9250 IMU;
#endif

SPIClass sdSPI(VSPI);
#define IP5306_ADDR         0X75
#define IP5306_REG_SYS_CTL0 0x00

uint8_t state = 0;
Button2 *pBtns = nullptr;
uint8_t g_btns[] = BUTTONS_MAP;
char buff[512];
Ticker btnscanT;

bool setPowerBoostKeepOn(int en) {
    Wire.beginTransmission(IP5306_ADDR);
    Wire.write(IP5306_REG_SYS_CTL0);
    Wire.write(en ? 0x37 : 0x35);
    return Wire.endTransmission() == 0;
}

void button_handle(uint8_t gpio) {
    switch (gpio) {
#ifdef BUTTON_1
    case BUTTON_1: state = 1; break;
#endif
#ifdef BUTTON_2
    case BUTTON_2: state = 2; break;
#endif
#ifdef BUTTON_3
    case BUTTON_3: state = 3; break;
#endif
#ifdef BUTTON_4
    case BUTTON_4: state = 4; break;
#endif
    default: break;
    }
}

void button_callback(Button2 &b) {
    for (int i = 0; i < sizeof(g_btns) / sizeof(g_btns[0]); ++i) {
        if (pBtns[i] == b) {
            Serial.printf("btn: %u press\n", pBtns[i].getPin());  // ✅ Corectat
            button_handle(pBtns[i].getPin());                     // ✅ Corectat
        }
    }
}

void button_init() {
    uint8_t args = sizeof(g_btns) / sizeof(g_btns[0]);
    pBtns = new Button2[args];
    for (int i = 0; i < args; ++i) {
        pBtns[i] = Button2(g_btns[i]);
        pBtns[i].setPressedHandler(button_callback);
    }
}

void button_loop() {
    for (int i = 0; i < sizeof(g_btns) / sizeof(g_btns[0]); ++i) {
        pBtns[i].loop();
    }
}

// 🔊 PWM complet pentru buzzer
#define CHANNEL_0 0
#define PWM_FREQ 1000
#define PWM_RESOLUTION 8

void buzzer_test() {
    if (SPEAKER_OUT > 0) {
        if (SPEAKER_PWD > 0) {
            pinMode(SPEAKER_PWD, OUTPUT);
            digitalWrite(SPEAKER_PWD, HIGH);
        }
        ledcSetup(CHANNEL_0, PWM_FREQ, PWM_RESOLUTION);  // ✅ PWM configurat
        ledcAttachPin(SPEAKER_OUT, CHANNEL_0);          // ✅ Atașare pin la PWM
    }
}

void playSound() {
    if (SPEAKER_OUT > 0) {
        ledcWriteTone(CHANNEL_0, 2000);  // ✅ Sunet la 2kHz
        delay(300);
        ledcWriteTone(CHANNEL_0, 0);     // ✅ Oprire sunet
    }
}

void wifi_scan() {
    tft.fillScreen(TFT_BLACK);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    int16_t n = WiFi.scanNetworks();
    if (n == 0) {
        tft.drawString("No networks found", tft.width() / 2, tft.height() / 2);
    } else {
        for (int i = 0; i < n; ++i) {
            sprintf(buff, "[%d]:%s(%d)", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
            tft.println(buff);
        }
    }
}

void spisd_test() {
    tft.fillScreen(TFT_BLACK);
    sdSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, sdSPI)) {
        tft.setTextFont(2);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("SDCard MOUNT FAIL", tft.width() / 2, tft.height() / 2);
    } else {
        uint32_t cardSize = SD.cardSize() / (1024 * 1024);
        String str = "SDCard Size: " + String(cardSize) + "MB";
        tft.setTextFont(2);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString(str, tft.width() / 2, tft.height() / 2);
    }
}

void setup() {
    Serial.begin(115200);
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    spisd_test();
    buzzer_test();
    button_init();
    btnscanT.attach_ms(30, button_loop);
}

void loop() {
    switch (state) {
        case 1:
            wifi_scan();
            state = 0;
            break;

        case 2:
            tft.fillScreen(TFT_BLACK);
            tft.drawString("Buzzer Test", tft.width() / 2, tft.height() / 2);
            playSound();
            state = 0;
            break;

        case 3:
            tft.fillScreen(TFT_BLACK);
            tft.drawString("Button 3 pressed", tft.width() / 2, tft.height() / 2);
            state = 0;
            break;

        case 4:
            spisd_test();
            state = 0;
            break;

        default:
            break;
    }
}
