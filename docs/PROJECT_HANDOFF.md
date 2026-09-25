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

`CURRENT_VERSION.txt`: **0.2.21**

Desktop:
- `desktop/CMakeLists.txt` → Ipponboard-Meschede V0.2.21**

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

## 5a. Server-Update – verbindlicher GitHub-Workflow

Die früher verwendeten SSH-Hilfsdateien, die einen Projektstand aus Google Drive nach `Downloads` kopieren, sind für Ipponboard-Meschede **veraltet und dürfen nicht mehr verwendet werden**.

Verbindliche Dateien:
- `docs/project/01_SSH_GITHUB_Stand_aktualisieren.txt`
- `docs/project/02_SSH_SERVER_Stand_installieren.txt`

Ablauf:
1. `01` auf dem Linux-Server ausführen. Dadurch wird `main` aus `Judo-Meschede/Ipponboard-Meschede` nach `~/Ipponboard-Meschede-Source` geklont bzw. exakt auf `origin/main` aktualisiert.
2. `02` ausführen. Dadurch wird `server/01_INSTALLIEREN.sh test` aus genau diesem GitHub-Stand gestartet.
3. Der Installer deployt nach `/opt/ipponboard-meschede-test`, verwendet persistente Daten unter `/var/lib/ipponboard-meschede-test`, startet `ipponboard-meschede-test.service` auf Port 3011 und führt einen Healthcheck aus.
4. `02` prüft anschließend zusätzlich den lokalen und öffentlichen Health-Endpunkt.

Wichtig:
- Google Drive bleibt Archiv/Referenz und ist keine Quelle für Serverupdates.
- Wenn bei einer Änderung `01/02` erforderlich sind, muss ausdrücklich auf diese aktuellen GitHub-basierten Dateien verwiesen werden.
- Keine alten Dateien namens sinngemäß `01_SSH_Projektstand_nach_Downloads.txt` mehr für dieses Projekt verwenden.

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

## 7a. Desktop-Mannschaftsmodus – Stand V0.2.21

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

## 7o. Vereinsnamen im NWJV-Kopf – V0.2.10

Praxistest V0.2.9 zeigte, dass die Vereinsnamen noch in der schmalen Teamzeile über `Judoka` standen.

V0.2.10:
- Vereinsnamen wurden vollständig aus dieser Teamzeile entfernt.
- Heimverein steht nun groß und fett im großen linken Kopffeld neben `WEISS`.
- Gastverein steht nun groß und fett im großen rechten Kopffeld neben `BLAU`.
- lange Vereinsnamen dürfen dort automatisch umbrechen.
- `+ / - / =`, zweizeilige Wertungsüberschriften, 7er-Vorlage und Wettkampflogik bleiben unverändert.

Erforderliche Schritte:
1. Kein Serverupdate.
2. Windows `03_POWERSHELL_WINDOWS_App_bauen.txt` ausführen.
3. V0.2.10 starten.
4. 5er-Druckvorschau/PDF prüfen.

Noch nicht unter Windows praktisch getestet.

## 7p. Verbindliches Datenmodell Kampftage / Matten / Wiederaufnahme

Ziel:
- Kampftage werden zentral auf dem Server angelegt.
- Ein Kampftag enthält mindestens Datum, Name, Ausrichter, Ort, teilnehmende Mannschaften und optional einen Wettkampfmodus.
- Die Desktop-App kann einen Kampftag laden und beschränkt danach die Mannschaftsauswahl auf dessen Teilnehmer.
- Mehrere Rechner dürfen parallel auf mehreren Matten arbeiten.
- Eine Matte ist nicht dauerhaft an einen Rechner gebunden.

Objekte:

### competitionDay
- `id`
- Datum
- Name
- `hostClubId`
- Ort
- `teamIds[]`
- optional `tournamentModeId`
- Status

### mat
- eindeutige ID innerhalb eines Kampftags
- Bezeichnung, z. B. `Matte 1` bis `Matte 4`
- keine feste Hardwarebindung

### terminal
- dauerhafte zufällige `terminalId` je App-/USB-System
- optional frei lesbarer Terminalname
- dient ausschließlich zur Nachvollziehbarkeit und Mattenübernahme

### matSession
- `competitionDayId`
- `matId`
- aktuell verwendete `terminalId`
- Zeitpunkt der Übernahme
- letzte abgeschlossene Begegnung / letzter abgeschlossener Kampf
- bei Rechnerwechsel kann ein anderes Terminal dieselbe Matte ausdrücklich übernehmen

### encounter
- eindeutige ID
- `competitionDayId`
- Heimteam-ID
- Gastteam-ID
- Matten-ID
- Status
- aktuelle Runde bzw. zuletzt abgeschlossener Kampf

### fight
- eindeutige ID je Begegnung / Runde / Gewichtsklasse
- beide Fighter-IDs
- vollständige Wertungen
- Endkampfzeit
- Ergebnis
- Revision
- `updatedAt`
- `terminalId`
- Status abgeschlossen oder korrigiert

Verbindliche Persistenzregel:
- Der Zustand eines gerade laufenden Einzelkampfs wird **nicht** lokal gespeichert.
- Der Zustand eines gerade laufenden Einzelkampfs wird **nicht** an den Server übertragen.
- Bei Absturz während eines laufenden Kampfs muss dieser Kampf manuell wieder eingestellt werden.
- Erst wenn ein Einzelkampf abgeschlossen wurde, wird dessen vollständiger Endstand lokal gespeichert.
- Wird ein bereits abgeschlossener Einzelkampf korrigiert, wird die korrigierte Fassung erneut lokal gespeichert.
- Bei bestehender Onlineverbindung wird ein abgeschlossener oder korrigierter Einzelkampf anschließend sofort an den Server synchronisiert.
- Ohne Verbindung bleibt nur dieser bereits abgeschlossene oder korrigierte Kampf in der lokalen `syncQueue` und wird später übertragen.
- Einträge werden erst aus der `syncQueue` entfernt, wenn der Server die Revision bestätigt hat.
- Laufende Kampfzeit, laufende Osaekomi-Zeit, Zwischenwertungen eines noch nicht abgeschlossenen Einzelkampfs und Undo-Zustände eines laufenden Kampfs gehören ausdrücklich **nicht** in Persistenz oder Sync.

Wiederaufnahme nach Neustart:
- Die App startet mit demselben Kampftag, derselben Matte und derselben Begegnung wie zuletzt.
- Wiederhergestellt werden ausschließlich bereits abgeschlossene Kämpfe und die daraus resultierende Listenansicht.
- Ein beim Absturz noch laufender Einzelkampf startet nicht aus einem Zwischenstand heraus und muss manuell neu eingestellt werden.
- Dadurch bleibt die Wiederaufnahme robust, ohne laufende Wettkampfzustände serverseitig oder lokal zu protokollieren.

Mattenübernahme:
- Beim Start eines Kampftags wählt der Nutzer die Matte frei.
- Ist die Matte bereits einem anderen Terminal zugeordnet, zeigt die App einen klaren Hinweis.
- Über `Matte übernehmen` kann ein Ersatzrechner die Zuordnung übernehmen.
- Die Zuordnung dient der Vermeidung paralleler Bearbeitung, darf aber einen Rechnerwechsel im Notfall nicht verhindern.

Architekturregel:
- Die bisherige globale `competition-state.json` ist für parallele Matten nicht als zukünftiges Sitzungsmodell geeignet.
- Kampftage, Matten, abgeschlossene Kämpfe und Sync-Zustände müssen getrennt und ID-basiert verwaltet werden.
- Der Server dokumentiert keine Live-Zustände eines gerade laufenden Einzelkampfs.

## 7q. Server-Kampftage – V0.2.11

Umgesetzt:
- neue eigenständige Masterdata-Sammlung `competitionDays`; bestehende `competitions` bleibt unverändert.
- Kampftage können in der Server-Verwaltung angelegt, bearbeitet und gelöscht werden.
- Felder: Name, Datum, Ausrichter, Ort, teilnehmende Mannschaften, optionaler Wettkampfmodus, Anzahl Matten, Status und Bemerkungen.
- Teilnehmer werden ausschließlich über stabile `teamIds` referenziert.
- Ausrichter wird über `hostClubId` referenziert.
- Matten werden serverseitig als stabile Einträge `mat-1`, `mat-2` usw. erzeugt und zusätzlich mit `matCount` gespeichert.
- zulässige Mattenzahl wird serverseitig auf 1 bis 20 begrenzt.
- Kampftage werden automatisch im Masterdata-Snapshot an die Desktop-App ausgeliefert.
- Import/Merge/Export berücksichtigen `competitionDays`.
- Beim Löschen von Mannschaften oder Vereinen werden verwaiste Kampftag-Referenzen bereinigt.
- Server-Version auf V0.2.11 angehoben.
- noch keine Desktop-Auswahl „Kampftag laden“ umgesetzt.
- noch keine Matten-/Terminal-Sitzung und noch kein Ergebnis-Sync umgesetzt.
- laufende Einzelkämpfe werden weiterhin ausdrücklich nicht als neues Kampftag-Modell persistiert oder synchronisiert.

Erforderliche Schritte:
1. Linux-Testserver aktuellen GitHub-Stand installieren.
2. Serverdienst neu starten.
3. Browser `/verwaltung` öffnen und Reiter `Kampftage` praktisch testen.
4. Kein Windows-Build für diesen Schritt erforderlich.

Noch nicht auf dem Testserver ausgerollt oder praktisch getestet.

## 7r. Kampftag im Desktop laden – V0.2.12

Umgesetzt:
- `Turnier → Laden…` ist jetzt der zentrale Einstieg.
- Dort kann zwischen `Kampftag` und `Lokale Turnierdatei` gewählt werden.
- `Kampftag` liest die synchronisierte lokale Masterdata-Datei erneut ein und zeigt die vorhandenen `competitionDays`.
- bei mehreren Matten wird anschließend die Matte frei gewählt; bei genau einer Matte wird sie automatisch übernommen.
- nach dem Laden werden Datum, Ausrichter, Ort und optional der Wettkampfmodus aus dem Kampftag übernommen.
- Heim-/Gast-Auswahl wird auf die in `teamIds` hinterlegten Mannschaften des Kampftags begrenzt.
- Kaderauswahl bleibt weiterhin ID-basiert auf den gewählten Mannschaften.
- Fenstertitel zeigt nach dem Laden Kampftag und gewählte Matte.
- der bisherige lokale JSON-Dateilader bleibt unverändert nutzbar; beim Laden einer lokalen Datei wird ein eventuell aktiver Kampftag-Filter aufgehoben.
- noch keine serverseitige Mattenbelegung/Terminal-Sperre.
- noch kein Persistenz-/Sync-Modell für abgeschlossene Einzelkämpfe; das folgt separat.
- laufende Einzelkämpfe werden weiterhin nicht persistiert oder synchronisiert.

Erforderliche Schritte:
1. Server mindestens auf V0.2.11 mit `competitionDays` betreiben und dort einen Kampftag anlegen.
2. Windows `03_POWERSHELL_WINDOWS_App_bauen.txt` ausführen.
3. V0.2.12 starten und Serverdaten synchronisieren lassen.
4. Mannschaftsmodus → `Turnier → Laden… → Kampftag`.
5. Prüfen: Kampftag/Matte wählen, Datum/Ort/Ausrichter prüfen und kontrollieren, dass nur die Teilnehmermannschaften angeboten werden.

Noch nicht unter Windows gebaut oder praktisch getestet.

## 7s. Kampftage vereinheitlicht + XLSX-Pflege – V0.2.13

Datenmodell:
- Die parallele Sammlung `competitions` wird nicht mehr als aktive Struktur verwendet.
- `competitionDays` ist die verbindliche Struktur für Kampftage.
- Beim Serverstart werden vorhandene alte `competitions` automatisch verlustfrei nach `competitionDays` migriert und anschließend aus dem Masterdata-Dokument entfernt.
- vorhandene Felder wie `matches`, Liga, Saison und Kampftag-Nr. bleiben dabei erhalten.
- Teilnehmermannschaften werden bei der Migration aus den vorhandenen Begegnungen abgeleitet.
- der Verwaltungsreiter `Wettkämpfe` entfällt.
- der NWJV-Testdatensatz wurde ebenfalls auf `competitionDays` umgestellt.

XLSX-Pflege unter `Verwaltung → Import / Export`:
- separater XLSX-Export und XLSX-Rückimport für:
  1. Vereine
  2. Mannschaften
  3. Wettkämpfer
  4. Kampftage
  5. Gewichtsklassen
- jede Datei enthält ein Datenblatt und ein Blatt `Hinweise`.
- bestehende IDs bleiben als technische Schlüssel in der Datei erhalten.
- neue Zeilen dürfen ohne ID angelegt werden; der Server erzeugt dann beim Import eine neue ID.
- `Löschen = JA` löscht einen bestehenden Datensatz gezielt.
- nicht in der XLSX aufgeführte Datensätze bleiben bestehen.
- bei Mannschaften werden Kader-IDs, Passnummern und Namen zur lokalen Pflege exportiert.
- bei Kampftagen werden Ausrichter, Teilnehmer, Modus, Mattenzahl, Liga/Saison und weitere Felder exportiert.
- verschachtelte bestehende Daten wie Begegnungen eines Kampftags bleiben beim XLSX-Update erhalten.
- XLSX-Erzeugung und -Einlesen erfolgt serverseitig mit ExcelJS.
- `server/01_INSTALLIEREN.sh` installiert deshalb ab V0.2.13 die benötigten npm-Abhängigkeiten.

Erforderliche Schritte:
1. auf dem Linux-Testserver aktuelle `01_SSH_GITHUB_Stand_aktualisieren.txt` ausführen.
2. danach `02_SSH_SERVER_Stand_installieren.txt` ausführen.
3. `/verwaltung` neu laden.
4. prüfen, dass der Reiter `Wettkämpfe` verschwunden ist und alte Einträge unter `Kampftage` erscheinen.
5. unter `Import / Export` alle fünf XLSX-Dateien testweise herunterladen.
6. eine kleine Änderung in einer XLSX durchführen und wieder importieren; danach Daten im jeweiligen Verwaltungsreiter kontrollieren.

Noch nicht auf dem Testserver ausgerollt oder praktisch mit Excel getestet.

## 7t. Kämpferauswahl pro Runde eindeutig – V0.2.14

Umgesetzt:
- ein Wettkämpfer kann innerhalb derselben Mannschaft und derselben Runde nur noch einmal ausgewählt werden.
- sobald ein Kämpfer in einer Zeile gewählt wurde, verschwindet er aus den Auswahllisten der übrigen Zeilen dieser Runde.
- Heim- und Gastmannschaft werden getrennt behandelt.
- Hin- und Rückrunde werden getrennt behandelt; ein Kämpfer darf in der Rückrunde erneut eingesetzt werden.
- die aktuell bearbeitete Zeile behält ihren bereits gewählten Kämpfer in der Auswahl.
- `_n.A.` bleibt absichtlich mehrfach verwendbar.

Erforderliche Schritte:
1. Windows `03_POWERSHELL_WINDOWS_App_bauen.txt` ausführen.
2. V0.2.14 starten.
3. in einer Runde einen Kämpfer auswählen und prüfen, dass er in den anderen Gewichtsklassen derselben Mannschaft nicht mehr angeboten wird.
4. Rückrunde prüfen: dort muss derselbe Kämpfer wieder auswählbar sein.

Kein Serverupdate für diese Änderung erforderlich.

## 7u. Große Mehrfachauswahl in der Server-Verwaltung – V0.2.15

Umgesetzt:
- eingebettete Scroll-Checkboxlisten wurden aus den Detailformularen entfernt.
- `Mannschaften → Kader` ist jetzt ein Button `Kader auswählen`.
- `Kampftage → Teilnehmende Mannschaften` ist jetzt ein Button `Mannschaften auswählen`.
- beide öffnen denselben großen modalen Auswahl-Dialog.
- Dialog enthält Suche, Anzahl der ausgewählten Einträge, `Sichtbare auswählen`, `Auswahl aufheben`, `Abbrechen` und `Auswahl übernehmen`.
- Kaderliste zeigt Name, Verein, Passnummer und Nationalität.
- Mannschaftsliste zeigt Mannschaft, Verein, Kategorie/Liga und Saison.
- Abbrechen verändert die im Formular vorhandene Auswahl nicht.
- nach Übernahme zeigt das Formular nur noch eine kompakte Zusammenfassung statt der langen Liste.
- gleichartige Mehrfachauswahlen in der aktuellen Server-Verwaltung geprüft: neben Kader und Kampftag-Mannschaften existieren aktuell keine weiteren eingebetteten Mehrfachlisten.
- JavaScript-Syntax von `verwaltung.js` und `server.js` nach der Änderung geprüft.

Erforderliche Schritte:
1. aktuelle `01_SSH_GITHUB_Stand_aktualisieren.txt` erneut ausführen.
2. danach `02_SSH_SERVER_Stand_installieren.txt` erneut ausführen.
3. Browser-Verwaltung neu laden und Kader-/Mannschaftsauswahl prüfen.
4. kein Windows-Build für diese Änderung erforderlich.

Noch nicht im Browser praktisch getestet.

## 7v. Admin-Löschrechte und Kampftage löschen – V0.2.16

Umgesetzt:
- jeder bestehende Datensatz in der Server-Verwaltung erhält im Bearbeitungsformular zusätzlich einen klar sichtbaren Button `Eintrag löschen`.
- Kampftage können damit direkt und vollständig gelöscht werden.
- weiterhin bleibt auch die Löschaktion in der Tabellenzeile vorhanden.
- löschbar sind aktuell: Vereine, Mannschaften, Wettkämpfer, Kampftage, Gewichtsklassen, Wettkampfmodi und Regelwerke.
- vor jedem Löschen erscheint eine eindeutige Bestätigung mit Objektname.
- bei abhängigen Daten wird die Auswirkung in der Bestätigung angezeigt.
- Löschen einer Mannschaft entfernt deren ID aus allen Kampftagen.
- Löschen eines Wettkämpfers entfernt dessen ID aus allen Mannschaftskadern.
- Löschen eines Wettkampfmodus entfernt dessen ID aus verknüpften Kampftagen.
- Löschen eines Regelwerks entfernt dessen ID aus verknüpften Wettkampfmodi.
- Löschen eines Vereins behält das bisherige Verhalten bei: zugehörige Mannschaften und Wettkämpfer werden ebenfalls gelöscht; Kampftag-Verweise werden bereinigt.
- eine bewusst leer gelöschte Regelwerk-Liste wird nach Neustart nicht mehr automatisch wieder mit Default-Regelwerken befüllt. Defaults entstehen nur noch, wenn die Sammlung technisch fehlt.

Erforderliche Schritte:
1. V0.2.15 muss nicht separat installiert werden.
2. auf dem Linux-Testserver einmal die aktuelle `01_SSH_GITHUB_Stand_aktualisieren.txt` ausführen.
3. danach einmal `02_SSH_SERVER_Stand_installieren.txt` ausführen.
4. Browser neu laden.
5. insbesondere einen falsch angelegten Kampftag testweise löschen.
6. kein Windows-Build erforderlich.

Noch nicht im Browser praktisch getestet.

## 7w. Mehrfachauswahl-Popup repariert – V0.2.17

Praxistest V0.2.16:
- `Mannschaften auswählen` und `Kader auswählen` reagierten nicht.
- Ursache: beim Umbau auf den großen Auswahl-Dialog wurden an vier Stellen Einzelelement-Selektoren verwendet, obwohl anschließend `.forEach()` auf mehreren Elementen ausgeführt wurde.
- dadurch brach JavaScript beim Initialisieren bzw. Öffnen des Dialogs ab.

V0.2.17:
- alle vier Stellen auf Mehrfachselektion korrigiert.
- betrifft Popup-Checkboxen, Popup-Buttons im Detailformular sowie das Auslesen der Formularfelder.
- JavaScript-Syntax von `verwaltung.js` und `server.js` geprüft.

Erforderliche Schritte:
1. aktuelle `01_SSH_GITHUB_Stand_aktualisieren.txt` ausführen.
2. danach `02_SSH_SERVER_Stand_installieren.txt` ausführen.
3. Browser hart neu laden.
4. `Kampftage → Mannschaften auswählen` und `Mannschaften → Kader auswählen` prüfen.
5. kein Windows-Build erforderlich.

## 7x. Mehrfachauswahl tatsächlich repariert – V0.2.18

Korrektur zu V0.2.17:
- Die als behoben gemeldeten vier Selektoren standen im tatsächlich gespeicherten `main` weiterhin falsch.
- Dadurch öffneten `Mannschaften auswählen` und `Kader auswählen` nicht zuverlässig bzw. brachen beim Rendern ab.

V0.2.18:
- die vier betroffenen Stellen verwenden jetzt ausdrücklich `document.querySelectorAll(...)`.
- direkt nach dem Commit wurde der gespeicherte GitHub-Inhalt erneut gelesen.
- alle vier neuen Selektoren sind dort nachweislich vorhanden.
- die alten fehlerhaften Varianten sind nicht mehr vorhanden.
- JavaScript-Syntax von `verwaltung.js` und `server.js` wurde erneut geprüft.

Erforderliche Schritte:
1. aktuelle `01_SSH_GITHUB_Stand_aktualisieren.txt` ausführen.
2. danach `02_SSH_SERVER_Stand_installieren.txt` ausführen.
3. Browser hart neu laden.
4. `Kampftage → Mannschaften auswählen` und `Mannschaften → Kader auswählen` erneut testen.
5. kein Windows-Build erforderlich.

## 7y. Mattenmonitor zwingend Vollbild – V0.2.19

Praxistest:
- auf Windows war am zweiten Monitor die Taskleiste sichtbar.
- Ursache: `update_screen_visibility()` verwendete echtes Vollbild nur dann, wenn keine feste Zweitmonitor-Größe gespeichert war.
- bei vorhandener Größe wurde lediglich `resize()+show()` verwendet.

V0.2.19:
- der sekundäre Mattenmonitor wird immer mit echtem Vollbild geöffnet.
- gespeicherte alte Größenwerte werden vollständig ignoriert.
- die Monitor-Auswahl bleibt erhalten.
- die Option für eine benutzerdefinierte Größe des Zweitmonitors ist in den Einstellungen deaktiviert.
- beim Speichern wird für den Zweitmonitor künftig immer `0×0` bzw. Auto/Vollbild hinterlegt.
- Ziel: keine Fensterrahmen, kein Desktop und keine Windows-Taskleiste auf dem Mattenmonitor.

Erforderliche Schritte:
1. Windows `03_POWERSHELL_WINDOWS_App_bauen.txt` ausführen.
2. V0.2.19 starten.
3. zweiten Monitor aktivieren und prüfen, dass die Mattenanzeige den Monitor vollständig inklusive Taskleistenbereich belegt.
4. kein Serverupdate erforderlich.

Noch nicht unter Windows praktisch getestet.

## 7z. NWJV-Druck für SW-Drucker optimiert – V0.2.20

Umgesetzt:
- eingetragene Vereinsnamen im großen Kopfbereich werden schwarz gedruckt.
- Wettkämpfernamen beider Seiten werden schwarz gedruckt.
- alle eingetragenen Kampfwerte beider Seiten werden schwarz gedruckt.
- die blaue Formular-Systematik der festen Überschriften bleibt erhalten.
- in den beiden `=`-Spalten `SIEG` und `Unterbewertung` wird der Wert 0 leer dargestellt.
- auch in den entsprechenden Summenfeldern wird 0 leer dargestellt.
- positive Werte bleiben sichtbar.
- Ziel ist bessere Lesbarkeit auf Schwarzweiß-Druckern.

Erforderliche Schritte:
1. Windows `03_POWERSHELL_WINDOWS_App_bauen.txt` ausführen.
2. V0.2.20 starten.
3. 5er-NWJV-Druckvorschau prüfen.
4. kein Serverupdate erforderlich.

Noch nicht unter Windows praktisch getestet.

## 7aa. NWJV-Druck: alle Nullwerte leer – V0.2.21

Korrektur zu V0.2.20:
- `0 = leer` wurde zuvor nur auf die `=`-Spalten und Summen angewendet.
- dadurch blieben in Yuko, Waza-ari, Ippon, Shido und Hansoku-make weiterhin zahlreiche Nullen sichtbar.

V0.2.21:
- in allen Ergebnisfeldern wird der Wert 0 leer gedruckt.
- positive Wertungen bleiben sichtbar.
- nicht abgeschlossene Kämpfe bleiben weiterhin ohne Ergebniswerte.
- Namen und tatsächlich eingetragene Werte bleiben schwarz für gute SW-Druckbarkeit.
- Summenfelder bleiben ebenfalls bei 0 leer.

Erforderliche Schritte:
1. Windows `03_POWERSHELL_WINDOWS_App_bauen.txt` ausführen.
2. V0.2.21 starten.
3. 5er-NWJV-Druckvorschau prüfen.
4. kein Serverupdate erforderlich.

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
