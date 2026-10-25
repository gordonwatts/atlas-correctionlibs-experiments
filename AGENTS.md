# Working agreement

- Preserve completed implementation and validation work with descriptive local Git commits, including relevant verification results. Small talk-outline edits happen only in the Windows repository and can remain uncommitted while a section is being drafted.
- Do not push without explicit user authorization.
- This WSL checkout is authoritative for source, builds, and numerical validation. Build and validate here, not on the Windows-mounted filesystem.
- Keep talk outlines, presentation assets, reports, and inspection copies only in the Windows CorrectionLib repository. Do not duplicate them into this WSL repository. Source configuration and scripts remain in WSL; sync inspection copies to Windows when relevant to a report.
- Preserve previous benchmark and acceptance artifacts in WSL; use a new result directory for each campaign. Put human-readable result reports in the Windows repository.
