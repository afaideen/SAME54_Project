# SAME54_Project – Knowledge Index

This file defines the authoritative documentation set.
Recommended reading order: HARDWARE_PROFILE → BOOT_FLOW → ARCHITECTURE_DESIGN → REVIEW_ACTIONS → HARDWARE_PROFILE_CHECKS.

Authority order (highest wins):
1) HARDWARE_PROFILE.md
2) BOOT_FLOW.md
3) ARCHITECTURE_DESIGN.md  (includes DESIGN1, DESIGN2, and other DESIGN sections)
4) REVIEW_ACTIONS.md
5) HARDWARE_PROFILE_CHECKS.md

Rules:
- Hardware facts must come from HARDWARE_PROFILE.md
- Init order must follow BOOT_FLOW.md
- Any design text (inside ARCHITECTURE_DESIGN.md) may not contradict HARDWARE_PROFILE.md or BOOT_FLOW.md
- Diagrams are explanatory; if a diagram conflicts with text, text wins

Conflict examples:
- If a diagram shows different LED/SW/UART pins than HARDWARE_PROFILE.md, HARDWARE_PROFILE.md wins.
- If any design text suggests logging is safe before UART2_DMA_Init, BOOT_FLOW.md wins.
- If REVIEW_ACTIONS.md mandates a fix (e.g., timeouts), treat it as required until closed.
