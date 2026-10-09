# Traditional IRT loops reach undefined behaviour in G4ManyFastLists::end()

In Geant4 11.4.3, `G4ManyFastLists<G4Track>::end()` dereferences the end
iterator of its list of sublists and calls a `G4FastList` member through
the resulting null pointer. That is undefined behaviour. Traditional IRT
reaches it in three main-list loops of `G4DNAIRT`: the no-reaction branch
of `Initialize`, `SpaceBinning` and `IRTSampling`. UBSan, set to halt on
error, stops at the first call. A Release build without sanitizers runs
on, and in our test it gives the expected output.

Source: Geant4 commit `2ee379e6843e55b9c6bbb835d2a2b03cc998fef0`.
`source/processes/electromagnetic/dna/management/include/G4ManyFastLists.hh`
has SHA256
`86d3e55d7ffe435d7eec6f4f8374451ac338f68b1981c9437413361a6fbf5442`;
`end()` is at line 418 and is also called from `begin()` at line 399.
`source/processes/electromagnetic/dna/models/src/G4DNAIRT.cc` (SHA256
`cdd2bec595262ac672f4d5d5f4b15b28d04f06d11579aca254692ca6e23b4686`) loops
over `fTrackHolder->GetMainList()` at line 131 (`Initialize`), 139–140
(`SpaceBinning`) and 163–164 (`IRTSampling`).

Environment: Ubuntu 24.04.4 (glibc 2.39, x86_64); GCC 13.3.0; Geant4
11.4.3 built from source as Release with `GEANT4_BUILD_MULTITHREADED=ON`;
the CLHEP bundled with Geant4 (default engine `MixMaxRng`; version number
not recorded); data sets G4EMLOW 8.8, G4ENSDFSTATE 3.0 and
PhotonEvaporation 6.1.2. The test was built Release and runs one thread.
Its sanitizer build adds
`-g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all`
and runs with `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1` and
`ASAN_OPTIONS=abort_on_error=1:detect_leaks=1:symbolize=1`. The installed
libraries are not instrumented; the `G4ManyFastLists` template code is
compiled into the test itself.

The attached test, `order_old.cc`, links the unchanged installed library
and copies the three loops from `G4DNAIRT.cc`. Its `SpaceBinning` matches
lines 138–159 byte for byte, apart from the class name. Its `IRTSampling`
copy carries one extra line from the cache guard in our separate report
(`IRTCacheGuard.hh`, attached), which does not touch the track lists. The
test builds real `G4TrackManyList` objects in 12 layouts (no sublists,
empty and occupied sublists, and sublists appended during `Sampling`) and
runs the three loops on each, with no transport or chemistry. Expected:
every track is visited without undefined behaviour. Observed with UBSan
(paths and addresses shortened; the raw output is attached):

```
G4ManyFastLists.hh:418:76: runtime error: member call on null pointer of type 'struct G4FastList'
    #0 in G4ManyFastLists<G4Track>::end() G4ManyFastLists.hh:418
    #1 in G4ManyFastLists<G4Track>::begin() G4ManyFastLists.hh:399
    #2 in FixtureIRT::SpaceBinning() order_old.cc:81
    #3 in main order_old.cc:152
```

`order_old.cc:81` is the copy of stock line 139. We declared this
diagnostic and stack as the expected failure before the run. Built
without sanitizers, the same test completes all 12 layouts.

The proposed patch, `irt-ordered-traversal-proposal.patch`, replaces the
three loops with `forEachIRTTrack` from `IRTOrderedTracks.hh`. The helper
walks the many-list's public sublists in order, and the ordinary
`G4TrackList` inside each, so it never forms the end sentinel. Loop bodies,
visit order and the insertion of each track into its bin before `Sampling`
are unchanged, and it adds no random draw. Sublists appended during the
walk are reached in the same pass; in stock code a first-order scavenger
reaction in `Sampling` can append an empty one. The visitor must not remove
or reorder sublists, or insert, remove or reorder tracks within an existing
sublist. The patch is written against our working copy, which already
carries the cache guard. It therefore also shows an `IRTCacheGuard.hh`
include and a `RadIRTCacheGuard::inserted` call in `IRTSampling`; neither
is in stock 11.4.3, and neither belongs to this fix. With the patch
(`order_fixed.cc`), all 12 layouts complete under ASan and UBSan with no
diagnostic, and the output is byte-identical to the unpatched Release
build's. In our full application builds, which also carry the other three
changes, adding the patch left the scientific output byte-identical in 40
three-primary water and plasmid-DNA runs and in four `chem6` replays. An
untested alternative is to fix `end()` itself so that it never
dereferences the end of its list of sublists.

With the changes from all four of our reports combined in one build, four
same-seed `chem6` replays with address randomisation enabled were identical,
and the build's OH and e_aq G(t) matched the published IRT simulation of
Ramos-Méndez et al. (2020), doi:10.1002/mp.14490, with a relative RMS error
of 3.50%. Both curves stay systematically above that simulation at
1–100 ns (OH +2.4–3.5%, e_aq +3.3–6.3%); the cause has not been isolated.

Attachments:

- `geant4-irt-manyfastlists-reproducer-v1.tar.gz`, SHA256
  `8551d335484296b74236b688d6246117b18d9b1e4ecdb09f89eaf7557d54353c`:
  both tests, both headers, the CMake file we used, the raw UBSan output,
  the Release output and our test record.
- `irt-ordered-traversal-proposal.patch`, SHA256
  `03d9986cfaf441d7c2d21148f6258291667434e7581143f259c1746eaa6bc127`.
- `IRTOrderedTracks.hh`, SHA256
  `82ee9278ad71c2ab21248e90ec59f14bb74eee879d96156935246f07b433330e`.
