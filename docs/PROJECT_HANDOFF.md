# Projektübergabe – Ipponboard-Meschede

Stand: 24.09.2026

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

`CURRENT_VERSION.txt`: **0.2.9**

Desktop:
- `desktop/CMakeLists.txt` → Ipponboard-Meschede V0.2.9**

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

## 7a. Desktop-Mannschaftsmodus – Stand V0.2.9

Neu umgesetzt:
- Desktop liest den lokalen Offline-Snapshot `data/masterdata.json`.
- Heim-/Gastmannschaften werden aus `teams` geladen und intern über `team.id` referenziert.
- Mannschaftskader werden über `team.fighterIds` aufgelöst.
- Die Kämpferauswahl in beiden Runden schlägt ausschließlich den Kader der gewählten Mannschaft vor.
- Ausgewählte Kämpfer erhalten zusätzlich zur Anzeige den stabilen `fighter.id`.
- Turnier-/Autosave speichert zusätzlich `HostClubId`, `HomeTeamId`, `GuestTeamId` und `FighterId`.
- Alte Turnierdateien ohne diese Felder bleiben lesbar.
- Ohne gültigen Masterdata-Cache fällt der Teammodus auf die bisherige lokale Club-Verwaltung zurück.

Zusätzlich in V0.1.5:
- Die mit dem Original-Ipponboard ausgelieferten Beispielvereine/-mannschaften aus `clubs.config` wurden entfernt.
- Ohne Masterdata-Cache startet die Mannschaftsauswahl leer statt mit den alten süddeutschen Beispieldaten.
- Eigene Vereine/Mannschaften kommen aus der Server-Verwaltung bzw. dem synchronisierten lokalen Masterdata-Cache.

Noch offen:
- Windows-Build von V0.1.5 ausführen und praktisch testen.
- Bedienung der Mannschafts-/Kaderauswahl nach Praxistest ggf. verfeinern.
- Danach Synchronisation/Offline-Änderungskonflikte weiter ausbauen.

## 7b. NWJV-Testdaten – Bezirksliga Männer Arnsberg 2026

Neu in V0.1.6:
- Verwaltung unterstützt bei Wettkämpfern zusätzlich `Pass-Nr.` und `Lizenz-Nr.`.
- Ligakader dürfen Fremdstarter aus anderen Stammvereinen enthalten.
- Stammverein bleibt am Kämpfer gespeichert; Ligazugehörigkeit erfolgt ausschließlich über `team.fighterIds`.
- neuer sicherer Merge-Import `POST /api/masterdata/merge`, der vorhandene andere Stammdaten nicht komplett ersetzt.
- Import-Schaltfläche `NWJV Bezirksliga Arnsberg 2026 laden` unter Import / Export.
- Testdatensatz: `server/public/data/nwjv-bezirksliga-arnsberg-2026.json`.
- enthalten: 32 Vereine, 9 Ligamannschaften, 261 Kämpfer/Kaderzuordnungen, 4 Kampftage, 36 Begegnungen.
- Kampfplan basiert auf der offiziellen NWJV-Seite Bezirksliga Männer Arnsberg 2026.
- Vereinswebseiten wurden für die neun Ligavereine recherchiert.
- eindeutig zuordenbare offizielle Logos sind aktuell für VfL Gevelsberg, 1. JJJC Hattingen und DSC Wanne-Eickel hinterlegt; unsichere Logo-Treffer wurden nicht übernommen.
- Gevelsberg ist als unvollständig markiert: hochgeladen wurden nur Seiten 1 und 2, obwohl Seite 2 als „Seite 2 von 3“ gekennzeichnet ist.
- einzelne in den Ausgangslisten leere Passnummern bleiben bewusst leer.
- eine nicht als Staatsangehörigkeit interpretierbare Quellenangabe wurde nicht als Nationalität übernommen.

Noch nicht durchgeführt:
- Serverstand V0.1.6 auf `test-liga.paul-meschede.de` ausrollen.
- NWJV-Testimport dort ausführen.
- Desktop V0.1.6 unter Windows bauen und mit den importierten Mannschaften praktisch testen.

## 7c. Windows-Sync-Fix V0.1.7

Aus Praxistest V0.1.5:
- Startseite zeigte trotz erreichbarem Server `Offline - noch kein lokaler Datenstand`.
- `Jetzt aktualisieren` war optisch ungünstig und lieferte keine verwertbare Fehlermeldung.
- Ursache im Source: Windows-Portable-Paket hatte Qt5Network, aber keine OpenSSL-Laufzeit. Der HTTPS-Abruf konnte dadurch auf Windows scheitern.

In V0.1.7 geändert:
- Windows lädt den Masterdata-Snapshot über native Windows-WinHTTP-API statt über Qt/OpenSSL.
- keine zusätzliche SSL/OpenSSL-Installation erforderlich.
- Zertifikatsprüfung bleibt über Windows/WinHTTP aktiv.
- Netzwerkfehler werden auf der Startseite bzw. im Tooltip konkret angezeigt.
- Sync-Schaltfläche neu gestaltet und größer/sauberer positioniert.
- Status nach Erfolg: `Server verbunden` und `Daten aktuell`.
- `winhttp` wird im Windows-Build verlinkt.

Noch nicht durchgeführt:
- Windows-Build V0.1.7.
- Praxistest des WinHTTP-Syncs gegen den bereits befüllten Testserver.

Für später vorgemerkt:
- Subdomain `ipponboard.paul-meschede.de` statt/zusätzlich zu `test-liga.paul-meschede.de` einrichten.

## 7d. Offizielle NWJV-Wettkampflisten – V0.1.8

Vom Nutzer bereitgestellte offizielle Excel-Vordrucke:
- Mannschaftswettkampfliste 5 Kämpfe Hin-/Rückrunde
- Mannschaftswettkampfliste 7 Kämpfe Hin-/Rückrunde

Umgesetzt:
- `desktop/base/templates/list_output_nwjv_5_hinundrueck.html`
- `desktop/base/templates/list_output_nwjv_7_hinundrueck.html`
- beide erscheinen automatisch in der Modusverwaltung unter `Vorlage`.
- offizieller Aufbau, Beschriftungen, Blau/Weiß-Darstellung, Hin-/Rückrunde, Unterbewertung, Summenfelder, Unterschriftsfelder und NWJV-Logo wurden übernommen.
- Für diese Vorlagen gibt es einen eigenen HTML-Zeilenexport in der offiziellen Wertungsreihenfolge `Yuko – Waza-ari – Ippon – Shido – Hansoku-make – Sieg – Unterbewertung`.
- bisherige Ipponboard-Druckvorlagen und deren Reihenfolge bleiben unverändert.
- zusätzliche Platzhalter für Rückrundensummen: `SECOND_WINS_*` und `SECOND_SCORE_*`.

Wichtig:
- Die beiden Excel-Originaldateien wurden nicht verändert.
- Noch kein Windows-Build/Praxistest der HTML-Druckvorschau durchgeführt.
- Die smarte Kader-/Freitextauswahl direkt in den Kämpferzellen ist weiterhin der nächste offene Funktionsschritt.

## 7e. Smarte Kämpferauswahl V0.1.9

Umgesetzt:
- Kämpferzellen in beiden Runden sind editierbare Dropdowns.
- Heimseite erhält ausschließlich den Kader der aktuell gewählten Heimmannschaft.
- Gastseite erhält ausschließlich den Kader der aktuell gewählten Gastmannschaft.
- Tippen filtert die Vorschläge unabhängig von Groß-/Kleinschreibung und auch innerhalb des Namens.
- Auswahl eines exakten Kadernamens speichert zusätzlich die stabile `fighter.id`.
- beliebiger Freitext bleibt erlaubt.
- bei Freitext wird eine eventuell alte `fighter.id` ausdrücklich geleert, damit keine falsche Person verknüpft bleibt.
- gilt identisch für Hin- und Rückrunde.
- Wechsel der Mannschaft ändert die bereits eingetragenen Namen nicht automatisch; lediglich die Vorschlagsliste wird auf den neuen Kader umgestellt.
- Modusauswahl/Gewichtsklassen werden bewusst nicht automatisch verändert. Der Nutzer pflegt die Modi manuell in der Anwendung.

Noch nicht durchgeführt:
- Windows-Build V0.1.9.
- Praxistest der editierbaren Kaderauswahl.

## 7f. Globale serverseitige Modusverwaltung – V0.2.1

Fachliche Korrektur:
- Wettkampfmodi sind **keine persönlichen Benutzereinstellungen**, sondern globale Systemeinstellungen.
- Der in V0.2.0 kurzzeitig eingebaute benutzerspezifische AppConfig-Ansatz wurde wieder entfernt.

Ab V0.2.1:
- Server-Masterdata enthält die globale Sammlung `tournamentModes`.
- Server-Endpunkt `PUT /api/masterdata/tournamentModes` ersetzt die komplette globale Modusliste atomar.
- Der Sync-Snapshot enthält die globalen Modi automatisch als Teil von `masterdata`.
- Desktop lädt beim Start zuerst den synchronisierten Masterdata-Cache und verwendet daraus die globalen Modi.
- Nur wenn der Server/Cache noch gar keine globalen Modi enthält, dient die mitgelieferte `TournamentModes.ini` als Erststart-/Notfall-Fallback.
- Änderungen unter `Modi verwalten` werden erst übernommen, wenn der Server sie erfolgreich gespeichert hat.
- Nach erfolgreichem Serverspeichern aktualisiert die App zusätzlich ihren lokalen Masterdata-Cache. Damit bleibt der zuletzt synchronisierte globale Stand offline verwendbar.
- Ein neuer Portable-Build über `03` kann die globalen Modi nicht überschreiben, weil der produktive Stand vom Server bzw. dessen lokalem Sync-Cache kommt.
- Neue Rechner/Nutzer erhalten beim Synchronisieren dieselben Modi.

Wichtig:
- Server V0.2.1 muss vor der Nutzung dieser globalen Modusverwaltung ausgerollt werden.
- Bereits verlorene, ausschließlich lokal vorhandene Modusanpassungen können dadurch nicht rückwirkend rekonstruiert werden.
- Nach dem Rollout müssen die gewünschten Modi einmal korrekt angelegt/angepasst und gespeichert werden. Danach sind sie global.

Noch nicht durchgeführt:
- Server V0.2.1 auf dem Testserver ausgerollt.
- Windows-Build V0.2.1.
- Praxistest: Modus auf Rechner A ändern → Server speichern → frischer Build/Rechner B synchronisiert denselben Stand.

## 7g. Server-Verwaltung Wettkampfmodi – V0.2.2

Umgesetzt:
- neuer Reiter `Wettkampfmodi` unter `/verwaltung`.
- globale Modi können serverseitig angelegt, bearbeitet und gelöscht werden.
- Felder: Titel, Untertitel, Gewichtsklassen, Vorlage, Rundenzahl, Kampfzeit, doppelte Gewichtsklassen, Regelwerk, abweichende Kampfzeiten und Optionen.
- Vorlagenauswahl enthält auch die beiden offiziellen NWJV-Vorlagen.
- Webverwaltung arbeitet direkt auf `masterdata.tournamentModes`.
- Änderungen erhöhen die Masterdata-Revision und landen damit automatisch im Sync-Snapshot.
- Desktop V0.2.2 lädt diese Modi beim Start aus dem synchronisierten Masterdata-Cache.
- Desktop-`Modi verwalten` kann weiterhin die komplette globale Modusliste an den Server zurückschreiben.

Erforderliche Schritte nach diesem Stand:
1. Linux-Server: Schritt 01 ausführen.
2. Linux-Server: Schritt 02 ausführen.
3. Windows: `03_POWERSHELL_WINDOWS_App_bauen.txt` ausführen.
4. Browser: `https://test-liga.paul-meschede.de/verwaltung` → `Wettkampfmodi` prüfen/anlegen.
5. Desktop V0.2.2 starten und Synchronisation prüfen.

Noch nicht durchgeführt:
- Server V0.2.2 ausgerollt.
- Windows-Build V0.2.2.
- End-to-End-Praxistest Servermodus → Sync → Desktop.

## 7h. Regelwerk-Verwaltung – V0.2.3

Umgesetzt:
- Reiter `Regelwerke` in der Server-Verwaltung.
- vorhandene Ipponboard-Regelwerke werden beim ersten Serverstart als zentrale Regelprofile bereitgestellt.
- Regelwerk kann zentral gepflegt werden: Yuko vorhanden, Waza-ari-awasete-Ippon, Golden-Score-Verhalten, Shido-Verhalten, maximale Shido/Waza-ari, Osaekomi-Zeiten, Unterbewertungen und Wertungsbegriffe.
- Wettkampfmodus wählt das Regelwerk aus einer Liste statt über Freitext.
- Kampfzeit im Wettkampfmodus wird im Web ausschließlich in Minuten angezeigt/eingegeben; intern bleiben Sekunden gespeichert.
- Fehler im Button `Änderungen speichern` behoben: Formularfelder werden jetzt korrekt vollständig gesammelt.
- Desktop registriert synchronisierte Regelprofile dynamisch in der bestehenden Rules-Engine.
- Osaekomi-Schwellen, Yuko-Verfügbarkeit, Waza-ari-awasete-Ippon, Shido-Grenzen, Golden-Score-Verhalten und Unterbewertung werden dadurch aus dem zentralen Regelprofil verwendet.
- Ippon/Waza-ari/Yuko-Begriffe werden im Mattenmonitor aus dem Regelprofil übernommen.
- offizielle NWJV-Druckvorlagen bleiben unverändert.

Erforderliche Schritte:
1. Linux-Server aktuellen GitHub-Stand installieren.
2. Browser `/verwaltung` öffnen und `Regelwerke` sowie `Wettkampfmodi` prüfen.
3. Windows `03_POWERSHELL_WINDOWS_App_bauen.txt` ausführen.
4. V0.2.3 starten und synchronisieren.
5. Mannschaftsmodus praktisch testen.

Noch nicht durchgeführt:
- Server V0.2.3 ausgerollt.
- Windows-Build V0.2.3.
- End-to-End-Praxistest der dynamischen Regeln.

## 7i. Kämpferauswahl-Fix – V0.2.4

Praxistest V0.2.3:
- Kämpfer-Dropdown erschien erst nach Doppelklick.
- Gastspalten boten fälschlich den Heimkader an.

In V0.2.4 geändert:
- jede der vier Kämpfer-Spalten besitzt einen dauerhaft eindeutig zugeordneten Delegate: Heim Hinrunde, Heim Rückrunde, Gast Hinrunde, Gast Rückrunde.
- Heim-Delegates erhalten ausschließlich `m_FighterNamesHome/m_FighterIdsHome`.
- Gast-Delegates erhalten ausschließlich `m_FighterNamesGuest/m_FighterIdsGuest`.
- vor jedem Öffnen einer Kämpferzelle werden die Kader aus den aktuell ausgewählten Mannschaften nochmals frisch zugeordnet.
- einfacher Klick auf eine Kämpferzelle startet direkt den Editor und öffnet das Dropdown.
- Freitext und Suchfilter bleiben erhalten.

Erforderliche Schritte:
1. Kein Serverupdate erforderlich.
2. Windows `03_POWERSHELL_WINDOWS_App_bauen.txt` ausführen.
3. V0.2.4 starten und synchronisieren.
4. unterschiedliche Heim-/Gastmannschaften auswählen und beide Kader in Hin- und Rückrunde prüfen.

Noch nicht unter Windows gebaut/getestet.

## 7j. Live-Kaderauflösung – V0.2.5

Praxistest V0.2.4 zeigte weiterhin auf Heim- und Gastseite den Heimkader.

V0.2.5:
- Kämpfereditor verwendet keine vorher in den Delegate kopierten Kaderlisten mehr.
- Beim Öffnen jeder einzelnen Namenszelle wird der Kader live ermittelt.
- Spalte `eCol_name1` liest zwingend die aktuell ausgewählte Heimmannschaft.
- Spalte `eCol_name2` liest zwingend die aktuell ausgewählte Gastmannschaft.
- Fighter-ID wird aus dem tatsächlich geöffneten ComboBox-Eintrag übernommen, nicht mehr aus einer eventuell veralteten Delegate-ID-Liste.
- Ein-Klick-Öffnung bleibt erhalten.
- Testdaten geprüft: TV Wickede und JC Samurai Schwelm besitzen im Seed unterschiedliche IDs und unterschiedliche Kader.

Erforderliche Schritte:
1. Kein Serverupdate erforderlich.
2. Windows `03_POWERSHELL_WINDOWS_App_bauen.txt` ausführen.
3. V0.2.5 starten.
4. Heim TV Wickede / Gast JC Samurai Schwelm wählen.
5. Erwartung Heim: u.a. Hussein Alasaad, Matthias Gedig.
6. Erwartung Gast: u.a. Sean-Philip Baltzer, Stefan Bende, Felix Tietzsch.

Noch nicht unter Windows praktisch getestet.

## 7k. Nicht aufgestellt – V0.2.6

Ausgangspunkt ist ausdrücklich der praktisch geprüfte und vom Nutzer freigegebene Stand V0.2.5.

Neu:
- In jeder Kämpferauswahl steht `_n.A.` als erster Eintrag.
- `_n.A.` bedeutet „nicht aufgestellt“.
- Die Auswahl ist für Heim und Gast sowie getrennt je Hin-/Rückrunde und Gewichtsklasse möglich.
- Für `_n.A.` wird bewusst keine `fighter.id` gespeichert.
- Die Wertungslogik für Fälle mit einem oder zwei nicht aufgestellten Kämpfern ist noch nicht festgelegt und wurde bewusst nicht verändert.

Erforderliche Schritte:
1. Kein Serverupdate.
2. Windows `03_POWERSHELL_WINDOWS_App_bauen.txt` ausführen.
3. V0.2.6 starten.
4. Prüfen, ob `_n.A.` in jeder Kämpferauswahl ganz oben erscheint und unabhängig für Heim/Gast gewählt werden kann.

Noch nicht unter Windows praktisch getestet.

## 7l. Pixelgenaue NWJV-5er-Druckliste – V0.2.7

Auf ausdrückliche Vorgabe des Nutzers wurde die Druckausgabe für die offizielle
NWJV-Mannschaftswettkampfliste mit 5 Kämpfen Hin-/Rückrunde neu umgesetzt.

Verbindliche Referenz:
- vom Nutzer bereitgestelltes offizielles PDF
  `mannschaftswettkampfliste_5_hinundrueck (1).pdf`
- A4 quer
- Originalgeometrie 842 x 595 PDF-Punkte
- keine freie Gestaltung und keine HTML-Tabellen-Nachbildung

Technische Umsetzung:
- nur die 5er-Vorlage `list_output_nwjv_5_hinundrueck.html` wird beim Drucken/PDF-Export erkannt.
- Druck erfolgt direkt per QPainter in dem festen Koordinatensystem der offiziellen Vorlage.
- Linien, Spalten, Überschriften, WEISS/BLAU, Summenbereiche, Unterschriftslinien und Hinweis sind fest positioniert.
- NWJV-Logo wird aus der bereits vorhandenen offiziellen Vorlagengrafik übernommen.
- variable Daten werden positionsgenau eingetragen: Art, Ort, Datum, Mannschaften, Gewichtsklassen, Judoka, Einzelwertungen, Sieg, Unterbewertung, Kampfzeit und Summen.
- Druckvorschau und PDF-Export verwenden für diese Vorlage Full-Page-A4-Querformat ohne HTML-Skalierung.
- der zusätzliche Ipponboard-Copyright-Footer wird bei dieser exakten Druckausgabe nicht ergänzt.
- die 7er-Vorlage bleibt vollständig unverändert.

Erforderliche Schritte:
1. Kein Serverupdate.
2. Windows `03_POWERSHELL_WINDOWS_App_bauen.txt` ausführen.
3. V0.2.7 starten.
4. 5er-Wettkampfmodus auswählen.
5. Druckvorschau/PDF-Export öffnen und direkt gegen das offizielle Referenz-PDF vergleichen.
6. Noch nicht unter Windows praktisch getestet.

## 7m. NWJV-5er-Druckabgleich – V0.2.8

Praxistest V0.2.7 gegen das offizielle 5er-PDF zeigte noch sichtbare Abweichungen.
V0.2.8 korrigiert ausschließlich die 5er-Druckausgabe:
- offizielles NWJV-Logo wird mit Original-PDF-Boundingbox positioniert.
- Portable-Build enthält nun das Qt-JPEG-Plugin, damit das bereits eingebettete offizielle Logo sicher geladen wird.
- Hauptlinien werden stärker an die Original-PDF-Linienstärke angeglichen.
- feste Kopftexte, WEISS/BLAU, Verbandsbezeichnung, Spaltenüberschriften und Fußtexte werden nach den aus dem Original-PDF ausgelesenen Positionen/Schriftgrößen korrigiert.
- 7er-Vorlage und Wettkampflogik unverändert.

Noch nicht unter Windows praktisch getestet.

## 7n. NWJV-5er-Kopfzeile – V0.2.9

Praxistest V0.2.8 zeigte noch vier sichtbare Abweichungen zur offiziellen Vorlage.
V0.2.9 korrigiert ausschließlich diese Punkte:
- die internen Wertungsspalten-Linien laufen in der Team-Kopfzeile nicht mehr durch die Gruppen `+`, `-` und `=`.
- Vereinsnamen werden in der Teamzeile etwas höher und kräftiger dargestellt.
- `Hansoku-make` wird in der vertikalen Beschriftung zweizeilig gesetzt.
- `Unterbewertung` wird in der vertikalen Beschriftung zweizeilig gesetzt.
- 7er-Vorlage, Wettkampflogik und Server bleiben unverändert.

Erforderliche Schritte:
1. Kein Serverupdate.
2. Windows `03_POWERSHELL_WINDOWS_App_bauen.txt` ausführen.
3. V0.2.9 starten.
4. 5er-Druckvorschau/PDF erneut direkt gegen die offizielle Referenz prüfen.

Noch nicht unter Windows praktisch getestet.

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

- Nach dem Einlesen des Projektstands **nicht automatisch mit der nächsten Umsetzung beginnen**.
- Zuerst den aktuellen Stand und den im Handoff genannten nächsten sinnvollen Schritt kurz zusammenfassen.
- Danach den Nutzer fragen, **was als Nächstes gemacht werden soll**.
- Änderungen an Code, Server, Buildskripten oder Daten erst nach ausdrücklicher Freigabe des Nutzers beginnen.


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

> Arbeite am Projekt Ipponboard-Meschede weiter. Lies zuerst im GitHub-Repository Judo-Meschede/Ipponboard-Meschede die Datei docs/PROJECT_HANDOFF.md und prüfe CURRENT_VERSION.txt sowie den aktuellen main-Stand. Fasse mir danach den aktuellen Stand und den dort dokumentierten nächsten sinnvollen Schritt kurz zusammen. **Noch nichts umsetzen oder ändern. Frage mich anschließend, was ich als Nächstes machen möchte.** Keine alten Google-Drive-Projektstände als Quellcodebasis verwenden.

Danach sollte der neue Chat zuerst GitHub lesen und erst dann Änderungen vornehmen.
