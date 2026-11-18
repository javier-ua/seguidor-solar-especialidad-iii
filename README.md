# Sistema de seguimiento solar automático

Prototipo de seguidor solar de dos ejes con Arduino Uno, cuatro LDR y dos servomotores SG90.
Proyecto final de **Especialidad III** (Ingeniería Civil Informática, Universidad Autónoma de Chile) · Equipo E11 · Feria de Proyectos IoT 2026.

El panel compara la luz que reciben cuatro sensores separados por una cruz opaca y gira un paso a la vez hacia el lado más iluminado, hasta que las diferencias quedan dentro de una banda muerta (control incremental en lazo cerrado).

## Equipo

| Integrante | Rol |
|---|---|
| Pablo Allendes | Diseño mecánico e impresión 3D |
| Hans Osses | Sensores y calibración |
| Javier Rojas | Electrónica y montaje |
| Matías Soto | Firmware y control |

Docente: Juan Carlos Molina Duarte, PhD.

## Hardware

- Arduino Uno
- 4 LDR + 4 resistencias de 10 kΩ (divisores de tensión)
- Cruz opaca que separa los cuatro LDR
- 2 servos SG90: azimut (D9) y elevación (D10)
- Fuente externa de 5 V (≥ 2 A) para los servos, con GND común con el Arduino
- Condensador de 470–1000 µF junto a los servos

## Conexiones

| Señal | Pin |
|---|---|
| LDR superior izquierdo (SI) | A0 |
| LDR superior derecho (SD) | A1 |
| LDR inferior izquierdo (II) | A2 |
| LDR inferior derecho (ID) | A3 |
| Servo azimut | D9 |
| Servo elevación | D10 |

Cada LDR va del pin analógico a GND, y una resistencia de 10 kΩ va del pin a 5 V. Con más luz baja el voltaje, por eso el programa invierte la lectura (`1023 - ADC`).

## Cómo funciona el control

Cada 50 ms el programa:

1. Lee los 4 LDR (promedio de 4 muestras cada uno).
2. Calcula el error de cada eje:
   - horizontal: `dH = (SD + ID) - (SI + II)` (derecha menos izquierda)
   - vertical: `dV = (SI + SD) - (II + ID)` (arriba menos abajo)
3. Si `|error| > 30`, mueve ese servo 1° hacia el lado con más luz; si no, lo deja quieto.
4. Limita los ángulos a 0–180° (azimut) y 0–90° (elevación).

| Parámetro | Valor |
|---|---|
| Banda muerta | ±30 cuentas ADC |
| Paso | 1° por ciclo |
| Ciclo | 50 ms (hasta 20°/s por eje) |
| Posición inicial | azimut 90°, elevación 45° |

Si un eje gira al revés por cómo quedó montado, cambia `SENTIDO_AZ` o `SENTIDO_EL` a `-1`.

## Monitor serie (115200 baudios)

- `MODO_CSV = false` (por defecto): reporte legible cada 1 s con la luz de cada LDR, los errores, la posición y el estado (alineado o en movimiento).
- `MODO_CSV = true`: una línea por ciclo con `t_ms,dH,dV,az,el`. Copia la salida a un archivo `.csv` para graficar el error en el tiempo.

## Estructura

```
seguidor_solar/
  seguidor_solar.ino   programa del Arduino
```
