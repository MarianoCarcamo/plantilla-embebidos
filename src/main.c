/* Copyright 2022, Laboratorio de Microprocesadores
 * Facultad de Ciencias Exactas y Tecnología
 * Universidad Nacional de Tucuman
 * http://www.microprocesadores.unt.edu.ar/
 * Copyright 2022, Esteban Volentini <evolentini@herrera.unt.edu.ar>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/** \brief Simple sample of use LPC HAL gpio functions
 **
 ** \addtogroup samples Sample projects
 ** \brief Sample projects to use as a starting point
 ** @{ */

/* === Headers files inclusions =============================================================== */

#include "bsp.h"
#include "reloj.h"
#include <stdbool.h>
#include <stddef.h>

/* === Macros definitions ====================================================================== */

#ifndef TICS_POR_SEC
#define TICS_POR_SEC 1000
#endif

#ifndef PERIODO_PARPADEO
#define PERIODO_PARPADEO 200
#endif

/* === Private data type declarations ========================================================== */

typedef enum {
    SIN_CONFIGURAR,
    MOSTRANDO_HORA,
    AJUSTANDO_MINUTOS,
    AJUSTANDO_HORA,
    AJUSTANDO_MINUTOS_ALARMA,
    AJUSTANDO_HORA_ALARMA
} modo_t;

/* === Private variable declarations =========================================================== */

/* === Private function declarations =========================================================== */

void DisparoAlarma(clock_t reloj);

void CambiarModo(modo_t valor);

void IncrementarBCD(uint8_t numero[2], const uint8_t limite[2]);

void DecrementarBCD(uint8_t numero[2], const uint8_t limite[2]);

void EncenderPunto(int posicion);

void ApagarPunto(int posicion);

bool ContarSegundosMientras(int segundos, digital_input_t input);

void ActivarAlarma(clock_t reloj);

void DesactivarAlarma(clock_t reloj);
/* === Public variable definitions =============================================================
 */

static board_t board;

static clock_t reloj;

static modo_t modo;

static int current_tic_value;

/* === Private variable definitions ============================================================ */

static const uint8_t LIMITE_MINUTOS[] = {5, 9};

static const uint8_t LIMITE_HORAS[] = {2, 3};

/* === Private function implementation ========================================================= */

void DisparoAlarma(clock_t reloj) {
    DigitalOutputActivate(board->buzzer);
}

void CambiarModo(modo_t valor) {
    modo = valor;

    switch (modo) {
    case SIN_CONFIGURAR:
        DisplayBlinkDigits(board->display, 0, 3, PERIODO_PARPADEO);
        EncenderPunto(1);
        ApagarPunto(0);
        ApagarPunto(2);
        ApagarPunto(3);
        break;
    case MOSTRANDO_HORA:
        DisplayBlinkDigits(board->display, 0, 3, 0);
        ApagarPunto(0);
        ApagarPunto(1);
        ApagarPunto(2);
        ApagarPunto(3);
        break;
    case AJUSTANDO_MINUTOS:
        DisplayBlinkDigits(board->display, 2, 3, PERIODO_PARPADEO);
        EncenderPunto(1);
        ApagarPunto(0);
        ApagarPunto(2);
        ApagarPunto(3);
        break;
    case AJUSTANDO_HORA:
        DisplayBlinkDigits(board->display, 0, 1, PERIODO_PARPADEO);
        EncenderPunto(1);
        ApagarPunto(0);
        ApagarPunto(2);
        ApagarPunto(3);
        break;
    case AJUSTANDO_MINUTOS_ALARMA:
        DisplayBlinkDigits(board->display, 2, 3, PERIODO_PARPADEO);
        EncenderPunto(0);
        EncenderPunto(1);
        EncenderPunto(2);
        EncenderPunto(3);
        break;
    case AJUSTANDO_HORA_ALARMA:
        DisplayBlinkDigits(board->display, 0, 1, PERIODO_PARPADEO);
        EncenderPunto(0);
        EncenderPunto(1);
        EncenderPunto(2);
        EncenderPunto(3);
        break;
    default:
        break;
    }
}

void IncrementarBCD(uint8_t numero[2], const uint8_t limite[2]) {
    numero[1]++;
    if ((numero[0] >= limite[0]) && numero[1] > limite[1]) {
        numero[0] = 0;
        numero[1] = 0;
    }
    if (numero[1] > 9) {
        numero[1] = 0;
        numero[0]++;
    }
}

void DecrementarBCD(uint8_t numero[2], const uint8_t limite[2]) {
    numero[1]--;
    if (numero[1] > 9) {
        numero[1] = 9;
        numero[0]--;
    }
    if ((numero[0] >= limite[0]) && numero[1] >= limite[1]) {
        numero[0] = limite[0];
        numero[1] = limite[1];
    }
}

void EncenderPunto(int posicion) {
    if (!DisplayToggleDot(board->display, posicion)) {
        DisplayToggleDot(board->display, posicion);
    }
}

void ApagarPunto(int posicion) {
    if (DisplayToggleDot(board->display, posicion)) {
        DisplayToggleDot(board->display, posicion);
    }
}

bool ContarSegundosMientras(int segundos, digital_input_t input) {
    int current = current_tic_value;
    int count_second = 0;
    bool condicion = true;

    while (count_second < segundos && condicion) {
        if (current_tic_value == 0) {
            count_second++;
            while (current_tic_value == 0) {
                __asm("NOP");
            }
        }
        condicion = DigitalInputGetState(input);
    }
    while (current_tic_value < current && condicion) {
        condicion = DigitalInputGetState(input);
    }
    if (current == current_tic_value && count_second == segundos) {
        return true;
    } else {
        return false;
    }
}

void ActivarAlarma(clock_t reloj) {
    if (!AlarmToggel(reloj)) {
        AlarmToggel(reloj);
    }
    EncenderPunto(3);
}

void DesactivarAlarma(clock_t reloj) {
    if (AlarmToggel(reloj)) {
        AlarmToggel(reloj);
    }
    ApagarPunto(3);
}

/* === Public function implementation ========================================================= */

int main(void) {
    uint8_t entrada[4];

    reloj = ClockCreate(TICS_POR_SEC, DisparoAlarma);
    board = BoardCreate();

    SisTick_Init(TICS_POR_SEC);
    CambiarModo(SIN_CONFIGURAR);

    while (true) {
        if (DigitalInputHasActivated(board->accept)) {
            if (modo == MOSTRANDO_HORA) {
                ActivarAlarma(reloj);
            } else if (modo == AJUSTANDO_MINUTOS) {
                CambiarModo(AJUSTANDO_HORA);
            } else if (modo == AJUSTANDO_HORA) {
                ClockSetTime(reloj, entrada, sizeof(entrada));
                CambiarModo(MOSTRANDO_HORA);
            } else if (modo == AJUSTANDO_MINUTOS_ALARMA) {
                CambiarModo(AJUSTANDO_HORA_ALARMA);
            } else if (modo == AJUSTANDO_HORA_ALARMA) {
                ClockSetAlarm(reloj, entrada, sizeof(entrada));
                ActivarAlarma(reloj);
                CambiarModo(MOSTRANDO_HORA);
            }
        }

        if (DigitalInputHasActivated(board->cancel)) {
            if (ClockGetTime(reloj, entrada, sizeof(entrada)) && (modo != MOSTRANDO_HORA)) {
                CambiarModo(MOSTRANDO_HORA);
            } else if (modo == MOSTRANDO_HORA) {
                DesactivarAlarma(reloj);
            } else {
                CambiarModo(SIN_CONFIGURAR);
            }
        }

        if (ContarSegundosMientras(3, board->set_time)) {
            CambiarModo(AJUSTANDO_MINUTOS);
            ClockGetTime(reloj, entrada, sizeof(entrada));
            DisplayWriteBCD(board->display, entrada, sizeof(entrada));
        }

        if (ContarSegundosMientras(3, board->set_alarm) &&
            ClockGetTime(reloj, entrada, sizeof(entrada))) {

            CambiarModo(AJUSTANDO_MINUTOS_ALARMA);
            ClockGetAlarm(reloj, entrada, sizeof(entrada));
            DisplayWriteBCD(board->display, entrada, sizeof(entrada));
        }

        if (DigitalInputHasActivated(board->increment)) {
            if (modo == AJUSTANDO_MINUTOS) {
                IncrementarBCD(&entrada[2], LIMITE_MINUTOS);
                DisplayWriteBCD(board->display, entrada, sizeof(entrada));
            } else if (modo == AJUSTANDO_HORA) {
                IncrementarBCD(entrada, LIMITE_HORAS);
                DisplayWriteBCD(board->display, entrada, sizeof(entrada));
            } else if (modo == AJUSTANDO_MINUTOS_ALARMA) {
                IncrementarBCD(&entrada[2], LIMITE_MINUTOS);
                DisplayWriteBCD(board->display, entrada, sizeof(entrada));
            } else if (modo == AJUSTANDO_HORA_ALARMA) {
                IncrementarBCD(entrada, LIMITE_HORAS);
                DisplayWriteBCD(board->display, entrada, sizeof(entrada));
            }
        }

        if (DigitalInputHasActivated(board->decrement)) {
            if (modo == AJUSTANDO_MINUTOS) {
                DecrementarBCD(&entrada[2], LIMITE_MINUTOS);
                DisplayWriteBCD(board->display, entrada, sizeof(entrada));
            } else if (modo == AJUSTANDO_HORA) {
                DecrementarBCD(entrada, LIMITE_HORAS);
                DisplayWriteBCD(board->display, entrada, sizeof(entrada));
            } else if (modo == AJUSTANDO_MINUTOS_ALARMA) {
                DecrementarBCD(&entrada[2], LIMITE_MINUTOS);
                DisplayWriteBCD(board->display, entrada, sizeof(entrada));
            } else if (modo == AJUSTANDO_HORA_ALARMA) {
                DecrementarBCD(entrada, LIMITE_HORAS);
                DisplayWriteBCD(board->display, entrada, sizeof(entrada));
            }
        }

        for (int index = 0; index < 20; index++) {
            for (int delay = 0; delay < 25000; delay++) {
                __asm("NOP");
            }
        }
    }
}

void SysTick_Handler(void) {
    static const int half_sec = TICS_POR_SEC / 2;
    uint8_t hora[6];

    DisplayRefresh(board->display);
    current_tic_value = ClockTic(reloj);

    if (current_tic_value == half_sec || current_tic_value == 0) {
        if (modo <= MOSTRANDO_HORA) {
            ClockGetTime(reloj, hora, sizeof(hora));
            DisplayWriteBCD(board->display, hora, sizeof(hora));
            if (modo == MOSTRANDO_HORA)
                DisplayToggleDot(board->display, 1);
        }
    }
}

/* === End of documentation ==================================================================== */

/** @} End of module definition for doxygen */
