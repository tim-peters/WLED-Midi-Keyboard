# 🎹 Keyboard → WLED

> Play a MIDI keyboard (or tap on your phone) — your LED strip lights up in real time.

![Demo](demo.gif)

**WLED MIDI Keyboard** is a single HTML file that bridges any USB MIDI controller with a [WLED](https://kno.wled.ge/)-controlled LED strip — polyphonic, velocity-sensitive, zero install. No server, no build step, no dependencies. Just open the page and play.

No MIDI keyboard? No problem — the built-in touch visualization works on any smartphone or tablet and lets you control WLED directly from the browser.

---

## Screenshots

![Screenshot of the keyboard-wled web UI](screenshot.png)

---

## Features

- 🎵 **Polyphonic** — up to 25 simultaneous keys, each with its own independent fade state
- 🎚️ **Velocity-sensitive** — strike strength scales brightness (can be disabled)
- 🤍 **White-keys-only mode** — optionally ignore black keys for simpler LED mapping
- 🔢 **Octave offset** — shift the active MIDI range by ±36 semitones to fit your keyboard
- 🎛️ **Hardware CC mapping** — map three knobs of any MIDI controller to color, brightness, and fade-out
- 📏 **Variable strip length** — LED range freely configurable, no hard maximum
- 👁️ **Live visualization** — individual LEDs rendered in the browser, in sync with WLED
- 📱 **Touch control** — tap the visualization directly (great for smartphones without a MIDI keyboard)
- 🔍 **MIDI monitor** — collapsible debug view for incoming MIDI messages

---

## Requirements

| Component | Requirement |
|---|---|
| **Browser** | Chrome 90+ or Edge 90+ (Web MIDI API — not available in Firefox) |
| **MIDI keyboard** | Any USB MIDI controller (tested with Donner DMK 25 Pro) |
| **WLED device** | Reachable on the local network, JSON API enabled (default) |

> **No MIDI keyboard?** You can still use the touch visualization in any modern browser — including Safari on iOS — to trigger LEDs by tapping directly on the strip preview.

---

## Getting Started

### Option A — Run directly from WLED *(recommended)*

The page is a static HTML file that lives inside the WLED filesystem. The WLED device serves it, so the page is on the **same origin** as the JSON API — no CORS, no extra server.

1. Open the WLED web UI (`http://<wled-ip>`)
2. Click **File Editor** (just below the color palette)
3. Upload `keyboard-wled.html` from this repo
4. Open `http://<wled-ip>/keyboard-wled.html` in Chrome or Edge

> ⚠️ **MIDI requires a secure context.** Chrome blocks Web MIDI on plain `http://` addresses (other than `localhost`). If MIDI shows *"Blocked: insecure context"*, use one of the two sub-options below:

**A1 — Chrome insecure-origin flag (dev / quick test only)**

1. Open `chrome://flags/#unsafely-treat-insecure-origin-as-secure`
2. Add `http://<wled-ip>`
3. Restart Chrome

Not recommended for everyday use — doesn't work on phones or other browsers.

**A2 — Reverse proxy via localhost (recommended)**

Run a one-liner on your computer to tunnel the WLED address through `localhost` — no changes to the device needed:

```bash
# socat (install once: brew install socat)
socat TCP-LISTEN:8000,reuseaddr,fork TCP:192.168.x.x:80
```

```bash
# or Caddy (install once: brew install caddy)
echo ':8000 { reverse_proxy 192.168.x.x:80 }' > Caddyfile && caddy run
```

Then open `http://localhost:8000/keyboard-wled.html`. `localhost` is a secure context → MIDI works.

---

### Option B — Run locally from your computer

Open the file directly from disk or a local web server:

```bash
# Quick local server
python3 -m http.server
# → open http://localhost:8000/keyboard-wled.html
```

Or just double-click `keyboard-wled.html` to open it as a `file:///` URL.

> **Set the WLED IP manually** — the field is prefilled with the page's own host, so change it to your WLED device's IP (e.g. `192.168.x.x`).

> ⚠️ **CORS:** When running locally, the browser will block cross-origin requests to WLED. Either use the local server approach (`localhost` counts as secure) combined with a reverse proxy, or temporarily install a CORS-unblock browser extension during development.

---

## Configuration

### LED Range

| Field | Default | Description |
|---|---|---|
| **LED Start** | `0` | First LED index of the controlled range |
| **LED End** | `67` | Last LED index (inclusive) |

Works with any strip length. The browser visualization renders up to 500 LEDs.

### Keyboard

| Setting | Default | Description |
|---|---|---|
| **Offset** | `0` | Shift active MIDI range (−36 … +36 semitones). `0` = C3–C5 (Donner default) |
| **White keys only** | On | Distribute only the 15 white keys per octave across the LED range |
| **Velocity sensitivity** | On | Scale brightness by strike strength. Off = always 100% |

### Controls

| Control | Range | Default |
|---|---|---|
| **Color** | RGB | `#ff6400` |
| **Brightness** | 0–100% | `100%` |
| **Fade-Out** | 0–3000 ms | `300 ms` |

### MIDI CC Mapping (Donner DMK 25 Pro default)

| Knob | Ch | CC | Function |
|---|---|---|---|
| 1 | 1 | 7 | **Color** (hue) |
| 2 | 2 | 7 | **Brightness** |
| 3 | 3 | 7 | **Fade-Out** |

Other controllers: adjust the `Ch` and `CC` values in the UI. `Ch = 0` matches any channel. Use the MIDI monitor to identify what your controller sends.

---

## Troubleshooting

### "WLED not reachable" despite correct IP

- **CORS issue?** If the page is served locally, the browser blocks cross-origin requests to WLED. Upload the file to WLED (Option A) or use a reverse proxy.
- **Wrong IP?** Test directly: `http://<ip>/json/info` should return JSON.
- **AP isolation?** Some routers prevent Wi-Fi clients from seeing each other. Try wired or check router settings.
- **Old firmware?** The JSON API has been standard for years, but very old WLED versions may behave differently.

### "MIDI access denied — secure context required"

The Web MIDI API only works on `https://` or `http://localhost`. A plain `http://<ip>` page is not a secure context. Use Option A1 (reverse proxy) or A2 (Chrome flag) from the Getting Started section.

### "MIDI access denied" (other causes)

- **Use Chrome or Edge** — Firefox does not support the Web MIDI API.
- Check site permissions in your browser settings and set MIDI to "Allow".

### Keys light up at the wrong position

- **Adjust the offset** — ±12 semitones = one octave.
- **Check the LED range** — make sure Start/End match your physical strip.
- **Disable white-keys-only** if you want black keys to trigger LEDs too.

### Hardware knobs have no effect

1. Expand the **MIDI Monitor** at the bottom of the page.
2. Turn a knob — do `CC` entries appear?
   - **No:** the knobs aren't sending CC (check the controller's setup menu).
   - **Yes:** compare the `Ch` and `CC` values shown with those in the knob cells and adjust.

### Fade-out stutters or cuts off

- Close other tabs or apps that claim the MIDI device.
- Enable hardware acceleration in Chrome (`chrome://settings/system`).
- Close CPU-heavy tabs — the `requestAnimationFrame` loop gets throttled under load.

---

## Known Limitations

- **Chrome / Edge only** for MIDI (Web MIDI API not in Firefox or Safari)
- **Touch control works in all browsers**, including Safari on iOS
- **MIDI requires a secure context** — needs a reverse proxy or Chrome flag when served from WLED over plain HTTP
- **Visualization capped at 500 LEDs** (browser performance) — WLED communication still uses the full strip length
- **No native WLED integration** — this is an external tool, not a usermod
- **Occupies a few KB of flash** on the WLED device's filesystem

---

## Technical Notes

- **Single HTML file** — no build step, no dependencies, no server component
- **Latency:** ≤ 33 ms (30 req/s throttle)
- **Polyphonic fade:** `Map<note, {brightness, fadeStartTime, isFading}>` updated via `requestAnimationFrame`. Each note fades independently.
- **Retriggering:** a new strike during fade cancels it and restarts with the new velocity.
- **WLED payload:** `{ "seg": [{ "id": 0, "i": [idx, [r,g,b], …] }] }`
- **Throttle:** timestamp comparison + `setTimeout` to batch changes within a 33 ms window

---

## About this project

This is a hobby project — built for fun, used at home, shared in case it's useful to anyone else. If it saves you some time or lights up your room in a cool way, I'd genuinely appreciate a ⭐ on GitHub. It's a small thing, but it means a lot to a solo developer.

Issues and pull requests are welcome!
