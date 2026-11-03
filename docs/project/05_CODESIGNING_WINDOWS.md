# Codesignierung der portablen Windows-Version

## Ziel

Eine vertrauenswürdige digitale Herausgebersignatur für beide Programme:

- `Ipponboard-Meschede.exe` im USB-Stammverzeichnis (Starter)
- `app/Ipponboard-Meschede.exe` (Qt-Anzeigetafel)

Damit zeigt Windows den verifizierten Herausgeber an. Microsoft Defender SmartScreen bewertet zusätzlich die Datei- und Herausgeberreputation; eine unmittelbare Warnungsfreiheit ist auch mit Zertifikat **nicht** garantiert.

## Voraussetzung

Ein gültiges Codesignaturzertifikat einer öffentlich anerkannten Zertifizierungsstelle mit privatem Schlüssel muss unter **Aktueller Benutzer → Eigene Zertifikate** verfügbar sein. Selbstsignierte Zertifikate verbessern die SmartScreen-Reputation nicht.

Das Zertifikat und sein privater Schlüssel dürfen niemals in GitHub, auf USB-Exportvorlagen oder unverschlüsselt im Projekt gespeichert werden.

## Aktivierung am Windows-Entwicklungsrechner

Vor dem Ausführen des vorhandenen Buildskripts in derselben PowerShell-Sitzung den SHA1-Fingerabdruck des vorhandenen Zertifikats in die Umgebungsvariable `IPPONBOARD_SIGN_CERT_THUMBPRINT` eintragen. Der Fingerabdruck ist kein privater Schlüssel.

Das Buildskript sucht `signtool.exe` aus dem Windows SDK, signiert die beiden EXE-Dateien mit SHA-256 und einem RFC-3161-Zeitstempel und überprüft jede Signatur. Jeder Fehler stoppt den Build vor der Erstellung der USB-ZIP.

Ohne Zertifikat entsteht nur eine **nicht signierte Vorschauversion** mit ausdrücklicher Warnmeldung. Diese ist nicht als warnungsfreier Release zu bezeichnen.

## Grenzen

Bei Hardware-Token, Cloud-HSM oder Microsoft Artifact Signing kann eine gesonderte Signierintegration notwendig sein; der Build unterstützt zunächst ein lokal für SignTool zugängliches Zertifikat. Code-Signing-Reputation kann sich erst durch vertrauenswürdige Verteilung aufbauen.
