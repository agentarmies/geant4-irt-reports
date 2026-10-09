# Traditional IRT spatial bins retain expired track identities

In Geant4 11.4.3, traditional IRT can examine a spatial-bin entry after
the track originally inserted there has been destroyed. A later track
can also occupy the same address. This occurs in pure-water `chem6`,
without DNA or a user action that removes molecules.

Source: Geant4 commit `2ee379e6843e55b9c6bbb835d2a2b03cc998fef0`,
`source/processes/electromagnetic/dna/models/src/G4DNAIRT.cc`, SHA256
`cdd2bec595262ac672f4d5d5f4b15b28d04f06d11579aca254692ca6e23b4686`.
`IRTSampling` inserts pointers at line 169 and `MakeReaction` inserts
products at 501. `Sampling` copies a bin and checks its entries at
200–205. The retained entry carries no lifetime identity. Checking the
current track status or molecule at that address cannot establish that
it is the molecule originally inserted. Geant4's pooled allocation means
that an ordinary address-sanitizer pass is not an absence test for reuse.

Environment: Ubuntu 24.04.4 (glibc 2.39, x86_64); GCC 13.3.0; Geant4
11.4.3 built from source as Release with `GEANT4_BUILD_MULTITHREADED=ON`;
the CLHEP bundled with Geant4 (default engine `MixMaxRng`; version number
not recorded); data sets G4EMLOW 8.8, G4ENSDFSTATE 3.0 and
PhotonEvaporation 6.1.2. The package's executables were built Release
against that installation, and every run used one thread.

The attached package builds `chem6` with observation at that lookup;
the installed Geant4 library stays unchanged. Follow its README, with
`RAD_G4_PREFIX` naming a Geant4 11.4.3 installation and its data environment
loaded. Run `beam-one-primary.mac` from an empty output directory. The
macro transports a 999.999 keV electron, uses physics option 2, chemistry
option 3, traditional IRT, Meesungnoen2002 solvation, and a 1 µs end time.
Seeds are 2026140001 and 101; the example runs one thread.

The observer records a track ID and an independent generation at each
insertion. Track-owned auxiliary information removes its live registry
entry on destruction. Classification checks that registry before touching
a cached address. Counts are repeated partner checks, not distinct tracks.
Expected: every entry a partner check reads is the molecule inserted
there, so both right-hand columns are zero. Rebuilding and executing the
package as distributed gave:

| One-primary executable | Partner checks | Absent insertion identity | Reused address |
| --- | ---: | ---: | ---: |
| Unmodified lookup | 85,478 | 4,679 | 2,404 |
| ID+generation guard | 85,088 | 4,356 | 2,367 |

The guarded executable skipped all 6,723 invalid checks. Both packaged
executables also completed the supplied 256-primary macro. Exact counts
can depend on process layout; the structural witness is the presence of
expired or replaced identities in a bin.

An eight-seed preliminary comparison and a separate 32-seed comparison
measured reaction counts, final species, and G(t). In the 32-seed
comparison all five species' G(t) intervals stayed within the unmodified
build's seed-to-seed standard deviation. Both builds matched 12 digitised
OH/e_aq points from the **published IRT simulation** in Ramos-Méndez et al.
(2020), doi:10.1002/mp.14490, with pooled relative RMS errors of 3.5614%
and 3.5637%. One rare reaction count with zero variance in the unmodified
build prevented an overall equivalence claim. Both builds kept the
separately reported parent-order defect; these results do not validate
every trajectory or a combined repair.

With the changes from all four of our reports combined in one build, four
same-seed `chem6` replays with address randomisation enabled were identical,
and the build's OH and e_aq G(t) matched the published IRT simulation of
Ramos-Méndez et al. (2020), doi:10.1002/mp.14490, with a relative RMS error
of 3.50%. Both curves stay systematically above that simulation at
1–100 ns (OH +2.4–3.5%, e_aq +3.3–6.3%); the cause has not been isolated.

The tested guard validates track ID **and generation** before reading an
entry. In the package it is `native/BinImpact.hh` compiled into
`impact-G4DNAIRT.cc` with `-DRAD_BIN_IMPACT_SKIP=1`. A guard that checks
only the track ID fails a direct test in which a new track with the same ID
reuses the old track's storage. Untested alternatives include removing bin
entries on track deletion or using a library-managed lifetime handle. They
must handle address and ID reuse, including event boundaries.

Attachment: `geant4-irt-cache-reproducer-v2.tar.gz`, SHA256
`2bc85e638808d2577a0826a26c828e6bd18804c662452af00cac1cfd34188631`.
