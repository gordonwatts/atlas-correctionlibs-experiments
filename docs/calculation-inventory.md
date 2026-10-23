# Calculation and payload inventory

The numerical authority is AnalysisBase 25.2.100, x86_64-el9-gcc15-opt. Source paths below are relative to that release's installed src directory. This is a deliberately pinned Run-3 MC23a FullSim, AntiKt4EMPFlow configuration.

## Loading and conversion

The original tools resolve their configured calibration areas and filenames through PathResolver. JetCalibrationTool reads its top-level ROOT TEnv, then each calibration stage merges its additional settings and loads named objects from ROOT files. The compiler uses ROOT and PathResolver in the release environment, exports numerical information to schema-v2 JSON, and records each resolved source and SHA-256. Runtime numerical evaluation uses that JSON, not the original ROOT calibration files.

The nominal and uncertainty manifests associate every correction with its original configuration setting or ROOT object. The bundle manifest hashes runtime files and records the required evaluator patch. Runtime text configuration and NNJVT files are copied into their original logical paths so ATLAS's unchanged PathResolver configuration can resolve them through DATAPATH.

## Nominal stages

| Stage | Numerical representation | Inputs and details | Shared implementation |
|---|---|---|---|
| Residual coefficients | Piecewise linear functions from text arrays | Absolute constituent eta; intercept plus accumulated slopes; mu/NPV reference values and mu scaling from settings | residual_mu and residual_npv corrections |
| Area/residual subtraction | Arithmetic formula | MeV pt, rho and transverse area; subtract rho×area and 1000×residual GeV offset. Negative result becomes exactly 10 MeV; small positive results are not clamped | pileup_scale |
| Absolute JES | TH1 interpolation from TList etaJes plus low-energy extrapolation | Energy in GeV and detector eta; preserve centre interpolation, low-pt slope, high-energy freeze and inverse response | jes_scale |
| Eta correction | Polynomial in log energy | Clamp energy at 8×cosh(detector eta) and 2500 GeV; change direction and reconstruct four-vector with ROOT arithmetic | eta_shift plus C++ four-vector layer |
| GSC | Five eta-selected TH2 response functions | Charged PFO fraction, Tile0 and EM3 energy fractions, track multiplicity and track width; original float feature arithmetic and PV indexing | Five ordinary corrections and one CompoundCorrection |

GSC order is charged fraction → Tile0 → EM3 → track count → track width. The compound updates pt multiplicatively after each stage and returns the product of inverse responses. Binnings select interpolation cells; formulas perform interpolation. A bin lookup alone would not reproduce the source calculation.

Sources: Reconstruction/Jet/JetCalibTools/Root/JetCalibrationTool.cxx, JetPileupCorrection.cxx, EtaJESCorrection.cxx, GlobalSequentialCorrection.cxx.

## Uncertainties

The selected Category/Full configuration has 52 input component recipes, eight correlated groups, and 44 independent components: 30 JES and 14 JER. Including the pseudo-data JER copies and both directions gives 116 variations plus nominal, or 117 kinematic states. This is a reduced JES model; it is not All/All.

Ordinary JES components evaluate exported TH1/TH2 functions. Group members add linearly because all eight selected groups are explicitly Correlated. Pile-up components multiply the appropriate table by mu−24.2 or NPV−13.2, or combine both terms. For uncertainty NPV, the source counts vertices with more than one associated track; this differs from nominal calibration's vertex-type count.

Flavour composition combines gluon/light-quark response tables with gluon fractions and fraction uncertainties. Flavour response scales the response difference by the gluon fraction. The non-b terms vanish for the source IsBjet decoration; bJES applies to b jets. AnalysisBase derives IsBjet from PartonTruthLabelID==5. The columnar implementation derives the same flag independently.

JER tables use OnlyX interpolation, preserving discrete eta bins. The nominal width uses max(resolution_MC,resolution_data). The smearing width is sqrt((nominal+abs(variation))²−nominal²) for the selected absolute-resolution components. Positive shifts smear MC; negative shifts smear pseudo-data. TRandom3 seeds, zero-seed substitutions, Gaussian draws and negative-factor rejection match JetUncertaintiesTool. Each variation starts from the independently calibrated nominal jet.

Sources: Reconstruction/Jet/JetUncertainties/Root/{JetUncertaintiesTool,UncertaintyHistogram,FlavourUncertaintyComponent,PileupUncertaintyComponent,UncertaintyGroup}.cxx.

## NNJVT and efficiency

The bundled network is evaluated with the release's standalone lwtnn FastGraph<double>. Features are float RpT, retained JVFCorr, pt-bin and eta-bin. RpT uses the retained SumPtTrkPt500 for the selected hard-scatter vertex and the varied jet pt. Binning uses lower_bound (right-closed bins), not ordinary left-closed histogram intervals.

Inspection corrected an assumption in the initial plan: this frozen working-point configuration leaves JetEtaName at its default, eta, for both selection and efficiency. The NN also bins using the jet's calibrated eta. DetectorEta is used by nominal calibration, not substituted for the JVT tools' configured input.

Truth hard-scatter labels are independently recomputed using original input jet directions and AntiKt4TruthDressedWZJets: float deltaR<0.3 and truth pt>10 GeV. The standard labeling algorithm runs before recalibration. Both paths then propagate this label.

JVT efficiencies and scale factors are constant ROOT bin contents and errors, exported without interpolation. Passing SF is base±error. Failing SF is (1−SF×efficiency)/(1−efficiency), with both SF and efficiency shifted and with source float rounding. Out-of-range SF is −1. Event products exclude out-of-validity jets. Two NNJVT efficiency variations accompany the 117 kinematic states.

Sources: Reconstruction/Jet/JetMomentTools/Root/{JetVertexNNTagger,NNJvtBinning}.cxx; Reconstruction/Jet/JetJvtEfficiency/Root/{JvtEfficiencyToolBase,NNJvtSelectionTool}.cxx; PhysicsAnalysis/AnalysisCommon/ParticleJetTools/Root/JetPileupLabelingTool.cxx.

## Precision issue found during validation

Unpatched CorrectionLib v2.7.0 uses RapidJSON's default floating-point parser, which can move a decimal constant by one double-precision bit. A 500-value round-trip regression test demonstrates this. Near-zero jet mass reconstruction amplified those changes enough to fail the fixed acceptance threshold.

patches/correctionlib-full-precision-json.patch enables kParseFullPrecisionFlag for all relevant JSON entry points. It changes no schema nodes. After rebuilding, all nominal stage coordinates, including mass, match the reference exactly over 196,606 jets. The base commit and patch hash are recorded separately; the built package has a development version because the source tree is patched.

No new CorrectionLib node was needed. The full four-vector workflow remains in C++, while composable scalar functions live in CorrectionLib. A future custom node would need schema, parser/evaluator, serialization, scalar/vector tests, and an explicit evaluator-version requirement.
