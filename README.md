# hyprdeck

`hyprdeck` is a purely vibeslopped Hyprland workspace overview plugin.

> [!WARNING]
> VIBESLOP ahead

## Lua Configuration

```lua
if hl.plugin.hyprdeck then
    hl.config({
        plugin = {
            hyprdeck = {
                named_special_workspaces = "scratch, music",
                active_workspace_background = false,
                default_zoom = 0.48,
                animations = true,
                font_family = "JetBrainsMono Nerd Font",
                shortcuts_footer = "hint",
                output_colors = "DP-1:#73a7ff, HDMI-A-1:#f08c46",
                blocking_overlays = "rofi, wofi, fuzzel",
                non_blocking_overlays = "dunst, mako, notifications",
                display_capture_overlays = "grim, slurp",
            },
        },
    })

    hl.bind("SUPER + TAB", hl.plugin.hyprdeck.toggle)
end
```

`shortcuts_footer` accepts `full`, `hint`, or `none`.

Set `animations = false` to disable hyprdeck's overview animations while keeping Hyprland's global animation config unchanged.

`output_colors` accepts comma-separated `OUTPUT:#RRGGBB` mappings. Outputs without a configured color receive a deterministic rainbow color based on their name.

## Multiple Outputs

hyprdeck renders one overview at a time. Opening it from another focused output immediately moves the overview there. The output where it opens hosts the UI; the output boxes at the top-left always show their assigned colors, and the selected box controls the destination for workspace actions. Use `Tab` and `Shift+Tab` to cycle through outputs in Hyprland order.

The normal row shows one global, contiguous workspace list, including numeric gaps and one empty workspace after the highest existing ID. Active workspace borders use their owner output's color. A foreign workspace also shows its output name and keeps a live, aspect-preserving preview of that output.

Existing name-based normal workspaces appear alphabetically at the left of the normal row. A thin divider separates them from numeric workspaces when both groups are visible. Renamed numeric workspaces display both their ID and custom name.

`h`/`l` and the arrow keys focus foreign normal workspaces without switching to them. `Enter`/`f` moves the focused workspace to the selected output, activates it, and closes hyprdeck; `Space` or a click moves and activates it while keeping hyprdeck open. A numeric shortcut focuses a foreign workspace on the first press and moves it when that workspace is already focused. Existing active or non-empty special workspaces are visible across all outputs and move to the selected output when activated.

## Animation Leaves

hyprdeck reads Hyprland animation leaves directly for enabled state and interpolation timing/curve. Animation `style` values are ignored.

| hyprdeck animation | Hyprland leaf |
| --- | --- |
| Overview open opacity | `layersIn` |
| Overview close opacity | `layersOut` |
| Normal workspace row centering | `workspacesIn` |
| Special workspace row centering | `specialWorkspaceIn` |
| New special card opacity | `specialWorkspaceIn` |
| Closing special card opacity | `specialWorkspaceOut` |
| Zoom changes | `workspacesIn` |

Comma-separated workspace names are trimmed, de-duplicated, and may include or omit a leading `special:` prefix. Overlay lists are comma-separated case-insensitive namespace/class substrings; `$VAR` and `${VAR}` expansion is supported there.
