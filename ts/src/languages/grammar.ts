// Words languages build their readings out of: the number engine never reads these, only the languages do

// Words read in place of the symbols between numbers.
export interface Joiners {
  readonly point: string; // 3.8.10: three point eight point ten
  readonly dot: string; // 10.0.0.51 (four parts): ten dot zero dot zero dot fifty-one
  readonly range: string; // 10-15, with its own spaces: " to ", " bis ", "-"
  readonly by: string; // 1920x1080
  readonly score: string; // 2:1, with its own spaces: " to "
  readonly minus: string; // -5 and 5 - 3
  readonly plus: string; // 2 + 2
  readonly fraction?: string; // 3 3/4, with its own spaces: " and ", " y "; a space if empty
  readonly half?: string; // 3 1/2 in place of the fraction, with its own spaces: " and a half"; unused if empty
  readonly rating?: string; // 4.5/5: four point five out of five; the slash stays if empty
}

// The forms of a noun after a number: one dollar, two dollars.
export interface Forms {
  readonly one: string;
  readonly many: string;
}

export const Case = {
  NOMINATIVE: 0,
  GENITIVE: 1,
  DATIVE: 2,
  ACCUSATIVE: 3,
  INSTRUMENTAL: 4,
  LOCATIVE: 5,
} as const;
export type Case = (typeof Case)[keyof typeof Case];

export const Gender = { MASCULINE: 0, FEMININE: 1, NEUTER: 2 } as const;
export type Gender = (typeof Gender)[keyof typeof Gender];

// The unit a number of a prime match counts: 1'27"23 counts a MINUTE and SECONDs, 5'9" FEET and INCHes
export const Prime = { HOUR: 0, MINUTE: 1, SECOND: 2, FOOT: 3, INCH: 4 } as const;
export type Prime = (typeof Prime)[keyof typeof Prime];
