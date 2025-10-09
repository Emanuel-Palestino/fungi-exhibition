#include <FastLED.h>
#include "DFRobotDFPlayerMini.h"
#include <SoftwareSerial.h>

// ====== CONFIGURACIÓN GENERAL ======
#define LED_PIN     6
#define NUM_LEDS    480
#define LED_TYPE    WS2812B
#define COLOR_ORDER RGB
#define BRIGHTNESS  255
#define TRIG_PIN 9
#define ECHO_PIN 8

CRGB ledsDummy[1];

// ====== SENSOR DE DISTANCIA ======
long duracion;

// ====== DFPLAYER MINI ======
SoftwareSerial mySoftwareSerial(4, 5); // RX, TX (ajusta según tus pines)
DFRobotDFPlayerMini myDFPlayer;

// ====== COLORES ======
CRGB colorActual = CRGB(255, 255, 255);
CRGB coloresTristeza[] = {
  CRGB(0, 150, 255),
  CRGB(0, 60, 255),
  CRGB(50, 10, 255),
  CRGB(120, 1, 255),
};
int numColoresTristeza = sizeof(coloresTristeza) / sizeof(coloresTristeza[0]);

CRGB coloresNeutral[] = {
  CRGB(80, 255, 0),
  CRGB(40, 255, 0),
  CRGB(0, 255, 0),
  CRGB(0, 255, 8),
};
int numColoresNeutral = sizeof(coloresNeutral) / sizeof(coloresNeutral[0]);

CRGB coloresEnojo[] = {
  CRGB(255, 0, 0),
  CRGB(255, 30, 0),
  CRGB(255, 65, 0),
  CRGB(255, 100, 0),
};
int numColoresEnojo = sizeof(coloresEnojo) / sizeof(coloresEnojo[0]);

// ====== CONTROL DE BRILLO ======
uint8_t minBrightness = 0;
uint8_t maxBrightnessGlobal = 150;
uint8_t targetMaxBrightness = 100;

// ====== CONTROL DE CICLOS ======
unsigned long breathStartTime = 0;
unsigned long breathDuration = 3000;
bool inhaling = true;
int ciclosRestantesTipo = 0;
bool enojoFaseProfunda = true;

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
TipoRespiracion normalNeutral   = {1200, 1600,  80, 110, 2, 4};
TipoRespiracion profundaNeutral = {1800, 2500, 120, 150, 2, 3};
TipoRespiracion cortaNeutral    = {900, 1300,  60,  90, 3, 6};

TipoRespiracion normalTriste   = {1500, 1900,  10,  80, 3, 5};
TipoRespiracion profundaTriste = {2000, 3000,  0,  90, 2, 3};
TipoRespiracion cortaTriste    = {1000, 1400,  25,  70, 3, 5};

TipoRespiracion profundaEnojo = {1200, 1600, 100, 150, 1, 2};
TipoRespiracion cortaEnojo    = {300, 700,  80, 110, 5, 7};

// ====== PROBABILIDADES ======
int probRespNormal   = 50;
int probRespProfunda = 30;
int probRespCorta    = 20;

// ====== SETUP ======
void setup() {
  Serial.begin(115200);

  // LEDs
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(ledsDummy, NUM_LEDS);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setDither(true);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setTemperature(TypicalLEDStrip);
  FastLED.showColor(CRGB::Black);

  // Aleatorio
  randomSeed(analogRead(A0));
  elegirTipoRespiracion();

  // Sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // DFPlayer
  mySoftwareSerial.begin(9600);
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("DFPlayer no encontrado");
    while(true);
  }
  myDFPlayer.volume(20); // Volumen 0-30
  myDFPlayer.loop(1);    // Arranca con neutral en loop
  Serial.println("Reproduciendo NEUTRAL en loop");
}

// ====== CONTROL DE REPRODUCCIÓN ======
void checkAudioLoop() {
  // Procesar el buffer del DFPlayer constantemente para que los comandos se ejecuten
  if (myDFPlayer.available()) {
    int type = myDFPlayer.readType();
    // Puedes agregar aquí manejo de errores si lo necesitas
    if (type == DFPlayerError) {
      Serial.print("DFPlayer Error: ");
      Serial.println(myDFPlayer.readType());
    }
  }
}

// ====== SENSOR DE DISTANCIA ======
int getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duracion = pulseIn(ECHO_PIN, HIGH, 20000);
  if (duracion == 0) return -1;

  int dist = duracion * 0.034 / 2;
  return dist;
}

unsigned long ultimoCambio = 0;
unsigned long cooldown = 1000; // Cooldown reducido para respuesta más rápida

void checkSensor() {
  int d = getDistanceCM();
  unsigned long ahora = millis();
  
  // Si hay un objeto a 100cm o menos, cambiar a ENOJO
  if (d > 0 && d <= 100) {
    if (estadoActual != ENOJO && ahora - ultimoCambio > cooldown) {
      cambiarAEnojo();
      ultimoCambio = ahora;
    }
  }
  // Si no hay objeto cerca (o está fuera de rango), regresar a NEUTRAL
  else if (d > 100 || d == -1) {
    if (estadoActual == ENOJO && ahora - ultimoCambio > cooldown) {
      regresarANeutral();
      ultimoCambio = ahora;
    }
  }
}

// ====== CAMBIAR EMOCIÓN ======
void cambiarAEnojo() {
  estadoActual = ENOJO;
  Serial.println("Objeto detectado - Reproduciendo ENOJO en loop");
  
  myDFPlayer.loop(2);        // 002.mp3 en loop
  
  elegirTipoRespiracion();
}

void regresarANeutral() {
  estadoActual = NEUTRAL;
  Serial.println("Objeto alejado - Reproduciendo NEUTRAL en loop");
  
  myDFPlayer.loop(1);        // 001.mp3 en loop
  
  elegirTipoRespiracion();
}

// ====== ELECCIÓN DE NUEVO TIPO ======
void elegirTipoRespiracion() {
  if (estadoActual == ENOJO) {
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
  colorActual = getColorByEstado(estadoActual);
}

// ====== OBTENER COLOR ======
CRGB getColorByEstado(Estado estado) {
  switch (estado) {
    case TRISTEZA:
      return coloresTristeza[random(numColoresTristeza)];
    case ENOJO:
      return coloresEnojo[random(numColoresEnojo)];
    default:
      return coloresNeutral[random(numColoresNeutral)];
  }
}

// ====== EFECTO RESPIRACIÓN ======
void efectoRespiracion(Estado estado) {
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
            elegirTipoRespiracion();
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

  CRGB c = colorActual;
  c.nscale8_video(brillo);
  FastLED.showColor(c);
}

// ====== LOOP ======
void loop() {
  checkSensor();
  efectoRespiracion(estadoActual);
  checkAudioLoop();
}
