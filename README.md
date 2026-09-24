# Ipponboard-Meschede

Weiterentwicklung von Ipponboard für den Wettkampfbetrieb des SSV Meschede Judo.

## Aktueller Stand

Entwicklungsreihe: **V0.x**  
Aktueller Stand: **V0.2.16**  
Erste freigegebene Version: **V1.0**

## Repository-Struktur

- `desktop/` – portable Qt/C++-Windows-App
- `server/` – Server-App, Web-Verwaltung und API
- `docs/` – Projektdokumentation
- `CURRENT_VERSION.txt` – aktueller Entwicklungsstand

## Architektur

- **GitHub** ist die zentrale Quelle für Quellcode und Versionen.
- **Server-Datenbank** enthält Vereine, Mannschaften, Kämpfer, Kampftage und vorbereitete Aufstellungen.
- **Portable App** lädt bei vorhandener Verbindung aktuelle Daten vom Server und arbeitet offline mit dem letzten lokalen Stand weiter.
- **Google Drive** bleibt nur Archiv/Referenzmaterial und ist nicht mehr die primäre Entwicklungsquelle.

## Ziel

- Einzel- und Mannschaftswettkämpfe
- vorbereitete Vereine, Mannschaften und Mannschaftskader
- vollständig nutzbarer Offline-/USB-Betrieb
- Synchronisation mit dem Vereinsserver
- Windows 10/11 als erstes Zielsystem
- später Linux und macOS

## Ursprung und Lizenz

Ipponboard-Meschede basiert auf **Ipponboard 2.4.2** von Florian Mücke:

https://github.com/fmuecke/Ipponboard

Der ursprüngliche Quellcode steht unter der BSD-2-Clause-Lizenz. Die ursprünglichen Copyright- und Lizenzhinweise bleiben erhalten. Siehe [LICENSE.txt](LICENSE.txt).
