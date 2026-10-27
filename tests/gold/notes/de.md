# German gold set: edits and unreachable cases

Edits to `tests/gold/de.jsonl`, each with its reason. A reading is added to a case's `outputs` only where a native
speaker accepts it as equally correct.

- `1/2 ft`: "ein halber Fuß" added beside "ein halb Fuß"; before a noun the fraction declines like an adjective.

## Unreachable

These cases stay failing.

- The gold reading leaves the input as written, which a front end for speech can't do with digits: `5-3-Ws`,
  `401-ks`, `1,000,123`.
- A list of numbers joined by commas without spaces, which gold reads as decimals paired from the right ("eins , zwei
  komma drei"): `1,2,3`, `1,2,3,4`, `1,2,3,4,5`. The reading depends on what the list means.
- A URL with a query, read with its scheme and spelled letters: `https://www.abc.com/ab?=3-/abs/1`.
