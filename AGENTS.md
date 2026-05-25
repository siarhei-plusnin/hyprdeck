# AGENTS.md

## Commands
- Configure/build exactly as HyprPM does: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` then `cmake --build build`.
- For a focused compile check after source edits, use `cmake --build build` when `build/` already exists.
- After a successful build, reload the plugin with `hyprctl plugin unload "$XDG_CONFIG_HOME/hypr/plugins/hyprdeck.so"` then `hyprctl plugin load "$XDG_CONFIG_HOME/hypr/plugins/hyprdeck.so"`.
- `.clangd` expects `build/compile_commands.json`; re-run the CMake configure command if compile commands or Hyprland include paths go stale.
- No test target, CI, or test files were found; the CMake build is the available verification step.

## Project Shape
- This is a Hyprland C++ plugin named `hyprdeck`; HyprPM expects the artifact at `build/hyprdeck.so`.
- Treat `build/` as generated output except for reading `compile_commands.json`; source of truth is `CMakeLists.txt`, `hyprpm.toml`, and `src/`.
- Plugin entrypoints are in `src/main.cpp`: `PLUGIN_API_VERSION`, `PLUGIN_INIT`, and `PLUGIN_EXIT`.
- `PLUGIN_INIT` registers dispatcher `hyprdeck:toggle`, Lua function `hyprdeck.toggle`, config keys `plugin:hyprdeck:named_special_workspaces`, `plugin:hyprdeck:default_zoom`, `plugin:hyprdeck:active_workspace_background`, `plugin:hyprdeck:font_family`, and `plugin:hyprdeck:shortcuts_footer`, and render/input hooks.
- Global runtime state is `hyprdeck::state()` in `src/state.*`; use nested groups (`session`, `interaction`, `layout`, `selection`, `naming`, `filter`, `confirmation`, `shortcuts`, `renderCache`, `hooks`) directly.

## Module Map
- `colors.*` owns named UI color palette helpers; avoid inline `CHyprColor(...)` literals outside this file.
- `config.*` owns typed plugin config access, enum normalization, and cached parsing of named special workspace presets.
- `keyboard.*` owns keyboard modifier and repeat-rate helpers shared by overview, naming, and shortcut search input.
- `overview.*` opens/closes/toggles the overview and manages pointer locking.
- `overview_controller.*` owns overview-mode keyboard command mapping/dispatch using shared shortcut command IDs.
- `overview_interaction.*` owns overview mouse drag/scroll handling and render-stage handling.
- `input.*` owns Hyprland callback entrypoints and modal keyboard routing.
- `layout.*` owns dirty/signature-based card recalculation, camera offsets, hit testing, and centering.
- `naming.*` owns create/rename prompt state and text input routing; `naming_render.*` draws the prompt.
- `confirmation.*` owns modal confirmation prompts for destructive overview actions.
- `workspace_filter.*` owns overview workspace filter input, matching by window class/title, and filter status rendering.
- `navigation.*` switches workspaces and creates/closes special workspaces.
- `selection.*` owns selected-card lookup, mouse/keyboard selection actions, selected workspace window-close actions, and selection-driven camera centering.
- `shortcut_catalog.*` is the single source for shortcut command IDs and keybinding descriptions used by overview/naming/search dispatch, footer, and searchable help.
- `shortcuts.*` owns shortcut menu lifecycle and command-based search input routing; `shortcuts_menu.*` owns filtering/sizing text, and `shortcuts_render.*` draws footer/menu overlays without mutating menu measurements.
- `textinput.*` owns reusable text state, key-to-character mapping, cursor movement, and word/line editing helpers.
- `textinput_repeat.*` owns shared text input key-repeat timers used by modal text inputs.
- `ui.*` owns render-pass primitives and shared text texture rendering/cache behavior.
- `workspaces.*` filters normal numeric workspaces and special workspaces for the active monitor.
- `rendering.*` draws workspace previews, cursor overlay, and labels; it builds per-render window/layer snapshots before drawing cards, and prompt drawing is delegated to `naming.*`.

## Behavior Gotchas
- Named special workspace config is read from the Hyprland config string; current behavior trims whitespace, strips a leading `special:`, and de-duplicates names.
- `plugin:hyprdeck:active_workspace_background = false` draws an empty-workspace backdrop by rendering the monitor background layer surfaces, so wallpaper remains visible without active workspace windows.
- The normal row only shows positive numeric workspaces; the special row only shows special workspaces on the current monitor that are active or have windows.
- Layout recalculation is cached by a signature of monitor geometry, visible workspace lists, zoom/camera, and selection state; call `invalidateLayout()` after direct mutations to those inputs.
- Rendering happens on `RENDER_LAST_MOMENT` for the overview monitor; state/layout changes generally need `g_pHyprRenderer->damageMonitor(monitor)`.

## Style
- CMake builds as C++26 and depends on `pkg-config` resolving `hyprland`.
- `.clang-format` is LLVM-derived but preserves include order, uses 4-space indentation, pointer-left alignment, and a 180-column limit; do not sort includes.
- New UI components must use square corners, opaque backgrounds, and neutral/dark component backgrounds; do not introduce rounded borders, blue backgrounds, or semi-transparent component backgrounds.
