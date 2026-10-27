# Portuguese gold set: edits and unreachable cases

## Edits

- `nemo/fraction` `1/2`: added "meio". "Meio" is one half, as in "meio litro"; "um meio" is the arithmetic reading.
- `nemo/fraction` `1 1/2`: added "um e meio", the usual reading ("um e meio quilo"); "um e um meio" is rare.
- `nemo/telephone` `+55 (11) 3333-4444`: added the country code read as a number, "mais cinquenta e cinco", as it is
  usually said.
- `nemo/telephone` `(11) 3333-4444 ext. 12`: added the extension read as a number, "extensão doze".
- `nemo/telephone` `192.168.1.1`: added the parts of an IP address read as numbers, "cento e noventa e dois ponto
  cento e sessenta e oito ponto um ponto um", as network staff say them.
- `nemo/measure` `1 kg`, `5 kg` and `nemo/money` `$ 75,63`: split readings written as one string joined by "~"
  ("um quilo~um quilograma") into separate readings. `build.py` keeps NeMo's "~" inside the string; es, it, and en-us
  have such cases too.

- `nemo/decimal`: added two decimal digits read as one number ("um vírgula vinte e seis"), as Portuguese speakers
  usually say them; gold reads each digit ("um vírgula dois seis"): `0,10`, `0,11`, `0,15`, `0,20`, `0,25`,
  `0,50`, `0,99`, `1,10`, `1,15`, `1,20`, `1,26`, `1,33`, `1,50`, `-1,26`, `1,25 milhões`, `3,75 bilhões`, `3,14`,
  `12,27`, `87,69`.
- `nemo/electronic`: added a URL read without its scheme and with its names as words: `https://www.nvidia.com`,
  `http://site.com.br`, `https://www.amazon.com.br`.

## Unreachable

- `nemo/time` `14.30`: a bare dot between two numbers is a decimal as often as a time in Portuguese text (the
  baseline has `1.71`, `2.90`, and `31.44` next to `21.30`). Verbalis reads a dot time after an hour word (`às 21.30`,
  `pelas 21.30`) or before `h` (`21.30h`), and leaves a bare `14.30` as a decimal.
- `nemo/fraction` `47701/913`: an improper fraction with a five-digit numerator is a case or law number in running
  text (`Processo 1649/23`, `Decreto nº 913/2010`), never a fraction. Verbalis reads a slashed pair as a fraction only
  after "de", "da", or "do", or when it is proper with a small denominator (`3/8`, `5/13`).
