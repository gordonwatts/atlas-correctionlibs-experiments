# Architecture comparison graphic

Files: [editable SVG](columnar-architecture-comparison.svg) and [PNG preview](columnar-architecture-comparison.png). The talk outline uses this graphic on slide 1. Edit the SVG as the source; regenerate the PNG after a change.

## Why this comparison opens the talk

The existing ATLAS columnar-tool route was chosen partly to limit migration work for Combined Performance (CP) groups. Coding agents make a more substantial rewrite practical to investigate. This experiment asks what CorrectionLib contributes to a real jet-correction chain and how fast a columnar application can run while the AnalysisBase and Athena interfaces stay the same. The graphic presents those two structures before the talk explains the experiment and timings.

## Sources and mapping

- **Left, ATLAS columnar design:** Nils Krumnack's section of [R2R4 AMG S&C Week - Columnar (2025)](https://docs.google.com/presentation/d/1CTlFzuWv8yvz1zD8Eemk4Yx8fvOGjAugkifxTgl5S0A/edit), especially slides 12–16. Slide 12 explains the migration goal, slide 13 describes an access layer with xAOD and columnar modes, and slides 14–16 show the infrastructure diagram. The older [Modern Analysis Tools Summary (2024)](https://docs.google.com/presentation/d/1ZEvrgVMoMdCe3ax0oCuZq-Tpf1uGBKbMgK2OvVeS6Bk/edit), slides 8–9, records a related layered design. The SVG is a simplified redraw of these concepts, not a copy of a source slide.
- **Right, this experiment:** [implementation record](../implementation-log.md), [nominal backend](../columnar-source/src/backend.hxx), [systematics backend](../columnar-source/src/systematics.hxx), [B integration](../columnar-source/atlas-overlay/JetMomentTools/CMakeLists.txt), and [C-nice interface](../c-nice/README.md). The B and C adapters use shared C++ jet-correction logic. CorrectionLib evaluates calibration payloads within that logic. Versioned JSON payloads hold the extracted calibration constants and lookup structure.

## Reading the graphic

The arrows show where calls and data pass, not a claim that the two designs are interchangeable. On the left, shared CP-tool code uses a data access layer with xAOD and columnar modes. On the right, the B and C adapters reach the same experimental C++ backend. CorrectionLib provides the payload format and evaluator; handwritten C++ still handles ordering, four-vectors, systematic combinations, and selection. NNJVT inference and ROOT random sampling remain separate components. The right-hand structure represents this jet experiment, not a proposed migration plan for every CP tool.

The PNG was rendered from the SVG at 1600 × 900 pixels. Keep the attribution line visible if the graphic is reused in a slide.
