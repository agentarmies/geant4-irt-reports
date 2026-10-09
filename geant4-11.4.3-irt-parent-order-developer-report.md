# Traditional IRT assigns encounter positions by pointer order

Geant4 11.4.3 `G4DNAIRT::MakeReaction` calculates encounter positions
using caller A/B identity, then assigns them using pointer-ordered
`G4ITReactionChange::GetTrackA/B`. When those orders differ, positions
are assigned to the opposite labelled parents. Product placement then
reads the caller's parents again. This makes the position calculation
depend on addresses, rather than just molecular identities and the seed.

Source: Geant4 commit `2ee379e6843e55b9c6bbb835d2a2b03cc998fef0`.
`source/processes/electromagnetic/dna/models/src/G4DNAIRT.cc` has SHA256
`cdd2bec595262ac672f4d5d5f4b15b28d04f06d11579aca254692ca6e23b4686`.
Lines 400–445 compute `r1/r2` from caller `trackA/trackB` and `D1/D2`.
Lines 448–452 assign them through the reaction-change accessors.
Lines 470–483 use caller identities to place one, two or three products.
`G4ITReactionChange.hh:110` declares a `std::map<const G4Track*, …>`;
its `.cc:65–67` inserts both parents, and 119–148 returns its first and
second keys. The map does not preserve caller order.

Environment: Ubuntu 24.04.4 (glibc 2.39, x86_64); GCC 13.3.0; Geant4
11.4.3 built from source as Release with `GEANT4_BUILD_MULTITHREADED=ON`;
the CLHEP bundled with Geant4 (default engine `MixMaxRng`; version number
not recorded); data sets G4EMLOW 8.8, G4ENSDFSTATE 3.0 and
PhotonEvaporation 6.1.2. The test was built Release against that
installation and runs one thread.

The attached fixture links the unchanged installed class. Two
stack-owned tracks keep their labels while exercising both address
orders. It then performs the assignment/readback sequence with
illustrative positions 1 and 9 nm and square-root diffusion weights
3 and 1. Build and run it as described in the README:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$RAD_G4_PREFIX"
cmake --build build --parallel 1
./build/parent_order
```

Expected: each parent keeps the position calculated for it, so the
product coordinate does not depend on which parent has the lower address.

| Caller A's address | Returned parent IDs | Illustrative product coordinate |
| --- | --- | ---: |
| Lower | 11, 22 | 7 nm |
| Higher | 22, 11 | 3 nm |

The portable package was rebuilt and executed as distributed and returned
these results. This direct test performs no transport or chemistry; its
numbers illustrate the assignment, not a molecular displacement.

A separate bounded `MakeReaction` trace in `chem6` observed reversed
assignments in 24,430 of 46,091 calls. Of these, 17,986 had unequal
diffusion coefficients, and 10,366 changed the locally calculated product
position; the maximum local difference was about 0.71 nm. Four executions
of the unmodified shipped `chem6` binary with the same seed pair and
ASLR enabled produced three distinct G-value outputs. Two ASLR-disabled
executions were identical after verifying that address randomisation was
actually disabled. The trace establishes that the path is used; it does
not by itself attribute every full-trajectory divergence.

A controlled with/without comparison has since completed. Both builds
carried the other three changes from this set of reports and differed
only by the two-line repair below. Each ran the same 32 seeds with ASLR
enabled: `chem6` with 256 primaries, and three primaries in each of
five geometries of our plasmid application (four with DNA, one pure
water). The first primary's physical transport was identical in all 160
geometry pairs. Across 40 G(t) values (OH, e_aq, H2O2, H2 and H at eight
times from 1 ps to 1 µs), the largest mean paired difference was +0.0017
molecules/100 eV (OH at 500 ns; 95% CI +0.0004 to +0.0030), about 5% of
the unrepaired build's seed-to-seed SD of 0.032. Three pointwise
intervals excluded zero (OH at 100 and 500 ns, H2O2 at 100 ns); all 40
stayed within one SD. None of the 17 reaction and final-species count
intervals excluded zero, in `chem6` or in pure water. Both builds matched
the published IRT simulation cited below with a relative RMS error of
3.49%. DNA reactions were too rare in these three-primary runs to
estimate an effect: at most one of 32 seeds per DNA geometry had a
deoxyribose reaction, against a minimum of 8 that we fixed in advance.

The proposed two-line repair, `mapping-fix.patch`, assigns `r1/r2`
through `&trackA` and `&trackB`. It leaves the reaction-change object's
bookkeeping alone. An untested alternative is to make its accessors
preserve initial caller order; that requires auditing other users of
those accessors.

With the changes from all four of our reports combined in one build, four
same-seed `chem6` replays with address randomisation enabled were identical,
and the build's OH and e_aq G(t) matched the published IRT simulation of
Ramos-Méndez et al. (2020), doi:10.1002/mp.14490, with a relative RMS error
of 3.50%. Both curves stay systematically above that simulation at
1–100 ns (OH +2.4–3.5%, e_aq +3.3–6.3%); the cause has not been isolated.

Attachments:

- `geant4-irt-parent-order-reproducer-v1.tar.gz`, SHA256
  `ed83acb9970866ce8a96cf511020667e728015ee78398534ca398e57f4b26e8a`.
- `mapping-fix.patch`, SHA256
  `88d893f5751322b87300d04d7d796b423eab7fcf4eda6673eb41af9a79f2d00f`.
- The `beam-256.mac` used for the stock repeat check is in the cache
  reproducer package; SHA256
  `0a3e102eb16b186ecf98a104626bd69c5b8f2cbf3531c11191c9527c2bf1a7ca`.
