# Supplied airline artwork — Overview mockup

These 17 native 25×25 color PDCs are copied byte-for-byte from the user's
`Airline_PDC_all_17_airlines/Color` collection. The 12 added airlines join the
original five, whose files remain unchanged. `manifest.json` records hashes.
No vector geometry or colors have been edited. The attachment's notes identify
HBehrens pdc_tool 0.3.4 as the converter.

Only the 25 px variants are bundled for the Overview's center stack on Emery.
The supplied 50/80 px variants are not needed in this layout.

| Airline | IATA | ICAO | Resource file |
| --- | --- | --- | --- |
| Air China | CA | CCA | Air_China_color_25px.pdc |
| Air France | AF | AFR | Air_France_color_25px.pdc |
| Alaska | AS | ASA | Alaska_airlines_color_25px.pdc |
| American | AA | AAL | American_airlines_color_25px.pdc |
| Cathay Pacific | CX | CPA | Cathay_Pacific_color_25px.pdc |
| China Eastern | MU | CES | China_Eastern_airlines_color_25px.pdc |
| Delta | DL | DAL | Delta_airlines_color_25px.pdc |
| Frontier | F9 | FFT | Frontier_airlines_color_25px.pdc |
| Hawaiian | HA | HAL | Hawaiian_airlines_color_25px.pdc |
| JSX | XE | JSX | JSX_color_25px.pdc |
| KLM | KL | KLM | KLM_color_25px.pdc |
| Korean Air | KE | KAL | Korean_Air_color_25px.pdc |
| LATAM | LA | LAN | LATAM_color_25px.pdc |
| Lufthansa | LH | DLH | Lufthansa_color_25px.pdc |
| Southwest | WN | SWA | Southwest_heart_color_25px.pdc |
| Turkish | TK | THY | Turkish_airlines_color_25px.pdc |
| United | UA | UAL | United_globe_color_25px.pdc |

Mapping uses the saved flight-number prefix (marketing airline); a prefix must
be followed by a digit, optionally after spaces. Codeshare operating carriers
are not guessed. Unsupported carriers use a neutral aircraft in the logo slot.
The airline logo sits below the progress percentage and above the flight number.
The moving aircraft is a separate white/black PDC centered on the progress stroke.
Its 50/80 px source variants are documented in [progress-plane](../progress-plane/README.md).
