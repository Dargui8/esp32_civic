# À faire — D16Y3 / Speeduino

Dernière mise à jour : 25 novembre 2026 (plan n° 2 : séance 1 bis, puis séance 2). Les constats et leurs sources sont dans `cartographies/README.md` (essais, journaux) et `fiches-techniques/README.md` (pièces, recherches).

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

### Ce que montrent les photos (25/11)

- **Pastilles JP6 à JP10** (dessous de la carte) : **ouvertes**. Les sorties ventilateur (JP7) et compte-tours (JP9) n'arrivent pas au connecteur : c'est pourquoi le ventilateur n'a jamais démarré sur commande du Speeduino.
- **Capteur MAP MPX4250AP soudé sur la carte**, en plus du Denso de la voiture. A3 lit 0,77 V moteur arrêté : ni ≈ 2,9 V (Denso), ni ≈ 1,8 V (MPX4250A, formule NXP citée dans `speeduino.ini`). Le **cavalier JP11** choisit le capteur qui va sur A3.
- **Lambda à 1 fil**, non chauffée : signal seulement très chaude, masse par l'échappement.
- Un **fil jaune** soudé sur la carte : rôle à identifier.
- Masses des sondes refaites (propriétaire, 25/11).

## Séance 1 bis — moteur arrêté (≈ 45 min)

| # | Action | Attendu | Sinon |
|---|---|---|---|
| A | Lister les sondes sur la masse de la carte (eau, air, papillon, MAP, capteurs Hall) | Toutes | Finir avant la séance 2 |
| B | **MAP** : position de **JP11** (3 broches près du MPX4250) ; tension sur A3, contact mis, masses refaites | Le capteur voulu sur A3 | — |
| C | **MAP Denso** : tension au connecteur du capteur, puis sur A3 | ≈ 2,9 V aux deux | Bon au capteur, faux sur A3 = JP11 ou piste |
| D | **Ventilateur** : IDC 15 (FAN, proto 2, reliée à JP7) est **libre et non routée** sur l'adaptateur (circuit imprimé du propriétaire, 25/11). Pour sortir : fil volant comme le fil jaune, ou fil vers une voie libre de J2 (9, 12 ou 13 marquées « x », à confirmer) | Voie choisie | — |
| E | **Ponter JP7 à l'étain**, relier IDC 15 au **fil GRN** du thermocontact (côté relais, manuel Honda 23-112) | — | — |
| F | Test : `fanSP` à 20 °C → Burn → contact mis | Le ventilateur tourne | Revoir E |
| G | Remettre `fanSP` à 90 °C → Burn | — | — |
| H | **Fil jaune** : ses deux bouts (photo) | Noté | — |
| I | **Lambda** : le fil va-t-il à l'entrée O2 (Arduino A8) ? | Oui | Le raccorder |

*Compte-tours : JP9 → IDC 17 (TACH, proto 4) → fil **BLU** de l'ancien module d'allumage, au combiné borne C9 (manuel 23-114). Plus tard.*

### Résultats de la séance 1 bis (26/11, propriétaire)

| # | Lecture | Interprétation |
|---|---|---|
| A | Toutes les sondes sur la masse de la carte | Fait |
| B | A3 = 0,72 V ; JP11 : position non relevée | Toujours ≈ 26 kPa |
| C | Au connecteur du capteur MAP : 5,01 V, signal **1,75 V**, masse OK | Le signal se perd entre le capteur (1,75 V) et A3 (0,72 V). Et 1,75 V est bas pour un capteur Honda à l'arrêt (≈ 2,9 V attendus, données revendeur) : la carte tire peut-être le signal vers le bas |
| D-E | IDC 15 libre ; JP7 ponté ; relié au fil GRN | Fait |
| F | `fanSP` = 0 °C, Burn : **le ventilateur ne s'enclenche pas** ; mis à la masse à la main, il tourne | Le relais et le fil GRN sont bons ; la commande de la carte n'arrive pas |
| H | Fil jaune = électrovanne de ralenti (IDC 37) | Confirmé |
| I | Lambda sur IDC 21 ; « pas de continuité » entre 21 et A8 | Normal en mode bip : une résistance de **470 Ω** (R31) est en série sur la carte. À mesurer en ohms |

### Contrôles suivants (moteur arrêté)

- [x] **MAP, capteur seul sur une alimentation 5 V externe** (propriétaire, 26/11) : **signal = 5 V**, collé à l'alimentation. Capteur probablement hors service (hypothèse), ou broches signal/5 V confondues. Vérifier : dépression sur l'embout (seringue, bouche) → le signal bouge-t-il ? Sinon : passer au **MPX4250AP de la carte** (plan B) ou remplacer le capteur.
- [x] **Nouveau capteur MAP monté** (propriétaire, 26/11) : **Denso 079800-3000 identique**, calibration inchangée ; contrôles : signal au capteur et sur A3 moteur arrêté, valeur dans TunerStudio (≈ 100 kPa attendus), dépression → la valeur baisse ; calibration du tune (`mapMin`/`mapMax`) à adapter au nouveau capteur.
- [x] Nouveau capteur : **A3 toujours 0,78 V** ; **470 Ω** entre IDC 11 et A3 : la résistance série est bonne (propriétaire, 26/11). Hypothèse : quelque chose tire A3 vers la masse sur la carte (cavalier JP11, protection « ADC-Clamp-2 » ou condensateur de l'entrée, ou broche A3 de l'Arduino abîmée lors de l'incident du 25/11).
- [x] **Capteur neuf seul, sans la carte, 5 V et masse appliqués : ≈ 0,7 V** (propriétaire, 26/11) = ce que lit A3 : **pas de fuite sur la carte** (hypothèse précédente écartée). Ancien capteur au même test : 5 V. Deux capteurs « faux » dans deux sens opposés = **brochage probablement mal identifié** (hypothèse). Piste non officielle pour un 079800-4250 : rouge/vert = +5 V, jaune/rouge = signal, vert/blanc = masse ([Scribd](https://www.scribd.com/document/651664678/MAP-Sensor-Data)) — à vérifier.
- [x] Autres branchements du capteur neuf sur le banc : aucun ne donne de signal (propriétaire, 26/11).
- [ ] Capteur neuf, branchement qui donne 0,7 V : **aspirer sur l'embout** — la tension bouge-t-elle ? Brochage et tension contact mis à lire dans le **manuel d'atelier Honda** (section PGM-FI, test du capteur MAP).
- [x] Précision (propriétaire, 26/11) : les **1,7 V** relevés venaient du **fil signal côté carte, capteur débranché**. Lecture : ≈ 1,82 V attendus pour le **MPX4250A de la carte à la pression atmosphérique** (formule NXP) → le capteur intégré vit et arrive déjà sur le fil MAP (JP11 probablement ponté 2-1). Capteur Denso branché, ses 0,7 V l'emportent → A3 0,78 V. La calibration du tune ne fait que convertir la tension : pour garder le Denso il faudrait sa vraie courbe (2ᵉ point avec un vacuomètre).
- JP7 ponté et fil vert soudé (photos du 26/11).
- [x] **Plan B en place** (propriétaire, 26/11) : Denso débranché, **1,8 V sur A3**, durite raccordée au capteur de la carte. Reste : préréglage « MPX4250A » (`mapMin` 10, `mapMax` 260) dans TunerStudio → ≈ 100 kPa à l'arrêt (10 + 1,8 / 5 × 250), baisse au ralenti. À reporter dans la carto 7.
- [ ] **Plan B recommandé** : capteur **MPX4250AP de la carte** — JP11 déjà ponté 2-1 selon toute vraisemblance (à confirmer : ≈ 1,8 V sur A3 capteur Denso débranché), **Denso débranché**, durite de dépression du collecteur vers l'embout de la carte, préréglage « MPX4250A » 10-260 kPa ; contrôle ≈ 1,8 V sur A3 à l'arrêt.
- [ ] **Brochage** : capteur débranché, contact mis, côté faisceau : quel fil a 5 V, quel fil est à la masse, quel fil va à l'IDC 11 ? Puis capteur branché, signal mesuré en piquant le bon fil ; aspirer sur l'embout → la tension doit baisser.
- [ ] Tension au connecteur du capteur, carte branchée (≈ 2,9 V attendus) ; **résistance A3 ↔ masse, tout hors tension** (attendu : élevée, des dizaines de kΩ ou plus) ; photo de JP11 ; puis même mesure Arduino Mega retiré du shield pour savoir si c'est le shield ou l'Arduino.
- [ ] **MAP** : tension sur l'IDC 11 côté carte ; résistance IDC 11 ↔ A3 (≈ 470 Ω attendus) ; **photo de JP11**.
- [ ] **Plan B MAP** : utiliser le **MPX4250AP de la carte** (JP11 sur le capteur intégré, durite de dépression du collecteur vers l'embout de la carte, préréglage « MPX4250A » 10-260 kPa ; ≈ 1,8 V sur A3 à l'arrêt).
- [x] Calibration MPX4250A faite ; `fanSP` = 0 : **indicateur Fan allumé dans TunerStudio**, ventilateur arrêté (propriétaire, 26/11) → la commande logicielle part, la panne est entre l'Arduino et le fil GRN.
- [x] Fan ON dans TunerStudio : **0 V relevé sur l'Arduino 47** (propriétaire, 26/11). Or le firmware met la broche à l'état haut en même temps qu'il allume l'indicateur (`auxiliaries.cpp` 202402, `fanControl` : `FAN_ON()` puis le bit Fan ; `fanInv` = No). Donc : point de mesure à confirmer, ou broche tirée à la masse.
- [x] U6 ULN2003, Fan ON (propriétaire, 26/11) : broche 9 = 10,3 V, **broche 5 = 0 V, broche 12 = 0 V**, broche 8 = 0 V. Lecture : (1) la commande de l'Arduino 47 n'arrive pas (Arduino ou entrée de U6 en cause) ; (2) sortie au repos à 0 V alors que la bobine du relais devrait la tirer à 12 V : **le chemin U6 broche 12 → JP7 → IDC 15 → fil GRN est coupé**.
- [x] Continuité U6 broche 12 ↔ JP7 ↔ IDC 15 : **oui** (propriétaire, 26/11 ; première mesure sur une broche mal comptée). Le chemin vers le fil GRN est bon. Les tensions des broches 5 et 12 sont à **remesurer** avec le bon comptage.
- [x] U6 broche 5 ↔ masse, hors tension : **8,9 kΩ** = entrée de l'ULN2003 normale (ordre de grandeur des résistances internes de la puce). → **L'Arduino 47 ne sort pas sa commande** (hypothèse : sortie abîmée lors de l'incident du 25/11).
- [x] **Arduino Mega remplacé** (propriétaire, 26/11), tune rechargé : eau et air affichent **215** (valeur maximale). Cause : les tables des sondes ne sont **pas dans le tune** ; elles s'écrivent par Tools → Calibrate Thermistor Tables → **Write to Controller** ([wiki *Sensor Calibration*](https://github.com/speeduino/wiki.js/blob/master/configuration/Sensor_Calibration.md)). Idem pour la lambda (Calibrate AFR Table).
- [x] Nouvel Arduino, Fan ON : **U6 broche 5 = 5 V** (la commande arrive : l'ancien Arduino 47 était bien en cause), **broche 12 = 0 V**, ventilateur arrêté (propriétaire, 26/11).
- [ ] Fan OFF (`fanSP` 90) : broche 12 ≈ 12 V ? Si 0 V aussi : rien ne tire la sortie, donc le fil vert n'atteint pas le fil GRN sous tension (continuité IDC 15 ↔ fil GRN au thermocontact ; 12 V sur le fil GRN thermocontact débranché, contact mis).
- [ ] Recalibrer eau et air (provisoire : préréglage GM, résistance de rappel 2490 Ω ; définitif : 3 points mesurés), lambda (Narrowband), puis retester le ventilateur avec le nouvel Arduino (broches 5 et 12 de U6).
- [ ] **Test sans soudure** : « Fan output pin » = 49, Burn, `fanSP` = 0 : **U6 broche 7 ≈ 5 V** ? Puis continuité U6 broche 10 ↔ pastille JP9 ; si oui : ponter JP9, relier IDC 17 au fil GRN.
- [ ] Une pointe sur JP7, l'autre sur chaque broche de U6 côté sorties (10 à 16) : laquelle bipe ? Aucune = piste coupée → fil de réparation U6 broche 12 → JP7.
- [ ] Hors tension : continuité U6 broche 12 ↔ IDC 15 (à travers JP7) ↔ bout du fil vert côté thermocontact ; résistance U6 broche 5 ↔ masse (quasi nulle = U6 ou Arduino abîmé).
- Contournement si l'Arduino 47 est mort : « Fan output pin » = 49 (voie compte-tours de U6, broche 7 → 10, pont JP9 → IDC 17), tant que le compte-tours n'est pas câblé.
- [ ] Mesurer sur la puce **U6 ULN2003** (schéma v0.4) : **broche 5** = entrée I5 (Arduino 47) → ≈ 5 V si Fan ON ; **broche 12** = sortie O5 (FAN) → ≈ 0-1 V si Fan ON ; broche 8 = masse, broche 9 = 12 V. Basculer `fanSP` 0 ↔ 90 et voir si la broche 5 change. Si broche 5 reste à 0 V : hors tension, résistance broche 5 ↔ masse (quasi nulle = ULN2003 ou Arduino abîmé).
- [ ] Fan ON : tension **Arduino 47** (≈ 5 V attendus = entrée du ULN2003) ; tension **IDC 15 ↔ masse**, fil GRN branché (≈ 0-1 V = sortie active ; ≈ 12 V = ULN2003 ou pont JP7 en cause) ; continuité **IDC 15 → fil GRN** au thermocontact.
- [ ] **Ventilateur** : `fanSP` = 0, Burn, contact mis ; dans TunerStudio, l'indicateur « Fan » est-il allumé ? Tension IDC 15 ↔ masse : ≈ 12 V = sortie inactive ; ≈ 0-1 V = active. Continuité des deux pastilles de JP7.
- [ ] **Lambda** : résistance IDC 21 ↔ A8 en ohms (≈ 470 Ω).

## Séance 2 — moteur tournant, carto 6, journal lancé (≈ 30 min)

| # | Action | Attendu |
|---|---|---|
| 13 | Tension **au démarreur** (au plus bas) | Le chiffre |
| 14 | Ralenti jusqu'au ventilateur : régime à 40, 60, 80 °C ; eau surveillée au thermomètre infrarouge | ≈ 800-950 tr/min chaud ; ventilateur vers 90 °C lus |
| 15 | **Sauts de température disparus ?** Sinon : tension aux bornes de la batterie et masse carte ↔ − batterie | Plus de sauts |
| 16 | **Sonde d'eau à chaud** : résistance + température au thermomètre infrarouge | 2ᵉ point |
| 17 | **Sonde d'air** démontée, sèche-cheveux + thermomètre : résistance | 2ᵉ point |
| 18 | **Lambda** : 2-3 min vers 2 500 tr/min, signal fil ↔ bloc moteur, puis sur A8 | Bascule 0,1 ↔ 0,9 V |
| 19 | **Calage** : Fixed Angle 10° → stroboscope (rallonge, cylindre 1) → angle lu → Fixed Angle Off | Écart en degrés |
| 20 | Tension vers 2 000 tr/min ; couper ; **envoyer journal et mesures** | 13,5-14,5 V |

## Ensuite — sans la voiture : carto 7

- Eau et air calibrées (points mesurés, « Calibrate Thermistor Tables »).
- MAP : préréglage du capteur retenu (« Denso 079800 » 0-173 kPa ou « MPX4250A » 10-260 kPa).
- Lambda : `egoType` Narrow Band si le signal bascule.
- Trigger Angle corrigé d'après le stroboscope ; courbe d'électrovanne ajustée à chaud.

## Plus tard

- Pression d'essence au manomètre (régulateur AEM, usine 40 psi = 276 kPa, jamais mesurée).
- Lambda large bande (Bosch LSU 4.9 + contrôleur) ; sonde Denso 065500-2730 gardée en secours.
- Réglage de la VE à partir des journaux (VE Analyze Live absent de MS Lite).
- Compte-tours du tableau de bord (JP9).
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
