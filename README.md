# Keyboard → WLED

Browser-Tool, das ein MIDI-Keyboard in Echtzeit mit einem WLED-gesteuerten LED-Strip verbindet. Tastendruck → LED-Segment leuchtet auf, Loslassen → konfigurierbarer Fade-Out. Polyphon, anschlagsdynamisch, mit Live-Visualisierung.

---

## Features

- **Polyphon** — bis zu 25 Tasten gleichzeitig, jede mit eigenem Fade-State
- **Velocity-sensitiv** — Anschlagsstärke skaliert die Helligkeit (deaktivierbar)
- **Nur-weiße-Tasten-Modus** — schwarze Tasten wahlweise ignorierbar
- **Oktaven-Versatz** — Keyboard in Halbtonschritten verschiebbar
- **Hardware-CC-Mapping** — drei Regler eines MIDI-Controllers für Farbe, Helligkeit, Fade-Out
- **Variable Strip-Länge** — LED-Bereich frei konfigurierbar (kein hartes Maximum)
- **Live-Visualisierung** — einzelne LEDs im Browser, synchron zum WLED
- **MIDI-Monitor** — Debug-Ansicht für eingehende MIDI-Messages (aufklappbar)
- **30 req/s Throttle** — WLED-konform, max. ein Request pro 33 ms

---

## Voraussetzungen

| Komponente | Anforderung |
|---|---|
| **Browser** | Chrome 90+ oder Edge 90+ (Web MIDI API, in Firefox nicht verfügbar) |
| **MIDI-Keyboard** | USB-MIDI-Controller, getestet mit Donner DMK 25 Pro |
| **WLED** | Lokal im Netzwerk erreichbar, JSON API aktiv (Standard) |
| **CORS Unblock** | Browser-Extension (siehe unten — Pflicht) |

---

## How-To

### 1. CORS Unblock installieren (Pflicht!)

⚠️ **Ohne diese Extension funktioniert die WLED-Kommunikation nicht.** Der Browser blockiert Cross-Origin-Requests von der `file://`-Seite (oder einer anderen Domain) an die WLED-IP standardmäßig. Das WLED-Gerät antwortet zwar, aber der Browser verwirft die Antwort — der Status bleibt dauerhaft auf „Nicht erreichbar", obwohl das Gerät pingbar ist.

**Installation:**
- **Chrome / Edge:** [„CORS Unblock"](https://chromewebstore.google.com/detail/cors-unblock/lfhmikememgdcahcdlaciloancbhjino) aus dem Chrome Web Store installieren (oder „Allow CORS: Access-Control-Allow-Origin")
- **Firefox:** „CORS Everywhere" oder „CORS Unblock"

Nach der Installation den Browser einmal neu starten und die Extension für die Seite aktiviert lassen (Standard).

### 2. Datei öffnen

`keyboard-wled.html` in Chrome oder Edge öffnen:
- **Doppelklick** auf die Datei
- Oder direkt: `file:///pfad/zu/keyboard-wled.html` in die Adresszeile
- Oder über einen lokalen Webserver, z. B. `python3 -m http.server` im Ordner der Datei, dann `http://localhost:8000`

### 3. WLED verbinden

1. IP des WLED-Geräts im Feld **„WLED-IP"** eintragen (z. B. `192.168.178.183`)
2. Button **„Verbinden"** klicken — testet per `GET /json/info`
3. Status sollte auf grün „Verbunden" wechseln

### 4. MIDI aktivieren

1. Button **„Anfordern"** in der MIDI-Karte klicken
2. Browser fragt nach MIDI-Berechtigung — bestätigen
3. Gerät (z. B. „Donner DMK 25 Pro") aus dem Dropdown wählen
4. Status wechselt auf grün „Verbunden"

### 5. Spielen

Tasten drücken — die LEDs leuchten in Echtzeit, beim Loslassen fadet die Helligkeit über die eingestellte Fade-Out-Zeit auf 0.

---

## Konfiguration

### LED-Bereich

| Feld | Standard | Funktion |
|---|---|---|
| **LED Start** | `0` | Erster LED-Index des angesteuerten Bereichs |
| **LED Ende** | `78` | Letzter LED-Index (inklusiv) |

Kein hartes Maximum — funktioniert mit beliebiger Strip-Länge. Die Visualisierung zeigt bis zu 500 LEDs.

### Tastatur

- **Versatz** (Slider, −36 … +36 Halbtöne): Verschiebt den aktiven MIDI-Bereich. Standard `0` = Donners Standardoktave C3–C5. `+` = tiefer, `−` = höher.
- **Nur weiße Tasten** (Standard an): Ignoriert C#, D#, F#, G#, A#. Die 15 weißen Tasten pro Bereich werden gleichmäßig auf den LED-Bereich verteilt.
- **Anschlagsdynamik berücksichtigen** (Standard an): Velocity skaliert die Helligkeit. Aus = immer 100 % der Helligkeits-Slider-Einstellung.

### Regler

- **Fade-Out** (0–3000 ms): Abklingzeit nach Loslassen, Standard `300 ms`
- **Helligkeit** (0–100 %): Maximalhelligkeit bei Velocity 127, Standard `100 %`
- **Farbe** (RGB-Colorpicker): Standard `#ff6400`

### MIDI-CC-Mapping (Donner DMK 25 Pro)

Der DMK 25 Pro sendet CC #7 auf den Kanälen 1/2/3 für die drei Regler. Standard-Belegung in der App:

| Regler | Ch | CC | Funktion |
|---|---|---|---|
| 1 | 1 | 7 | **Farbe** (Hue) |
| 2 | 2 | 7 | **Helligkeit** |
| 3 | 3 | 7 | **Fade-Out** |

Andere Controller: einfach die `Ch`- und `CC`-Werte in den Zellen anpassen. `Ch = 0` matcht auf beliebigem Kanal. Die aktuelle Belegung im MIDI-Monitor prüfen (Details aufklappen).

---

## Fehlerbehebung

### „WLED nicht erreichbar" trotz korrekter IP

1. **CORS Unblock installiert und aktiv?** Häufigste Ursache.
2. **IP-Adresse korrekt?** Im Browser direkt testen: `http://<ip>/json/info` muss JSON liefern.
3. **Gleiches Netzwerk?** Manche Router haben AP-Isolation (WLAN-Clients sehen sich gegenseitig nicht).
4. **WLED-Firmware aktuell?** JSON API ist seit langem Standard, aber alte Versionen könnten Probleme machen.

### „MIDI-Zugriff verweigert"

- **Chrome oder Edge verwenden** — Firefox hat keine Web MIDI API.
- Browser-Berechtigung für MIDI in den Browser-Einstellungen (Site-Permissions) prüfen und auf „Erlauben" stellen.

### Tasten leuchten an der falschen Stelle

- **Versatz** anpassen: ±12 Halbtöne = eine Oktave. Wenn dein Keyboard in einer anderen Oktave sendet (z. B. über die Oktaven-Tasten am Controller), den Wert hier nachziehen.
- **LED-Bereich** prüfen — muss zum physischen Strip passen. Wenn die linkeste LED deines Strips nicht bei Index 0 liegt, den Start-Wert anpassen.
- **Nur weiße Tasten** deaktivieren, falls du auch schwarze Tasten ansteuern willst.

### Hardware-Regler ohne Effekt

1. **MIDI-Monitor** am unteren Rand der Seite aufklappen.
2. Regler drehen — erscheinen `CC`-Einträge? Wenn nein: Regler senden keine CC-Messages (im Controller-Setup aktivieren).
3. Wenn ja: Die angezeigten `Ch` und `CC`-Werte mit den Werten in den drei Regler-Zellen abgleichen und ggf. anpassen.

### Fade-Out ruckelt oder bricht ab

- Andere Tabs/Programme schließen, die MIDI-Geräte beanspruchen
- Browser-Hardware-Beschleunigung aktivieren (`chrome://settings/system`)
- Andere Webseiten mit hoher CPU-Last schließen (rAF-Loop wird sonst throttled)

---

## Technische Details

- **Eine einzige HTML-Datei** — kein Build-Step, keine Abhängigkeiten, keine Server-Komponente
- **Latenz:** ≤ 33 ms (Throttle auf 30 req/s)
- **Polyphoner Fade:** `Map<note, {brightness, fadeStartTime, isFading}>`, `requestAnimationFrame`-Loop. Jede Note fadet unabhängig — gleichzeitige Tasten blockieren sich nicht gegenseitig.
- **Retriggering:** Erneuter Anschlag während Fade bricht den Fade ab und startet mit neuer Velocity.
- **WLED-Payload-Format:** `{ "seg": [{ "id": 0, "i": [idx, [r,g,b], idx, [r,g,b], …] }] }`
- **Throttle-Implementierung:** Timestamp-Vergleich + `setTimeout` zum Batchen mehrerer Änderungen innerhalb eines 33-ms-Fensters
- **Visualisierungs-Cap:** 500 Divs (Performance). Bei längeren Strips wird die Visualisierung abgeschnitten, die WLED-Kommunikation arbeitet aber mit der vollen Länge.

---

## Bekannte Einschränkungen

- **Chrome/Edge only** wegen Web MIDI API
- **CORS Unblock erforderlich** bei `file://` oder Cross-Origin
- **Keine native WLED-Integration** — externes Tool, kein Usermod
- **Visualisierung auf 500 LEDs begrenzt** (Performance-Schutz für den Browser)
