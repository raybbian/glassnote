<div align="center">
  <h1>Glassnote</h1>
  <p>On-screen quick notes/annotation tool to untangle complex ideas.</p>
</div>

https://github.com/user-attachments/assets/b16f2b4e-cf1f-4f46-af81-26d6aba5a34c

#### [Jump to Usage](#example-usage)

#### [Jump to Installation](#buildinginstallation)

## Roadmap

- [ ] Configurable color palette
- [ ] Configurable keybinds (extend `gnctl` functionality)
- [x] Instanced line rendering
- [x] Antialiased line rendering
- [x] Rounded line joins + line caps
- [x] Online line simplification algorithm
- [x] Toggleable canvas
- [ ] Touch support
- [ ] Tablet support (lol)
- [ ] Undo support
- [ ] Eraser tool

## Building/Installation

> [!Important]
> - `glassnote` uses the `zwlr_layer_shell_v1` and `wp_cursor_shape_manager_v1` protocols, which are not supported by all Wayland compositors.
> - `gnctl` uses `systemd` and `dbus` to communicate with `glassnote`.

Install the following dependencies:

- `libsystemd`
- `wayland-client`
- `wayland-protocols` (≥ 1.32)
- `egl`
- `glesv2`
- `wayland-egl`
- `xkbcommon`

Then run:

```bash
git clone https://github.com/raybbian/glassnote
cd glassnote
meson setup build
cd build
meson compile
```

To optionally install both `glassnote` and `gnctl`, run:

```bash
meson install
```

## Example Usage

`gnctl` is a CLI tool that assists in the functionality of glassnotes, allowing you to dim the `glassnote` overlay to access the underlying wayland surface.

When `glassnote` is running, use

- `gnctl show` to show the overlay
- `gnctl hide` to hide the overlay
- `gnctl show || gnctl hide` to toggle the overlay

These CLI commands should then be dispatched using your Wayland compositor. 

#### Example Hyprland Config:

```conf
# Start glassnote if it is not running, otherwise toggle the overlay
bind = SUPER, R, exec, gnctl show || gnctl hide || glassnote
```

### Keybindings

`glassnote` allows you to change the cursor stroke and size while the overlay is active.

- `1-5` will change the color of the stroke
- `Q/-` will decrease the stroke size
- `W/=` will increase the stroke size
- `ESC` will close/kill `glassnote`
