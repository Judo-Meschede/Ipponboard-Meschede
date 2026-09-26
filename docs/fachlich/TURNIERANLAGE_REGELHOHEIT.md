# Architekturentscheidung – Turnieranlage und Regelhoheit

Stand: 26.09.2026  
Dokumentrevision: 1.0

## 1. Zweck

Dieses Dokument legt die im Anschluss an AP 06 gemeinsam getroffenen Grundsatzentscheidungen für Einzelturniere, Kampftage und die Regelkonfiguration verbindlich fest. Es ist Grundlage für die folgenden kleinen Umsetzungspakete.

## 2. Regelhoheit und Offline-Betrieb

- Verbindliche Regelkonfiguration liegt beim konkreten Turnier auf dem Server.
- Beim Laden eines Turniers erhält die Scoreboard-App einen vollständigen lokalen Regel-Snapshot.
- Der laufende Wettkampfbetrieb bleibt vollständig offline-fähig.
- Regeln dürfen bei Bedarf auch offline direkt in der Scoreboard-App korrigiert werden.
- Offline-Regelkorrekturen werden lokal gespeichert und nach Wiederherstellung der Verbindung zum Server zurücksynchronisiert.
- Dadurch erhalten weitere Matten anschließend den korrigierten Turnierstand.
- Regeländerungen sind auch während eines laufenden Turniers zulässig.
- Ein bereits gestarteter Kampf behält immer den Regel-Snapshot, mit dem er gestartet wurde.
- Änderungen gelten erst für danach gestartete Kämpfe.
- Bereits abgeschlossene Kämpfe werden durch spätere Regeländerungen nicht rückwirkend verändert.
- Bei späterer Mehrmatten-Synchronisation müssen Konflikte bei konkurrierenden Regeländerungen technisch erkannt und behandelt werden. Es darf keine unbemerkten zwei Wahrheiten geben.

## 3. Regelprofile

- Für jedes Turnier ist ein Regelprofil verpflichtend.
- Standardmäßig wird das hinterlegte aktuelle IJF-Regelprofil vorausgewählt.
- Eigene wiederverwendbare Regelprofile sind zulässig.
- Gespeicherte Regelprofile dürfen nur durch den Admin verändert werden.
- Eine Änderung der übernommenen Regeln in einem konkreten Turnier verändert das zugrunde liegende Regelprofil nicht.
- Abweichungen vom ausgewählten Regelprofil werden nicht besonders als Warnung markiert. Der im Turnier gespeicherte Wert ist die gültige Turnierregel.
- Wertungen, Strafen, Shido/Hansoku-make-Systematik, Osaekomi-Zeiten, Golden-Score-Regeln und weitere kampfrelevante Parameter gehören strukturiert in das Regelprofil bzw. den daraus erzeugten Turnier-Snapshot.
- Freitextfelder wie „Sonderbestimmungen“ dürfen nicht den Eindruck erwecken, eine technische Regelwirkung zu besitzen.
- Sobald eine bisher lokale Scoreboard-Einstellung in die zentrale Turnier-/Regelkonfiguration überführt wurde, darf sie nicht parallel als unabhängige zweite Einstellung weitergeführt werden.
- Entweder ist sie in der App nur lesbar oder eine Änderung in der App wird als Turnieränderung zum Server synchronisiert.

## 4. Altersklassen und Kampfzeiten

- Das Regelprofil gilt grundsätzlich für das gesamte Turnier.
- AK-abhängige Parameter werden innerhalb dieses Profils bzw. der Turnierkonfiguration je Altersklasse geführt.
- Die Kampfzeit wird pro ausgewählter Altersklasse gespeichert.
- Bei Auswahl einer Altersklasse wird die Vorgabe des gewählten Regelprofils automatisch eingesetzt.
- Die vorgeschlagene Kampfzeit ist für das konkrete Turnier editierbar.
- Sonder-Altersklassen benötigen ebenfalls eine definierte Kampfzeit.
- In der Turnieranlage zeigt die Altersklassenauswahl nur die für die Auswahl notwendigen Informationen und die jeweilige Kampfzeit. Weitere Regelparameter bleiben im Regelprofil.
- Altersklassen werden über einen kompakten Auswahldialog gewählt. In der geschlossenen Ansicht werden nur die gewählten Altersklassen samt Kampfzeit kompakt zusammengefasst.

## 5. Geschlecht, Gewichtsklassen und gewichtsnahe Einteilung

- Gewichtsklassen werden pro Altersklasse und Geschlecht getrennt verwaltet.
- Das Regelprofil darf passende Vorgaben liefern. Das konkrete Turnier darf davon abweichen.
- Gewichtsklassen werden nicht automatisch aus anderen Turnieren erfunden oder übernommen.
- Neben festen Gewichtsklassen wird eine gewichtsnahe Einteilung unterstützt.
- Bei gewichtsnaher Einteilung ist die gewünschte Poolgröße frei einstellbar.
- Beispiel: Poolgröße 4 bedeutet, dass das System möglichst Vierergruppen mit möglichst geringem Gewichtsunterschied bildet.
- Abweichungen aufgrund der tatsächlichen Teilnehmerzahl müssen sinnvoll behandelt werden.
- Die automatisch vorgeschlagene Poolzusammensetzung bleibt vor endgültiger Bestätigung manuell veränderbar.

## 6. Wettkampfmodus

- Der Wettkampfmodus wird nicht pauschal für das gesamte Turnier festgelegt.
- Er kann je tatsächlich gebildeter Wettkampfklasse gewählt werden.
- Damit können abhängig von Teilnehmerzahl und Klasse unterschiedliche Systeme verwendet werden, beispielsweise Jeder-gegen-jeden, Pools, Doppel-KO, vorgepooltes KO oder Brasilianisches KO.
- Klassenbildung, Wettkampfmodus, Kampfregelprofil und Platzierungslogik bleiben fachlich getrennte Konzepte.

## 7. Neue Verwaltungsoberfläche

Die bisherige dauerhafte Kombination aus Liste und schmalem Bearbeitungsformular wird für Einzelturniere und Kampftage aufgegeben.

### Einzelturniere
- Beim Öffnen des Bereichs erscheint fensterfüllend die Liste aller angelegten Einzelturniere.
- Die Liste ist sortier- und filterbar.
- Standardsortierung: Datum absteigend.
- Es werden standardmäßig alle Veranstaltungen angezeigt.
- Spalten mindestens: Datum, Turniername, Ort, Altersklassen, Status.
- Oben befindet sich die Aktion „Neues Einzelturnier“.
- Neuanlage öffnet einen nahezu fensterfüllenden Dialog.
- Klick auf eine bestehende Zeile öffnet denselben Dialog zur Bearbeitung.
- Rechts in der Zeile gibt es nur eine separate Löschaktion.

### Kampftage
- Gleiche Bedienlogik wie bei Einzelturnieren.
- Spalten mindestens: Datum, Kampftag/Veranstaltung, Ort, Mannschaften, Status.
- Oben befindet sich die Aktion „Neuer Kampftag“.

### Anlage Einzelturnier
Die normale Anlage wird kompakt gegliedert in:
1. Grunddaten
2. Altersklassen
3. männlich/weiblich
4. Gewichtsklassen bzw. gewichtsnahe Einteilung
5. Regelwerk

Altersklassen und Gewichtsklassen werden nicht als dauerhaft ausgebreitete Checkbox-Wände dargestellt, sondern über kompakte Auswahlfenster mit Zusammenfassung der Auswahl.

## 8. Löschen und abgeschlossene Veranstaltungen

- Löschen erfordert immer eine Sicherheitsabfrage mit dem konkreten Veranstaltungsnamen.
- Admin darf auch gestartete oder abgeschlossene Veranstaltungen löschen.
- Bei gestarteten oder abgeschlossenen Veranstaltungen ist eine verschärfte Bestätigung erforderlich: Der vollständige Veranstaltungsname muss eingegeben werden.
- Abgeschlossene Veranstaltungen sind zunächst schreibgeschützt.
- Über „Bearbeitung wieder freigeben“ können sie erneut bearbeitet werden.
- Der Status „abgeschlossen“ bleibt dabei bestehen.
- Die Bearbeitung kann anschließend wieder manuell gesperrt werden.
- Eine revisionssichere Änderungsprotokollierung ist für diesen Anwendungsfall nicht erforderlich.

## 9. PIN-Schutz der Serververwaltung

- Der gesamte Verwaltungsbereich des Scoreboard-Webservers erhält eine PIN-Schranke.
- Technische Scoreboard-Synchronisationsendpunkte bleiben davon getrennt, damit Matten ohne manuelle PIN-Eingabe synchronisieren können.
- Es gibt zunächst eine gemeinsame Verwaltungs-PIN, keine eigene Benutzer-/Rollenverwaltung.
- Initiale PIN: `SSV!`.
- „Auf diesem Gerät angemeldet bleiben“ wird unterstützt.
- Die Scoreboard-PIN wird ausschließlich in den Systemeinstellungen der Judo-App gesetzt bzw. geändert.
- In der Scoreboard-Verwaltung selbst gibt es keine Funktion zum Ändern der PIN.
- Damit ist keine separate Notfall-PIN-Logik erforderlich.
- Die technische Kopplung zwischen Judo-App und Scoreboard muss ohne Klartext-Doppelpflege umgesetzt werden.

## 10. Umsetzungsschnitt

Dieses Dokument beschreibt Zielarchitektur und Bedienlogik. Es behauptet nicht, dass diese Funktionen bereits implementiert sind.

Die Umsetzung erfolgt in kleinen Arbeitspaketen. Zuerst wird die bestehende Webverwaltung auf die neue fensterfüllende Listen-/Dialogstruktur umgestellt. Danach folgen schrittweise das strukturierte Turnierregelmodell, AK-Kampfzeiten, AK-/Geschlechts-bezogene Gewichtsklassen, gewichtsnahe Poolparameter, Regel-Synchronisation zur Desktop-App und PIN-Schutz.

Desktop/Qt wird erst in einem ausdrücklich dafür vorgesehenen Arbeitspaket verändert.
