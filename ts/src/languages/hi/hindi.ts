import { DatePattern, SuffixPattern } from "../../numbers/patterns.ts";
import { Kind, type Match, type Numeral } from "../../numbers/match.ts";
import { codeValue, isSpokenDigits, value } from "../../numbers/scanning.ts";
import { join } from "../../text/scan.ts";
import { PatternLanguage } from "../language.ts";
import {
  CARDINALS,
  CHARACTERS,
  CURRENCIES,
  DATE_FORMAT,
  JOINERS,
  LARGE_CARDINALS,
  MONTHS,
  NOTATION,
  ORDINAL_ENDINGS,
  ORDINAL_SUFFIXES,
  ORDINALS_SPECIAL,
  SYMBOLS,
  TERMS,
} from "./constants.ts";

// ------------------------
// ----- Number words -----
// ------------------------

// Every number up to 99 is its own word; then "<digit> सौ", and the Indian scales (हज़ार, लाख, करोड़)
function cardinal(n: bigint): string {
  if (n < 100n) {
    return CARDINALS[Number(n)];
  }
  if (n < 1000n) {
    const rest = n % 100n;
    return `${CARDINALS[Number(n / 100n)]} सौ` + (rest ? ` ${cardinal(rest)}` : "");
  }

  // Hindi keeps the "एक" of every scale: एक हज़ार, एक लाख
  for (let i = LARGE_CARDINALS.length - 1; i >= 0; i--) {
    const [base, word] = LARGE_CARDINALS[i];
    if (n >= base) {
      const low = n % base;
      return `${cardinal(n / base)} ${word}` + (low ? ` ${cardinal(low)}` : "");
    }
  }
  return "";
}

function digits(text: string): string {
  return join([...text].map((c) => CARDINALS[Number(c)]));
}

// --------------------
// ----- Language -----
// --------------------

// Hindi reads its numbers without context so far.
// TODO: outdated - Hindi readings predate the context-aware design and have no reference verbalizer.
// Missing: Indian digit grouping (1,00,000), Devanagari digits, units and context (gender) of the counted noun.
class Hindi extends PatternLanguage {
  constructor() {
    super({ characters: CHARACTERS, terms: TERMS, symbols: SYMBOLS }, NOTATION, JOINERS);
    this.addPattern(new DatePattern(DATE_FORMAT));
    this.addPattern(new SuffixPattern(Kind.ORDINAL, ORDINAL_SUFFIXES));
  }

  protected override readNumber(number: Numeral, _match: Match): string {
    if (number.fraction === "" && isSpokenDigits(number, 7)) {
      return digits(number.integer);
    }

    // The fraction is read digit by digit, without its trailing zeros: 0.50 -> शून्य दशमलव पाँच
    const fraction = number.fraction.replace(/0+$/, "");
    const integer = cardinal(value(number));
    return fraction === "" ? integer : `${integer} दशमलव ${digits(fraction)}`;
  }

  protected override readCodeDigits(text: string): string {
    const n = codeValue(text, 6);
    return n !== undefined ? cardinal(n) : digits(text);
  }

  protected override readDate(match: Match): string | undefined {
    // सत्ताईस मार्च दो हज़ार छब्बीस
    return (
      `${cardinal(value(match.numbers[0]))} ${MONTHS[Number(value(match.numbers[1]))]} ` +
      cardinal(value(match.numbers[2]))
    );
  }

  protected override readFraction(match: Match): string | undefined {
    // Common fractions have their own nouns, others are "<a> बटा <b>"
    const a = value(match.numbers[0]);
    const b = value(match.numbers[1]);
    if (a === 1n && b === 2n) {
      return "आधा";
    }
    if (b === 3n || b === 4n) {
      return cardinal(a) + (b === 3n ? " तिहाई" : " चौथाई");
    }
    return `${cardinal(a)} बटा ${cardinal(b)}`;
  }

  protected override readOrdinal(match: Match): string | undefined {
    // The suffix gives the form: masculine (वाँ), feminine (वीं) or plural (वें)
    const form = ORDINAL_SUFFIXES.indexOf(match.suffix);
    const n = value(match.numbers[0]);

    // पहला, दूसरा, तीसरा, चौथा, छठा: irregular
    const special = n < 10n ? ORDINALS_SPECIAL.get(Number(n)) : undefined;
    if (special !== undefined) {
      return special[form];
    }
    return cardinal(n) + ORDINAL_ENDINGS[form];
  }

  protected override readMoney(number: Numeral, match: Match): string | undefined {
    return `${this.readNumber(number, match)} ${CURRENCIES[match.unit]}`;
  }

  protected override readPercent(number: Numeral, match: Match): string | undefined {
    return `${this.readNumber(number, match)} प्रतिशत`;
  }
}

let instance: Hindi | undefined;

export function language(): PatternLanguage {
  return (instance ??= new Hindi());
}
