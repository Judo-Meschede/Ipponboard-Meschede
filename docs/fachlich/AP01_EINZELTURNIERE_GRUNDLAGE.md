# AP 01 - Fachliche Grundlage für Judo-Einzelturniere

Phase 1: Fachliche und technische Grundlage  
Dokumentrevision: 1.0 | Projektstand: V0.2.33 (nur Dokumentation)  
Recherchestand: 26.09.2026 | Ausgangsbasis: GitHub main, `5ad8c833c6fe0077a0fb22b82d7a0007a7f169a4`, V0.2.32

## 1. Auftrag, Geltung und Verbindlichkeit

Dieses Dokument ist die verbindliche fachliche Arbeitsgrundlage für die folgenden Einzelturnier-Arbeitspakete von Ipponboard-Meschede. AP 01 recherchiert und dokumentiert; es implementiert keine Turnierfunktion, verändert keine Stammdaten und erteilt keinen Auftrag für ein Folgepaket.

Gegenstand ist der Shiai-Einzelwettkampf in Deutschland: Vereins- und Einladungsturniere sowie Meisterschaften, mit Schwerpunkt DJB und NWJV. Andere Landesverbände werden als Beleg für Varianten berücksichtigt. Dies ist keine vollständige Prüfung sämtlicher Landesverbandsordnungen. Kata, Mannschaftswettkämpfe, Para- und ID-Judo sowie besondere Veteranenformate benötigen eigene Profile und sind nicht automatisch abgedeckt.

Die Begriffe werden bewusst getrennt:

- **Quellenbefund:** nachprüfbare Aussage einer offiziellen Quelle mit Fundstelle und Stand.
- **Projektvorgabe (F-xx):** aus dem Auftrag und der Recherche abgeleitete Anforderung an spätere Umsetzung; keine Behauptung, dass ein Verband genau diese Softwarestruktur vorschreibt.
- **Klärpunkt (K-xx):** eine nicht ausreichend eindeutige Regel oder Zuordnung. Hier darf später keine vermeintlich offizielle Automatik erfunden werden.

Verbindlich bedeutet innerhalb des Projekts: Folgepakete müssen diese Anforderungen beachten und Abweichungen dokumentieren. Das Dokument ersetzt keine Verbandsordnung, Ausschreibung oder Entscheidung der zuständigen sportlichen Leitung. Ein Regelprofil darf nur in seinem belegten Geltungsbereich verwendet werden.

## 2. Regelquellen richtig anwenden

Der DJB verlangt, das Wettkampfsystem in der Ausschreibung festzulegen. Für Deutsche und Gruppenmeisterschaften U15/U18/U21 nennt er grundsätzlich Doppel-KO, mit Ausnahmen bei kleinen Feldern. Die Nachwuchs-Altersklasse richtet sich nach dem Jahrgang. Startberechtigung, Wiegen und Wettkampfzeiten sind gesondert geregelt. [Q1, §§ 2.3, 2.8-2.9, 3.2.4, 3.3-3.4, 3.10][Q1]

**F-01 - Geltungsbereich vor Regelwahl:** Für ein Turnier sind Verband, Veranstaltungsebene, Meisterschaft/Turnier, Jahr, Ausschreibung und gegebenenfalls genehmigte Sonderbestimmungen festzuhalten. Eine Ausschreibung kann nur solche Abweichungen festlegen, die die zuständigen Regeln zulassen. Bei Widersprüchen entscheidet die sportliche Leitung; die Software darf keine Normenhierarchie erraten.

**F-02 - Getrennte Festlegungen:** Folgende vier Dinge sind unabhängig zu beschreiben:

| Festlegung | Beantwortete Frage |
|---|---|
| Klassenbildung | Wer gehört zusammen: Altersklasse, Geschlecht, Gewicht, gegebenenfalls weitere ausgeschriebene Kriterien? |
| Turniersystem | Wer kämpft gegen wen, und wohin führen Sieg und Niederlage? |
| Kampfregelprofil | Wie werden Zeit, Wertungen, Strafen, Verlängerung und Kampfende behandelt? |
| Rangfolge-/Platzierungsprofil | Wie entstehen Poolrang, Medaillen und Qualifikationsplätze? |

Ein Eintrag wie „U15, Doppel-KO“ ist dafür allein nicht ausreichend. Ebenso bestimmt die Zahl der Meldungen weder automatisch das zulässige System noch die Regeln eines Kampfes.

## 3. Relevante Wettkampfsysteme

### 3.1 Quellenbefund und begriffliche Abgrenzung

Die NWJV-Wettkampfordnung führt brasilianisches KO, vorgepooltes KO, KO mit doppelter Trostrunde, doppeltes KO mit Trostrunde, modifiziertes doppeltes KO sowie Jeder-gegen-jeden auf. Letzteres ist dort auf fünf Teilnehmende begrenzt. Die sportliche Leitung wählt nach Teilnehmerzahl; zugelassene Listen sind vorgeschrieben. [Q4, § 2.9][Q4]

Die NWJV-Downloadseite stellt Pool-, 8er-/16er-Doppel-KO-, 32er-modifizierte Doppel-KO- sowie 32er-/64er-Listen mit doppelter Trostrunde bereit. Die modifizierte 32er-Liste dient ausdrücklich der Ermittlung zweier Qualifizierter. Das Vorhandensein einer Vorlage ist kein Beleg für ihre universelle Anwendbarkeit. [Q6][Q6]

Die IJF unterscheidet ausdrücklich: direktes KO ohne Trostrunde; Viertelfinal-Trostrunde nur für Viertelfinalverlierer; doppelte Trostrunde für die zuvor gegen die vier Halbfinalisten unterlegenen Athleten; vollständige Trostrunde mit Aufnahme aller Hauptrundenverlierer. Bei den Trostrundenvarianten führen die Bronzewege gegen den Halbfinalverlierer der Gegenseite. Round Robin bedeutet Jeder-gegen-jeden; Best-of-three endet nach zwei Siegen. [Q8, §§ 2.5.1-2.5.6][Q8]

**F-03 - Keine Gleichsetzung anhand des Namens:** „Doppel-KO“, „Double Repechage“, „Full Repechage“ und „brasilianisch“ dürfen nicht als austauschbare Bezeichnungen programmiert werden. Maßgeblich ist die konkrete genehmigte Liste mit ihren Sieger-/Verliererwegen. Insbesondere ist ein Judo-Trostrundensystem nicht automatisch ein allgemeines Double-Elimination-System mit Rückkehr des Trostrundensiegers ins Goldfinale.

### 3.2 Systemkatalog für spätere Arbeitspakete

Die rechte Spalte enthält Projektanforderungen, keine bereits implementierten Funktionen.

| Systemfamilie | Bedeutung und Einsatz | Vor späterer Umsetzung festzulegen |
|---|---|---|
| Einzelpool / Jeder-gegen-jeden | Kleine, abgeschlossene Gruppe; jedes Paar tritt einmal an. | Erlaubte Gruppengröße, Kampfreihenfolge, Rangfolgeregel, Zahl der ausgezeichneten Plätze. |
| Zwei oder mehrere Pools mit Endrunde | Vorrundengruppen ermitteln Teilnehmende einer anschließenden KO-Phase. | Gruppeneinteilung, Aufsteigerzahl, Kreuzpaarungen, Ergebnisübernahme oder neue Kämpfe, Bronze-/Platzierungskämpfe. |
| Doppel-KO mit vollständiger Trostrunde | Hauptrunde und zusätzlicher Verliererweg nach konkreter Verbandsliste. | Exakte Listenvariante, Einstieg je Niederlage, Seitenwechsel, Wiederbegegnungen und Medaillenwege. |
| KO mit doppelter Trostrunde | Trostrundenberechtigung hängt vom Fortkommen des Bezwingers ab. | Berechtigungskriterium, Reihenfolge der nachrückenden Verlierer, Kreuzung vor Bronze. |
| KO mit Viertelfinal-Trostrunde | Engere Trostrunde ab Viertelfinale; international relevant. | Eigenes Profil, nicht stillschweigend als deutscher Doppel-KO-Standard verwenden. |
| Modifiziertes Doppel-KO / Qualifikationssystem | Ermittelt die in der Ausschreibung verlangten Qualifizierten. | Qualifikation und Medaillen getrennt; konkrete Zusatzkämpfe und Abbruchbedingung der Liste. |
| Direktes KO | Ausscheiden nach Niederlage, ohne Trostrunde. | Platzierungsregel ausdrücklich; ein Kampf um Platz drei ist nicht automatisch vorhanden. |
| Zwei Teilnehmende / „2 von 3“ | Einzelfinale oder Serie nach zulässiger Festlegung. | Serienentscheidung getrennt vom Einzelkampfergebnis, dritte Begegnung nur bei Bedarf. |
| „Brasilianisches KO“ | In der NWJV-Ordnung benannt. | K-03: Ohne eindeutige freigegebene Vorlage keine technische Zuordnung zu einer der anderen Varianten. |

Für vorgepoolte Systeme liefert der JVSH offizielle Listen für zweimal vier, fünf und sechs Teilnehmende sowie einzelne Vierer-, Fünfer- und Sechserpools. Das belegt regionale Varianten; daraus folgt keine Freigabe eines Sechserpools im NWJV. [Q7][Q7] Der WJV führt ebenfalls eigene Kleinfeldlisten für 1-4, 5 und 6-8 Teilnehmende. [Q10][Q10]

**F-04 - Auswahlhilfe statt stiller Festlegung:** Ein späterer Vorschlag darf bei 2 Personen Einzelfinale/Serie, bei kleinen Gruppen einen Pool und bei größeren Feldern geeignete Listen anbieten. Verbindlich wird erst die zulässige Auswahl der sportlichen Leitung. Bei null Teilnehmenden entsteht kein Kampf; bei einer Person weder ein Scheinkampf noch automatisch ein sportlich errungener Sieg.

**F-05 - Größen und Freilose:** Listenplätze und tatsächlich gestartete Personen sind verschiedene Größen. Freilose sind Strukturinformationen, keine mit Ippon gewonnenen Kämpfe. Ungerade Pools benötigen keine fiktive Person. Die mathematische Kontrollgröße für einen einfachen Pool ist `n × (n - 1) / 2`: drei Personen ergeben drei, vier ergeben sechs und fünf ergeben zehn Paarungen. Das ist eine Vollständigkeitsprüfung, keine zulässige Kampfreihenfolge.

**F-06 - Keine unbelegten Standardmodi:** Schweizer System, beliebige Ranglistenserien oder pädagogische Sonderformate werden ohne passende Ausschreibung und belastbare Regelgrundlage nicht als offizieller Standard vorausgesetzt. Der Katalog ist kein Auftrag, sämtliche Systeme im nächsten Paket umzusetzen.

## 4. Kampfregeln und Zeitführung

### 4.1 Grundprofil und Jugendvarianten

Die DJB-Kampfregeln 2026 führen Ippon, Waza-ari und Yuko; zwei Waza-ari ergeben Ippon. Osae-komi: Yuko ab 5, Waza-ari ab 10, Ippon bei 20 Sekunden. Bei technischem Gleichstand folgt im Grundprofil Golden Score; bisherige Wertungen und Strafen bleiben erhalten. Ein technischer Vorteil oder Hansoku-make entscheidet, nicht eine bloße Shido-Differenz. Die deutsche Erläuterung unterscheidet Anzeige und Unterbewertung: Ippon 10, Waza-ari 7, Yuko 5 Unterbewertungspunkte. Für Osae-komi im Golden Score nennt sie eine DJB-Abweichung zum frühen IJF-Ende. [Q2, Art. 4, 6-8, 13-18][Q2]

Die folgende NWJV-Tabelle wurde einschließlich der zusammengefassten Zellen und Fußnoten visuell geprüft. [Q5, S. 1, Fußnote 15 auf S. 2][Q5]

| NWJV-Altersklasse | Reguläre effektive Kampfzeit | Bei Gleichstand | Kampfpause |
|---|---|---|---|
| U11 | 2 Minuten | Hantei ohne Golden Score | Mit Kampfzeit gemeinsam dargestellte Zelle; siehe Hinweis unten |
| U13 | 3 Minuten | Hantei ohne Golden Score | Mit Kampfzeit gemeinsam dargestellte Zelle; siehe Hinweis unten |
| U15 | 3 Minuten | höchstens 3 Minuten Golden Score, dann Hantei | 6 Minuten |
| U18 / U21 | 4 Minuten | Golden Score ohne Zeitlimit | 10 Minuten |

Bei U11/U13 enthält das Blatt eine über die Zeilen Kampfzeit/Kampfpause zusammengefasste Zelle; daraus wird kein eigener Pausenwert erfunden. Die Fußnote verweist außerhalb von NWJV-Maßnahmen auf 6 Minuten bis U15, darüber 10 Minuten. K-02 bleibt für ein ausführbares NWJV-U11/U13-Profil offen. Die DJB-WKO bestätigt 4 Minuten für Frauen/Männer und 6 Minuten Pause für U15 bzw. 10 für U18/U21/Erwachsene auf DJB-Ebene; niedrigere Ebenen regeln die Landesverbände. [Q1, § 3.3][Q1]

**F-07 - Zeit und Entscheidung getrennt:** Reguläre Zeit, Golden-Score-Zeit, Haltegriffzeit, effektive Gesamtkampfzeit und tatsächlicher Endzeitpunkt sind getrennte fachliche Größen. Bei Mate läuft keine effektive Kampfzeit weiter. Ein laufender Haltegriff am Zeitende darf nicht durch einen pauschalen Timer-Stopp entschieden werden. Das konkrete Regelprofil und die Kampfrichterentscheidung bestimmen das Ende.

**F-08 - Manuelle Entscheidungen:** Hantei, besondere Disqualifikationen und medizinisch bedingte Entscheidungen bleiben Entscheidungen der zuständigen Personen. Das Programm dokumentiert Ergebnis und Grund. Es darf aus einem Punktestand keine vermeintliche Hantei-Entscheidung ableiten.

**F-09 - Pausen gehören zur Person:** Bei späterer Kampfplanung ist die Pausenfrist seit dem letzten tatsächlichen Kampfende zu berücksichtigen, auch bei Tatamiwechsel. Ein Freilos setzt keine neue Kampfbelastung. Fehlende Pausenparameter müssen vor Planfreigabe geklärt werden.

## 5. Poolrangfolge und Platzierungen

### 5.1 Drei ausdrücklich verschiedene Regelstände

| Quellenprofil | Belegter Inhalt | Konsequenz im Projekt |
|---|---|---|
| DJB „Platzierungen im Pool“, 2023 | Rangfolge nach Siegpunkten, danach Wertungspunkten, danach den Kämpfen der Gleichstehenden untereinander. Bleibt Gleichstand, folgen neu ausgeloste Stichkämpfe. Für drei Betroffene werden abhängig vom Platzierungs-/Qualifikationsziel UP/DOWN unterschieden. Das Blatt nennt für Einzel nur 7/10 Wertungspunkte. [Q3][Q3] | Die Verfahrensfolge ist eine dokumentierte Quelle, aber keine vollständig aktualisierte 2026-Punktetabelle. |
| NWJV, vorgepooltes KO | Bei Kreisschlagen mit gleicher Unterbewertung: direkter Vergleich, dann Zeit der gewonnenen Kämpfe, dann Wiederholung. [Q4, § 2.9.1 a][Q4] | Diese Sonderregel nicht in alle DJB-Pools übertragen. Die Richtung des Zeitvergleichs muss für die ausführbare Fassung eindeutig bestätigt werden (K-01). |
| IJF SOR, Juli 2026 | Poolbewertung verwendet 100/10/1/0 und berücksichtigt auch erzielte Wertungen aus verlorenen Kämpfen. Zusätzlich enthält die Rangfolge Shido- und Zeitkriterien. [Q8, § 2.5.5.1][Q8] | Eigenes internationales Profil; nicht mit deutscher Sieger-Unterbewertung vermischen. |

**F-10 - Rangfolge reproduzierbar:** Ein Profil benötigt eine geordnete Kriterienfolge einschließlich der Behandlung von Untergruppen, Kreisschlagen und entscheidungsbedürftigen Qualifikationsplätzen. Eine Gleichstandsauflösung nach Name, Vereinsname, Importreihenfolge oder zufälliger Tabellenposition ist unzulässig.

**F-11 - Rohdaten erhalten:** Pro abgeschlossenem Kampf müssen Endwertungen beider Seiten, Strafen, Sieger bzw. Sonderausgang, Entscheidungsgrund und Zeiten fachlich verfügbar sein. Sieger-Unterbewertung, Tabellenpunkte und Anzeigezahlen sind getrennt. Sie dürfen nicht über eine einzige universelle „Punkte“-Zahl ersetzt werden.

**F-12 - Medaille ist nicht Qualifikation:** Rang, Medaillenrang und Qualifikationsstatus sind getrennte Ergebnisse. Geteilte dritte/fünfte/siebte Plätze sind je Liste möglich. Zusätzliche Platzierungskämpfe werden nicht allein zur Erzeugung einer lückenlosen Rangliste erfunden.

**F-13 - Stichkampf ist ein eigener Kampf:** Bei zusätzlicher Entscheidung bleibt das ursprüngliche Poolergebnis erhalten. Neue Auslosung, Entscheidungskampf und daraus resultierende Platzierung werden nachvollziehbar zugeordnet.

## 6. Meldung, Klassenbildung, Wiegen und Auslosung

Die DJB-Übersicht 2026 unterscheidet Einzel- und Mannschaftsgewichtsklassen und enthält bei U11 gewichtsnahe Gruppen. Beispiel: U15 umfasst dort die Jahrgänge 2012-2014, U18 die Jahrgänge 2009-2011. Der PDF-Metadatentitel nennt noch 2025; maßgeblich für diesen Befund sind Tabelleninhalt und Überschrift 2026. [Q9][Q9]

Der NWJV erlaubt bei Turnieren unter bestimmten Bedingungen Zusammenlegungen angrenzender Gewichtsklassen durch die sportliche Leitung. Bei zwei Judoka ist auch „2 von 3“ vorgesehen. Gemischte Begegnungen U11/U13 sind auf Kreisebene an die ausdrückliche Ausschreibung gebunden. [Q4, §§ 2.1 c, 3.2.1][Q4]

**F-14 - Keine Rückkehr zur automatischen GK-Ableitung:** Die in V0.2.32 eingeführte explizite Auswahl der Gewichtsklassen bleibt verbindlich. Aus einem frei eingetragenen „U17“ oder „+83 kg“ darf keine amtliche Freigabe abgeleitet werden. Eine spätere Regelvorlage ist ein ausdrücklich gewählter, datierter Vorschlag.

**F-15 - Kategorien je Kombination:** Werden mehrere Altersklassen in einem Turnier angeboten, muss später je Altersklasse und Geschlecht die passende Klassenkonfiguration ausdrückbar sein. Die vorhandenen turnierweiten Listen `maleWeightClasses`/`femaleWeightClasses` bilden diese Unterscheidung allein noch nicht ab. AP 01 ändert das Datenmodell nicht.

**F-16 - Meldung ist keine Wiegefreigabe:** Gemeldete Klasse, gemeldetes Gewicht, tatsächlich ermitteltes Gewicht, gegebenenfalls zulässiger Abzug, Startfreigabe und endgültige Gruppe sind getrennt zu betrachten. Eine erfolgreich importierte XLSX ist kein Nachweis der Startberechtigung. Dokumenten-/Graduierungsprüfung muss bei Bedarf als geprüft/offen/abgelehnt nachvollziehbar sein; AP 01 verlangt keine Sammlung zusätzlicher Passdokumente.

**F-17 - Gewichtsnahe Gruppen:** Kein universeller zulässiger Kilogrammabstand wird festgelegt. Ein späterer Vorschlag muss Teilnehmergewichte und Gruppengröße sichtbar machen; die sportliche Leitung bestätigt die Einteilung nach Ausschreibung. Änderungen, Zusammenlegungen und Geschlechtermischung benötigen eine dokumentierte Grundlage.

**F-18 - Auslosung:** Starterfeld vor Auslosung bestätigen; Setzpositionen, Verein-/Verbandsverteilung und Ausnahmen nur nach gewählter Vorschrift verwenden. Auslosungsergebnis einschließlich Freilosen sichern. Nach Veröffentlichung keine unbemerkte Neuauslosung beim erneuten Öffnen. Die Herkunft jeder Paarung muss erklärbar bleiben.

## 7. Nichtantreten, Aufgabe, Ausschluss und Korrektur

Die DJB-Kampfregeln unterscheiden Fusen-gachi (Nichtantreten) und Kiken-gachi (Aufgabe). Auch ein Ergebnis ohne Sieger ist möglich, etwa bei doppeltem Hansoku-make. Konsequenzen können der sportlichen Leitung vorbehalten sein. Daher sind Niederlage, weiterer Startausschluss und Verlust einer Platzierung nicht dasselbe. [Q2, Art. 18.3-20][Q2]

**F-19 - Eindeutige Sonderausgänge:** Freilos, Nichtantreten, Aufgabe, Verletzungsabbruch, direkte/indirekte Disqualifikation und beidseitige Niederlage dürfen nicht als identischer Ippon-Datensatz gespeichert werden. Der konkrete Entscheidungsgrund ist Teil des Ergebnisses.

**F-20 - Rückzug im Pool:** Für ein ausführbares Profil muss geklärt sein, was mit bereits ausgetragenen und noch offenen Kämpfen eines zurückgezogenen Athleten geschieht. Keine pauschale Annullierung aller Ergebnisse und keine automatische Vergabe fiktiver Siege ohne passende Regel. Offene Entscheidung = vorläufige Rangfolge.

**F-21 - Folgewirkungen einer Korrektur:** Wird ein Ergebnis korrigiert, müssen betroffene Rangfolgen, Weiterleitungen und Platzierungen neu geprüft werden. Bereits begonnene oder abgeschlossene Folgekämpfe dürfen nicht still überschrieben oder auf andere Personen umgehängt werden. Solche Konflikte müssen vor weiterer Freigabe durch die Wettkampfleitung aufgelöst werden.

## 8. Verbindliche technische Leitplanken, noch ohne Implementierung

Aus dem bestehenden Projekt-Handoff und den vorstehenden fachlichen Anforderungen folgen:

1. **Stabile Identitäten:** Turnier, Meldung, Kategorie, Gruppe, Kampf und Person fachlich eindeutig zuordnen; Anzeigenamen sind keine technischen Schlüssel. Eine Person kann mehrere zulässige Meldungen haben, ohne mehrfach als Stammperson angelegt zu werden.
2. **Versionierte Regelkonfiguration:** Zu einem gestarteten Turnier gehört eine unveränderliche Kopie der verwendeten Regelparameter und Quellenstände. Eine spätere Änderung eines globalen Regelwerks darf abgeschlossene Ergebnisse nicht rückwirkend verändern.
3. **Explizite Kampfwege:** Eine Paarung kann aus einer Startposition, einem Poolrang, einem Sieger oder einem berechtigten Verlierer stammen. Nur die überprüfte Systemvariante bestimmt diese Abhängigkeiten. Ein freies Textfeld „Doppel-KO“ reicht nicht.
4. **Offlinebetrieb:** Ein vorbereiteter Wettkampf muss ohne Internet fortsetzbar bleiben. Serververwaltung und Synchronisation dürfen keine Voraussetzung für laufende Kämpfe werden.
5. **Bestehende Persistenzgrenze:** Nach Projektvorgabe werden nur abgeschlossene oder korrigierte Kämpfe als Wiederanlauf-/Sync-Ergebnisse persistiert. Kein automatisches Speichern laufender Kampfzeit, Haltegriffzeit, Zwischenwertungen oder Undo-Verläufe. Stammdaten, bestätigte Einteilung und Auslosung bleiben davon getrennt.
6. **Prüfbare Freigabe:** Ergebnisübersicht und Druckausgabe müssen erkennen lassen, ob Einteilung, Auslosung, Ergebnisse und Platzierungen vorläufig oder bestätigt sind.
7. **Kein Umbau in AP 01:** Desktop bleibt Qt/C++; Mannschaftsmodus, Monitor, Server-Endpunkte, Build und Datenmigration werden hier nicht verändert.

Codeabgleich des Ausgangsstands: `server/server.js` enthält Einzelturniere mit Meldungen, Altersklassen, Geschlechtern, Gewichtsmodus und geschlechtsbezogenen Gewichtsklassenlisten (insbesondere `cleanRecord`, `individualWeightOptions`). Die hier beschriebenen Anforderungen sind keine Behauptung, eine vollständige Einzelturnier-Engine sei bereits vorhanden. Server `APP_VERSION` und `desktop/CMakeLists.txt` stehen tatsächlich beide auf 0.2.32. Der frühere Handoff-Satz „APP_VERSION='0.1.0'“ war veraltet.

## 9. Quellenkonflikte und begrenzte offene Entscheidungen

Diese Klärpunkte blockieren nicht den Abschluss der Recherche. Sie sperren nur die jeweils betroffene spätere Automatik, bis eine eindeutige, dokumentierte Entscheidung vorliegt.

| ID | Befund | Verbindlicher Umgang |
|---|---|---|
| K-01 | DJB-Poolblatt 2023 nennt nur 7/10; DJB-Kampfregeln 2026 nennen Yuko-Unterbewertung 5; NWJV-WKO § 2.9 nennt weiterhin 1-7-10. Die NWJV-Zeitregel benennt nicht ausdrücklich die Richtung des Vergleichs. | Vor einer automatischen NWJV-Poolwertung aktuelle verbindliche Auslegung festhalten. Keine Mischung und keine unkommentierte Ergänzung der alten Tabelle. |
| K-02 | NWJV-Jugendblatt ist als 2026/27.03.2026 verlinkt, trägt intern den Fußstand 28.01.2025; U11/U13-Pausenzelle ist nicht eindeutig separat ausgefüllt. | Quellenfassung so kennzeichnen; Pausenregel für NWJV-U11/U13 vor Planungsautomatik bestätigen. |
| K-03 | Systemnamen allein legen die deutsche Listenverdrahtung nicht zweifelsfrei fest; „brasilianisch“ ist nur benannt. | Für die jeweils zuerst beauftragte Variante genehmigte Originalliste und alle Sieger-/Verliererwege prüfen. Die NWJV-Exceldateien wurden hier als offiziell angebotene Vorlagen identifiziert, nicht als ausführbare Algorithmen validiert. |
| K-04 | Aktuelle IJF-SOR und deutsche Quellen haben unterschiedliche Poolbewertungen. Eine spätere internationale Veröffentlichung belegt noch keine deutsche Übernahme. | Separate Geltungsbereiche; kein automatischer Austausch eines DJB-/NWJV-Profils durch „neueste IJF“. |
| K-05 | Keine konkrete Turnierausschreibung ist Teil von AP 01. | Noch keine verbindliche Auswahl von Turniersystem, Starterfeld, Setzung, Zusammenlegung oder konkreten Sonderregeln für eine Veranstaltung. |
| K-06 | Sonderausgänge und Rückzug können Listen- und Veranstaltungsentscheidungen auslösen. | Die später gewählte Variante muss eine vollständige Entscheidungstabelle erhalten; unbelegte Fälle bleiben manuell entscheidungspflichtig. |

## 10. Fachliche Prüffälle für spätere Umsetzung

Dies sind Abnahmekriterien für künftige Pakete, keine in AP 01 ausgeführten Softwaretests.

| Fall | Erwartung |
|---|---|
| Pool mit 3/4/5 Personen | Genau 3/6/10 eindeutige Paarungen; keine Selbstpaarung oder erfundene Person. |
| Zwei Personen, Serie | Nach zwei Siegen beendet; bei 1:1 dritter Kampf; Einzelresultate bleiben erhalten. |
| Ein Teilnehmer / Freilos | Kein erfundener Ippon, keine erfundene Kampfzeit; Platzierungsentscheidung gemäß Profil. |
| Doppel-KO vs. doppelte Trostrunde | Dieselbe frühe Niederlage kann unterschiedliche weitere Startberechtigung bewirken; Originalvorlage entscheidet. |
| Drei Personen schlagen sich im Kreis | Kein alphabetischer Sieger; korrektes ausgewähltes Rangfolge-/Stichkampfverfahren. |
| U13 / U15 / U18 bei Gleichstand | Hantei / begrenztes Golden Score mit Hantei / unbegrenztes Golden Score nach bestätigtem Profil. |
| Sieg durch Yuko | Technische Wertung bleibt erhalten; Unterbewertung nur aus dem bestätigten Profil. |
| Tatamiwechsel | Personenbezogene Kampfpause bleibt wirksam. |
| Mehrere Altersklassen eines Turniers | Gewichtsklassen werden nicht pauschal für alle Altersklassen gleichgesetzt. |
| Nichtantreten, Rückzug, Doppel-Hansoku-make | Unterschiedliche Zustände; kein erzwungener Sieger oder ungeprüfter Folgeplatz. |
| Ergebnisänderung nach Folgekampf | Konflikt wird sichtbar; keine stille Umschreibung des Folgekampfs. |
| Offline-Neustart | Bestätigte Vorbereitung und abgeschlossene Ergebnisse verfügbar; laufender Kampf nicht aus Zwischenstand rekonstruiert. |

## 11. Quellenregister

Alle Quellen am **26.09.2026** abgerufen. Verwendet wurden offizielle Verbandsseiten und Verbandsdokumente, keine Suchmaschinen-Zusammenfassung als alleiniger Beleg. PDF-Seitenangaben beziehen sich auf die PDF-Seite (ab 1); bei NWJV-WKO kann die gedruckte Zählung abweichen. Es werden kurze eigene Zusammenfassungen und Fundstellen dokumentiert, keine vollständigen Regelwerke übernommen.

| ID | Herausgeber, Fassung | Relevante Fundstelle / Prüfung |
|---|---|---|
| Q1 | [DJB-Wettkampfordnung, Stand Dezember 2025][Q1] | Aktuell über DJB-WKO-Seite verlinkt; §§ 2.3, 2.8-2.9, 3.2-3.4, 3.10, 3.12; insbesondere PDF S. 13, 17-19, 28. |
| Q2 | [DJB-Kampfregeln 2026, Grundlage März 2026][Q2] | Art. 4, 6-8, 13-20 und Nachwuchsanhang; PDF S. 10-11, 23-24, 34-35, 58-60. Direkt heruntergeladen und Text geprüft. |
| Q3 | [DJB: Platzierungen im Pool, Datei 2023][Q3] | S. 1, Einzelwettkämpfe und UP/DOWN; nicht ungeprüft als aktuelle vollständige Punktetabelle verwenden. |
| Q4 | [NWJV-Wettkampfordnung, 25.04.2026][Q4] | §§ 2.1, 2.9, 2.9.1, 3.2.1; PDF S. 5, 9-12. Die aktuellere Fachseite wurde gegenüber dem Januar-Hinweis der Startseite bevorzugt. |
| Q5 | [NWJV-Jugendregeln 2026, Linkfassung 27.03.2026][Q5] | Beide Seiten visuell geprüft; abweichender interner Fußstand 28.01.2025 ist in K-02 dokumentiert. |
| Q6 | [NWJV: Wettkampflisten][Q6] | Offizieller Katalog der Listen und Qualifikationsvariante; keine technische Excel-/Formelprüfung durchgeführt. |
| Q7 | [JVSH: Downloadbereich, Wettkampflisten][Q7] | Beleg für unterschiedliche Poolgrößen und Doppelpool-Listen; kein bundesweiter Standard daraus abgeleitet. |
| Q8 | [IJF SOR, 24.07.2026][Q8] | §§ 2.5-2.6, PDF S. 37-42; Systemdefinitionen und Abgrenzung der internationalen Poolwertung. |
| Q9 | [DJB: Alters- und Gewichtsklassen 2026][Q9] | Einseitige Übersicht; inhaltlich 2026 trotz älterem PDF-Metadatentitel. |
| Q10 | [WJV: Wettkampf-Downloads][Q10] | Katalog regionaler Kleinfeldlisten; nicht als NWJV-Vorschrift verwendet. |

## 12. Abschluss AP 01 und Betriebsschritte

AP 01 ist als Recherche und Dokumentation abgeschlossen. Diese Grundlage und die Klärpunkte sind vor der Umsetzung eines betroffenen Einzelturnier-Pakets zu lesen. Es wurde kein Folgepaket begonnen.

- Projekt-Dokumentationsstand: **V0.2.33**.
- Unveränderter Softwarestand von Server und Desktop: **V0.2.32**.
- **01 nicht erforderlich. 02 nicht erforderlich. 03 nicht erforderlich.**
- Keine Installation, kein Build und kein Praxistest erforderlich, um diese Dokumentation zu verwenden. Es wird keine neue Softwarefunktion als getestet behauptet.

[Q1]: https://www.judobund.de/fileadmin/user_upload/judobund.de/Downloads/Regeln_und_Ordnungen/2026_neue_Ordnungen/2025-DJB-WKO_final.pdf
[Q2]: https://www.judobund.de/fileadmin/user_upload/judobund.de/Downloads/Regeln_und_Ordnungen/2026_neue_Ordnungen/2026_03_DJB-Kampfregeln_ueberarbeitet.pdf
[Q3]: https://www.judobund.de/fileadmin/user_upload/judobund.de/Downloads/Regeln_und_Ordnungen/20763-DJB_Regeln_und_Ordnungen_Platzierungen_im_Pool_2023.pdf
[Q4]: https://www.nwjv.de/fileadmin/nwjv/dokumente/ordnungen/2026-04-25_NWJV-Wettkampfordnung.pdf
[Q5]: https://www.nwjv.de/fileadmin/dokumente/jugend/2026-03-27_Jugendregeln_NWJV.pdf
[Q6]: https://www.nwjv.de/vereinsservice/downloads/wettkampflisten
[Q7]: https://www.jvsh.net/content/downloads/
[Q8]: https://78884ca60822a34fb0e6-082b8fd5551e97bc65e327988b444396.ssl.cf3.rackcdn.com/up/2026/07/IJF_Sport_and_Organisation_Rul-1784897879.pdf
[Q9]: https://www.judobund.de/fileadmin/user_upload/judobund.de/Downloads/Regeln_und_Ordnungen/2026_Alters-_und_Gewichtsklassen.pdf
[Q10]: https://www.wjv.de/de/service/downloads/wettkampf
