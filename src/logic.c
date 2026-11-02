/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Application logic: worker thread and parameter callbacks.
 *
 * Available resources:
 *  - GPIO (io.h): io_set_output(IO_MOTOR_1/IO_MOTOR_2/IO_PUMP/IO_HEATER, on),
 *    io_get_input(IO_LOADER/IO_SENSOR/IO_BUTTON), io_get_output().
 *  - Parameters (parameters.h, app_params.h): params_get()/params_set() by PARAM_* ID.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app_params.h"
#include "io.h"
#include "logic.h"
#include "parameters.h"

LOG_MODULE_REGISTER(logic, LOG_LEVEL_DBG);

/** @file logic.c
 *
 * Implementacja logiki sterowania podajnikami dwóch paliw na podstawie sygnału sterującego
 * podajnika pojedynczego.
 *
 * Sterownik wpięty jest za piecem, który obsługuje tylko jedno paliwo.
 * Piec podaje sygnał sterujący do jednego podajnika, a nasz sterownik rozdziela go
 * na dwa podajniki paliwa dając możliwość mieszania dwóch rodzajów paliwa.
 *
 * W początkowym etapie rozpalania pieca, tylko paliwo 1 jest podawane do pieca.
 * Może ono być podawane w innej proporcji niż to żąda piec ze względu na inną wydajność
 * podajnika paliwa 1 niż znamionowa wydajność, którą spodziewa się piec.
 * Następnie, podczas normalnej pracy, nasz sterownik reguluje proporcje podawania
 * paliwa 1 i 2 zgodnie z ustawionymi parametrami.
 *
 * Nasz sterownik odpowiada za optymalizację pracy pompy rozprowadzającej ciepło w instalacji.
 * Pompa ma zacząć pracować jak tylko zostanie wykryty sygnał wejściowy z podajnika.
 * Powinna wyłączyć się po określonym (w parametrze) czasie od ostatniej aktywności podajnika.
 */


#define STAN_INPUT_SENSOR (1 << 0)
#define STAN_INPUT_PODAJNIK (1 << 1)
#define STAN_INPUT_BUTTON (1 << 2)

#define STAN_OUTPUT_PODAJNIK_1 (1 << 3)
#define STAN_OUTPUT_PODAJNIK_2 (1 << 4)
#define STAN_OUTPUT_POMPA (1 << 5)
#define STAN_OUTPUT_NAGRZEWANIE (1 << 6)

#define STAN_POMPA_PODAJNIK (1 << 7)
#define STAN_POMPA_PRZEDLUZENIE (1 << 8)
#define STAN_POMPA_WYLACZONA (1 << 9)
#define STAN_POMPA (STAN_POMPA_PODAJNIK | STAN_POMPA_PRZEDLUZENIE | STAN_POMPA_WYLACZONA)

#define STAN_PODAJNIK_OCZEKIWANIE (1 << 10)
#define STAN_PODAJNIK_NASYP_ROZPALKI (1 << 11)
#define STAN_PODAJNIK_ROZPALANIE_TRWA (1 << 12)
#define STAN_PODAJNIK_PRACA_TRWA (1 << 13)
#define STAN_PODAJNIK (STAN_PODAJNIK_OCZEKIWANIE | STAN_PODAJNIK_NASYP_ROZPALKI | STAN_PODAJNIK_ROZPALANIE_TRWA | STAN_PODAJNIK_PRACA_TRWA)

#define STAN_DAWKOMIERZ_1_OCZEKIWANIE (1 << 14)
#define STAN_DAWKOMIERZ_1_PRZEKAZANIE_W_CALOSCI (1 << 15)
#define STAN_DAWKOMIERZ_1_POSZERZENIE_SYGNALU (1 << 16)
#define STAN_DAWKOMIERZ_1_PRZEKAZANIE_W_CZESCI (1 << 17)
#define STAN_DAWKOMIERZ_1_BLOKOWANIE_SYGNALU (1 << 18)
#define STAN_DAWKOMIERZ_1 (STAN_DAWKOMIERZ_1_OCZEKIWANIE | STAN_DAWKOMIERZ_1_PRZEKAZANIE_W_CALOSCI | STAN_DAWKOMIERZ_1_POSZERZENIE_SYGNALU | STAN_DAWKOMIERZ_1_PRZEKAZANIE_W_CZESCI | STAN_DAWKOMIERZ_1_BLOKOWANIE_SYGNALU)

#define STAN_DAWKOMIERZ_2_OCZEKIWANIE (1 << 19)
#define STAN_DAWKOMIERZ_2_PRZEKAZANIE_W_CALOSCI (1 << 20)
#define STAN_DAWKOMIERZ_2_POSZERZENIE_SYGNALU (1 << 21)
#define STAN_DAWKOMIERZ_2_PRZEKAZANIE_W_CZESCI (1 << 22)
#define STAN_DAWKOMIERZ_2_BLOKOWANIE_SYGNALU (1 << 23)
#define STAN_DAWKOMIERZ_2 (STAN_DAWKOMIERZ_2_OCZEKIWANIE | STAN_DAWKOMIERZ_2_PRZEKAZANIE_W_CALOSCI | STAN_DAWKOMIERZ_2_POSZERZENIE_SYGNALU | STAN_DAWKOMIERZ_2_PRZEKAZANIE_W_CZESCI | STAN_DAWKOMIERZ_2_BLOKOWANIE_SYGNALU)


static int podglad_stanu = 0;
static int now = 0;
static int param_minimalny_czas_pracy;
static int param_proporcja_rozpalki;
static int param_proporcja_paliwa_1;
static int param_proporcja_paliwa_2;
static int param_czas_rozpalania;
static int param_czas_wykrycia_nasypu_rozpalki;
static int param_czas_wykrycia_wygaszenia;
static int param_maksymalny_czas_przedluzenia_sygnalu;
static int param_czas_pracy_pompy;
static bool input_sensor;
static bool input_podajnik;
static bool input_button;
static bool output_podajnik_1;
static bool output_podajnik_2;
static bool output_pompa;
static bool output_nagrzewanie;

//--------------------------------------------------------------
#pragma region Start Procesowania


static void stan_grupy(int grupa, int stan_mask)
{
	podglad_stanu = (podglad_stanu & ~(grupa)) | (stan_mask);
}


static void stan_grupy_offset(int offset, int grupa, int stan_mask)
{
	grupa *= offset;
	stan_mask *= offset;
	podglad_stanu = (podglad_stanu & ~(grupa)) | (stan_mask);
}


static void stan(int stan_mask, bool on)
{
	if (on) {
		podglad_stanu |= stan_mask;
	} else {
		podglad_stanu &= ~stan_mask;
	}
}


static void start_procesowania(int aktualny_czas)
{
	now = aktualny_czas;
	param_minimalny_czas_pracy = TIME_FROM_MIN(params_get(PARAM_MINIMALNY_CZAS_PRACY));
	param_proporcja_rozpalki = params_get(PARAM_PROPORCJA_ROZPALKI);
	param_proporcja_paliwa_1 = params_get(PARAM_PROPORCJA_PALIWA_1);
	param_proporcja_paliwa_2 = params_get(PARAM_PROPORCJA_PALIWA_2);
	param_czas_rozpalania = TIME_FROM_MIN(params_get(PARAM_CZAS_ROZPALANIA));
	param_czas_wykrycia_nasypu_rozpalki = TIME_FROM_SEC(params_get(PARAM_CZAS_WYKRYCIA_NASYPU_ROZPALKI));
	param_czas_wykrycia_wygaszenia = TIME_FROM_MIN(params_get(PARAM_CZAS_WYKRYCIA_WYGASZENIA));
	param_maksymalny_czas_przedluzenia_sygnalu = TIME_FROM_SEC(params_get(PARAM_MAKSYMALNY_CZAS_PRZEDLUZENIA_SYGNALU));
	param_czas_pracy_pompy = TIME_FROM_MIN(params_get(PARAM_CZAS_PRACY_POMPY));
	input_sensor = io_get_input(IO_SENSOR);
	input_podajnik = io_get_input(IO_LOADER);
	input_button = io_get_input(IO_BUTTON);
	stan(STAN_INPUT_SENSOR, input_sensor);
	stan(STAN_INPUT_PODAJNIK, input_podajnik);
	stan(STAN_INPUT_BUTTON, input_button);
}


static void output(enum io_output id, bool on)
{
	switch (id) {
		case IO_MOTOR_1:
			output_podajnik_1 = on;
			stan(STAN_OUTPUT_PODAJNIK_1, on);
			break;
		case IO_MOTOR_2:
			output_podajnik_2 = on;
			stan(STAN_OUTPUT_PODAJNIK_2, on);
			break;
		case IO_PUMP:
			output_pompa = on;
			stan(STAN_OUTPUT_POMPA, on);
			break;
		case IO_HEATER:
			output_nagrzewanie = on;
			stan(STAN_OUTPUT_NAGRZEWANIE, on);
			break;
		default:
			break;
	}
}


#pragma endregion
//--------------------------------------------------------------


//--------------------------------------------------------------
#pragma region Sensor


static int poczatek_aktywnosci_sensora = -TIME_FROM_HOUR(24);


static void procesuj_sensor(bool wymuszenie_startu)
{
	static bool poprzedni_stan = false;
	static bool sensor_aktywny = false;

	if (!sensor_aktywny && ((input_sensor && !poprzedni_stan) || wymuszenie_startu)) {
		poczatek_aktywnosci_sensora = now;
	}
	poprzedni_stan = input_sensor;
	bool val = input_sensor || TIME_SINCE(poczatek_aktywnosci_sensora) < param_minimalny_czas_pracy;
	if (val != sensor_aktywny) {
		LOG_INF("Wyjście sensora %s", val ? "aktywne" : "nieaktywne");
	}
	sensor_aktywny = val;
	output(IO_HEATER, sensor_aktywny);
}


void param_write_wymuszona_praca(struct parameter *param, int32_t old_value)
{
	(void)(old_value);
	int x = now - param_minimalny_czas_pracy + TIME_FROM_HOUR(param->value);
	param->value = 0;
	int key = irq_lock();
	poczatek_aktywnosci_sensora = x;
	irq_unlock(key);
}


#pragma endregion
//--------------------------------------------------------------


//--------------------------------------------------------------
#pragma region Dawkomierz


struct Dawkomierz {
	void* stan;
	int poprzedni_okres;
	int czas_startu;
	int *procenty;
	int io_motor;
	int offset_grupy;
};


void dawkomierz_init(struct Dawkomierz *d, int *procenty, int io_motor, int poprzedni_okres, int offset_grupy)
{
	d->stan = NULL;
	d->poprzedni_okres = poprzedni_okres;
	d->czas_startu = -TIME_FROM_HOUR(24);
	d->procenty = procenty;
	d->io_motor = io_motor;
	d->offset_grupy = offset_grupy;
}

/** @brief Procesuje maszynę stanów dla jednego dawkomierza
 *
 * Na podstawie parametrów i sygnału wejściowego dawkuje odpowiednie proporcje
 * jednego rodzaju paliwa. Gdy więcej paliw jest mieszanych, wiele instancji
 * dawkomierza jest potrzebna.
 */
void dawkomierz_procesuj(struct Dawkomierz *d)
{
	int calkowity_czas;
	int procent = *(d->procenty);

	if (d->stan != NULL) {
		goto *d->stan;
	}

oczekiwanie: // Oczekiwanie na sygnał wejściowy

	stan_grupy_offset(d->offset_grupy, STAN_DAWKOMIERZ_1, STAN_DAWKOMIERZ_1_OCZEKIWANIE);
	output(d->io_motor, input_podajnik);

	// Jeżeli sygnał wejściowy jest aktywny, rozpoczynamy proces przekazywania sygnału do silnika
	// w całości lub części w zależności od procentu (częściowe przekazanie jest możliwe tylko jeśli
	// znamy czas poprzedniego okresu).
	if (input_podajnik) {
		d->czas_startu = now;
		if (procent >= 100 || d->poprzedni_okres == 0) {
			goto przekazanie_w_calosci;
		} else {
			goto przekazanie_w_czesci;
		}
	}

	d->stan = &&oczekiwanie;
	return;

przekazanie_w_calosci: // Przekazanie całego okresu do silnika

	stan_grupy_offset(d->offset_grupy, STAN_DAWKOMIERZ_1, STAN_DAWKOMIERZ_1_PRZEKAZANIE_W_CALOSCI);
	output(d->io_motor, true);

	// Jeżeli sygnał wejściowy przestał być aktywny, zapisujemy czas trwania okresu i przechodzimy do poszerzenia sygnału.
	if (!input_podajnik) {
		d->poprzedni_okres = TIME_SINCE(d->czas_startu);
		goto poszerzenie_sygnalu;
	}

	d->stan = &&przekazanie_w_calosci;
	return;

poszerzenie_sygnalu: // Poszerzenie sygnału wyjściowego o określony procent

	stan_grupy_offset(d->offset_grupy, STAN_DAWKOMIERZ_1, STAN_DAWKOMIERZ_1_POSZERZENIE_SYGNALU);
	output(d->io_motor, true);

	// Całkowity czas pracy silnika obliczony jest na podstawie okresu i zadanych procentów.
	calkowity_czas = (d->poprzedni_okres * procent + 50) / 100;
	if (calkowity_czas > d->poprzedni_okres + param_maksymalny_czas_przedluzenia_sygnalu) {
		calkowity_czas = d->poprzedni_okres + param_maksymalny_czas_przedluzenia_sygnalu;
	}

	// Jeżeli obliczy czas pracy silnika został osiągnięty, wracamy do oczekiwania.
	// Jeżeli sygnał wejściowy jest ponownie aktywny, też wracamy do oczekiwania, a stamtąd od razu
	// do kolejnego stanu.
	if (TIME_SINCE(d->czas_startu) > calkowity_czas || input_podajnik) {
		goto oczekiwanie;
	}

	d->stan = &&poszerzenie_sygnalu;
	return;

przekazanie_w_czesci: // Przekazanie początkowej części okresu sygnału do silnika

	stan_grupy_offset(d->offset_grupy, STAN_DAWKOMIERZ_1, STAN_DAWKOMIERZ_1_PRZEKAZANIE_W_CZESCI);
	output(d->io_motor, true);

	// Jeżeli sygnał przestał być aktywny za wcześnie, zapisujemy czas trwania okresu i wracamy do oczekiwania.
	if (!input_podajnik) {
		d->poprzedni_okres = TIME_SINCE(d->czas_startu);
		goto oczekiwanie;
	}

	// Całkowity czas pracy silnika obliczony jest na podstawie poprzedniego okresu i zadanych procentów.
	calkowity_czas = (d->poprzedni_okres * procent + 50) / 100;

	// Jak już osiągniemy obliczony czas i sygnał jest nadal aktywny, przechodzimy do jego blokowania.
	if (TIME_SINCE(d->czas_startu) > calkowity_czas) {
		goto blokowanie_sygnalu;
	}

	d->stan = &&przekazanie_w_czesci;
	return;

blokowanie_sygnalu: // Blokowanie sygnału wyjściowego do silnika po tym jak już część okresu została przekazana

	stan_grupy_offset(d->offset_grupy, STAN_DAWKOMIERZ_1, STAN_DAWKOMIERZ_1_BLOKOWANIE_SYGNALU);
	output(d->io_motor, false);

	// Jeżeli sygnał wejściowy przestał być aktywny, zapisujemy czas trwania okresu i wracamy do oczekiwania.
	if (!input_podajnik) {
		d->poprzedni_okres = TIME_SINCE(d->czas_startu);
		goto oczekiwanie;
	}

	// Obliczamy dopuszczalny czas blokady, który stanowi 3/2 poprzedniego okresu.
	calkowity_czas = (d->poprzedni_okres * 3 + 1) / 2;

	// Jeżeli zbyt długo blokujemy, przechodzimy do przekazania sygnału w całości.
	if (TIME_SINCE(d->czas_startu) > calkowity_czas) {
		goto przekazanie_w_calosci;
	}

	d->stan = &&blokowanie_sygnalu;
	return;
}


#pragma endregion
//--------------------------------------------------------------


//--------------------------------------------------------------
#pragma region Podajniki


/** @brief Główna maszyna stanów obsługująca podajnik i proces rozpalania.
 *
 * Słucha sygnału wejściowego z podajnika i wystawia sygnały sterujące dla podajników paliwa 1 i 2.
 *
 * Maszyna wykrywa nasyp rozpałki i wygaszenie podajnika i natychmiast przechodzi do odpowiedniego stanu.
 *
 * W stanie oczekiwania maszyna nic nie robi, czeka na aktywność podajnika.
 * W przypadku aktywności podajnika lub wykrycia nasypu rozpalki maszyna przechodzi do stanu nasyp_rozpalki,
 * gdzie przekazuje dalej sygnał do silnika paliwa 1. Po zakończeniu nasypu rozpałki maszyna przechodzi
 * do stanu rozpalania, gdzie następuje proces rozpalania pieca używając tylko paliwa 1 w ustalonym
 * parametrami stosunku do sygnału wejściowego. To trwa do momentu osiągnięcia odpowiedniego czasu rozpalania.
 * Następnie przechodzi do stanu normalnej pracy, gdzie paliwa 1 i 2 są mieszane
 * zgodnie z proporcjami ustawionymi w parametrach.
 */
static void procesuj_podajnik()
{
	static void *stan = NULL;
	static int czas_startu_rozpalania = -TIME_FROM_HOUR(24);
	static int czas_zmiany_stanu_podajnika = -TIME_FROM_HOUR(24);
	static bool podajnik_poprzednio_aktywny = false;
	static struct Dawkomierz dawkomierz_1;
	static struct Dawkomierz dawkomierz_2;

	// Wykrywanie zmiany stanu wejścia podajnika i czas ostatniej zmiany stanu
	if (podajnik_poprzednio_aktywny != input_podajnik) {
		czas_zmiany_stanu_podajnika = now;
		podajnik_poprzednio_aktywny = input_podajnik;
	}

	// Wykrywanie nasypu rozpalki: podajnik jest aktywny przez określony czas
	if (input_podajnik && TIME_SINCE(czas_zmiany_stanu_podajnika) > param_czas_wykrycia_nasypu_rozpalki) {
		if (stan != &&nasyp_rozpalki) {
			LOG_INF("Wykryto nasyp rozpalki");
		}
		goto nasyp_rozpalki;
	}

	// Wykrywanie wygaszenia pieca: podajnik nieaktywny przez określony czas
	if (!input_podajnik && TIME_SINCE(czas_zmiany_stanu_podajnika) > param_czas_wykrycia_wygaszenia) {
		if (stan != &&oczekiwanie) {
			LOG_INF("Wykryto wygaszenie podajnika");
		}
		goto oczekiwanie;
	}

	if (stan != NULL) {
		goto *stan;
	}

oczekiwanie: // Oczekiwanie na rozpoczęcie pracy

	stan_grupy(STAN_PODAJNIK, STAN_PODAJNIK_OCZEKIWANIE);
	stan_grupy(STAN_DAWKOMIERZ_1, 0);
	stan_grupy(STAN_DAWKOMIERZ_2, 0);
	output(IO_MOTOR_1, false);
	output(IO_MOTOR_2, false);

	if (input_podajnik) {
		procesuj_sensor(true);
		goto nasyp_rozpalki;
	}

	stan = &&oczekiwanie;
	return;

nasyp_rozpalki: // Nasypywanie rozpałki do czasu wyłączenia podajnika

	stan_grupy(STAN_PODAJNIK, STAN_PODAJNIK_NASYP_ROZPALKI);
	stan_grupy(STAN_DAWKOMIERZ_1, 0);
	stan_grupy(STAN_DAWKOMIERZ_2, 0);
	output(IO_MOTOR_1, true);
	output(IO_MOTOR_2, false);

	if (!input_podajnik) {
		LOG_INF("Rozpoczynam proces rozpalania");
		czas_startu_rozpalania = now;
		dawkomierz_init(&dawkomierz_1, &param_proporcja_rozpalki, IO_MOTOR_1, 0, 1);
		goto rozpalanie_trwa;
	}

	stan = &&nasyp_rozpalki;
	return;

rozpalanie_trwa: // Proces rozpalania trwa

	stan_grupy(STAN_PODAJNIK, STAN_PODAJNIK_ROZPALANIE_TRWA);
	stan_grupy(STAN_DAWKOMIERZ_2, 0);
	output(IO_MOTOR_2, false);

	dawkomierz_procesuj(&dawkomierz_1);

	if (!input_podajnik && TIME_SINCE(czas_startu_rozpalania) > param_czas_rozpalania) {
		LOG_INF("Rozpoczynam normalną pracę");
		output(IO_MOTOR_1, false);
		int poprzedni_okres = dawkomierz_1.poprzedni_okres;
		dawkomierz_init(&dawkomierz_1, &param_proporcja_paliwa_1, IO_MOTOR_1, poprzedni_okres, 1);
		dawkomierz_init(&dawkomierz_2, &param_proporcja_paliwa_2, IO_MOTOR_2, poprzedni_okres, STAN_DAWKOMIERZ_2 / STAN_DAWKOMIERZ_1);
		goto praca_trwa;
	}

	stan = &&rozpalanie_trwa;
	return;

praca_trwa: // Proces pracy trwa

	stan_grupy(STAN_PODAJNIK, STAN_PODAJNIK_PRACA_TRWA);
	dawkomierz_procesuj(&dawkomierz_1);
	dawkomierz_procesuj(&dawkomierz_2);

	stan = &&praca_trwa;
	return;
}


#pragma endregion
//--------------------------------------------------------------


//--------------------------------------------------------------
#pragma region Pompa


/** @brief Procesuje pracę pompy.
 *
 * Pompa działa jak wykryto aktywność podajnika i podtrzymuje działanie przez czas ustalony w parametrze.
 * Jeśli parametr ustawiony jest na zero, pompa działa cały czas.
 */
static void procesuj_pompe()
{
	static int ostatni_czas_aktywnosci = -TIME_FROM_HOUR(24);

	if (input_podajnik) {
		ostatni_czas_aktywnosci = now;
		output(IO_PUMP, true);
		stan_grupy(STAN_POMPA, STAN_POMPA_PODAJNIK);
	} else if (param_czas_pracy_pompy == 0 || TIME_SINCE(ostatni_czas_aktywnosci) < param_czas_pracy_pompy) {
		output(IO_PUMP, true);
		stan_grupy(STAN_POMPA, STAN_POMPA_PRZEDLUZENIE);
	} else {
		output(IO_PUMP, false);
		stan_grupy(STAN_POMPA, STAN_POMPA_WYLACZONA);
	}
}


#pragma endregion
//--------------------------------------------------------------


//--------------------------------------------------------------
#pragma region Przycisk


/** @brief Procesuje pracę przycisku.
 *
 * Przyciskiem można wymusić działanie podajnika 1 lub 2 (na przemian).
 */
static void procesuj_przycisk()
{
	static bool stan_poprzedni = false;
	static int numer_podajnika = 1;

	if (!stan_poprzedni && input_button) {
		if (numer_podajnika == 1) {
			numer_podajnika = 2;
		} else {
			numer_podajnika = 1;
		}
	} else if (stan_poprzedni && !input_button) {
		if (numer_podajnika == 1) {
			output(IO_MOTOR_1, false);
		} else {
			output(IO_MOTOR_2, false);
		}
	}

	if (input_button) {
		if (numer_podajnika == 1) {
			output(IO_MOTOR_1, true);
		} else {
			output(IO_MOTOR_2, true);
		}
	}

	stan_poprzedni = input_button;
}


#pragma endregion
//--------------------------------------------------------------


void procesuj(int aktualny_czas)
{
	static bool start = true;

	if (start) {
		LOG_INF("Rozpoczęto pracę sterownika");
		start = false;
		output(IO_MOTOR_1, false);
		output(IO_MOTOR_2, false);
		output(IO_PUMP, false);
		output(IO_HEATER, false);
	}

	start_procesowania(aktualny_czas);
	procesuj_sensor(false);
	procesuj_podajnik();
	procesuj_pompe();
	procesuj_przycisk();
	io_set_output(IO_MOTOR_1, output_podajnik_1);
	io_set_output(IO_MOTOR_2, output_podajnik_2);
	io_set_output(IO_PUMP, output_pompa);
	io_set_output(IO_HEATER, output_nagrzewanie);
	params_set(PARAM_PODGLAD_STANU, podglad_stanu);
}
