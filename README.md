# Geant4 11.4.3 traditional-IRT chemistry: four developer reports

These are four reports on `G4DNAIRT` (traditional independent reaction times, Geant4-DNA chemistry), source at Geant4 commit `2ee379e6843e55b9c6bbb835d2a2b03cc998fef0`. Each has stock pins, a minimal reproducer, observed against expected results, and a proposed patch.

| Report | Summary |
|---|---|
| [Spatial bins retain expired track identities](geant4-11.4.3-irt-cache-developer-report.md) | A bin entry can be read after its track is destroyed, and its address reused by a later track. |
| [Encounter positions assigned by pointer order](geant4-11.4.3-irt-parent-order-developer-report.md) | `MakeReaction` computes positions in caller order but assigns them through pointer-ordered accessors. Includes a two-line fix. |
| [Near-stationary product placed outside the world](geant4-11.4.3-irt-stationary-developer-report.md) | A positive near-zero diffusion sentinel such as 1e-150 m²/s is treated as mobile, putting a product about 1e136 nm away. |
| [Undefined behaviour in `G4ManyFastLists::end()`](geant4-11.4.3-irt-manyfastlists-end-sentinel-developer-report.md) | `end()` dereferences the end iterator of its list of sublists; UBSan stops on it. |

**Files.** `attachments/` holds the reproducer archives, patches and helper headers named in each report. `SHA256SUMS` covers every file:

    shasum -a 256 -c SHA256SUMS

**Licence.** Our reproducers, patches and headers may be used under the [Geant4 Software License](https://geant4.web.cern.ch/download/license), so they can go straight into Geant4. Code copied from Geant4 remains under that licence.

**Who wrote this.** These reports were prepared by AI agents working at Agent Armies Labs, a small independent lab, and are filed by Joel Green, who is responsible for them. Every reproducer was rebuilt and run as described. Questions and corrections are welcome: open an issue here, or reply on the Geant4 Bugzilla entries that link to this repository.
