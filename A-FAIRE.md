# À faire — D16Y3 / Speeduino

Dernière mise à jour : 25 novembre 2026 (résultats de la séance 1). Les constats et leurs sources sont dans `cartographies/README.md` (essais, journaux) et `fiches-techniques/README.md` (pièces, recherches).

## Méthode

- **Un seul changement par essai**, et **toujours un journal** (`.mlg`) dès que le moteur tourne.
- Chaque mesure est notée tout de suite, au format du bas de page.
- Une étape qui échoue n'arrête pas la séance : on la note et on passe à la suivante.
- L'électrique d'abord, le moteur tournant ensuite, la carto en dernier (une seule **carto 7** avec tous les résultats).

## Matériel à préparer

- Multimètre, thermomètre infrarouge.
- Stroboscope + **rallonge haute tension** de bougie (maison ou Lisle 20580).
- Chargeur : **batterie chargée la veille**.
- Fil, cosses, un « add-a-fuse ».
- PC avec TunerStudio et la **carto 6** (`cartographies/2026-11-15-test-6-ralenti-chaud-plus.msq`).

## Séance 1 — moteur arrêté (≈ 1 h)

| # | Action | Attendu | Sinon |
|---|---|---|---|
| 1 | **Masse du boîtier à fusibles** : cosse refaite, sur métal nu, serrée (incident du 25/11) | Aucun jeu | — |
| 2 | Batterie chargée : tension au repos | ≥ 12,4 V | Batterie à tester ou remplacer |
| 3 | Contact mis : **5 V de la carte** au multimètre | ≈ 5,0 V | Régulateur LM2940 : on s'arrête et on regarde ensemble |
| 4 | Contact mis, TunerStudio : papillon 0 → 100 %, eau et air ≈ ambiante, batterie ≈ 12 V | Cohérent | Noter la valeur fausse |
| 5 | **Masse carte ↔ − batterie** à l'ohmmètre, contact coupé | ≈ 0 Ω | Masse de la carte à refaire |
| 6 | **2ᵉ fil de la sonde d'eau** : masse capteurs de la carte, ou bloc moteur ? | Masse capteurs de la carte | Noter (piste des sauts de température) |
| 7 | **Électrovanne** (IDC 37) : l'autre fil a-t-il le 12 V **après contact** ? | 12 V contact mis, 0 V coupé | Le passer sur un après-contact (fusible 17 à vérifier) |
| 8 | **Ventilateur** : ponter les 2 bornes du connecteur du thermocontact | Il tourne | Autre relais, puis fusibles 17 (7,5 A, sous le tableau de bord) et 57 (sous le capot) — numéros US à vérifier |
| 9 | **Cavaliers JP7 (ventilateur) et JP9 (compte-tours)** sur la carte v0.4 : posés ? reliés à quoi ? | Notés | Câblage à part (« Plus tard ») |
| 10 | **MAP** : tension sur l'Arduino **A3**, contact mis | ≈ 2,9 V | ≈ 0,75 V = le signal se perd sur la carte |
| 11 | **Lambda** : repérer les 4 fils (paire de faible résistance = chauffage) ; 12 V sur le chauffage contact mis | Paire trouvée, 12 V | Noter |
| 12 | **Sondes eau et air à froid** : résistance de chacune + température ambiante au thermomètre | 2 chiffres par sonde | — |

### Résultats de la séance 1 (25/11, propriétaire)

| # | Mesure | Lecture |
|---|---|---|
| 1 | Masse du boîtier à fusibles refaite | OK |
| 2 | Batterie au repos | 12,6 V : bonne |
| 3 | 5 V de la carte | 5,1 V : bon |
| 4 | TunerStudio contact mis | Papillon OK (eau et air affichés : à noter) |
| 5 | Masse carte ↔ − batterie | 0,4 Ω : à recontrôler (cordons du multimètre à soustraire ; la chute de tension moteur tournant, étape 15, tranchera) |
| 6 | 2ᵉ fil de la sonde d'eau | **Va au boîtier à fusibles**, pas à la masse capteurs de la carte : **cause probable des sauts de température** (hypothèse forte) |
| 7 | Électrovanne, autre fil | 12,49 V contact mis (0 V contact coupé : à confirmer) |
| 8 | Ventilateur ponté | Tourne : relais, fusibles et moteur bons ; reste le thermocontact ou la commande Speeduino |
| 9 | Cavaliers JP7 / JP9 | Photo annoncée, pas reçue |
| 10 | MAP sur l'Arduino A3 | **0,77 V** (noté « ohm », lu comme volts) = les 26 kPa affichés : le signal du capteur n'arrive pas à A3. Piste : **cavalier JP11** (choix capteur MAP intégré / externe, schéma v0.4) et continuité broche MAP-EXT de l'IDC → A3 |
| 11 | Lambda | **Un seul fil branché** : ni chauffage, ni masse du signal. Explique l'absence de signal |
| 12 | Sondes à froid | Eau **3,1 kΩ** (bloc 17,2 °C, radiateur 14,7 °C) ; air **2,9 kΩ** (≈20 °C) |

### Corrections avant la séance 2

- [ ] **Masse de la sonde d'eau** (et de toutes les sondes : air, papillon, MAP, capteurs Hall) sur une **broche GND de la carte**, pas sur le boîtier à fusibles.
- [ ] **MAP** : position du cavalier JP11 ; tension sur la broche MAP-EXT de l'IDC et sur A3.
- [ ] **Lambda** : brancher les 4 fils (chauffage : 12 V après contact protégé par fusible + masse ; signal → entrée O2 ; masse du signal → GND capteurs de la carte).
- [ ] Ventilateur : thermocontact à l'ohmmètre dans l'eau chaude ; ou commande Speeduino par JP7.

## Séance 2 — moteur tournant, carto 6, journal lancé (≈ 30 min)

| # | Action | Attendu |
|---|---|---|
| 13 | Démarrage : tension **au démarreur** (au plus bas) | Le chiffre (7,9 V le 15/11) |
| 14 | Ralenti jusqu'au ventilateur ou ≈ 85 °C au thermomètre infrarouge : régime à 40, 60 et 80 °C. **Sans ventilateur : surveiller l'eau, couper si elle monte franchement** | ≈ 800-950 tr/min chaud |
| 15 | À chaque saut de température dans TunerStudio : **tension aux bornes de la batterie** et **masse carte ↔ − batterie** (en V) | Batterie stable, masse < 0,1 V |
| 16 | **Sonde d'eau à chaud** : résistance + température au thermomètre infrarouge (boîtier de thermostat) | 3ᵉ point de calibration |
| 17 | **Lambda** : signal côté sonde, puis sur l'Arduino **A8** | Bouge 0,1-0,9 V, ou reste haut si riche |
| 18 | **Calage** : Fixed Angle 10° → stroboscope sur la rallonge du cylindre 1 → angle lu → Fixed Angle Off | L'écart en degrés |
| 19 | Tension moteur à ≈ 2 000 tr/min | 13,5-14,5 V |
| 20 | Couper, **envoyer le journal** et les mesures | — |

## Ensuite — sans la voiture

1. Décoder le journal ; trancher la cause des sauts de température (masse ou sonde).
2. **Carto 7** en une fois : calibration eau et air (3 points), Trigger Angle corrigé, courbe d'électrovanne finale, lambda activée si son signal est bon, MAP recalibré si la cause est trouvée.
3. Mettre à jour `cartographies/README.md` et ce fichier.

## Plus tard

- Pression d'essence au manomètre (régulateur AEM, usine 40 psi = 276 kPa, jamais mesurée).
- Lambda large bande (Bosch LSU 4.9 + contrôleur) ; sonde Denso 065500-2730 gardée en secours.
- Réglage de la VE à partir des journaux (VE Analyze Live absent de MS Lite).
- Câblage Speeduino → relais de ventilateur (JP7) et → compte-tours du tableau de bord (JP9).
- Fréquence de l'électrovanne (160 Hz dans le tune, valeur Honda non trouvée).
- Temps d'ouverture réel des injecteurs (1,0 ms dans le tune, valeur Honda non trouvée).
- Étincelle à la mise sous tension : essai USB branché d'abord, puis contact.
- Compressions : confirmer l'unité (bar ?) et les conditions de mesure.

## Déjà fait

- Injecteurs 12 Ω ; électrovanne 11,3 Ω, clic en 12 V direct, raccordée à IDC 37 ; papillon calibré.
- Cartos 4 à 6 : dwell 1,8 ms, ralenti PWM qui tient, axes des tables jusqu'à 100 %.
- Incident du 25/11 (boîtier à fusibles sans masse, bruit de la carte) : tout est revenu sans intervention ; les étapes 1 à 5 servent aussi de contrôle.

## Format pour renvoyer les mesures

```
1 ok / 2 12,5 V / 3 4,98 V / 4 ok / 5 0,2 Ω / 6 bloc moteur / 7 12 V après contact / 8 tourne / 9 JP7 non, JP9 non / 10 2,9 V / 11 chauffage = fils blancs, 12 V ok / 12 eau 2,4 kΩ à 18 °C, air 2,3 kΩ à 18 °C
```
