# Italian gold set: edits to the NeMo cases, and unreachable cases (2026-10-27)

## Edits to NeMo cases

- All NeMo time cases: the alternatives were one string joined by `~` (`dodici e trenta minuti~dodici e mezza`); they
  are now separate readings.
- `33,32`, `23:45`, `3,23€`: NeMo drops the accent of compounds ending in -tre (`trentatre`); Italian writes
  `trentatré`, `ventitré`.
- `3453243534`: NeMo reads `tremila quattrocento cinquantatre milioni ...`; Italian says `tre miliardi
  quattrocentocinquantatré milioni ...`.
- `2,01 ₽`: NeMo reads `un copeca`, which mixes genders; corrected to `un copeco` (or `una copeca`), with and without
  `e`.
- `12:30`, `05:15`, `23:45`, `03:38`: added the reading without `minuti` (`dodici e trenta`), the most common one.
- `5 km/s`: added `al secondo`, the idiomatic form of `per secondo`.
- `3,23€`: added `tre euro e ventitré centesimi`, with the `e` Italian puts between euros and cents.
- `4,2 £`: added `quattro virgola due sterline`. Prices have two decimals; one decimal reads as a decimal number, as
  in `41,2 €/MWh`.
- Web addresses: added readings without the scheme (`https://` is not read aloud), with path segments and digits as
  Verbalis reads them (`abc`, `dodici df`, `punto py`), and `@wezyr1986` as `chiocciola wezyr
  millenovecentottantasei`.

## Unreachable cases

- `1034500` (cardinal): an ungrouped number of seven digits or more reads digit by digit. In Italian text, amounts
  that large are grouped (`1.034.500`); every ungrouped one in the parity baseline is a code or a phone number
  (`mercato 47944953`, `Tel. 081/ 8075411`).
- `3453243534` (cardinal): ten ungrouped digits starting with 3 are an Italian mobile number (`345 324 3534`); reading
  it as three billion would misread every phone number written without spaces.
- `abc.de!f@23d_f.sd.us` (electronic): not an address (`_` can't appear in a host name), so it isn't read as one.
- `www.nvidia.file&TN.com` (electronic): not an address (`&` can't appear in a host name); `&` reads `and` as in
  `AT&T`.

- `https://www.nvidia.it/abc/12df/like.py`: the accepted reading without the scheme and with spelled letters reads
  the digits of the address one by one ("uno due df"), as the gold reading does, instead of "dodici df".

- `https://www.nvidia.it/abc/12df/like.py`: the accepted reading without the scheme and with spelled letters reads
  the digits of the address one by one ("uno due df"), as the gold reading does, instead of "dodici df".
