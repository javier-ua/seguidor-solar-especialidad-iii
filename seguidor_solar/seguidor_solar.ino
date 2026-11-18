/*
  Seguidor solar automático de dos ejes — Especialidad III
  Arduino Uno + 4 LDR + 2 servos SG90
  Control incremental con banda muerta (lazo cerrado)

  Orden LDR (vista frontal del panel):
    A0 = sup. izq   A1 = sup. der
    A2 = inf. izq   A3 = inf. der

  Monitor serie: 115200 baudios
  MODO_CSV = true  -> una linea por ciclo: t_ms,dH,dV,az,el (para graficar)
  MODO_CSV = false -> reporte legible cada 1 s
*/

#include <Servo.h>

// ---------------- Pines ----------------
const uint8_t PIN_LDR[4]   = {A0, A1, A2, A3};
const uint8_t PIN_SERVO_AZ = 9;
const uint8_t PIN_SERVO_EL = 10;

// ---------------- Control ----------------
const int BANDA_MUERTA      = 30;   // cuentas ADC (0-1023)
const int PASO              = 1;    // grados por ciclo
const unsigned long T_CICLO = 50;   // ms entre correcciones
const int SENTIDO_AZ        = +1;   // -1 si el montaje gira al revés
const int SENTIDO_EL        = +1;
const int AZ_MIN = 0, AZ_MAX = 180;
const int EL_MIN = 0, EL_MAX = 90;

// ---------------- Monitor serie ----------------
const unsigned long T_REPORTE = 1000;  // ms entre reportes
const bool MODO_CSV = false;           // true: salida CSV en cada ciclo

Servo servoAz, servoEl;
int az = 90, el = 45;                  // orientación inicial (grados)
unsigned long tCiclo = 0, tReporte = 0;
bool alineadoPrev = false;

// ---------- Utilidades de impresión ----------
void imprimirNum(long v, uint8_t ancho, bool conSigno) {
  char buf[12];
  if (conSigno) snprintf(buf, sizeof(buf), "%+ld", v);
  else          snprintf(buf, sizeof(buf), "%ld", v);
  for (int i = strlen(buf); i < ancho; i++) Serial.print(' ');
  Serial.print(buf);
}

void linea(char c) {
  for (int i = 0; i < 52; i++) Serial.print(c);
  Serial.println();
}

// Describe la acción de un eje
void imprimirEje(int d, int pos, int minPos, int maxPos,
                 const __FlashStringHelper* masLuz, const __FlashStringHelper* menosLuz) {
  imprimirNum(d, 5, true);
  if (abs(d) <= BANDA_MUERTA) {
    Serial.println(F("  -> equilibrado (en banda muerta)"));
    return;
  }
  bool tope = (pos == minPos || pos == maxPos);
  Serial.print(F("  -> mas luz "));
  Serial.print(d > 0 ? masLuz : menosLuz);
  Serial.println(tope ? F(" (TOPE alcanzado)") : F(", corrigiendo"));
}

void imprimirReporte(int luz[4], int dH, int dV, bool alineado) {
  Serial.print(F("[t = "));
  imprimirNum(millis() / 1000, 4, false);
  Serial.println(F(" s]"));

  Serial.println(F("  Luz medida (0 = oscuro, 1023 = max)"));
  Serial.print(F("     Sup. izq: ")); imprimirNum(luz[0], 4, false);
  Serial.print(F("    Sup. der: ")); imprimirNum(luz[1], 4, false); Serial.println();
  Serial.print(F("     Inf. izq: ")); imprimirNum(luz[2], 4, false);
  Serial.print(F("    Inf. der: ")); imprimirNum(luz[3], 4, false); Serial.println();

  Serial.println(F("  Diferencia de luz"));
  Serial.print(F("     Horizontal (der - izq): "));
  imprimirEje(dH, az, AZ_MIN, AZ_MAX, F("a la DERECHA"), F("a la IZQUIERDA"));
  Serial.print(F("     Vertical   (sup - inf): "));
  imprimirEje(dV, el, EL_MIN, EL_MAX, F("ARRIBA"), F("ABAJO"));

  Serial.print(F("  Panel  ->  Azimut: "));
  imprimirNum(az, 3, false);
  Serial.print(F(" grados   Elevacion: "));
  imprimirNum(el, 2, false);
  Serial.println(F(" grados"));

  Serial.print(F("  Estado ->  "));
  Serial.println(alineado ? F("ALINEADO con la fuente de luz") : F("EN MOVIMIENTO"));
  linea('-');
}

// ---------- Programa ----------
void leerLDR(int luz[4]) {
  for (int i = 0; i < 4; i++) {
    long s = 0;
    for (int k = 0; k < 4; k++) s += analogRead(PIN_LDR[i]);  // promedio 4 muestras
    // Divisor LDR a GND / 10 kΩ a 5 V: más luz -> menor voltaje.
    // Se invierte para que "luz" crezca con la iluminación.
    luz[i] = 1023 - (int)(s / 4);
  }
}

void setup() {
  Serial.begin(115200);
  servoAz.attach(PIN_SERVO_AZ);
  servoEl.attach(PIN_SERVO_EL);
  servoAz.write(az);
  servoEl.write(el);
  delay(500);

  if (MODO_CSV) {                        // encabezado para planilla
    Serial.println(F("t_ms,dH,dV,az,el"));
    return;
  }

  linea('=');
  Serial.println(F("   SEGUIDOR SOLAR DE DOS EJES  -  Especialidad III"));
  linea('=');
  Serial.print(F("  Banda muerta: ")); Serial.print(BANDA_MUERTA);
  Serial.print(F(" | Paso: "));        Serial.print(PASO);
  Serial.print(F(" grado | Ciclo: ")); Serial.print(T_CICLO);
  Serial.println(F(" ms"));
  Serial.print(F("  Posicion inicial -> Azimut: ")); Serial.print(az);
  Serial.print(F("   Elevacion: "));                  Serial.println(el);
  Serial.println(F("  Cambie la luz sobre los LDR para mover el panel."));
  linea('-');
}

void loop() {
  unsigned long ahora = millis();
  if (ahora - tCiclo < T_CICLO) return;
  tCiclo = ahora;

  // 1) Medir
  int luz[4];
  leerLDR(luz);

  // 2) Comparar: error diferencial por eje
  int dH = (luz[1] + luz[3]) - (luz[0] + luz[2]);   // derecha - izquierda
  int dV = (luz[0] + luz[1]) - (luz[2] + luz[3]);   // arriba - abajo

  // 3) Actuar: paso fijo solo fuera de la banda muerta
  if (abs(dH) > BANDA_MUERTA) az = constrain(az + SENTIDO_AZ * (dH > 0 ? PASO : -PASO), AZ_MIN, AZ_MAX);
  if (abs(dV) > BANDA_MUERTA) el = constrain(el + SENTIDO_EL * (dV > 0 ? PASO : -PASO), EL_MIN, EL_MAX);
  servoAz.write(az);
  servoEl.write(el);

  // 4a) Salida CSV: una linea por ciclo
  if (MODO_CSV) {
    Serial.print(ahora); Serial.print(',');
    Serial.print(dH);    Serial.print(',');
    Serial.print(dV);    Serial.print(',');
    Serial.print(az);    Serial.print(',');
    Serial.println(el);
    return;
  }

  // 4b) Reporte: periodico o inmediato si cambia el estado
  bool alineado = abs(dH) <= BANDA_MUERTA && abs(dV) <= BANDA_MUERTA;
  if (alineado != alineadoPrev || ahora - tReporte >= T_REPORTE) {
    if (alineado && !alineadoPrev) Serial.println(F(">>> Panel alineado: ambos ejes dentro de la banda muerta"));
    if (!alineado && alineadoPrev) Serial.println(F(">>> Cambio de luz detectado: reorientando panel"));
    imprimirReporte(luz, dH, dV, alineado);
    tReporte = ahora;
  }
  alineadoPrev = alineado;
}
