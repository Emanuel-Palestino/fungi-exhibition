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
uint8_t maxBrightness = 150;

// ====== CONTROL DE CICLOS ======
unsigned long breathStartTime = 0;
unsigned long breathDuration = 3000;
bool inhaling = true;

// ====== ESTADOS ======
enum Estado { TRISTEZA, NEUTRAL, ENOJO };
Estado estadoActual = NEUTRAL;

// ====== CONFIGURACIÓN DE DURACIÓN POR EMOCIÓN ======
// Duración en milisegundos de un ciclo completo (inhalar + exhalar)
unsigned long duracionNeutral = 3000;  // 3 segundos por ciclo
unsigned long duracionTristeza = 4000; // 4 segundos por ciclo
unsigned long duracionEnojo = 1000;    // 1 segundo por ciclo (respiración rápida)

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
  inicializarRespiracion();

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
  
  actualizarRespiracion();
}

void regresarANeutral() {
  estadoActual = NEUTRAL;
  Serial.println("Objeto alejado - Reproduciendo NEUTRAL en loop");
  
  myDFPlayer.loop(1);        // 001.mp3 en loop
  
  actualizarRespiracion();
}

// ====== INICIALIZACIÓN Y ACTUALIZACIÓN DE RESPIRACIÓN ======
void inicializarRespiracion() {
  actualizarRespiracion();
}

void actualizarRespiracion() {
  // Establecer duración según el estado actual
  switch (estadoActual) {
    case TRISTEZA:
      breathDuration = duracionTristeza;
      break;
    case ENOJO:
      breathDuration = duracionEnojo;
      break;
    default: // NEUTRAL
      breathDuration = duracionNeutral;
      break;
  }
  
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
    actualizarRespiracion();
  }

  unsigned long now = millis();
  float t = (now - breathStartTime) / (float)breathDuration;

  // Reiniciar el ciclo cuando se completa
  if (t >= 1.0) {
    inhaling = !inhaling;
    breathStartTime = now;
    t = 0;
  }

  // Curva de respiración suave
  float curve = (inhaling) ? sin(t * PI / 2) : cos(t * PI / 2);
  uint8_t brillo = map(curve * 100, 0, 100, minBrightness, maxBrightness);

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
