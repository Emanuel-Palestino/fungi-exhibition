#include <FastLED.h>

// ====== CONFIGURACIÓN GENERAL ======
#define LED_PIN     6
#define NUM_LEDS    120
#define LED_TYPE    WS2812B
#define COLOR_ORDER RGB
#define BRIGHTNESS  255

CRGB leds[NUM_LEDS];

// ====== COLOR BASE (RGB) ======
uint8_t baseR = 100;
uint8_t baseG = 250;
uint8_t baseB = 1;

// ====== PARÁMETROS DE RESPIRACIÓN ======
uint8_t breathBrightness = 0; 
int breathDirection = 1; 
unsigned long lastUpdate = 0; 

// Velocidades base (ms entre pasos)
int breathSpeedNormal = 30;    // respiración normal
int breathSpeedAnormal = 10;   // más rápida o más lenta

// Porcentajes
int probRespNormal = 60; // porcentaje de respiraciones normales
int probRespAnormal = 40; // se calcula como 100 - probRespNormal

// Rango de brillo
uint8_t minBrightness = 10;   
uint8_t maxBrightness = 100;  

// Control de variaciones
bool esNormal = true;

// ====== PARÁMETROS DE VENAS ======
int pulsoPos = 0;
int pulsoAncho = 15;
int pulsoVelocidad = 60;
uint8_t pulsoBrillo = minBrightness;
int pulsoDireccionBrillo = 1;
unsigned long ultimoPulso = 0;

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  Serial.println("Envia color RGB como: R,G,B (0-255)");

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  breathBrightness = minBrightness;
  pulsoBrillo = minBrightness;

  randomSeed(analogRead(A0)); // Semilla aleatoria
}

// ====== LECTURA DE COLOR DESDE SERIAL ======
void leerColorSerial() {
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    data.trim(); 

    int firstComma = data.indexOf(',');
    int secondComma = data.indexOf(',', firstComma + 1);

    if (firstComma > 0 && secondComma > firstComma) {
      int r = data.substring(0, firstComma).toInt();
      int g = data.substring(firstComma + 1, secondComma).toInt();
      int b = data.substring(secondComma + 1).toInt();

      baseR = constrain(r, 0, 255);
      baseG = constrain(g, 0, 255);
      baseB = constrain(b, 0, 255);

      Serial.print("Nuevo color: ");
      Serial.print(baseR); Serial.print(", ");
      Serial.print(baseG); Serial.print(", ");
      Serial.println(baseB);
    }
  }
}

// ====== EFECTO RESPIRACIÓN ======
void efectoRespiracion(uint8_t r, uint8_t g, uint8_t b) {
  unsigned long now = millis();

  int velocidadActual = esNormal ? breathSpeedNormal : breathSpeedAnormal;

  if (now - lastUpdate > velocidadActual) {
    lastUpdate = now;
    
    breathBrightness += breathDirection;

    // Llegamos al máximo
    if (breathBrightness >= maxBrightness) {
      breathBrightness = maxBrightness;
      breathDirection = -1;
    }
    // Llegamos al mínimo → decidir próxima respiración
    else if (breathBrightness <= minBrightness) {
      breathBrightness = minBrightness;
      breathDirection = 1;

      // Decidir si la próxima será normal o anormal
      if (random(100) < probRespNormal) {
        esNormal = true;
      } else {
        esNormal = false;
      }
    }

    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB(r, g, b);
      leds[i].nscale8_video(breathBrightness);
    }

    FastLED.show();
  }
}

// ====== EFECTO VENAS ======
void efectoVenas(uint8_t r, uint8_t g, uint8_t b) {
  unsigned long ahora = millis();

  if (ahora - ultimoPulso > pulsoVelocidad) {
    ultimoPulso = ahora;

    pulsoBrillo += pulsoDireccionBrillo;
    if (pulsoBrillo >= maxBrightness) {
      pulsoBrillo = maxBrightness;
      pulsoDireccionBrillo = -1;
    }
    else if (pulsoBrillo <= minBrightness) {
      pulsoBrillo = minBrightness;
      pulsoDireccionBrillo = 1;
    }

    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeToBlackBy(40);
    }

    for (int i = 0; i < NUM_LEDS; i++) {
      if (leds[i].getAverageLight() < minBrightness) {
        leds[i] = CRGB(r, g, b);
        leds[i].nscale8_video(minBrightness);
      }
    }

    for (int i = 0; i < pulsoAncho; i++) {
      int ledIndex = (pulsoPos + i) % NUM_LEDS;
      leds[ledIndex] = CRGB(r, g, b);
      leds[ledIndex].nscale8_video(pulsoBrillo);
    }

    pulsoPos++;
    if (pulsoPos >= NUM_LEDS) {
      pulsoPos = 0;
    }

    FastLED.show();
  }
}

// ====== LOOP ======
void loop() {
  leerColorSerial();
  efectoRespiracion(baseR, baseG, baseB);
  //efectoVenas(baseR, baseG, baseB);
}
