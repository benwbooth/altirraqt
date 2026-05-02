# altirraqt

Qt port of Altirra (Atari 8-bit emulator). File-by-file translation from
the upstream Win32 codebase.

## Implementation rules

**No stubs. No TODOs. No placeholder menu items.**

If a feature can't be implemented end-to-end right now — because the
backend isn't ready, the API isn't there, or the scope is too large for
the current pass — leave it out entirely. Do not add a disabled menu
entry, a "not yet implemented" message box, a no-op handler, or a
comment promising future work.

When picking what to do next, pick something you can finish completely:
the menu item is wired, the backend supports it, and a user can
exercise the feature end-to-end. If the supporting work (e.g. a display
backend feature, a missing API surface) is needed first, do that first
in the same change.

This rule overrides any "match upstream menus exactly" instinct. A
faithful menu with stubs is worse than a smaller menu of working
features.
