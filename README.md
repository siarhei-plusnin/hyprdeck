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
                font_family = "JetBrainsMono Nerd Font",
                shortcuts_footer = "hint",
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

Comma-separated workspace names are trimmed, de-duplicated, and may include or omit a leading `special:` prefix. Overlay lists are comma-separated case-insensitive namespace/class substrings; `$VAR` and `${VAR}` expansion is supported there.
