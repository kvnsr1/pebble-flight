# Supplied airline artwork

These five 25×25 color PDC files are copied byte-for-byte from the user's
`Airline_PDC_color` attachment. `manifest.json` records their SHA-256 hashes.
The attachment's conversion notes identify HBehrens pdc_tool 0.3.4 as the converter.
No recoloring, rasterization or vector changes are applied by Jetebble.

Only the 25 px variants are bundled, appropriate for the Overview header on
Emery. The 50/80 px supplied variants are not needed by this layout.

Mapping uses the saved flight-number prefix (the marketing airline):

| Airline | IATA | ICAO | Resource |
| --- | --- | --- | --- |
| American | AA | AAL | American_airlines_color_25px.pdc |
| Delta | DL | DAL | Delta_airlines_color_25px.pdc |
| Frontier | F9 | FFT | Frontier_airlines_color_25px.pdc |
| Southwest | WN | SWA | Southwest_heart_color_25px.pdc |
| United | UA | UAL | United_globe_color_25px.pdc |

A prefix must be followed by a flight number, optionally after spaces. Unsupported
carriers retain the neutral Overview heading. Codeshare operating-carrier artwork
is not guessed. The moving airplane marker and launcher stabilizer are unchanged.
