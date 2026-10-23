# JetCalibTools

**Disclaimer: This README documents the NEW JetCalibTools, which JetEtMiss has not officially switched to yet. The JetCalibTools files currently in active use (ie. the implementation files in in Root/ and corresponding headers in JetCalibTools/) are NOT documented here**

`JetCalibTools` provides the machinery for applying calibrations to reconstructed jets. The calibration is composed of a set of independent **calibration steps**, each responsible for a specific correction or transformation. The calibration steps to include, and their configuration, is specified through a YAML configuration file.

#### Contact and Support

[atlas-cp-jetetmiss-jsv-conveners@cern.ch](mailto:atlas-cp-jetetmiss-jsv-conveners@cern.ch)

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [YAML configuration](#yaml-configuration-file)
3. [Calibration Steps](#calibration-steps)
4. [Example: Integrating into an Athena algorithm](#example-integrating-into-an-analysis-algorithm)
5. [For Developers:](#for-developers)
    - [How JetCalibTools Works](#how-jetcalibtools-works)
    - [Adding a New Calibration Step](#adding-a-new-calibration-step)
6. [Package Structure](#package-structure)

---

## Quick Start

JetCalibTools can be configured using a YAML configuration file and the helper function `calibToolFromConfigFile`.

```python
from JetCalibTools.JetCalibStepsConfig import calibToolFromConfigFile

# Inside a ComponentAccumulator setup function:
jetCalibTool = calibToolFromConfigFile(
    flags,
    configFile = "JetCalibTools/CalibConfigs/.../{config_file_name}.yaml",
    name       = "MyJetCalibTool",
)
```

See [Integrating into a Custom Algorithm](#integrating-into-a-custom-algorithm) for a more
complete example.

Central YAML config files for recommended calibrations are deployed to:

```
/cvmfs/atlas.cern.ch/repo/sw/database/GroupData/JetCalibTools/
```
You can view these files in a web-browser [here](https://atlas-groupdata.web.cern.ch/atlas-groupdata/JetCalibTools/).

When you provide a relative path, `PathResolver.FindCalibFile` locates the file automatically from the CVMFS area. Alternatively, an absolute path to a local YAML config file can be provided.

---

## YAML configuration file

A single YAML file defines the full calibration for a given jet collection and data-taking period. 
In addition to the central config files on CMVFS, examples can be found in `data/calibConfigExample.yaml` (for Small-R jets) and `data/calibConfigExample_largeR.yaml` (for Large-R jets). 

Each top-level block (except for `Sequence` and `Global`) corresponds to a calibration step.
See [Calibration Steps](#calibration-steps) for details of the name and options available for each step.

<details>
<summary>The ordering of the calibration steps for different sample types is determined by the Sequence block</summary>

```yaml
Sequence:
  Run2: 
    FullSim: [Residual, EtaJES, GSC, Smear, PtResidual, MC2MC]
    AF3: [Residual, EtaJES, GSC, Smear, AF3, PtResidual, MC2MC]
    Data: [Residual, EtaJES, GSC, Insitu]
  Run3: 
    FullSim: [Residual, EtaJES, GSC, Smear, PtResidual, MC2MC]
    AF3: [Residual, EtaJES, GSC, Smear, AF3, PtResidual, MC2MC]
    Data: [Residual, EtaJES, GSC, Insitu]
```
</details>
<br/>
<details>
<summary>
Run-specific settings can optionally be supplied using the Run2, Run3, Run4 sub-blocks. Any settings provided by such a sub-block will only be applied to samples for the corresponding run. 
</summary>

```yaml
EtaJES:
  ParametrizedVars:
    varE: "e"
    varEta: "DetectorEta"
  CalibConstantFile: JetCalibTools/CalibArea-00-04-82/CalibrationFactors/MC16a_MCJES_4EMPFlow_Oct2017.config
  Run2:
    UseSpline: False
  Run3:
    UseSpline: True # Note this is just an example, not a recommendation!
```
</details>

<br/>

Additionally, a top-level block `Global` can be used to set properties applied directly to the `JetCalibTool`. Note that no such properties currently exist.

---

## Calibration Steps

The following table lists all registered step names and their corresponding C++ classes. Further details on each step and the YAML options are available are also provided below.
 
| YAML key | C++ class | Notes |
|---|---|---|
| `EtaJES` | `EtaJESCalibStep` | Absolute JES + η correction from text or histogram file |
| `GSC` | `GSCCalibStep` | Global Sequential Calibration |
| `Insitu` | `InSituCalibStep` (+ optional `InSituJMSCalibStep`) | In-situ data/MC correction; skipped for MC by default |
| `JMS` | `JMSCalibStep` | MC jet mass scale calibration |
| `Smear` | `SmearingCalibStep` | JER smearing (MC only) |
| `JetArea` | `PileupAreaCalibStep` | Jet area four-vector subtraction. Note there is no Python/YAML configuration implemented, and this step is already folded into Residual below |
| `Residual` | `Pileup1DResidualCalibStep` | 1D residual pile-up (μ, NPV, or NJet) |
| `AF3` | `Generic4VecCorrectionStep` | ATLFAST3 fast-sim correction; skipped for data and full sim |
| `PtResidual` | `Generic4VecCorrectionStep` | Generic 4-vector correction |
| `MC2MC` | `Generic4VecCorrectionStep` | MC-generator-dependent correction; skipped for data and Pythia8 |

**Further details on each calibration step**

<details>
<summary>EtaJES — Absolute JES + η Calibration</summary>

**Source:** `src/EtaJESCalibStep.cxx`
 
**What it does:** Applies the absolute jet energy scale (JES) correction as a function of jet
energy and detector η, derived from MC simulation. It also applies a small η correction that
shifts the jet direction to better reflect the true particle-level η. Two implementations are
supported: a polynomial parametrisation read from a text file, and a P-spline (histogram)
representation, selected via `UseSpline`.
 
At low pT (below `MinPtForETAJES`) the calibration curve is extrapolated. At high energy
(above a per-η `EmaxJES` value, if `FreezeJEScorrectionatHighE` is set) the response is
frozen to avoid unphysical extrapolation.
 
**YAML options:**
 
| Key | Type | Default | Description |
|---|---|---|---|
| `OutScale` | string | `JetEtaJESScaleMomentum` | Output jet moment name |
| `ParametrizedVars.varE` | string | — | Variable used for the energy axis (e.g. `e`) |
| `ParametrizedVars.varEta` | string | — | Variable used for the η axis (e.g. `DetectorEta`) |
| `JetAlgo` | string | `AntiKt4EMPFlow` | Jet algorithm label used as key in the constants file |
| `CalibConstantFile` | string | — | Text file with JES and η-correction constants; also contains spline config when `UseSpline: true` |
| `HistoFile` | string | — | ROOT file with spline histogram constants (only used when `UseSpline: true`) |
| `UseSpline` | bool | `false` | Use spline (histogram) representation instead of polynomial parametrisation |
| `FreezeJEScorrectionatHighE` | bool | `false` | Freeze JES above the per-η `EmaxJES` value |
| `LowPtJESExtrapolationMethod` | int | `0` | `0` = freeze at min pT; `1` = linear extrapolation below min pT |
| `MinPtForETAJES` | float | `15` | Minimum pT (GeV) below which the calibration is extrapolated |
| `EtaBins` | list | 0.1-wide bins from −4.5 to +4.5 | Custom η bin edges |
 
**Expert options:**
 
| Key | Type | Default | Description |
|---|---|---|---|
| `UseSecondaryMinPtForETAJES` | bool | `false` | Use a different minimum pT for forward η |
| `EtaSecondaryMinPtForETAJES` | float | `1.9` | \|η\| threshold above which the secondary min pT applies |
| `SecondaryMinPtForETAJES` | float | `7.0` | Secondary minimum pT (GeV) for forward η |
| `MinPtForEtaCorr` | float | `8.0` | Minimum pT (GeV) for the η direction correction |
| `MaxEForEtaCorr` | float | `2500.0` | Maximum energy (GeV) for the η direction correction |
| `LowPtJESExtrapolationMinimumResponse` | float | `0.25` | Minimum allowed response value during low-pT extrapolation |
 
**Example YAML block:**
 
```yaml
EtaJES:
  OutScale: JetEtaJESScaleMomentum
  ParametrizedVars:
    varE:   "e"
    varEta: "DetectorEta"
  JetAlgo: AntiKt4EMPFlow
  CalibConstantFile: JetCalibTools/CalibArea-00-04-83/CalibrationFactors/AntiKt4EMPFlow_MC23a_PreRecR22_Phase2_JES_constants.config
  HistoFile: JetCalibTools/CalibArea-00-04-83/CalibrationFactors/AntiKt4EMPFlow_MC23a_PreRecR22_Phase2_JES_calibrationFactors.root
  UseSpline: true
  FreezeJEScorrectionatHighE: true
  LowPtJESExtrapolationMethod: 1
  MinPtForETAJES: 15
```
</details>

<details>
<summary>PileupArea — Jet Area Pile-up Subtraction</summary>

**Source:** `src/PileupAreaCalibStep.cxx`
 
**What it does:** Subtracts the average pile-up energy contribution using the jet catchment
area and the event-level transverse energy density ρ. Two modes are available: a full
four-vector subtraction (`p4_calib = p4 − ρ × A4vec`) or a pT-only scaling that protects
against negative pT/energy. A `PileupCorrected` integer decoration is set on each jet after
the correction is applied.
 
**YAML options:**
 
| Key | Type | Default | Description |
|---|---|---|---|
| `OutScale` | string | `JetAreaSubtractScaleMomentum` | Output jet moment name |
| `RhoKey` | string | `auto` | StoreGate key of the `xAOD::EventShape` containing ρ |
 
**Expert options:**
 
| Key | Type | Default | Description |
|---|---|---|---|
| `UseFull4vecArea` | bool | `false` | If `true`, subtract the full 4-vector `ρ·A`; if `false`, apply pT-only scaling with protection against negative pT |
 
**Example YAML block:**
 
```yaml
JetArea:
  OutScale: JetAreaSubtractScaleMomentum
  RhoKey:   Kt4EMPFlowEventShape
```
 </details>
 
 <details>
<summary>Residual — 1D Pile-up Residual Correction</summary>

**Source:** `src/Pileup1DResidualCalibStep.cxx`
 
**What it does:** Applies a residual pile-up correction parametrised linearly in μ and NPV
(or NJet), in bins of |η|. The correction is:
 
```
ΔpT = α(|η|) × (μ − μ_ref) + β(|η|) × (NPV − NPV_ref)
```
 
where α and β are piecewise-linear functions of |η| defined directly in the YAML. The
`DoJetArea` flag also allows the jet-area ρ subtraction to be included in this step instead
of as a separate `JetArea` step.
 
**YAML options:**
 
| Key | Type | Default | Description |
|---|---|---|---|
| `OutScale` | string | `JetPileupScaleMomentum` | Output jet moment name |
| `RhoKey` | string | `auto` | StoreGate key of the `xAOD::EventShape` containing ρ (used when `DoJetArea: true`) |
| `DefaultMuRef` | float | — | Reference μ value |
| `DefaultNPVRef` | float | — | Reference NPV value |
| `MuScaleFactor` | float | `1.0` | Scale factor applied to μ (MC only) |
| `ApplyNPVBeamspotCorrection` | bool | `false` | Apply NPV beamspot correction (MC only) |
| `AbsEtaBins` | list | — | Bin edges in \|η\| for the per-bin correction coefficients |
| `MuTerm` | list | — | Per-bin α coefficients (one per bin edge) |
| `NPVTerm` | list | — | Per-bin β coefficients (one per bin edge) |
| `DoJetArea` | bool | `false` | Also apply jet-area ρ subtraction within this step |
 
**Expert options:**
 
| Key | Type | Default | Description |
|---|---|---|---|
| `IsData` | bool | `false` | Set automatically from `flags.Input.isMC`; override only if needed |
| `averageInteractionsPerCrossingKey` | string | `EventInfo.averageInteractionsPerCrossing` | StoreGate decoration key for μ |
| `PrimaryVerticesContainerName` | string | `PrimaryVertices` | StoreGate key for the primary vertex container |
| `DoSequentialResidual` | bool | `false` | Apply μ and NPV terms sequentially rather than simultaneously |
| `OnlyResidual` | bool | `false` | Apply only the residual term, skipping the jet-area subtraction |
| `ApplyOnlyMuResidual` | bool | `false` | Apply only the μ-based term |
| `ApplyOnlyNPVResidual` | bool | `false` | Apply only the NPV-based term |
| `ApplyOnlyNJetResidual` | bool | `false` | Apply only the NJet-based term |
| `UseNjet` | bool | `false` | Use NJet instead of NPV |
| `DefaultNjetRef` | float | — | Reference NJet value (only when `UseNjet: true`) |
| `nJetContainerName` | string | — | Jet container for NJet counting (only when `UseNjet: true`) |
| `nJetThreshold` | int | `20` | pT threshold (GeV) for NJet counting |
| `nJetTerm` | list | — | Per-bin β coefficients when using NJet instead of NPV |
 
**Example YAML block:**
 
```yaml
Residual:
  OutScale: JetPileupScaleMomentum
  RhoKey:   Kt4EMPFlowNeutEventShape
  DefaultMuRef:  0
  DefaultNPVRef: 1
  MuScaleFactor: 1.00
  ApplyNPVBeamspotCorrection: false
  DoJetArea: true
  AbsEtaBins: [0.0, 0.1, 0.2, ...]
  MuTerm:    [-0.008, -0.059, ...]
  NPVTerm:   [0.024,  0.233, ...]
```
 </details>

<details>
<summary>GSC — Global Sequential Calibration</summary>

**Source:** `src/GSCCalibStep.cxx`
 
**What it does:** Reduces flavour dependence and improves jet energy resolution by applying
a sequence of multiplicative corrections based on jet sub-structure observables. Five
corrections are always applied in this order: charged fraction, Tile layer 0 energy fraction,
EM layer 3 energy fraction, track multiplicity (nTrk), and track width. An optional
punch-through correction based on muon segment counts can be enabled via `applyPunchThrough`.
 
Each correction uses a 2D histogram lookup as a function of jet pT and the relevant variable,
with separate histograms per η bin. The hard-scatter primary vertex index is determined from
the `PVIndex` decoration on `EventInfo` if available, otherwise from the vertex container.
For per-vertex reconstructed jets, the `OriginVertex` association is used instead.
 
The histogram reader tools are constructed automatically from `fileGSC` using the standard
naming convention, but the per-sub-correction histogram configuration can be overridden if
needed (see expert options).
 
**YAML options:**
 
| Key | Type | Default | Description |
|---|---|---|---|
| `OutScale` | string | `JetGSCScaleMomentum` | Output jet moment name |
| `fileGSC` | string | — | Calibration ROOT file (resolved via PathResolver) |
| `histTool_EM3` | dict | (auto) | Override config for the EM3 histogram readers; accepts `N_hist` + `histNameBase`, or a full list of reader configs |
| `histTool_CharFrac` | dict | (auto) | Override config for the charged-fraction histogram readers |
| `histTool_Tile0` | dict | (auto) | Override config for the Tile0 histogram readers |
| `histTool_nTrk` | dict | (auto) | Override config for the nTrk histogram readers |
| `histTool_trackWIDTH` | dict | (auto) | Override config for the track-width histogram readers |
| `applyPunchThrough` | bool | `false` | Enable the punch-through correction |
 
**Expert options:**
 
| Key | Type | Default | Description |
|---|---|---|---|
| `PunchThroughEtaBins` | list | — | η bin edges for the punch-through correction (required when `applyPunchThrough: true`) |
| `punchThroughMinPt` | float | `50000` | Minimum jet pT (MeV) for the punch-through correction |
| `histTool_PunchThrough` | dict | (auto) | Override config for the punch-through histogram readers |
| `VertexContainer` | string | `PrimaryVertices` | StoreGate key for the primary vertex container |
| `EventInfoKey` | string | `EventInfo` | StoreGate key for `EventInfo` |
 
**Example YAML block:**
 
```yaml
GSC:
  OutScale: JetGSCScaleMomentum
  fileGSC:  JetCalibTools/CalibArea-00-04-83/CalibrationFactors/[PLACEHOLDER]_GSC_calibrationFactors.root
  histTool_EM3:
    N_hist: 35
    histNameBase: "AntiKt4EMPFlow_EM3_interpolation_resp_eta"
  histTool_CharFrac:
    N_hist: 25
    histNameBase: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta"
  histTool_Tile0:
    N_hist: 18
    histNameBase: "AntiKt4EMPFlow_Tile0_interpolation_resp_eta"
  histTool_nTrk:
    N_hist: 25
    histNameBase: "AntiKt4EMPFlow_nTrk_interpolation_resp_eta"
  histTool_trackWIDTH:
    N_hist: 25
    histNameBase: "AntiKt4EMPFlow_trackWIDTH_interpolation_resp_eta"
```
 
 <details>
 <summary>   ↳ Minimal example that makes use of the histogram defaults:</summary>

 ```yaml
GSC:
  fileGSC: 'JetCalibTools/CalibArea-00-04-83/CalibrationFactors/AntiKt4EMPFlow_MC23a_PreRecR22_Phase2_GSC_calibrationFactors_240306.root'
  OutScale: "JetGSCScaleMomentum"
```
</details>

<details>
<summary>   ↳ Example setting up some custom variables: </summary>

```yaml
GSC_custom_vars:
  noRun: True
  fileGSC: 'JetCalibTools/CalibArea-00-04-83/CalibrationFactors/AntiKt4EMPFlow_MC23a_PreRecR22_Phase2_GSC_calibrationFactors_240306.root'
  histTool_EM3:
    N_hist: 35
    varX: "pt"
    varY:
      Name: "EM3"
      isJetVar: False
    histNameBase: "AntiKt4EMPFlow_EM3_interpolation_resp_eta"
  histTool_CharFrac:
    N_hist: 25
    histNameBase: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta"
    varX: "pt"
    varY: "ChargedFraction"
```
</details>

<details>
<summary>   ↳ If you are crazy you can full customise the list of histTools like this:</summary>

```yaml
GSC:
  noRun: True
  fileGSC: 'JetCalibTools/CalibArea-00-04-83/CalibrationFactors/AntiKt4EMPFlow_MC23a_PreRecR22_Phase2_GSC_calibrationFactors_240306.root'
  histTool_EM3:
    N_hist: 35
    varX: "pt"
    varY: 
      Name: "EM3"
      isJetVar: False
    histNameBase: "AntiKt4EMPFlow_EM3_interpolation_resp_eta"
    inputFile: '/cvmfs/atlas.cern.ch/repo/sw/database/GroupData/JetCalibTools/CalibArea-00-04-83/CalibrationFactors/AntiKt4EMPFlow_MC23a_PreRecR22_Phase2_GSC_calibrationFactors_240306.root'
  histTool_CharFrac:
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_0"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_1"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_2"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_3"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_4"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_5"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_6"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_7"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_8"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_9"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_10"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_11"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_12"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_13"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_14"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_15"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_16"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_17"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_18"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_19"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_20"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_21"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_22"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_23"
    - varX: "pt"
      varY:
        Name: "ChargedFraction"
        isJetVar: false
      histName: "AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_24"
```

</details>

</details>

<details>
<summary>Insitu — In-Situ Calibration</summary>

**Source:** `src/InSituCalibStep.cxx`, `src/InsituJMSCalibStep.cxx`
 
**What it does:** Corrects residual data/MC differences using in-situ measurements. The
correction is the product of an absolute scale factor (from Z/γ+jet or multijet balance, a 1D
function of pT) and a relative η-intercalibration factor (a 2D function of pT and η).
Time-dependence is handled by supplying one ROOT file per run period, selected at runtime
using the event run number.
 
By default this step is **skipped for MC**. It can be forced onto MC for uncertainty
evaluation by setting `CalibrateMC: true`.
 
An optional `JMS` sub-block configures an `InSituJMSCalibStep` that runs immediately after
the energy correction. It applies an in-situ jet mass scale correction, modifying only the
jet mass while keeping the pT fixed.
 
**YAML options:**
 
| Key | Type | Default | Description |
|---|---|---|---|
| `OutScale` | string | `JetInsituScaleMomentum` | Output jet moment name |
| `fileInsitu` | list of strings | — | One calibration ROOT file per run period, resolved via PathResolver |
| `RunNumbers` | list of int | — | Run number boundaries between periods; must have `len(fileInsitu) + 1` entries |
| `histEtaInterCalib` | dict | — | `HistoInput` config for the η-intercalibration 2D histogram; requires `histName`, `varX`, `varY` |
| `histAbsCalib` | dict | — | `HistoInput` config for the absolute 1D histogram; requires `histName`, `varX` |
| `CalibrateMC` | bool | `false` | If `true`, also apply the in-situ correction to MC |
| `JMS` | dict | — | If present, configures an `InSituJMSCalibStep`; requires `inputFile`, `histName`, `varX`, `varY` |
 
**Expert options:**
 
| Key | Type | Default | Description |
|---|---|---|---|
| `useOriginalHistCombination` | bool | `false` | Use the original method to pre-combine absolute and η-intercalibration histograms (reproduces legacy behaviour; for validation only) |
| `EventInfoKey` | string | `EventInfo` | StoreGate key for `EventInfo` |
 
**Example YAML block:**
 
```yaml
Insitu:
  OutScale: JetInsituScaleMomentum
  fileInsitu:
    - 'JetCalibTools/CalibArea-00-04-82/InsituCalibration/InsituCalibration_80ifb_1516_Nov_2018_4PF_Consolidated.root'
    - 'JetCalibTools/CalibArea-00-04-82/InsituCalibration/InsituCalibration_80ifb_17_Nov_2018_4PF_Consolidated.root'
  RunNumbers: [251102, 314199, 341649, 999999]
  histEtaInterCalib:
    histName: AntiKt4EMPFlow_EtaInterCalibration
    varX: pt
    varY: DetectorEta
  histAbsCalib:
    histName: AntiKt4EMPFlow_InsituCalib
    varX: pt
  CalibrateMC: false
  # Optional in-situ JMS (large-R jets only):
  # JMS:
  #   inputFile: "JetCalibTools/CalibArea-00-04-83/InsituCalibration/InsituJMSCalibration_140ifb_15161718_Sep_2024_10UFO_Precision.root"
  #   histName: "AntiKt10UFOCSSKSoftDropBeta100Zcut10_InsituCalibJMS_CaloMass"
  #   InterpType: "Full"
  #   varX: "pt"
  #   varY: "M"
```
</details>
 
<details>
<summary>JMS — Jet Mass Scale</summary>

**Source:** `src/JMSCalibStep.cxx`
 
**What it does:** Applies an MC-derived jet mass scale calibration, primarily for large-R
jets. The correction factor is read from a 3D histogram as a function of two configurable jet
variables plus η, and is applied to the jet mass. The pT is either kept fixed or recomputed
to preserve the energy, controlled by `pTfixed`. A protection against unphysical masses
(mass > energy) is applied. The correction is only applied to jets with `varX ≥ MinValueForJMS`
and `varZ` within the η range of the histogram.
 
**YAML options:**
 
| Key | Type | Default | Description |
|---|---|---|---|
| `OutScale` | string | `JetJMSScaleMomentum` | Output jet moment name |
| `HistoFile` | string | — | Calibration ROOT file (resolved via PathResolver) |
| `histoParams.histName` | string | — | Name of the 3D histogram in the ROOT file |
| `histoParams.varX` | string | — | Variable for histogram x-axis (e.g. `e`) |
| `histoParams.varY` | string | — | Variable for histogram y-axis (e.g. `LOGmOe`) |
| `histoParams.varZ` | string | — | Variable for histogram z-axis (η); also determines `maxEta` from the histogram boundary |
| `MinValueForJMS` | float | `180.0` | Minimum value of `varX` (GeV) below which no correction is applied |
 
**Expert options:**
 
| Key | Type | Default | Description |
|---|---|---|---|
| `pTfixed` | bool | `false` | If `true`, keep pT fixed and only vary mass; if `false`, recompute pT from the corrected mass and energy |
 
**Example YAML block:**
 
```yaml
JMS:
  OutScale: JetJMSScaleMomentum
  HistoFile: JetCalibTools/CalibArea-00-04-83/CalibrationFactors/AntiKt10UFOCSSKSoftDropBeta100Zcut10_JMSmCalo_calibFactors.root
  histoParams:
    histName: AntiKt10UFOCSSKSoftDropBeta100Zcut10_JMS_e_LOGmOe_eta
    varX: e
    varY: LOGmOe
    varZ: absDetEta
  MinValueForJMS: 180.
```
 </details>
 
 <details>

<summary>Smear — Jet Energy Resolution Smearing</summary>

**Source:** `src/SmearingCalibStep.cxx`
 
**What it does:** Smears the jet momentum to make the MC jet energy resolution (JER) match
that measured in data. The smearing factor is drawn from a Gaussian centred at 1 with width
σ\_smear = √(σ\_data² − σ\_MC²), applied only when σ\_data > σ\_MC (if σ\_MC > σ\_data no
smearing is applied, as this case is instead the source of a systematic uncertainty). The
random seed is set deterministically from the jet φ. Three smearing modes are available:
`pt`, `mass`, or `FourVec` (smears both pT and mass together).
 
The tool's resolution functions are also accessible
externally via `JetCalibTool::getNominalResolutionData` and `getNominalResolutionMC`.
 
**YAML options:**
 
| Key | Type | Default | Description |
|---|---|---|---|
| `OutScale` | string | `JetSmearedMomentum` | Output jet moment name |
| `SmearType` | string | `FourVec` | Smearing mode: `pt`, `mass`, or `FourVec` |
| `HistoReader` | dict | — | Convenience block combining `HistoReaderMC` and `HistoReaderData`; requires `histNameMC`, `histNameData`, plus common `HistoInput` parameters |
| `HistoReaderMC` | dict | — | `HistoInput` configuration for the MC JER histogram (used instead of `HistoReader` if separate config is needed) |
| `HistoReaderData` | dict | — | `HistoInput` configuration for the data JER histogram |
 
**Example YAML block:**
 
```yaml
Smear:
  OutScale: "JetSmearedMomentum" 
  SmearType: "FourVec"
  HistoReader:
    inputFile: "JetCalibTools/CalibArea-00-04-82/CalibrationFactors/JER_Nominal_Apr2019.root"
    histNameMC: "JER_Nominal_MC16_AntiKt4EMTopo"
    histNameData: "JER_Nominal_data_AntiKt4EMTopo"
    InterpType: "OnlyX"
    varX: "pt"
    varY: "abseta"
```

Example specifying HistoReaderMC and HistoReaderData separately and explicitly setting varX/varY parameters:

```yaml
SmearLong:
  HistoReaderMC:
    histName: "JER_Nominal_MC16_AntiKt4EMTopo"
    inputFile: "JetCalibTools/CalibArea-00-04-82/CalibrationFactors/JER_Nominal_Apr2019.root"
    InterpType: "OnlyX"
    varX:
      Name: "pt"
      Scale: 0.001
      isJetVar: True
      Type: "float"
    varY:
      Name: "abseta"
      Scale: 1.0
      Type: "float"
      isJetVar: True
  HistoReaderData:
    histName: "JER_Nominal_data_AntiKt4EMTopo"
    inputFile: "JetCalibTools/CalibArea-00-04-82/CalibrationFactors/JER_Nominal_Apr2019.root"
    InterpType: "OnlyX"
    varX:
      Name: "pt"
      Scale: 0.001
      Type: "float"
      isJetVar: True
    varY:
      Name: "abseta"
      Scale: 1.0
      Type: "float"
      isJetVar: True
  OutScale: "JetSmearedMomentum"
  SmearType: "FourVec"
  noRun: True
```
</details>

<details>
<summary>AF3 — ATLFAST3 Fast-Simulation Correction</summary>

**Source:** `src/Generic4VecCorrectionStep.cxx`
 
**What it does:** Applies a multiplicative 2D correction to the jet four-vector to remove
residual response differences between ATLFAST3 fast-simulation and full-simulation jets.
This step is automatically skipped for data and for full-simulation MC. Its YAML configuration
is identical in structure to `PtResidual`.
 
**YAML options:**
 
| Key | Type | Default | Description |
|---|---|---|---|
| `OutScale` | string | — | Output jet moment name |
| `CalibConstantFile` | string | — | Calibration ROOT file (resolved via PathResolver) |
| `histoParams.histName` | string | — | Histogram name in the ROOT file |
| `histoParams.varX` | string | — | Variable for histogram x-axis |
| `histoParams.varY` | string | — | Variable for histogram y-axis |
 
**Example YAML block:**
 
```yaml
AF3:
  OutScale: JetFastSimScaleMomentum
  CalibConstantFile: "JetCalibTools/CalibArea-00-04-83/CalibrationFactors/AF3_MC20_AntiKt4EMPFlow_R22ConsolidatedApril25.root"
  histoParams:
    varX:     pt
    varY:     absrapidity
    histName: h_respMap_recoPt_recoY
```
</details>
 
<details>
<summary>PtResidual — pT Residual Correction</summary>

**Source:** `src/Generic4VecCorrectionStep.cxx`
 
**What it does:** Applies a generic 2D multiplicative correction to the jet four-vector, read
from a histogram. Typically used for a residual pT correction after the GSC. Setting
`useBinCenter: true` snaps the η coordinate to the nearest histogram bin centre before
lookup, which avoids interpolation across η bin boundaries.
 
**YAML options:**
 
| Key | Type | Default | Description |
|---|---|---|---|
| `OutScale` | string | — | Output jet moment name |
| `CalibConstantFile` | string | — | Calibration ROOT file (resolved via PathResolver) |
| `histoParams.histName` | string | — | Histogram name in the ROOT file |
| `histoParams.varX` | string | — | Variable for histogram x-axis |
| `histoParams.varY` | string | — | Variable for histogram y-axis (if `useBinCenter: true`, set this to `binCenterEta`) |
| `useBinCenter` | bool | `false` | Snap to the η bin centre instead of interpolating; requires `histoParams.varYHisto` |
 
**Expert options:**
 
| Key | Type | Default | Description |
|---|---|---|---|
| `histoParams.varYHisto` | string | — | Variable used to select the η bin when `useBinCenter: true` (e.g. `DetectorEta`) |
 
**Example YAML block:**
 
```yaml
PtResidual:
  OutScale: JetPtResidualScaleMomentum
  CalibConstantFile: 'JetCalibTools/CalibArea-00-04-83/CalibrationFactors/PtResidual_MC20_AntiKt4EMPFlow_R22ConsolidatedApril25.root'
  useBinCenter: true
  histoParams:
    varX:      pt
    varY:      binCenterEta
    varYHisto: DetectorEta
    histName:  h_respMap_recoPt_DetEta
```
</details>
 
<details>
<summary>MC2MC — MC-to-MC Correction</summary>

**Source:** `src/Generic4VecCorrectionStep.cxx`
 
**What it does:** Corrects for differences between the jet response in different MC shower
models, so that in-situ calibrations derived with one generator can be applied consistently
to samples produced with other generators. Separate correction histograms are provided per
parton flavour label (light quark, gluon, and optionally c- and b-quark). The shower model
string is determined automatically from the generator name and version in the input file
metadata, with an optional DSID-based exceptions list (`MC2MC_exceptions_DSID.json`) and a
remapping file (`MC2MC_showerRemap.json`). This step is automatically skipped for data and
for Pythia8 samples (which define the reference).
 
**YAML options:**
 
| Key | Type | Default | Description |
|---|---|---|---|
| `OutScale` | string | — | Output jet moment name |
| `CalibConstantFileName` | string | — | Base path of the ROOT file; the shower model string and `.root` extension are appended automatically |
| `flavours` | list | — | Parton flavour labels to configure; subset of `['q', 'g', 'c', 'b']` |
| `histoParams.varX` | string | — | Variable for histogram x-axis |
| `histoParams.varY` | string | — | Variable for histogram y-axis |
| `histoParams.histNameBase` | string | — | Base histogram name; flavour label (e.g. `_q`, `_g`) is appended |
| `PIDLabel` | string | `PartonTruthLabelID` | Jet decoration name for the parton truth label ID |
 
**Example YAML block:**
 
```yaml
MC2MC:
  OutScale: JetMC2MCScaleMomentum
  CalibConstantFileName: JetCalibTools/CalibArea-00-04-83/CalibrationFactors/MC2MC_MC20_AntiKt4EMPFlow_R22ConsolidatedApril25
  flavours: ['q', 'c', 'b', 'g']
  PIDLabel: PartonTruthLabelID
  histoParams:
    varX: pt
    varY: absrapidity
    histNameBase: h_respMap_recoPt_recoY
```
</details>

---

## Example: Integrating into an analysis algorithm

The following example code shows how you can integrate a JetCalibTool instance into an analysis algorithm.

<details>
<summary>Python configuration</summary>

```python
# MyAnalysis/python/MyAlgorithmConfig.py

from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
from JetCalibTools.JetCalibStepsConfig import calibToolFromConfigFile

def MyAlgorithmCfg(flags, name="MyAlgorithm", **kwargs):
    acc = ComponentAccumulator()

    # Configure the jet calibration tool from a YAML config file
    jetCalibTool = calibToolFromConfigFile(
        flags,
        configFile = "exampleFile.yaml",
        name       = "MyJetCalibTool",
    )

    alg = CompFactory.MyAlgorithm(
        name,
        JetCalibTool    = jetCalibTool,
        JetContainerKey = "AntiKt4EMPFlowJets",
        **kwargs
    )
    acc.addEventAlgo(alg)
    return acc
```
</details>

<details>
<summary>C++ algorithm header</summary>

```cpp
// MyAnalysis/MyAnalysis/MyAlgorithm.h
#include <AnaAlgorithm/AnaAlgorithm.h>
#include "JetCalibTools/JetCalibTool.h"
#include "StoreGate/ReadHandleKey.h"
#include "xAODJet/JetContainer.h"

class MyAlgorithm : public EL::AnaAlgorithm {
public:
    using AnaAlgorithm::AnaAlgorithm;
    virtual StatusCode initialize() override;
    virtual StatusCode execute() override;

private:
    ToolHandle<IJetCalibTool> m_jetCalibTool
        {this, "JetCalibTool", "", "Jet calibration tool"};
    SG::ReadHandleKey<xAOD::JetContainer> m_jetKey
        {this, "JetContainerKey", "AntiKt4EMPFlowJets", "Input jet container"};
};
```
</details>

<details>
<summary>C++ algorithm implementation</summary>

```cpp
// MyAnalysis/src/MyAlgorithm.cxx
#include "MyAnalysis/MyAlgorithm.h"
#include "xAODCore/ShallowCopy.h"

StatusCode MyAlgorithm::initialize() {
    ATH_CHECK(m_jetCalibTool.retrieve());
    ATH_CHECK(m_jetKey.initialize());
    return StatusCode::SUCCESS;
}

StatusCode MyAlgorithm::execute() {
    // Retrieve jet container
    SG::ReadHandle<xAOD::JetContainer> jets(m_jetKey);
    ATH_CHECK(jets.isValid());

    auto shallowCopy = xAOD::shallowCopy(*jets);
    auto& calibJets = *shallowCopy.first;

    // The calibration tool operates on the full container
    ATH_CHECK(m_jetCalibTool->calibrate(calibJets));

    return StatusCode::SUCCESS;
}
```

Note that `JetCalibTool::calibrate` takes a `xAOD::JetContainer&` and applies all configured
steps to the whole collection in one call. There is no per-jet interface.
</details>

<details>
<summary>Optionally querying the jet resolution</summary>

If a `SmearingCalibStep` is configured, the nominal resolution of each jet can be retrieved from the
calibration tool directly:

```cpp
JetHelper::JetContext jc;
for(const auto jet: jets){
    double sigmaData = 0., sigmaMC = 0.;
    ATH_CHECK(m_jetCalibTool->getNominalResolutionData(*jet, jc, sigmaData));
    ATH_CHECK(m_jetCalibTool->getNominalResolutionMC(*jet, jc, sigmaMC));
}
```
</details>

---

## For Developers: 

### How JetCalibTools works

The calibration is organised as an **ordered sequence of independent steps**. Each step is a
self-contained Athena component (`asg::AsgTool`) implementing the `IJetCalibStep` interface, with the source file in `src/`.
A step reads the jet four-momentum as a named *input scale*, applies a correction, and writes the
updated four-momentum to a named *output scale*. The steps to be run, their parameters, and the step sequence ordering, 
are specified entirely through a **YAML configuration file**. The calibration sequence can therefore
be composed, reordered, or extended without touching C++.


#### Step-based architecture

The top-level tool (`JetCalibTool`) holds an ordered `ToolHandleArray` of `IJetCalibStep`
tools. On each call to `JetCalibTool::calibrate(jets)` it simply loops over the steps and
calls `step->calibrate(jets)` in order.

The ordering is determined by the `Sequence` block in the YAML configuration file, as described below.
Each step has two mandatory properties — `InScale` and `OutScale` — that name the
`xAOD::JetFourMom_t` moment read and written on the jet object. Note that the python configuration in `JetCalibStepsConfig.py` 
automatically sets `InScale` for each step to the `OutScale` of the previous step, following the ordering supplied in the `Sequence` block of the YAML config file. The first step uses `InScale = JetConstitScaleMomentum`. 
The `OutScale` is also automatically set to the conventional default for each step if it not provided in the YAML config file. 

<details>
<summary>Example for Small-R jets</summary>

```
JetConstitScaleMomentum  (starting point)
        │
        ▼  PileupArea (PileupAreaCalibStep)
JetPileupScaleMomentum
        │
        ▼  Residual (Pileup1DResidualCalibStep)
JetPileupResidualScaleMomentum
        │
        ▼  EtaJES (EtaJESCalibStep)
JetEtaJESScaleMomentum
        │
        ▼  GSC (GSCCalibStep)
JetGSCScaleMomentum
        │
        ▼  Insitu (InSituCalibStep) - data only
JetInsituScaleMomentum
```
</details>

Note that the `SmearingCalibStep` tool (optionally applied to MC only) can be queried separately for resolution values via
`getNominalResolutionData` / `getNominalResolutionMC`.

#### YAML configuration file

A single YAML file defines the full calibration for a given jet collection and data-taking period. An example can be found in `data/calibConfigExample.yaml` (for Small-R jets) and `data/calibConfigExample_largeR.yaml` (for Large-R jets). 

Each top-level block corresponds to a calibration step, keyed by the step name as registered in `JetCalibStepsConfig.calibStepDic`.
See [Calibration Steps](#calibration-steps) for details of the name and options available for each step.

The ordering of the calibration steps is determined by a `Sequence` block, for example:
```yaml
Sequence:
  Run2: 
    FullSim: [Residual, EtaJES, GSC, Smear, PtResidual, MC2MC]
    AF3: [Residual, EtaJES, GSC, Smear, AF3, PtResidual, MC2MC]
    Data: [Residual, EtaJES, GSC, Insitu]
  Run3: 
    FullSim: [Residual, EtaJES, GSC, Smear, PtResidual, MC2MC]
    AF3: [Residual, EtaJES, GSC, Smear, AF3, PtResidual, MC2MC]
    Data: [Residual, EtaJES, GSC, Insitu]
```
The ordering of the individual step blocks in the YAML file does not matter, the steps are always run in the order specified by the `Sequence` block.

Run-specific settings can optionally be supplied using the `Run2`, `Run3`, `Run4` sub-blocks. 
<details>
<summary>Any settings provided by such a sub-block will only be applied to samples for the corresponding run. For example: </summary>

```yaml
EtaJES:
  ParametrizedVars:
    varE: "e"
    varEta: "DetectorEta"
  CalibConstantFile: JetCalibTools/CalibArea-00-04-82/CalibrationFactors/MC16a_MCJES_4EMPFlow_Oct2017.config
  Run2:
    UseSpline: False
  Run3:
    UseSpline: True # Note this is just an example, not a recommendation!
```
</details>
<details>
<summary>This can also be used to override a setting only for specific runs:</summary>

```yaml
EtaJES:
  ParametrizedVars:
    varE: "e"
    varEta: "DetectorEta"
  CalibConstantFile: JetCalibTools/CalibArea-00-04-82/CalibrationFactors/MC16a_MCJES_4EMPFlow_Oct2017.config
  UseSpline: False
  Run4:
    UseSpline: True # Only set UseSpline: True for Run 4 samples
```
</details>
</br>

Additionally, a top-level block `Global` can be used to set properties applied directly to the `JetCalibTool`. Note that no such properties currently exist.

#### Python configuration

The Python-side configuration lives in `python/JetCalibStepsConfig.py`. The primary entry
point is `calibToolFromConfigFile`, which:

1. Loads and parses the YAML file using `load_yaml_cfg`.
2. Translates the YAML blocks into an ordered list of Athena tools via `calibConfigToToolList`.
3. Passes the configured sequence of calib steps to the top-level `JetCalibTool`.

Each step has a corresponding Python function (e.g. `etajesStep`, `gscStep`, …) that translates the YAML block into concrete Athena tool instances. This includes resolving calibration file paths via `PathResolver` and constructing `HistoInput` / `VarTool` helper tools from the `JetToolHelpers` package.

**Automatic step skipping:** Several steps are skipped automatically based on the input file metadata:

- `Insitu` is skipped for MC unless the property `CalibrateMC: true` is set.
- `MC2MC` is skipped for data and for Pythia8 samples.
- `AF3` is skipped for data and for full-simulation MC.

Note that the step sequence order supplied by the YAML config can be optionally overriden with the `calibSeqOverride` input argument
to `calibConfigToToolList`, which can be set to a `_`-separated sequence of steps, e.g. `"EtaJES_JMS"`. 
This is used to supply T0-specific step sequences in `JetCalibToolsCfg.py`.

---

### Adding a New Calibration Step

#### Step 1: Implement a new C++ tool
Implement a new C++ tool for your step, with the header in `JetCalibTools/` and source file in `src/`. 
You will also need to add the step to `src/components/JetCalibTools_entries.cxx` and `JetCalibTools/selection.xml`. 

<details>

<summary>Example .h</summary>

```cpp
// JetCalibTools/MyCalibStep.h
#ifndef JETCALIBTOOLS_MYCALIBSTEP_H
#define JETCALIBTOOLS_MYCALIBSTEP_H

#include "AsgTools/AsgTool.h"
#include "AsgTools/AsgToolMacros.h"
#include "AsgTools/ToolHandle.h"
#include <AsgTools/PropertyWrapper.h>

#include "JetAnalysisInterfaces/IJetCalibStep.h"
#include "JetAnalysisInterfaces/IVarTool.h"

class MyCalibStep
  : public asg::AsgTool,
      virtual public IJetCalibStep {

        ASG_TOOL_CLASS(MyCalibStep, IJetCalibStep)

    public:
        MyCalibStep(const std::string& name = "MyCalibStep");
  
        virtual StatusCode initialize() override;
        virtual StatusCode calibrate(xAOD::JetContainer&) const override;

    private:
        Gaudi::Property<std::string> m_jetInScale {this, "InScale", "JetConstitScaleMomentum", "Starting jet scale"};
        Gaudi::Property<std::string> m_jetOutScale {this, "OutScale", "JetMyScaleMomentum", "Ending jet scale"};
        Gaudi::Property<std::string> m_myProperty {this, "MyProperty", "default_value", "Description"};

        ToolHandle<JetHelper::IVarTool> m_histTool {this, "HistoReader", "HistoInput2D", "Instance of HistoInput2D for reading histogram"};
        ToolHandle<JetHelper::IVarTool> m_varTool {this, "VarTool", "VarTool", "Instance of VarTool for accessing a variable"};

};
#endif
```
</details>

<details>
<summary>Example .cxx</summary>

```cpp
// src/MyCalibStep.cxx
#include "JetCalibTools/MyCalibStep.h"

MyCalibStep::MyCalibStep(const std::string& name)
  : asg::AsgTool( name ){ }

StatusCode MyCalibStep::initialize() {
  
  ATH_CHECK( m_histTool.retrieve() ); 
  ATH_CHECK( m_varTool.retrieve() ); 

  return StatusCode::SUCCESS;
}

StatusCode MyCalibStep::calibrate(xAOD::JetContainer& jets) const {

  JetHelper::JetContext jc;
  for (xAOD::Jet* jet : jets){

    const xAOD::JetFourMom_t jetStartP4 = jet->getAttribute<xAOD::JetFourMom_t>(m_jetInScale);

    // Read variable
    float var = m_varTool->getValue(*jet, jc);

    // Extract histogram value
    float correction = m_histTool->getValue(*jet, jc); 

    // Calibrate jet
    xAOD::JetFourMom_t calibP4 = xAOD::JetFourMom_t(jetStartP4.pt()*correction,jetStartP4.eta(),jetStartP4.phi(),jetStartP4.mass());
    jet->setJetP4(calibP4);

    // Print result
    ATH_MSG_INFO("Initial pT = " << jetStartP4.pt()<<", calibrated pT = "<<calibP4.pt());

    // Set the output scale
    jet->setAttribute<xAOD::JetFourMom_t>(m_jetOutScale,calibP4);
  }  
  return StatusCode::SUCCESS;
}
```
</details>

<details>
<summary>Example XML</summary>

```xml
<!-- JetCalibTools/selection.xml  -->
<lcgdict>
...
  <class name="MyCalibStep" />
...
</lcgdict>
```
</details>

<details>
<summary>Example components file</summary>

```cpp
// src/components/JetCalibTools_entries.cxx
...
#include "JetCalibTools/MyCalibStep.h"
...
DECLARE_COMPONENT( MyCalibStep )
...
```

</details>

#### Step 2: Add the python configuration

Add a configuration function to `JetCalibStepsConfig.py`, with arguments `(flags, **configDic)`. 
The function should be registered in `calibStepDic`, which provides the mapping from the YAML block name.
The `**configDic` dictionary argument contains the YAML block settings, which should be used to set up the tool.

<details>
<summary>For example:</summary>

```python
# python/JetCalibStepsConfig.py

def myStep(flags, **configDic):

    # Set-up any JetToolHelpers HistTool's or VarTool's
    histoParams = configDic.pop('HistoKey') # here "HistoKey" is the YAML key which provides the histogram settings
    histoParams['inputFile'] = PathResolver.FindCalibFile(histoParams['inputFile']) # find the input file
    configDic["HistoReader"] = HistoInputCfg(flags, "MyHistTool", **histoParams)
    var = configDic.pop('VarKey') # where VarKey is a YAML key for the variable settings
    configDic["VarTool"] = VarToolCfg(flags, var, "MyVarTool")

    # Set-up the calibration step
    mystep = CompFactory.MyCalibStep("MyCalib", **configDic)
    return [mystep]

...

calibStepDic = dict(
  ...
  MyStep = myStep,
  ...
)
```
</details>

#### Step 3: Create an example YAML block 

Add this either to a new example YAML file or to one of the existing files (`data/calibConfigExample.yaml` or `data/calibConfigExample_largeR.yaml`).

<details>
<summary>Example YAML block</summary>

```yaml
MyStep:
  HistoKey:
    inputFile: "JetCalibTools/CalibArea-00-04-82/CalibrationFactors/JER_Nominal_Apr2019.root"
    histName: "JER_Nominal_MC16_AntiKt4EMTopo"
    varX: "pt"
    varY: "abseta"
  VarKey: pt
  MyProperty: "Test"
  OutScale: JetMyScaleMomentum
```

</details>

## Package Structure

```
JetCalibTools/
├── JetCalibTools/                   # Public headers
│   ├── IJetCalibrationTool.h        # Interface for JetCalibTool
│   ├── IJetCalibStep.h              # Interface for individual calibration steps
│   ├── EtaJESCalibStep.h
│   ├── GSCCalibStep.h
│   ├── InSituCalibStep.h
│   ├── InSituJMSCalibStep.h
│   ├── JetCalibTool.h
│   ├── JMSCalibStep.h
│   ├── Pileup1DResidualCalibStep.h
│   ├── PileupAreaCalibStep.h
│   ├── SmearingCalibStep.h
│   └── Generic4VecCorrectionStep.h
├── src/                             # New calibration step implementations
│   ├── EtaJESCalibStep.cxx
│   ├── Generic4VecCorrectionStep.cxx
│   ├── GSCCalibStep.cxx
│   ├── InSituCalibStep.cxx
│   ├── InsituJMSCalibStep.cxx
│   ├── JetCalibTool.cxx
│   ├── JMSCalibStep.cxx
│   ├── Pileup1DResidualCalibStep.cxx
│   ├── PileupAreaCalibStep.cxx
│   └── SmearingCalibStep.cxx
├── Root/                            # Old calibration step implementations
├── python/
│   └── JetCalibStepsConfig.py       # Python configuration; primary entry point
├── data/                           # Default/example YAML configuration files
│   └── calibConfigExample.yaml
│   └── calibConfigExample_largeR.yaml
├── util/                            # Test executables / scripts (note currently these are all deprecated)
└── CMakeLists.txt
```