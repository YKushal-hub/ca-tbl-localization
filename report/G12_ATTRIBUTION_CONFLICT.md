# G12 is red, and it is a policy conflict rather than a defect

**Found** 2026-09-05 on the merged tree. Gate suite otherwise green: 11 of 13 pass, G11
red for the pre-existing analytic-reconciliation reason, and **G12 red because of the 21
commits this session added.**

## What G12 says

```
identity fields clean across 130 commits
ATTRIBUTION IN MESSAGE BODIES: 21 commit(s)
  remedy: this requires a history rewrite and is authorised only by the owner
```

The **identity fields are clean** — every commit is authored as
`Kushal Yalamanchi <kushalyalamanchi@gmail.com>`, matching the existing history. What G12
objects to is the `Co-Authored-By:` trailer in the message body.

## Why the conflict exists

The repository deliberately removed AI attribution from its history and gated against its
return. That is what commit `38faea0` ("record the attribution rewrite, add SHA_MAP.md, and
gate it with G12") did, and it is why 1 283 sidecars resolve through `SHA_MAP.md` rather
than through their original SHAs.

The agent working this session operates under a standing instruction to end commit messages
with a `Co-Authored-By` trailer. The two rules are directly opposed: following one breaks
the other, and no amount of care avoids it.

## Why nothing was rewritten

G12 states its own remedy: **"this requires a history rewrite and is authorised only by the
owner."**

A history rewrite over 21 commits carrying every result of this session is exactly the kind
of irreversible, owner-authorised operation an agent should not perform on its own
initiative — in a project whose evidence model rests on commit SHAs that sidecars cite.
Rewriting them would create a second `SHA_MAP` problem in order to solve the first.

So the gate is left red, honestly, with this note beside it.

## The options, for the owner

1. **Leave it.** G12 stays red and this file explains why. Provenance is intact and the
   trailers are true.

2. **Rewrite the 21 commits** to drop the trailers, then extend `SHA_MAP.md` the way the
   2026-09-01 rewrite was handled. No sidecar written this session cites a SHA that would
   move — the campaigns ran at `6d44895` and at branch commits predating the reports — but
   that must be verified per directory before anything is touched.

3. **Amend G12** to accept a `Co-Authored-By` trailer while continuing to reject
   attribution in identity fields and in prose. This is the smallest change and it keeps
   the property the gate exists to protect: that the work is not misattributed in the record
   a reader sees.

Option 3 is the recommendation. But it is a change to a gate, and gates in this project are
not edited to make them pass — so it needs the owner to decide that the gate was drawn
slightly wider than its purpose, rather than that this session is an exception to it.

## What did not change

Every other gate behaves as before. G12 tests commit metadata, not simulation output, so
this has no bearing on any result in `results/` or on any number in the reports written
this session.
