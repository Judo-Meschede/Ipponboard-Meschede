# Projektübergabe – Ipponboard-Meschede

Stand: 22.09.2026

## 1. Projektziel

Ipponboard 2.4.2 wird schrittweise zu **Ipponboard-Meschede** weiterentwickelt.

Ziel:
- Einzel- und Mannschaftswettkämpfe
- portable Windows-App, die ohne Installation vom USB-Stick laufen kann
- Windows 10/11 zuerst, später Linux und macOS
- Server-Webverwaltung zur Vorbereitung von Vereinen, Mannschaften, Kämpfern und Wettkämpfen
- Offline-First: Wettkampfbetrieb darf niemals von einer Internetverbindung abhängen
- beim Start lokale Daten sofort laden, parallel Server prüfen, neueren Stand übernehmen
- Änderungen lokal puffern und später synchronisieren
- zentrale Historie auf dem Server

**Grundregel:** Es dürfen keine einmaligen oder laufenden Kosten entstehen.

## 2. Zentrale Architektur

### GitHub
Repository:
`Judo-Meschede/Ipponboard-Meschede`

GitHub ist ab jetzt die zentrale Quelle für:
- Quellcode
- Versionsstände
- spätere Releases
- spätere automatisierte Builds
- perspektivisch kostenlose Signierung

### Server
Server bleibt zentrale Datenquelle für:
- Vereine
- Vereinslogos
- Mannschaften
- Mannschaftskader
- Wettkämpfer
- Wettkämpfe
- vorbereitete Aufstellungen
- Historie

Aktuelle Testadresse:
`https://test-liga.paul-meschede.de`

Verwaltung:
`https://test-liga.paul-meschede.de/verwaltung`

Sync-Snapshot:
`https://test-liga.paul-meschede.de/api/sync/snapshot`

### Portable Windows-App
- lokale Daten/Cache im portablen Ordner
- Online-Sync wenn möglich
- Offlinebetrieb mit letztem Stand
- keine Serverpflicht während eines Wettkampfs

### Google Drive
Nur noch:
- Archiv
- Referenzmaterial
- Notfall-/Projektstände

Nicht mehr primäre Quellcodequelle.

## 3. Aktueller Versionsstand

`CURRENT_VERSION.txt`: **0.1.4**

Desktop:
- `desktop/CMakeLists.txt` → Ipponboard-Meschede V0.1.4

Server:
- aktueller Server-Source liegt in `server/`
- `server/server.js` trägt intern noch `APP_VERSION='0.1.0'`

Regel:
- Entwicklung immer **V0.x**
- erste freigegebene Version **V1.0**

## 4. Aktuelle Startseite

Die alte Ipponboard-Begrüßungs-/Donate-Seite wurde ersetzt.

Verbindliche Designrichtung:
- schwarz / rot
- SSV-Meschede-Judo-Logo
- drei Hauptbereiche:
  - Einzelmodus
  - Mannschaftsmodus
  - Verwaltung
- Statusbereich für Server, Sync, lokale Daten und Version

Wichtige Dateien:
- `desktop/base/SplashScreen.cpp`
- `desktop/base/SplashScreen.ui`
- `desktop/base/images/start_mockup_bg.png`
- `desktop/base/images/meschede_hero.png`
- `desktop/base/images/ssv_meschede_logo.png`
- `desktop/base/images/icon_single.png`
- `desktop/base/images/icon_team.png`
- `desktop/base/images/icon_admin.png`

Die zuletzt freigegebene gestalterische Referenz ist das Mockup hinter `start_mockup_bg.png`.
Keine freie Neuinterpretation ohne Rücksprache.

Aktuell implementiert:
- drei Kacheln
- Einzelmodus öffnet Original-Einzelmodus
- Mannschaftsmodus öffnet Original-Mannschaftsmodus
- Verwaltung öffnet die Server-Verwaltung
- Startseite versucht automatisch Server-Sync
- 3-Sekunden-Timeout
- bei Nichterreichbarkeit lokaler Cache
- Kachelinhalte sind in V0.1.3 dauerhaft sichtbar, nicht nur per Hover

## 5. Build unter Windows

Die Buildumgebung liegt lokal auf dem Windows-Laptop unter ungefähr:
`C:\Users\LokalAdmin_2\Ipponboard-Meschede-Build`

Sie enthält:
- Visual Studio 2022 C++ Build Tools
- Qt 5.15.2 / msvc2019
- CMake 3.31.12
- Boost 1.81
- Python nur für die ursprüngliche Einrichtung

Der bisherige Buildbefehl heißt:
`03_POWERSHELL_WINDOWS_App_bauen.txt`

Er wurde bereits so umgestellt, dass er den Source direkt aus GitHub holt.

Zielausgabe:
`C:\Users\LokalAdmin_2\Ipponboard-Meschede-Build\Ausgabe\Ipponboard-Meschede_V0.x.x_Portable.zip`

Portable Startdatei:
`START_IPPONBOARD_MESCHEDE.cmd`

Aktuell kann auch `app\Ipponboard-Meschede.exe` direkt gestartet werden.

## 6. Smart App Control / Signierung

Auf dem Stand-alone-Firmenlaptop war Windows Smart App Control aktiv und blockierte die neue unsignierte EXE.

Der Nutzer hat Smart App Control auf diesem Entwicklungsrechner deaktiviert.

Wichtig:
- Defender/Firewall wurden nicht deaktiviert.
- Für spätere USB-Nutzung auf beliebigen Windows-10/11-Rechnern soll **keine** Sicherheitsfunktion abgeschaltet werden müssen.
- Signierung bleibt daher ein späterer Pflichtpunkt.
- Nur kostenlose Lösungen sind zulässig.
- GitHub wurde ausdrücklich auch deshalb eingebunden.
- SignPath Foundation wurde als mögliche kostenlose Open-Source-Lösung angesprochen, aber noch nicht eingerichtet.

## 7. Server-Verwaltung

Bereits vorhanden:
- Vereine
- Mannschaften
- Wettkämpfer
- Wettkämpfe
- Gewichtsklassen
- CRUD
- Suche
- Vereinslogo
- Kaderzuordnung
- JSON Import/Export
- persistente Serverdaten
- Sync-Snapshot für die Desktop-App

Servercode:
`server/server.js`

Webverwaltung:
`server/public/verwaltung.html`
`server/public/verwaltung.js`

## 7a. Desktop-Mannschaftsmodus – Stand V0.1.4

Neu umgesetzt:
- Desktop liest den lokalen Offline-Snapshot `data/masterdata.json`.
- Heim-/Gastmannschaften werden aus `teams` geladen und intern über `team.id` referenziert.
- Mannschaftskader werden über `team.fighterIds` aufgelöst.
- Die Kämpferauswahl in beiden Runden schlägt ausschließlich den Kader der gewählten Mannschaft vor.
- Ausgewählte Kämpfer erhalten zusätzlich zur Anzeige den stabilen `fighter.id`.
- Turnier-/Autosave speichert zusätzlich `HostClubId`, `HomeTeamId`, `GuestTeamId` und `FighterId`.
- Alte Turnierdateien ohne diese Felder bleiben lesbar.
- Ohne gültigen Masterdata-Cache fällt der Teammodus auf die bisherige lokale Club-Verwaltung zurück.

Noch offen:
- Windows-Build von V0.1.4 ausführen und praktisch testen.
- Bedienung der Mannschafts-/Kaderauswahl nach Praxistest ggf. verfeinern.
- Danach Synchronisation/Offline-Änderungskonflikte weiter ausbauen.

## 8. Nächster fachlicher Schwerpunkt

Als Nächstes **nicht** zuerst weitere Optik bauen.

Priorität:

1. Mannschaftsverwaltung und Kaderauswahl im Desktop vollständig funktional machen.
2. Datenmodell lokal und Server identisch halten.
3. Vereine/Mannschaften/Kämpfer über IDs referenzieren.
4. Mannschaft für eine Kampfrunde auswählen.
5. gespeicherten Mannschaftskader automatisch vorschlagen.
6. daraus die aktiven Kämpfer für die Gewichtsklassen auswählen/ziehen.
7. komplette Funktion offline nutzbar machen.
8. danach Server-Synchronisation für diese Daten sauber ergänzen.

Wichtig:
Die früher genannten „3 Teams“ waren nur ein Beispiel.
Das System muss mit 2, 3, 10, 100 oder mehr Mannschaften funktionieren.

## 9. Fachliche Grundidee Mannschaftsmodus

Der Nutzer erhält Mannschaftslisten im Vorfeld.

Daher:
- Vereine dauerhaft speichern
- Mannschaften dauerhaft speichern
- je Mannschaft einen Kader speichern
- bei einer Kampfrunde nur Mannschaft auswählen
- Kader wird vorgeschlagen
- daraus aktive Kämpfer den Gewichtsklassen zuordnen
- später vorbereitete Aufstellungen speichern und wiederverwenden

Das ist der zentrale Mehrwert gegenüber dem Original-Ipponboard.

## 10. Original-Ipponboard

Basis:
**Ipponboard 2.4.2** von Florian Mücke

Original:
`https://github.com/fmuecke/Ipponboard`

Lizenz:
BSD-2-Clause

Originale Copyright-/Lizenzhinweise müssen im Source/Distributionspaket erhalten bleiben.

Wichtig aus der Analyse:
- Original-Teammodus ist echtes Qt/C++ und soll funktional möglichst weiterverwendet werden.
- Nicht wieder auf eine Electron/Web-Wettkampfoberfläche zurückwechseln.
- Mattenmonitor/Scoreboard-Verhalten des Originals ist Referenz für Proportionen und Bedienung.
- Rechtsklick/Undo des Originals soll erhalten bleiben.

## 11. Arbeitsregeln

- Keine Kosten.
- Aussagen realistisch einordnen.
- Nie behaupten, etwas sei getestet, wenn nur Source geändert wurde.
- Nie behaupten, etwas liege im Drive/GitHub/Server, bevor es geprüft wurde.
- Nutzer möchte fertige Dateien/ZIPs statt Codeblöcke, wenn Dateien benötigt werden.
- Bei Änderungen kurz nennen:
  - was geändert wurde
  - was unverändert blieb
  - was der Nutzer ausführen muss
- Keine unnötigen Rückfragen zwischen klaren Arbeitsschritten.
- Optik nur nach freigegebener Referenz ändern, keine eigenmächtigen Neuinterpretationen.
- Server darf nicht Voraussetzung für laufenden Wettkampf sein.

## 12. Start eines neuen Chats

Im neuen Chat reicht dieser Satz:

> Arbeite am Projekt Ipponboard-Meschede weiter. Lies zuerst im GitHub-Repository Judo-Meschede/Ipponboard-Meschede die Datei docs/PROJECT_HANDOFF.md und prüfe CURRENT_VERSION.txt sowie den aktuellen main-Stand. Danach direkt beim dort dokumentierten nächsten Schritt weitermachen. Keine alten Google-Drive-Projektstände als Quellcodebasis verwenden.

Danach sollte der neue Chat zuerst GitHub lesen und erst dann Änderungen vornehmen.
