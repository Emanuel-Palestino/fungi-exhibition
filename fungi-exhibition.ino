#include <FastLED.h>

// ====== CONFIGURACIÓN GENERAL ======
#define LED_PIN     6
#define NUM_LEDS    50
#define LED_TYPE    WS2812B // WS2815 se maneja igual que WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS  255

CRGB leds[NUM_LEDS];

// ====== COLOR BASE (usa valores RGB) ======
// Ejemplo: rojo = (255, 0, 0), azul = (0, 0, 255), blanco = (255, 255, 255)
uint8_t baseR = 138;   // Rojo
uint8_t baseG = 30;   // Verde
uint8_t baseB = 59; // Azul

// ====== PARÁMETROS DE RESPIRACIÓN ======
uint8_t breathBrightness = 0; 
int breathDirection = 1; 
unsigned long lastUpdate = 0; 
int breathSpeed = 5; // Velocidad de respiración
uint8_t minBrightness = 10;   // Brillo mínimo
uint8_t maxBrightness = 100;  // Brillo máximo

// ====== PARÁMETROS DE VENAS ======
int pulsoPos = 0;              // Posición actual del pulso
int pulsoAncho = 15;           // Ancho del pulso
int pulsoVelocidad = 60;       // Milisegundos entre pasos
uint8_t pulsoBrillo = minBrightness;
int pulsoDireccionBrillo = 1;  // 1 = sube, -1 = baja
unsigned long ultimoPulso = 0;

// ====== SETUP ======
void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  breathBrightness = minBrightness;
  pulsoBrillo = minBrightness;
}

// ====== EFECTO RESPIRACIÓN ======
void efectoRespiracion(uint8_t r, uint8_t g, uint8_t b) {
  unsigned long now = millis();

  if (now - lastUpdate > 15) {
    lastUpdate = now;
    
    breathBrightness += breathDirection;

    if (breathBrightness >= maxBrightness) {
      breathBrightness = maxBrightness;
      breathDirection = -1;
    } 
    else if (breathBrightness <= minBrightness) {
      breathBrightness = minBrightness;
      breathDirection = 1;
    }

    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB(r, g, b);
      leds[i].nscale8_video(breathBrightness);
    }

    FastLED.show();
  }
}

// ====== EFECTO VENAS (fondo tenue + rastro) ======
void efectoVenas(uint8_t r, uint8_t g, uint8_t b) {
  unsigned long ahora = millis();

  if (ahora - ultimoPulso > pulsoVelocidad) {
    ultimoPulso = ahora;

    // Ajustar brillo del pulso
    pulsoBrillo += pulsoDireccionBrillo;
    if (pulsoBrillo >= maxBrightness) {
      pulsoBrillo = maxBrightness;
      pulsoDireccionBrillo = -1;
    }
    else if (pulsoBrillo <= minBrightness) {
      pulsoBrillo = minBrightness;
      pulsoDireccionBrillo = 1;
    }

    // 1️⃣ Desvanecer el rastro
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeToBlackBy(40);
    }

    // 2️⃣ Fondo mínimo
    for (int i = 0; i < NUM_LEDS; i++) {
      if (leds[i].getAverageLight() < minBrightness) {
        leds[i] = CRGB(r, g, b);
        leds[i].nscale8_video(minBrightness);
      }
    }

    // 3️⃣ Pulso principal
    for (int i = 0; i < pulsoAncho; i++) {
      int ledIndex = (pulsoPos + i) % NUM_LEDS;
      leds[ledIndex] = CRGB(r, g, b);
      leds[ledIndex].nscale8_video(pulsoBrillo);
    }

    // Mover el pulso
    pulsoPos++;
    if (pulsoPos >= NUM_LEDS) {
      pulsoPos = 0;
    }

    FastLED.show();
  }
}

// ====== LOOP ======
void loop() {
  //efectoRespiracion(baseR, baseG, baseB);
  efectoVenas(baseR, baseG, baseB);
}
