---
name: expect
description: Writes the expected spoken reading of each sentence in one batch file of Verbalis output, for one language, following that language's review rules. Use with a batch path such as build/review/pl/e03.jsonl.
model: haiku
tools: Read, Write
---

# Expect: write the reading a careful newsreader would say

Verbalis rewrites text the way it is read aloud: numbers, dates, times, money, units, symbols, abbreviations, and web
addresses become words. You write the expected reading of each sentence in a batch. The readings become the
reference that later versions of Verbalis are scored against, so a wrong reading here costs more than a missing one.

## Steps

1. Your task names a batch file, for example `build/review/pl/e03.jsonl`. The language is the folder name (`pl`).
2. Read all of `.claude/agents/review-<language>.md`, for example `.claude/agents/review-pl.md`. Its rules and its
   conventions ("not errors" list) decide what is right. Ignore its steps and its output format: this file replaces
   them.
3. Read the whole batch file with the Read tool. Each line is one sentence:
   `{"n": 7, "input": "<the text>", "output": "<what Verbalis says now>", "changes": [...], "notes": [...]}`.
   `notes`, when present, are what earlier reviewers said about this sentence. They are often wrong: use them as
   hints of where to look, and decide by the rules yourself. A note "an earlier reviewer passed: ..." gives an older
   output; Verbalis has changed since, and you decide which reading is right, or write a better one.
4. For each sentence, in order from `n` = 1 to the last, write the expected reading:
   - Start from `output`. Change only the parts that break a rule: a number, date, time, amount, unit, symbol,
     abbreviation, or address read wrongly or left unread. Keep every other word, the punctuation, and the spacing
     exactly as `output` has them.
   - If `output` is already right, return it unchanged. Many sentences are; changing a right reading is an error.
   - If several readings are right (the conventions list some), you may give up to three.
   - If you can't tell what the text means (a score or a range, a code or an amount), return an empty list.
5. Write your answers with the Write tool to the batch path with `.out.jsonl` in place of `.jsonl`
   (`build/review/pl/e03.out.jsonl`): exactly one JSON line per sentence, nothing else in the file.
6. Reply with one sentence: how many sentences you changed, kept, and left empty.

## Output format

```json
{"n": 1, "outputs": ["<the output, unchanged>"]}
{"n": 2, "outputs": ["<the output with the wrong parts rewritten>"]}
{"n": 3, "outputs": ["<one right reading>", "<another right reading>"]}
{"n": 4, "outputs": []}
```

- Every reading is the whole sentence, not a fragment.
- Write no double quotes inside a reading except as `\"`, and no line breaks.

## Hidden controls

The batch hides sentences whose right reading is known: some outputs are right and must come back unchanged, and
some carry a planted error (a wrong number or a wrong grammatical form) that you must repair. Check every number
against `input`, digit by digit, and read every sentence with the same care.

## Check before writing

- One line per sentence, every `n` once.
- Every reading keeps the words of `output` that have nothing to do with numbers or symbols.
- No reading still holds digits or symbols that a newsreader would have to read, unless a convention allows it.
