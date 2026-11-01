/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_PARAMS_H_
#define APP_PARAMS_H_

/*_Parameter_IDs_(indexes_in_params_table)._*/
enum app_param_id {

	/* Next read index */
	PARAM_NEXT_READ = 0,
	/* Minimalny czas pracy */
	PARAM_MINIMALNY_CZAS_PRACY = 1,
	/* Proporcja rozpałki (czystego paliwa 1) */
	PARAM_PROPORCJA_ROZPALKI = 2,
	/* Proporcja paliwa 1 */
	PARAM_PROPORCJA_PALIWA_1 = 3,
	/* Proporcja paliwa 2 */
	PARAM_PROPORCJA_PALIWA_2 = 4,
	/* Czas rozpalania (czystym paliwem 1) */
	PARAM_CZAS_ROZPALANIA = 5,
	/* Czas wykrycia nasypu rozpałki */
	PARAM_CZAS_WYKRYCIA_NASYPU_ROZPALKI = 6,
	/* Czas wykrycia wygaszenia */
	PARAM_CZAS_WYKRYCIA_WYGASZENIA = 7,
	/* Maksymalny czas przedłużenia sygnału */
	PARAM_MAKSYMALNY_CZAS_PRZEDLUZENIA_SYGNALU = 8,
	/* Czas pracy pompy */
	PARAM_CZAS_PRACY_POMPY = 9,
	/* Wymuszona praca */
	PARAM_WYMUSZONA_PRACA = 10,
	/* Podgląd stanu */
	PARAM_PODGLAD_STANU = 11,

	PARAM_COUNT
};

#endif /* APP_PARAMS_H_ */
