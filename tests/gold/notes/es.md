# Spanish gold set: edits and unreachable cases

Every edit of `tests/gold/es.jsonl` that changes a NeMo case, with its reason. Cases with `"source": "verbalis"` are
written by the Spanish worker as a native-level speaker.

## All classes

- Seven readings joined by `~` (NeMo's separator of alternatives, which `build.py` kept in one string) are split into
  separate readings.

## Money

- `$1,50`, `1,50 $`, `€5,50`, `5,50 €`: added "un dólar con cincuenta", "cinco euros con cincuenta". "Con" before the
  cents, without naming them, is the common spoken price.
- `5,01 €`, `5,01 £`, `5,02 £`, `1,01 $`: added "cinco euros con un céntimo" and the like, the same convention with
  the cents named, since "con uno" alone is unclear.
- `21 nio`, `nio21`: "veintiuna córdobas" corrected to "veintiún córdobas". The córdoba is masculine (el córdoba).
- `¥12,05`, `¥ 0,01`: "doce yenes cinco centavos" and "un centavo" corrected to "doce coma cero cinco yenes" and
  "cero coma cero un yenes". The yen has no centavos.
- `5 us$`: added "cinco dólares", as US$ is read in running text.
- `q50,10`: added "cincuenta quetzales con diez".

## Measure

- `1 1/2 mph`, `1 1/2 m`: added "una milla y media por hora" and "un metro y medio", the usual order.
- `0,0101 s`: added "cero coma cero uno cero uno segundos". The gold apocope of the last digit ("cero un segundos")
  is one reading; the digits read as written are another.
- `2 + 2 - 1 = 3`: added "dos más dos menos uno igual a tres".

## Normalize with audio

- `1/2`: added "un medio", the bare fraction as said in arithmetic.

## Unreachable

- `3/2` (normalize_with_audio, "tres medios"): a bare day/month with a day after the month is read as a date (3/2:
  tres de febrero), as in "(15/06)" and "11/10 a 15/10"; an improper fraction without a unit or a "de" is rarer in
  running text than a date.
- `1 5/3` (normalize_with_audio, "uno y cinco tercios"): a mixed number with an improper fraction is malformed; the
  bare 5/3 reads as a date (cinco de marzo).
