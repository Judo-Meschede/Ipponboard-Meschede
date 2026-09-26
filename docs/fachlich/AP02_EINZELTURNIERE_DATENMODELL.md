# AP 02 - Datenmodell Einzelturniere

Phase 1: Fachliche und technische Grundlage  
Dokumentrevision: 1.0 | Projektstand: V0.2.34 (nur Dokumentation)  
Ausgangsbasis: AP 01, GitHub main V0.2.33

## 1. Auftrag und Grenze

AP 02 legt das fachlich-technische Datenmodell für Einzelturnier, Kategorie, Meldung, Auslosung, Kampf und Platzierung fest. Es implementiert keine UI, keine Turnierlogik, keine automatische Auslosung, keine Rangberechnung und keine Migration produktiver Daten.

Das Modell ist der Vertrag für spätere Arbeitspakete. Bestehende `individualTournaments` und deren `registrations` bleiben bis zu einer ausdrücklich beauftragten Migration unverändert.

## 2. Grundsätze

1. Jedes Kernobjekt besitzt eine stabile, technisch erzeugte `id`. Anzeigenamen sind nie Schlüssel.
2. Beziehungen werden über IDs hergestellt. Personendaten werden nicht unnötig dupliziert.
3. Turnierkonfiguration, Kategorie, Meldung, Auslosung, Kampf und Platzierung sind getrennte Lebenszyklen.
4. Altersklasse, Geschlecht und Gewichtsklasse ergeben nicht automatisch eine amtliche Kategorie. Kategorien werden ausdrücklich festgelegt.
5. Regel- und Rangfolgeparameter müssen für gestartete Kategorien als Snapshot versionierbar sein. Änderungen globaler Regelwerke dürfen laufende oder abgeschlossene Ergebnisse nicht rückwirkend verändern.
6. Auslosungen sind versioniert und nach Veröffentlichung unveränderlich. Änderungen erzeugen eine neue Revision statt stiller Neuauslosung.
7. Kampfwege werden als explizite Quellen modelliert. Freilos ist Struktur, kein Kampf.
8. Ergebnisdaten bleiben Rohdaten. Sieger-Unterbewertung, Tabellenpunkte, Rang, Medaille und Qualifikation sind getrennte Größen.
9. Offlinebetrieb bleibt möglich. Laufende Kampfzeit, Haltegriffzeit, Zwischenwertungen und Undo-Verlauf gehören weiterhin nicht in die persistente Einzelturnierdomäne.
10. Status/Freigabe ist auf jeder fachlich relevanten Stufe nachvollziehbar.

## 3. Beziehungen

```text
IndividualTournament 1 --- n Category
IndividualTournament 1 --- n Registration
Category             1 --- n Registration (nach Einteilung)
Category             1 --- n Draw
Draw                 1 --- n Bout
Category             1 --- n Placement
Registration         1 --- n BoutParticipant/Source
Bout                 1 --- n BoutSource (Sieger/Verlierer als Folgequelle)
```

Eine Stammperson (`fighterId`) kann mehrere Meldungen in verschiedenen Turnieren besitzen. Innerhalb derselben Kategorie darf eine Meldung nur einmal als Starter vorkommen.

## 4. IndividualTournament

Pflichtkern:

| Feld | Typ | Bedeutung |
|---|---|---|
| id | string | stabile Turnier-ID |
| name | string | Turniername |
| date | string/date | Veranstaltungstag bzw. Beginn |
| hostClubId | string | Ausrichter, optional leer |
| location | string | Ort |
| status | enum | planned, registration, prepared, active, closed, archived |
| ruleScope | object | Verband/Ebene/Art/Jahr/Ausschreibung/Sonderbestimmung |
| categoryIds | string[] | Kategorien des Turniers |
| registrationIds | string[] | Meldungen des Turniers |
| createdAt/updatedAt | timestamp | Nachvollziehbarkeit |

Bestehende Felder wie `ageClasses`, `genders`, `weightMode`, `maleWeightClasses`, `femaleWeightClasses` und `ruleSetId` bleiben vorerst Legacy-/Vorbereitungsdaten. Sie werden in AP 02 nicht entfernt.

## 5. Category

Eine Kategorie ist das tatsächlich auszulösende Starterfeld.

| Feld | Typ | Bedeutung |
|---|---|---|
| id | string | stabile Kategorie-ID |
| tournamentId | string | Elternturnier |
| name | string | frei lesbare Bezeichnung |
| ageClass | string | z. B. U15 |
| gender | enum/string | m, w oder ausdrücklich genehmigte Sonderkonfiguration |
| classMode | enum | weight-class, weight-near, custom |
| weightClass | string | bei fester GK, sonst leer |
| weightRange | object/null | optionale bestätigte Grenzen/Gruppe |
| systemProfileId | string | gewählte Systemvariante |
| systemSnapshot | object | eingefrorene Struktur-/Systemparameter |
| ruleSetId | string | Referenz zum gewählten Regelprofil |
| ruleSnapshot | object | eingefrorene Kampfregelparameter |
| rankingProfileId | string | gewähltes Rangfolgeprofil |
| rankingSnapshot | object | eingefrorene Rang-/Tie-Break-Regeln |
| status | enum | draft, confirmed, drawn, active, completed, locked |
| registrationIds | string[] | bestätigte Starter |
| drawId | string | aktuell freigegebene Auslosung |
| placementIds | string[] | Ergebniszuordnungen |

`systemProfileId` bezeichnet eine konkrete Listen-/Systemvariante, nicht nur einen Namen wie „Doppel-KO“.

## 6. Registration

Meldung und Startfreigabe bleiben getrennt.

| Feld | Typ | Bedeutung |
|---|---|---|
| id | string | stabile Meldungs-ID |
| tournamentId | string | Turnier |
| fighterId | string | Stammperson |
| clubId | string | meldender/startender Verein |
| source | string | z. B. xlsx, manual |
| enteredAgeClass | string | gemeldete AK |
| enteredGender | string | gemeldetes Geschlecht |
| enteredWeightClass | string | gemeldete GK |
| enteredWeightKg | number/null | gemeldetes Gewicht |
| weighInWeightKg | number/null | tatsächlich festgestelltes Gewicht |
| weighInAt | timestamp/null | Wiegezeit |
| eligibilityStatus | enum | open, approved, rejected |
| documentStatus | enum | open, approved, rejected, not-required |
| weighInStatus | enum | open, passed, failed, not-required |
| startStatus | enum | registered, confirmed, withdrawn, excluded, no-show |
| categoryId | string | endgültige Kategorie, erst nach Einteilung |
| seed | object/null | Setzinformation mit Grund/Quelle |
| notes | string | fachliche Hinweise |
| createdAt/updatedAt | timestamp | Nachvollziehbarkeit |

Eine XLSX-Meldung erzeugt keine automatische Startfreigabe. Bei gewichtsnaher Einteilung ist `categoryId` die bestätigte Gruppenzuordnung.

## 7. Draw

Eine Auslosung ist eine versionierte, reproduzierbare Struktur.

| Feld | Typ | Bedeutung |
|---|---|---|
| id | string | stabile Auslosungs-ID |
| tournamentId | string | Turnier |
| categoryId | string | Kategorie |
| revision | integer | fortlaufende Revision |
| status | enum | draft, published, superseded, locked |
| systemProfileId | string | konkrete Systemvariante |
| systemSnapshot | object | eingefrorene Systemdefinition |
| participantRegistrationIds | string[] | bestätigtes Starterfeld |
| seedAssignments | object[] | gesetzte Positionen mit Begründung |
| slots | object[] | Start-/Pool-/Listenpositionen |
| boutIds | string[] | zugehörige Kämpfe |
| generatedAt | timestamp | Erzeugungszeit |
| generatedBy | string | Nutzer/Terminal/Prozess |
| supersedesDrawId | string | Vorgänger bei Korrektur |
| publishedAt | timestamp/null | Veröffentlichung |

Ein `slot` kann eine Meldung oder bewusst `bye` enthalten. Ein Freilos erzeugt keinen abgeschlossenen Scheinkampf.

## 8. Bout

Der Kampf ist systemneutral. Seine Teilnehmer können direkt oder aus vorherigen Ergebnissen entstehen.

| Feld | Typ | Bedeutung |
|---|---|---|
| id | string | stabile Kampf-ID |
| tournamentId/categoryId/drawId | string | Kontext |
| stage | string | z. B. pool, main, repechage, semifinal, bronze, final, tiebreak |
| groupId | string | Pool/Listenbereich, falls vorhanden |
| round | integer/string | Runde innerhalb der Struktur |
| order | integer | vorgesehene Reihenfolge |
| tatamiId | string | Matte, optional bis Planung |
| whiteSource/blueSource | object | Herkunft des Startplatzes |
| whiteRegistrationId/blueRegistrationId | string | aufgelöste Starter, sobald bekannt |
| status | enum | pending, ready, active, completed, corrected, void |
| result | object/null | persistiertes Endergebnis |
| revision | integer | Ergebnisrevision |
| completedAt | timestamp/null | tatsächliches Kampfende |
| correctedFromRevision | integer/null | Korrekturbezug |

Zulässige Quellen (`BoutSource`):
- `registration`: direkte Meldung/Startposition
- `pool-rank`: Rang aus einer Gruppe
- `bout-winner`: Sieger eines vorherigen Kampfes
- `bout-loser`: berechtigter Verlierer eines vorherigen Kampfes
- `tiebreak-winner`: Ergebnis eines Entscheidungskampfes
- `bye`: nur als Strukturquelle, nicht als sportliches Ergebnis

`result` hält mindestens fest:
- Sieger-Meldungs-ID oder ausdrücklich kein Sieger
- Endwertungen beider Seiten getrennt nach Ippon/Waza-ari/Yuko
- Strafen beider Seiten
- reguläre effektive Kampfzeit
- Golden-Score-Zeit
- Entscheidungsart/Grund, z. B. score, hansoku-make, fusen-gachi, kiken-gachi, injury, hantei, double-loss
- für das gewählte Profil benötigte Unterbewertungs-/Rang-Rohdaten
- Bestätigungsstatus und Herkunft der Korrektur

Zwischenstände und laufende Timer werden hier nicht persistiert.

## 9. Placement

Platzierung ist ein eigenes Ergebnisobjekt und kein Feld am Kämpfer.

| Feld | Typ | Bedeutung |
|---|---|---|
| id | string | stabile Platzierungs-ID |
| tournamentId/categoryId | string | Kontext |
| registrationId | string | betroffene Meldung |
| rank | integer/string/null | sportlicher Rang, auch geteilt darstellbar |
| medal | enum/null | gold, silver, bronze oder leer |
| qualification | object/null | Qualifikationsstatus/Ziel |
| status | enum | provisional, confirmed, revoked |
| basis | object | Profil, Auslosungsrevision und relevante Kämpfe |
| reason | string | Sonder-/manuelle Entscheidung |
| confirmedAt | timestamp/null | Freigabe |
| updatedAt | timestamp | Nachvollziehbarkeit |

Mehrere Meldungen dürfen denselben Rang besitzen. Eine Qualifikation kann ohne eigene Medaille bzw. abweichend vom Medaillenrang dokumentiert werden.

## 10. Invarianten

- Alle Fremdschlüssel müssen innerhalb desselben Turniers konsistent sein.
- Eine veröffentlichte Auslosung wird nicht in-place neu erzeugt.
- Ein abgeschlossener Kampf verliert bei einer Ergebnisänderung nicht seine Identität; `revision` steigt.
- Folgekämpfe dürfen nach Korrektur nicht still auf andere Starter umgehängt werden, wenn sie bereits begonnen/abgeschlossen sind.
- Platzierungen referenzieren die Auslosungs-/Ergebnisrevision, auf der sie beruhen.
- `category.registrationIds` enthält nur startberechtigte, bestätigte Meldungen.
- Freilose zählen nicht als Kämpfe und erzeugen keine Kampfwertung.
- Keine Rangentscheidung anhand Name, Verein, Import- oder Tabellenreihenfolge.
- Keine automatische amtliche GK-Ableitung aus AK/Geschlecht.
- Kategorien dürfen erst ausgelost werden, wenn Starterfeld und Regel-/System-/Rangprofile bestätigt sind.

## 11. Übergang vom aktuellen Modell

Aktuell liegen Meldungen eingebettet unter `individualTournaments[].registrations`. AP 02 definiert das Zielmodell, migriert aber noch nichts.

Für eine spätere Migration gilt:
1. bestehende Turnier-ID bleibt erhalten;
2. jede bestehende eingebettete Meldung erhält eine eigene stabile `registrationId`;
3. `fighterId` und `clubId` bleiben erhalten;
4. bestehende AK/GK/Gewicht/Kyu-Werte werden verlustfrei übernommen;
5. ohne bestätigte Einteilung wird kein `categoryId` erfunden;
6. ohne Auslosung entstehen keine Kämpfe oder Platzierungen;
7. Legacy-Felder werden erst entfernt, wenn Migration und Rückwärtskompatibilität ausdrücklich freigegeben sind.

## 12. Abgrenzung für Folgepakete

AP 02 enthält ausdrücklich noch nicht:
- UI oder Verwaltungsreiter,
- automatische Kategorienbildung,
- konkrete Algorithmik für Pool/KO/Doppel-KO,
- Setz-/Vereinstrennungsalgorithmus,
- automatische Rangberechnung,
- Migration bestehender Serverdaten,
- neue Server-Endpunkte,
- Desktop-Integration,
- Drucklisten.

Diese Punkte benötigen eigene kleine Arbeitspakete.
