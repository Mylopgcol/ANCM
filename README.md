Controlador ANC FxNLMS sobre STM32F407 con validación en lazo digital

Firmware en C y scripts de MATLAB del trabajo de grado Diseño e implementación de un controlador de cancelación activa de ruido basado en el algoritmo FxNLMS sobre un microcontrolador STM32, con validación en lazo digital en tiempo real.

Autor: Camilo Perdomo Gómez
Programa: Ingeniería Electrónica, Universidad de San Buenaventura Cali
Tutor: Carlos Mauricio Betancur Vargas
Año: 2026

Alcance

Este repositorio contiene la implementación de un controlador ANC feedforward monocanal (FxNLMS) que se ejecuta en tiempo real a 8 kHz sobre la tarjeta STM32F407G-DISC1. Las trayectorias acústicas primaria P(z) y secundaria S(z) se emulan como filtros FIR dentro del propio firmware (lazo digital), con cuantización de 12 bits y ruido ambiente en el micrófono de error.

Los resultados corresponden a un lazo digital y constituyen una cota superior del desempeño esperable con acústica real. 
Resultados principales (L = 32, μ = 0.05, fs = 8 kHz)
Escenario	Atenuación (dB)
Tono de 300 Hz	43.9
Tono de 500 Hz	43.3
Tono de 800 Hz	42.5
Tonos de 300 + 500 Hz	42.1
Barrido de 200 a 1000 Hz	28.0
Ruido blanco	38.9
500 Hz, Ŝ(z) con 3 muestras de retardo	43.1
500 Hz, Ŝ(z) con 4 muestras de retardo (90°)	−2.55

Error relativo de la identificación de Ŝ(z): 0.84 %.
Carga máxima de CPU: 25.9 % (5439 ciclos, 32.4 µs por muestra, compilación con -O2).
Diferencia máxima frente a la réplica de doble precisión en MATLAB: 0.25 dB.

Requisitos
Tarjeta STM32F407G-DISC1 y cable USB (ST-LINK integrado).
STM32CubeIDE (versión: 2.2.0) con la capa HAL de la familia F4.

Compilación y ejecución del firmware
Abrir el proyecto de firmware/ en STM32CubeIDE (File > Open Projects from File System).
Verificar la optimización del compilador en -O2 (Project > Properties > C/C++ Build > Settings). Con -O0 el costo por muestra se triplica.
Compilar y cargar a la tarjeta (Run > Debug).
El temporizador TIM2 (preescalador 83, periodo 124, reloj de 84 MHz) genera la interrupción de 8 kHz. Cada interrupción ejecuta un paso completo del lazo en HAL_TIM_PeriodElapsedCallback.
El botón USER enciende y apaga el ANC.
Las variables cyc_max, cyc_avg y las métricas de atenuación se leen con Live Expressions. Las señales d(n) y e(n) se exponen por DAC1 y DAC2 para observarlas con un osciloscopio.


Ejecución de los scripts de MATLAB
matlab
cd matlab
barrido_identificacion_Sz      % genera resultados/banco_pruebas_Sz.xlsx y las figuras
gemelo_doble_precision         % imprime la atenuación por escenario y la di

Los scripts usan semilla fija (rng). El STM32 y MATLAB generan el ruido ambiente con secuencias pseudoaleatorias distintas, por lo que una parte de la diferencia entre ambos proviene de la realización del ruido.

Cómo citar

C. Perdomo Gómez, “Diseño e implementación de un controlador de cancelación activa de ruido basado en el algoritmo FxNLMS sobre un microcontrolador STM32, con validación en lazo digital en tiempo real”, Trabajo de grado, Ingeniería Electrónica, Universidad de San Buenaventura Cali, 2026.
