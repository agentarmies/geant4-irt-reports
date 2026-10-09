# Traditional IRT places a near-stationary product outside the world

Geant4 11.4.3 G4DNAIRT::MakeReaction treats a positive DNA diffusion sentinel
as mobile. With D=1e-150 m²/s as caller A, the general placement formula can
put a product about 1e136 nm from the origin. The shipped dnadamage2 example
uses that coefficient for deoxyribose and its damaged products; its reaction
table includes deoxyribose with OH, H and the solvated electron.

Source: Geant4 commit `2ee379e6843e55b9c6bbb835d2a2b03cc998fef0`,
`source/processes/electromagnetic/dna/models/src/G4DNAIRT.cc`, SHA256
`cdd2bec595262ac672f4d5d5f4b15b28d04f06d11579aca254692ca6e23b4686`.
Lines 427–428 set s12=2D1dt and s22=2D2dt. The stationary branches at
429–430 test exact zero. Lines 433–435 use s12+s22*s22/s12 for the Gaussian
width, so a positive near-zero D1 produces a huge width. Lines 448–483
assign encounter and product positions. FindBin (344–352) clamps to edge
bins and therefore cannot establish that a molecular position is in bounds.

Environment: Ubuntu 24.04.4 (glibc 2.39, x86_64); GCC 13.3.0; Geant4
11.4.3 built from source as Release with `GEANT4_BUILD_MULTITHREADED=ON`;
the CLHEP bundled with Geant4 (default engine `MixMaxRng`; version number
not recorded); data sets G4EMLOW 8.8, G4ENSDFSTATE 3.0 and
PhotonEvaporation 6.1.2. The tests were built Release against that
installation and run one thread; the sanitizer builds below add
`-g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all`.

The attached fixture links the unchanged installed library and calls
MakeReaction directly in a ±1 µm water world, with 1 ns elapsed time and
seed 2026190001. The mobile coefficient is 4.9e-9 m²/s; the stationary
parent has zero or 1e-150 m²/s in both caller positions. One inert product
makes the position observable. Build and execute the four commands in its
README; no transport or damage scoring occurs. Expected: each 1e-150 m²/s
row matches the exactly-zero row for the same caller, with the product at
the stationary parent and inside the world.

| Stationary coefficient and caller | Product position (nm) | Raw world bound |
| --- | --- | --- |
| Exactly zero, A | (1, 2, 3) | Pass |
| Exactly zero, B | (3, 4, 5) | Pass |
| 1e-150 m²/s, A | approximately (−1.96e136, 2.60e135, −2.52e136) | Fail |
| 1e-150 m²/s, B | (3, 4, 5) | Pass |

The portable archive was independently rebuilt and run as distributed; all
four observations reproduce, including the caller-A nonzero exit from the
raw position assertion. The direct test establishes a reachable API defect;
it does not measure its frequency or its effect on a dose response.

A controlled with/without comparison has since completed. Both builds
carried the other three changes from this set of reports and differed
only by the floor below. Each ran the same 32 seeds with ASLR enabled:
`chem6` with 256 primaries, and three primaries in each of five
geometries of our plasmid application (four with DNA, one pure water).
`chem6` and pure water contain no near-stationary species, so the floor
never engaged there and water benchmarks cannot exercise it: all 40 G(t)
values and all 34 reaction and final-species counts were identical between
builds, and both matched the published IRT simulation cited below with a
relative RMS error of 3.75%. In the DNA geometries (384 primaries per
build), the floor replaced the general placement in three reactions, each
OH with deoxyribose as caller B, the order that stayed inside the world in
the direct test; no caller-A case occurred. That is too few to estimate an
effect on DNA reaction counts: at most one of 32 seeds per DNA geometry
engaged the floor, against a minimum of 8 that we fixed in advance.

The proposed local repair treats positive placement coefficients at or below
1e-100 m²/s as exactly zero inside MakeReaction. Molecular tables and IRT
reaction-time sampling remain unchanged. At that floor, the RMS displacement
in 20 µs is about 1.1e-43 nm. Because it skips the general-placement random
draws in either caller order, it changes the random-number sequence for
affected reactions, so results are not bit-identical to unpatched runs.
If both reactants are floored, the existing both-stationary branch (lines
466–468) returns without creating products, so the patch must be paired
with a check that no reaction-table entry pairs two such species. The
patch does not include that check. We ran it separately, before any
comparison, on the 16 reaction tables of our configurations: no entry pairs
two floored species, and the only entries with a floored reactant are
deoxyribose with OH, H and the solvated electron. Unchecked alternatives
are to make example sentinels exactly zero, or derive a numerically stable
limiting formula for positive D; both need a broader sampling audit. Eight direct tests of the
repair (zero and 1e-150 m²/s, both caller positions, two builds) completed
under ASan and UBSan with no suppressions. A deliberate 137-byte leak under
the same flags and options produced the expected LSan failure.

With the changes from all four of our reports combined in one build, four
same-seed `chem6` replays with address randomisation enabled were identical,
and the build's OH and e_aq G(t) matched the published IRT simulation of
Ramos-Méndez et al. (2020), doi:10.1002/mp.14490, with a relative RMS error
of 3.50%. Both curves stay systematically above that simulation at
1–100 ns (OH +2.4–3.5%, e_aq +3.3–6.3%); the cause has not been isolated.

One separate source question remains: the general branch supplies a
variance-like quantity to a Gaussian API whose second argument is a standard
deviation. No correction to that formula is included in this proposal.

Attachments:

- `geant4-irt-stationary-reproducer-v1.tar.gz`, SHA256
  `80ae21a1f5fcc1569fe9901e2d021601f32d9c0af6dfe44602833898e1a811f7`.
- `stationary-placement-proposal.patch`, SHA256
  `a25f0d662ba5137a882933c93475009f6e330ad67c0d38dedc6641f292145b74`.
