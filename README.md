# hyprlandMonocle
This plugin is a simple implementation of a 'monocle' layout for Hyprland

# Configuration
None

Use Hyprland's 'cyclenext' dispatcher to change windows. 

# Dispatchers

Two new dispatchers
 * `resetsplits` Reset all the window splits to default sizes.
 * `setstackcount` Change the number of stacks for the current workspace. Windows will be re-tiled to fit the new stack count.

Two new-ish orientations
 * `orientationhcenter` Master is horizontally centered with stacks to the left and right. 
 * `orientationvcenter` Master is vertically centered with stacks on the top and bottom. 
 * `orientationcenter` An alias for `orientationhcenter`
 

# Installing

## Hyprpm, Hyprland's official plugin manager (recommended)
1. Run `hyprpm add https://github.com/zakk4223/hyprlandMonocle` and wait for hyprpm to build the plugin.
2. Run `hyprpm enable hyprlandMonocle`
3. Set your hyprland layout to `monocle`.

# Other plugins to consider
If you want monocle on only one monitor/workspace, use this plugin:
https://github.com/zakk4223/hyprWorkspaceLayouts

