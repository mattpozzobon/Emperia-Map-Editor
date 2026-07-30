# Emperia Map Editor modernization

This checklist tracks the modernization work in deliverable-sized batches. Items
marked complete have been implemented in the current working tree.

## P0 - Responsiveness and correctness

- [x] Route key-up events to `MapCanvas::OnKeyUp`.
- [x] Coalesce canvas paint requests instead of forcing synchronous redraws.
- [x] Normalize wheel and high-resolution trackpad input for zoom, floors, and brush sizes.
- [x] Add opt-in smoothed canvas frame-time and slow-frame diagnostics.
- [x] Add opt-in map open, save, and import profiling.
- [ ] Move map open/save/import/export work off the UI thread.
- [ ] Add cancellable progress UI for long-running operations.
- [ ] Profile and batch tile, overlay, and sprite rendering.

## P0 - Responsive application shell

- [x] Convert the palette's fixed pixel sizing to flexible DPI-aware sizing.
- [x] Increase the main toolbar's icon and interaction target size.
- [x] Give status-bar fields useful responsive proportions.
- [x] Add toolbar overflow handling for narrow windows.
- [x] Add compact, comfortable, and spacious UI-density preferences.
- [ ] Audit remaining dialogs and controls for raw pixel sizes.

## P0 - Asset discovery

- [x] Add live name/ID filtering to terrain, doodad, item, and RAW palettes.
- [x] Make filtered palette lists safe when no brushes match.
- [x] Debounce palette filtering to avoid rebuilding large grids per keystroke.
- [ ] Add a cross-category asset browser.
- [ ] Add favorites and recently used brushes.
- [ ] Add tags and richer item/category filters.
- [ ] Virtualize large icon grids and thumbnail loading.

## P1 - Faster workflows

- [x] Add a searchable `Ctrl+K` command palette for common editor actions.
- [x] Include navigation, palette, view, window, file, and map commands.
- [x] Add direct brush/item search by name and ID to the command palette.
- [x] Add direct XYZ parsing and navigation to the command palette.
- [x] Add searchable recent maps to the command palette.
- [x] Add a dockable contextual tile Inspector panel.
- [x] Persist Inspector visibility and dock/floating layout.
- [x] Show position, contents, IDs, creature, spawn, house, zone, and tile state.
- [x] Add editable properties and copy actions to the Inspector.
- [ ] Support sensible multi-selection property editing.
- [x] Add a compact floor switcher and zoom control beside the canvas.
- [ ] Add configurable mouse and keyboard bindings.
- [ ] Improve selection handles and drag feedback.

## P1 - Reliability

- [x] Add opt-in timed autosave for the active named local map.
- [x] Add configurable autosave snapshot retention.
- [ ] Add crash recovery and recovery-file cleanup.
- [x] Restore open local maps from the previous clean shutdown.
- [x] Show unsaved state clearly in map tabs.
- [x] Preserve and restore the previous map files around interrupted saves.

## P2 - Visual design and accessibility

- [x] Use system theme colours for custom-drawn brush-list text.
- [x] Use system theme colours for the custom-drawn action history.
- [x] Use semantic system colours in custom item, search, debug, and replacement lists.
- [ ] Add system, light, and dark application themes.
- [ ] Replace remaining hard-coded UI colours with semantic colours.
- [ ] Add a high-contrast mode and colour-blind-safe zone overlays.
- [ ] Audit keyboard navigation, focus indicators, and screen-reader labels.
- [ ] Replace remaining bitmap-only icons with scalable assets.

## Validation

- [x] Build the x64 Release configuration.
- [x] Run a startup smoke test.
- [ ] Run an interactive palette/search smoke test.
- [ ] Test at 100%, 150%, and 200% display scaling.
- [ ] Test narrow, standard, ultrawide, and multi-monitor layouts.
- [ ] Benchmark large maps before and after renderer changes.
