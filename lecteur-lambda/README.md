# Lecteur de sonde lambda Bosch LSU ADV

Programme pour lire une sonde large bande **Bosch LSU ADV** avec un **ESP32**, un **Arduino Mega 2560** ou un **Raspberry Pi Pico**. Il fait trois choses :

1. **Affiche la richesse** : lambda à 3 décimales, AFR, barre riche/pauvre, état de la chauffe. Écrans pris en charge : TFT ST7735 (celui de `jan31a`), OLED SSD1306 ou LCD 16 × 2 en I2C.
2. **Envoie la richesse au Speeduino** : tension 0-5 V sur son entrée O2, avec 0 V = AFR 10 et 5 V = AFR 20, la même relation que le croquis `jan31a`.
3. **Pilote le chauffage de la sonde** : PWM selon la stratégie de chauffe de la fiche Bosch, puis régulation de la température par la résistance interne Ri.

La partie électronique n'est pas encore définie : le programme tourne dès maintenant avec une **sonde simulée**, pour essayer l'écran, la sortie Speeduino et le cycle de chauffe sur table.

Source principale : [fiche Bosch LSU ADV](../fiches-techniques/lambda/adv_pre_turbo.pdf) (51946891, 06/05/2021), page 2.

## État

| Partie | État |
|---|---|
| Conversion de la mesure en lambda (table Bosch) | Faite, testée sur PC |
| Stratégie de chauffe de la fiche, régulation, défauts | Faite, testée sur PC avec la sonde simulée |
| Écrans ST7735, SSD1306, LCD 16 × 2 | Écrits, pas encore essayés sur un écran |
| Sortie Speeduino (PWM filtrée ou MCP4725) | Écrite, pas encore mesurée au multimètre |
| Compilation pour les trois cartes | **Pas encore faite** : seule la syntaxe a été contrôlée, avec des en-têtes Arduino simplifiés. C'est la première chose à faire au premier essai. |
| Gains de la régulation de chauffe | Réglés sur la sonde simulée : **à revérifier sur la vraie sonde** |
| Circuit d'évaluation de la sonde | **À choisir** (voir « Ce que l'électronique devra fournir ») |

## Fichiers

| Fichier | Rôle |
|---|---|
| `LecteurLambda/LecteurLambda.ino` | Croquis à ouvrir dans l'IDE Arduino |
| `LecteurLambda/config.h` | **Tous les réglages** : carte, écran, sortie, mesure, brochage |
| `LecteurLambda/lsu_adv.*` | Table Ip → lambda de la fiche, conversions |
| `LecteurLambda/heater.*` | Stratégie de chauffe et régulation |
| `LecteurLambda/sensor_sim.*` | Sonde simulée (essais sans électronique) |
| `LecteurLambda/frontend_analog.*` | Lecture du vrai circuit : UA, UR, tension batterie (ADC interne ou ADS1115) |
| `LecteurLambda/speeduino_out.*` | Sortie vers le Speeduino |
| `LecteurLambda/display.*` | Écrans |
| `LecteurLambda/hal.*` | PWM, convertisseur et chien de garde de chaque carte |
| `LecteurLambda/app.*` | Boucle principale, journal série, commandes |
| `platformio.ini` | Compilation avec PlatformIO |
| `tests/` | Tests sur PC du cœur du programme et tracé du cycle de chauffe |
| `docs/cycle-chauffe-simule.*` | Cycle de chauffe simulé : données (CSV) et graphe (SVG) |

## Premier essai, en simulation

**En simulation, aucune sonde ne doit être branchée** : la sortie de chauffe suit la sonde simulée, pas la vraie.

### Avec l'IDE Arduino

1. Installer le paquet de la carte :
   - ESP32 : « esp32 » d'Espressif ;
   - Mega : « Arduino AVR Boards » ;
   - Pico : « Raspberry Pi Pico/RP2040 » d'Earle Philhower (pas le core Arduino Mbed).
2. Installer les bibliothèques de l'écran choisi : « Adafruit GFX Library », puis « Adafruit ST7735 and ST7789 Library » ou « Adafruit SSD1306 ». Le LCD 16 × 2 et le MCP4725 n'en demandent pas.
3. Ouvrir `LecteurLambda/LecteurLambda.ino`, choisir l'écran dans `config.h` (`CFG_ECRAN`), téléverser.

### Avec PlatformIO

```bash
cd lecteur-lambda
```

```bash
pio run -e esp32 -t upload
```

Remplacer `esp32` par `mega` ou `pico` selon la carte.

### Ce qu'on doit voir

- L'écran passe par les phases de chauffe : attente moteur (3 s), condensation (10 s), 12 V, 9 V, rampe, puis régulation. La mesure devient exploitable vers 31 s.
- Ensuite lambda oscille lentement entre environ 0,85 et 1,15.
- Sur le moniteur série (115200 bauds), une ligne CSV toutes les 100 ms : temps, phase, UA, Ip, lambda, AFR, Ri, tension batterie, tension de chauffe, rapport cyclique, tension de sortie, mesure prête, défauts.

Commandes série :

| Commande | Effet |
|---|---|
| `sortie 2.35` | Impose 2,35 V au Speeduino : sert à calibrer la sortie au multimètre |
| `sortie auto` | Revient à la mesure |
| `lambda 0.90` | Impose lambda en simulation |
| `lambda auto` | Revient au profil automatique de la simulation |

Sur ESP32 et Mega, un potentiomètre peut aussi faire varier lambda de 0,70 à 1,30 en simulation (`SIMULATION_POTENTIOMETRE`, broche 33 ou A3).

## Brochage par défaut

À modifier dans `config.h`, section 7.

| Signal | ESP32 | Mega 2560 | Pico |
|---|---|---|---|
| UA (image de Ip) | 34 | A0 | GP26 |
| UR (image de Ri) | 35 | A1 | GP27 |
| Tension batterie (pont 10 kΩ / 2 kΩ) | 32 | A2 | GP28 |
| PWM de chauffe vers le MOSFET | 26 (200 Hz, 12 bits) | 9 (490 Hz, 8 bits) | GP15 (200 Hz, 12 bits) |
| Sortie PWM vers le Speeduino | 25 (15 kHz, 12 bits) | 11 (15,6 kHz, 10 bits) | GP13 (30 kHz, 12 bits) |
| TFT : CS, DC, RST | 5, 2, 4 | 53, 49, 48 | GP17, GP20, GP21 |
| TFT : SCK, MOSI | 18, 23 | 52, 51 | GP18, GP19 |
| I2C : SDA, SCL | 21, 22 | 20, 21 | GP4, GP5 |

La fiche demande une fréquence de chauffe d'au moins 100 Hz. Sur le Pico, chauffe et sortie sont sur deux tranches PWM différentes pour avoir deux fréquences.

## Chauffe : la stratégie de la fiche

![Cycle de chauffe simulé](docs/cycle-chauffe-simule.svg)

| Phase | Tension efficace | Durée | Source |
|---|---|---|---|
| Attente moteur | 0 V | jusqu'à Ubat ≥ 13,2 V pendant 2 s (alternateur qui charge) | choix du programme |
| Condensation | 2,0 V (fiche : 1,8 à 2 V) | 10 s, réglable : la fiche la dit « propre à l'application » | fiche + réglage |
| Chauffe rapide | 12 V | 8 s au maximum | fiche |
| Palier | 9 V | 1,5 s | fiche |
| Rampe | +0,3 V/s au plus, plafond 12 V | jusqu'au point de fonctionnement (Ri ≤ 300 Ω) | fiche ; plafond et limite de 20 s ajoutés |
| Régulation | régulateur PI sur ln(Ri ÷ 300 Ω), départ à 7,5 V, 0 à 12 V | permanente | 300 Ω et 7,5 V : fiche ; régulateur : programme |

- **Tension efficace** : avec une PWM sous la tension batterie, U_eff = Ubat × √(rapport cyclique). Le programme mesure Ubat et calcule le rapport cyclique = (U_eff ÷ Ubat)².
- **Passage en régulation** dès que Ri atteint 300 Ω, même pendant la chauffe à 12 V ou le palier : une sonde déjà chaude n'est pas surchauffée.
- **Mesure exploitable** quand Ri reste à ±15 % de 300 Ω pendant 0,5 s ; perdue au-delà de ±30 %.
- **Sécurités** (ajoutées, absentes de la fiche) :
  - chauffe coupée si la batterie sort de 10,8-16,5 V, la plage d'alimentation de la fiche ;
  - aucune chauffe au-delà de la condensation si Ri n'est pas mesurée ;
  - défaut si le point de fonctionnement n'est pas atteint 20 s après le début de la rampe ;
  - défaut si Ri dépasse 3 000 Ω ou descend sous 100 Ω plus de 3 s en régulation ;
  - après un défaut, nouvel essai 10 s plus tard, deux fois au plus, puis chauffe bloquée jusqu'au redémarrage ;
  - chien de garde : la carte redémarre si le programme se bloque. La sortie de chauffe est mise à 0 avant tout le reste.

Sur la sonde simulée, la régulation est atteinte 17,5 s après le début de la chauffe rapide. 15 s après un saut de température des gaz (+150 °C, puis −200 °C), Ri est revenue à 1,5 % près de 300 Ω. **Ce modèle thermique est une approximation, pas la sonde réelle** : les gains `kp_V` et `ki_Vps` (`heater.h`) devront être vérifiés sur la vraie sonde, avec le journal série.

## Mesure de la richesse

1. **UA → Ip.** La fiche donne, à côté de Ip, la tension UA du circuit d'évaluation pour deux gains (v = 17 et v = 8). La relation UA = 1,5 V + v × 61,9 Ω × Ip redonne les 23 lignes à 0,5 mV près, aux deux gains (vérifié par les tests). Réglage par défaut : v = 17. Toute la plage de la sonde (lambda 0,65 à 5) tient alors entre 0,05 et 2,97 V, ce qui convient aux entrées 3,3 V.
2. **Ip → lambda** par la table de la fiche (23 points). Entre deux points, l'interpolation est linéaire en 1/lambda, qui suit mieux la physique de la sonde qu'une interpolation linéaire en lambda. En retirant un point de la table puis en le recalculant, l'écart maximal est de 1,24 %, contre 1,61 % en linéaire. Avec la table complète, les points sont deux fois plus proches et l'erreur est plus faible.
3. **Hors table** : sous lambda 0,65, l'écran affiche `<0.65` ; au-dessus de 5 (air libre, coupure d'injection), `AIR`.

**Précision.** Autour de lambda 1, 0,001 de lambda vaut 1,14 mV sur UA (v = 17) : la précision affichée dépend de la chaîne de mesure.

- **ESP32** : son ADC n'est précis qu'entre 150 et 2 450 mV à 11 dB ([Espressif](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/peripherals/adc.html)), soit lambda 0,67 à 2,7 environ avec v = 17.
- **Mega** : son ADC fait 10 bits sur 5 V.
- **Toutes les cartes** :
  - la correction `UA_CORR_GAIN` et `UA_CORR_DECALAGE_V` se règle avec un voltmètre, en comparant à la colonne UA_V du journal ;
  - pour une précision homogène, un **ADS1115** (16 bits, I2C) est prévu : `CFG_CONVERTISSEUR = CONVERTISSEUR_ADS1115`.

La correction de pression de la fiche (graphe « Pressure Compensation ») n'est pas appliquée : moteur atmosphérique, sonde à pression proche de l'atmosphère.

## Sortie vers le Speeduino

- **Relation** : AFR = 10 + 2 × V (0 V = AFR 10,0 ; 2,35 V = lambda 1 ; 5 V = AFR 20,0), en essence à 14,7. Réglable dans `config.h`, section 3.
- **TunerStudio** :
  - *Tools → Calibrate AFR Table*, choisir *Custom Linear WB*, 0 V = 10, 5 V = 20 ;
  - puis passer *EGO sensor type* (`egoType`) sur *Wide band*. Il est sur *Disabled* dans la carto de test et sur *Narrow Band* dans le tune actuel.
- **Entrée du Speeduino** : broche A8 sur la carte v0.4 (`init.cpp` 202402, ligne 1484). Le schéma v0.4 montre une résistance série de 470 Ω, des condensateurs et des diodes d'écrêtage, sans pont diviseur : la tension arrive telle quelle.
- **Pendant la chauffe et en défaut**, la sortie est à **5 V (AFR 20)**. Le firmware 202402 ne corrige que si `ego_min < O2 < ego_max` ([corrections.cpp, ligne 618](https://github.com/speeduino/speeduino/blob/202402/speeduino/corrections.cpp#L618)) ; au-delà, la correction revient à 100 %. Avec `ego_max_afr = 17` dans le tune, aucune correction n'est donc appliquée tant que la mesure n'est pas prête. **Garder `ego_max_afr` sous 20.**
- Si la protection AFR (`afrProtectEnabled`, coupée dans le tune) est activée un jour : elle verra AFR 20 pendant la chauffe et en défaut. Ses conditions du tune (180 kPa, 4 000 tr/min, 80 % de papillon) la rendent peu probable à la chauffe ; un défaut de sonde en pleine charge déclencherait la coupure.

**Étage de sortie** (`CFG_SORTIE`) :

| Choix | Mega | ESP32 et Pico |
|---|---|---|
| PWM filtrée | filtre RC direct (sortie 5 V) | filtre RC puis ampli ×1,52 pour passer de 3,3 à 5 V ; sans ampli, mettre `SORTIE_PLEINE_ECHELLE_V` à 3.3 et revoir la relation |
| MCP4725 alimenté en 5 V | direct | adaptateur de niveaux I2C à prévoir (circuit en 5 V, carte en 3,3 V ; à confirmer sur la fiche du MCP4725) |

Pour la calibration : `sortie 1.00` puis `sortie 4.00` au moniteur série, mesurer au multimètre sur l'entrée O2 du Speeduino, et reporter l'écart dans `SORTIE_CORR_GAIN` et `SORTIE_CORR_DECALAGE_V`.

## Ce que l'électronique devra fournir

La fiche indique que la LSU ADV ne fonctionne qu'avec une unité d'évaluation spécialisée (celle du LT4 ADV de Bosch) : la sonde ne donne pas directement une tension exploitable. Le programme attend de ce circuit :

| Signal | Attendu | Dans le programme |
|---|---|---|
| UA | Image du courant de pompe : UA = 1,5 V + v × 61,9 Ω × Ip | `LSU_UA_ZERO_V`, `LSU_SHUNT_OHM`, `LSU_GAIN` |
| UR | Image de la résistance interne Ri de la cellule de Nernst. Fiche : 300 Ω nominal, courant dans la cellule ≤ 80 µA | `UR_DECALAGE_V`, `RI_OHM_PAR_V`, fonction `urToRi()` |
| Ubat | Tension d'alimentation du chauffage, par pont diviseur | `UBAT_PONT_DIVISEUR` |
| Chauffage | MOSFET côté masse commandé par la PWM ; 8,7 W en régime établi d'après la fiche | broche `PIN_CHAUFFE` |

- **Mesure de Ri obligatoire.** La compilation en mesure analogique est **bloquée** tant que la relation UR → Ri n'est pas renseignée et `RI_CALIBRATION_FAITE` défini. Une régulation sur une Ri fausse surchaufferait la sonde.
- **Courant de chauffe** : en crête, Ubat ÷ résistance du chauffage ; il est plus élevé à froid. La résistance à froid ne figure pas dans la fiche : elle est à mesurer pour dimensionner le MOSFET et le fusible.

**Brochage de la sonde** (fiche, pages 2 et 3) :

| Broche | LSU ADV, connecteur auto 1 928 404 669 | LSU ADV pre Turbo, connecteur auto 1254.488.136 |
|---|---|---|
| 1 | IP / APE | IP / APE |
| 2 | VM / IPN | VM / IPN |
| 3 | Uh- / H- | Uh- / H- |
| 4 | Uh+ / H+ | Uh+ / H+ |
| 5 | non connectée | UN / RE |
| 6 | UN / RE | — |

Contre-pièces : F02U.B00.725-01 (ADV) et F02U.B00.937-01 (ADV pre Turbo). La version à connecteur motorsport a un autre brochage (fiche, pages 2 et 3).

## Tests sur PC

```bash
cd lecteur-lambda/tests
```

```bash
make
```

Ils vérifient :
- la table Bosch et la relation UA ↔ Ip, colonne par colonne ;
- la conversion en lambda : nœuds, monotonie, aller-retour, hors table ;
- la sortie 0-5 V ;
- la mise en forme de l'écran ;
- le cycle de chauffe complet sur la sonde simulée : durées, tensions, pente de la rampe, plafond de 12 V, tenue de Ri, justesse de lambda ;
- les sauts de température des gaz ;
- les défauts et le blocage.

Résultat actuel : 11 728 vérifications, 0 échec. `make trace` régénère `docs/cycle-chauffe-simule.csv` et le graphe.

## Sources

- Fiche Bosch Motorsport « Lambda Sensor LSU ADV/ADV pre Turbo », 51946891, 06/05/2021 : [fiches-techniques/lambda/adv_pre_turbo.pdf](../fiches-techniques/lambda/adv_pre_turbo.pdf), table Ip/lambda/UA et stratégie de chauffe p. 2, brochage p. 2-3.
- Firmware Speeduino 202402 :
  - [corrections.cpp, ligne 618](https://github.com/speeduino/speeduino/blob/202402/speeduino/corrections.cpp#L618) : fenêtre `ego_min` / `ego_max` ;
  - [sensors.cpp, `readO2`](https://github.com/speeduino/speeduino/blob/202402/speeduino/sensors.cpp#L596) : lecture de l'entrée O2 ;
  - [init.cpp, ligne 1484](https://github.com/speeduino/speeduino/blob/202402/speeduino/init.cpp#L1484) : `pinO2 = A8` sur la v0.4.
- Schéma Speeduino v0.4 : [fiches-techniques/speeduino/](../fiches-techniques/speeduino/), entrée O2.
- Tune actuel : [cartographies/2026-11-02-tune-actuel.msq](../cartographies/2026-11-02-tune-actuel.msq) (`ego_min_afr` 10, `ego_max_afr` 17, `ego_sdelay` 15 s).
