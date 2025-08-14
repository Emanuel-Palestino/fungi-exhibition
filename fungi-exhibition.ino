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

// ====== CONTROL DE BRILLO ======
uint8_t minBrightness = 10;        // Brillo mínimo global
uint8_t maxBrightnessGlobal = 150; // Brillo máximo global
uint8_t targetMaxBrightness = 100; // Brillo máximo para el ciclo actual

// ====== CONTROL DE CICLOS ======
unsigned long breathStartTime = 0;
unsigned long breathDuration = 3000; 
bool inhaling = true;
int ciclosRestantesTipo = 0; 
bool enojoFaseProfunda = true; // Control de patrón en enojo

// ====== ESTADOS ======
enum Estado { TRISTEZA, NEUTRAL, ENOJO };
Estado estadoActual = NEUTRAL;

// ====== CONFIGURACIÓN DE TIPOS ======
struct TipoRespiracion {
  int duracionMin;
  int duracionMax;
  int brilloMin;
  int brilloMax;
  int ciclosMin;
  int ciclosMax;
};

TipoRespiracion tipoActual;

// ====== PERFILES SEGÚN ESTADO ======
TipoRespiracion normalNeutral   = {3000, 4500,  80, 110, 2, 4};
TipoRespiracion profundaNeutral = {4000, 6000, 120, 150, 2, 3};
TipoRespiracion cortaNeutral    = {2000, 3000,  60,  90, 3, 6};

// Tristeza → más lento, menos brillo
TipoRespiracion normalTriste   = {6000, 8000,  40,  80, 3, 5};
TipoRespiracion profundaTriste = {7000, 9000,  50,  90, 2, 4};
TipoRespiracion cortaTriste    = {5000, 6500,  35,  70, 3, 5};

// Enojo → más rápido y brillante
TipoRespiracion profundaEnojo = {2500, 3500, 130, 180, 1, 1}; // Siempre 1 ciclo profundo
TipoRespiracion cortaEnojo    = {1000, 1500,  90, 130, 3, 5}; // Varias cortas rápidas

// ====== PROBABILIDADES (solo para tristeza/neutral) ======
int probRespNormal   = 50;
int probRespProfunda = 30;
int probRespCorta    = 20;

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  randomSeed(analogRead(A0));
  elegirTipoRespiracion();
}

// ====== ELECCIÓN DE NUEVO TIPO ======
void elegirTipoRespiracion() {
  if (estadoActual == ENOJO) {
    // Patrón fijo: profunda primero
    tipoActual = profundaEnojo;
    ciclosRestantesTipo = 1;
    enojoFaseProfunda = true;
    elegirNuevoCiclo();
    return;
  }

  TipoRespiracion *opNormal;
  TipoRespiracion *opProfunda;
  TipoRespiracion *opCorta;

  switch (estadoActual) {
    case TRISTEZA:
      opNormal   = &normalTriste;
      opProfunda = &profundaTriste;
      opCorta    = &cortaTriste;
      break;
    default: // NEUTRAL
      opNormal   = &normalNeutral;
      opProfunda = &profundaNeutral;
      opCorta    = &cortaNeutral;
      break;
  }

  int tipo = random(100);
  if (tipo < probRespNormal) {
    tipoActual = *opNormal;
  } else if (tipo < probRespNormal + probRespProfunda) {
    tipoActual = *opProfunda;
  } else {
    tipoActual = *opCorta;
  }

  ciclosRestantesTipo = random(tipoActual.ciclosMin, tipoActual.ciclosMax + 1);
  elegirNuevoCiclo();
}

// ====== ELECCIÓN DE NUEVO CICLO ======
void elegirNuevoCiclo() {
  breathDuration = random(tipoActual.duracionMin, tipoActual.duracionMax);
  targetMaxBrightness = random(tipoActual.brilloMin, tipoActual.brilloMax + 1);
  targetMaxBrightness = constrain(targetMaxBrightness, minBrightness, maxBrightnessGlobal);
  breathStartTime = millis();
  inhaling = true;
}

// ====== LECTURA DE COLOR DESDE SERIAL ======
void leerColorSerial() {
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    data.trim(); 

    int firstComma = data.indexOf(',');
    int secondComma = data.indexOf(',', firstComma + 1);

    if (firstComma > 0 && secondComma > firstComma) {
      baseR = constrain(data.substring(0, firstComma).toInt(), 0, 255);
      baseG = constrain(data.substring(firstComma + 1, secondComma).toInt(), 0, 255);
      baseB = constrain(data.substring(secondComma + 1).toInt(), 0, 255);
    }
  }
}

// ====== EFECTO RESPIRACIÓN ======
void efectoRespiracion(uint8_t r, uint8_t g, uint8_t b, Estado estado) {
  if (estado != estadoActual) {
    estadoActual = estado;
    elegirTipoRespiracion();
  }

  unsigned long now = millis();
  float t = (now - breathStartTime) / (float)breathDuration;

  if (t >= 1.0) {
    inhaling = !inhaling;
    breathStartTime = now;
    t = 0;

    if (inhaling) {
      ciclosRestantesTipo--;

      if (estadoActual == ENOJO) {
        if (enojoFaseProfunda) {
          tipoActual = cortaEnojo;
          ciclosRestantesTipo = random(tipoActual.ciclosMin, tipoActual.ciclosMax + 1);
          enojoFaseProfunda = false;
          elegirNuevoCiclo();
        } else {
          if (ciclosRestantesTipo <= 0) {
            elegirTipoRespiracion(); // Reinicia patrón enojo
          } else {
            elegirNuevoCiclo();
          }
        }
      } else {
        if (ciclosRestantesTipo <= 0) {
          elegirTipoRespiracion();
        } else {
          elegirNuevoCiclo();
        }
      }
    }
  }

  float curve = (inhaling) ? sin(t * PI / 2) : cos(t * PI / 2);
  uint8_t brillo = map(curve * 100, 0, 100, minBrightness, targetMaxBrightness);

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(r, g, b);
    leds[i].nscale8_video(brillo);
  }
  FastLED.show();
}

// ====== LOOP ======
void loop() {
  leerColorSerial();

  // Ejemplo: cambiar el estado manualmente
  efectoRespiracion(baseR, baseG, baseB, ENOJO);
}
