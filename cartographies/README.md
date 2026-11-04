# Cartographies Speeduino du D16Y3

| Fichier | Rôle |
|---|---|
| `2026-11-02-tune-actuel.msq` | Le tune actuel (`CurrentTune.msq` du 2 novembre 2026), copié tel quel. **Retour arrière** : recharger ce fichier. |
| `2026-11-03-test-demarrage.msq` | Carto de test du 3 novembre 2026 : démarrage, ralenti et calage, **au garage**. Ne pas rouler avec. |
| `2026-11-04-test-2-moins-riche.msq` | Carto de test 2 du 4 novembre 2026 : la même, **VE × 0,85 sauf la colonne 500 tr/min** (le lancement ne change pas). Motif : essence dans l'échappement après l'essai de 16 h 02, sans lambda pour mesurer. Au ralenti après l'enrichissement de démarrage : 8,5 ms au lieu de 9,7. Ne pas rouler avec. |
| `2026-11-04-test-3-avance-table.msq` | Carto de test 3 du 4 novembre 2026 : la carto 2 avec **Fixed Angle sur Off** (avance de la table : environ 17° au ralenti, au lieu de 10° fixes) et **seuil de lancement (`crankRPM`) à 300 tr/min** au lieu de 400. Motif : journal de 19 h 29. Ne pas rouler avec. |

## Ce que change la carto de test

| Réglage | Avant | Test | Pourquoi | Source |
|---|---|---|---|---|
| Injector Staging (`alternate`) | Simultaneous | **Alternating** | En séquentiel : 2 squirts + Alternating | Wiki *Engine Constants* ; aide du réglage dans `speeduino.ini` 202402 (« it will still affect the req_fuel value ») |
| Required Fuel (`reqFuel`) | 5,6 ms | **11,1 ms** | En séquentiel, le firmware injecte reqFuel × VE une fois par cycle ; 5,6 ms ne fait que la moitié du besoin | `init.cpp` 202402, l. 438-441 et 611 ; calcul ci-dessous |
| Injector Open Time (`injOpen`) | 5,0 ms | **1,0 ms** | 5,0 ms est hors de la plage habituelle (0,9-1,5 ms) ; valeur Honda non trouvée | Wiki *Injector Characteristics* |
| Table VE | 34-91 | **53-82** | Recalculée pour injecter **la même chose qu'avant une fois chaud** : VE' = (5,6 × VE + 400) ÷ 11,1 | calcul ci-dessous |
| Fixed Angle | Off | **On, 10°** | Calage au stroboscope ; au lancement, c'est toujours Cranking Advance (5°) | Wiki *Spark Settings* |
| Lambda (`egoType`) | Narrow Band | **Disabled** | La lambda reste figée à 25,5 dans le journal : pas de correction sur une fausse valeur | journal du 3/11, 15 h 28 |
| Limiteurs | 6 000 / 7 000 tr/min | **3 500 / 4 000** | Sécurité pendant les essais (choix, pas une règle) | — |

Rien d'autre ne change (vérifié par comparaison des deux fichiers).

### Calcul du Required Fuel

- Hypothèses : 1 590 cm³ (75 × 90 mm, à confirmer dans le manuel Honda), 4 cylindres, 240 cc/min (donnée du projet), essence 14,7:1, air à 100 kPa et 21 °C.
- Air par cylindre : 397,5 cm³ × 1,184 g/L = 0,471 g ; essence : 0,471 ÷ 14,7 = 0,032 g.
- Injecteur : 240 cc/min = 22,9 lb/h = 2,88 g/s ; temps : 0,032 ÷ 2,88 = **11,1 ms**.
- Le calculateur de TunerStudio, réglé sur Alternating, doit donner à peu près la même valeur : **la sienne prime**.

### Effet sur l'injection (formule du firmware, contrôlée sur le journal du 3/11)

Formule : PW = reqFuel × VE × corrections + temps d'ouverture × correction batterie. Elle redonne les 8,59 ms et 10,88 ms du journal.

| Moment | Avant | Test |
|---|---|---|
| Moteur chaud, 14 V | 7,2 ms | 7,2 ms (écart maximal 0,05 ms sur toute la table) |
| Mise en température à 13 °C (corrections 161 %) | 8,6 ms | 11,0 ms |
| Lancement à 10,6 V (corrections plafonnées à 255 %) | 10,9 ms | 16,9 ms |

Une fois chaud, rien ne change. Au lancement et pendant la mise en température, le moteur reçoit plus d'essence : c'est le test.

## Protocole d'essai

**Avant** : batterie chargée (12,3 V moteur arrêté, 7,9-10,6 V au démarreur dans le journal).

1. **Charger** `2026-11-03-test-demarrage.msq` dans TunerStudio et l'envoyer au calculateur (Burn).
2. **Couper complètement le calculateur**, câble USB débranché, 10 s, puis le rallumer. Le firmware ne lit reqFuel qu'au démarrage du calculateur ([init.cpp l. 375](https://github.com/speeduino/speeduino/blob/202402/speeduino/init.cpp#L375)), alors que le temps d'ouverture s'applique tout de suite ([corrections.cpp l. 102](https://github.com/speeduino/speeduino/blob/202402/speeduino/corrections.cpp#L102)). **Contrôle** : au lancement, PW ≈ 17 ms. PW ≈ 9 ms = le calculateur n'a pas redémarré (cas du journal du 4/11, 13 h 31). Le rallumage déclenche aussi l'amorçage automatique (`primePulse`, environ 23 ms par injecteur à 12 °C) : il n'a lieu qu'à la mise sous tension du calculateur.
   - **Le Speeduino reste alimenté par l'USB du PC contact coupé** (indiqué par le propriétaire le 4/11). Tant que l'USB est branché, remettre le contact ne redémarre pas le calculateur : ni nouveau reqFuel, ni amorçage.
   - **Pompe** : pendant les essais, elle est alimentée à la main, en direct (indiqué par le propriétaire le 4/11). Elle doit tourner **avant** le contact : l'amorçage part 0,5 s après la mise sous tension (`primingDelay`), il lui faut la pression.
   - **Ordre à suivre à chaque essai** : USB débranché, contact coupé, 10 s → **pompe alimentée** → **contact mis** (le calculateur démarre sur le 12 V et amorce avec la pompe alimentée) → **USB rebranché**, TunerStudio connecté. Rebrancher l'USB avant le contact ferait l'amorçage sans 12 V, donc probablement sans essence (si la pompe et les injecteurs sont alimentés par le contact : montage habituel, à vérifier).
3. **Lancer l'enregistrement du journal** dès que TunerStudio est connecté.
4. **Démarrer sans toucher la pédale.** Pédale à fond = aucune injection (`tpsflood = 100 %`).
5. **Au ralenti, régler le calage au stroboscope** sur le cylindre 1 : le calculateur impose 10°. Noter l'écart lu. Moins de 20° : on corrige Trigger Angle ; plus de 20° : autre problème (wiki *Honda D17*, même méthode).
6. **Remettre Fixed Angle sur Off** (Spark Settings), Burn, puis 2 minutes de ralenti et quelques montées douces en régime.
7. **Envoyer le journal** (`.mlg`).

| Ce qui se passe | Ce que ça veut dire |
|---|---|
| Démarre et tient le ralenti | Le manque d'essence au lancement était la cause |
| Démarre, puis cale | Noter la seconde ; on regarde le journal ensemble |
| Ne démarre pas, odeur d'essence, bougies mouillées | Trop riche : on redescend la VE |
| Ne démarre pas, bougies sèches | Manque d'essence ou d'étincelle : à chercher ailleurs |

## Résultats des essais

| Journal | Résultat |
|---|---|
| 4/11, 13 h 31 | Pas de démarrage. Calculateur non redémarré : 9,2 ms au lancement au lieu de 17 (ancien reqFuel). Essai non valable. |
| 4/11, 13 h 40 | Pas de démarrage. Commande correcte (16,9 ms × 4, étincelle 5°, synchro complète). **Réservoir vide** (indiqué par le propriétaire). Bougies mouillées et noires. |
| 4/11, 16 h 02 | **Démarre seul, sans mode test** (indiqué par le propriétaire), après environ 6 s de lancement. **Ralenti 82 s** à 700-870 tr/min, calage fixe 10°, eau de 13 à 25 °C, injection de 11,0 à 9,2 ms à mesure que l'enrichissement de mise en température baisse. Montées jusqu'à environ 3 000 tr/min. 3 pertes de synchro au moment où il prend (7,7 à 8,8 s), aucune ensuite. Contact coupé à 91 s (tension batterie à 0), pas un calage. |
| 4/11, 19 h 17 | **Carto 2** (VE 48 au ralenti dans le journal, contre 56-57 avec la carto du 3/11). Démarre après environ 2 s de lancement. **Ralenti libre stable pendant 44 s** (9,7 à 53,5 s) à 690-800 tr/min, injection de 9,5 à 8,2 ms à mesure que l'enrichissement baisse : 15 % de moins ne le déstabilise pas. Puis **3 calages** (59, 95 et 111 s) en essayant une marche arrière (indiqué par le propriétaire) : à chaque fois pédale relâchée, régime qui descend sous 500 tr/min en quelques secondes, allumage et injection normaux jusqu'au bout, aucune perte de synchro avant. Redémarre en 0,5 à 1 s. Attendu avec ce tune : pas de pilotage du ralenti (`iacAlgorithm = None`) et dosage au papillon seul (Alpha-N, MAP figé), donc rien ne compense la charge à pied levé. |
| 4/11, 19 h 29 | Carto 2. Démarre en 1,3 s. **Ralenti qui baisse tout seul**, pédale relâchée et sans charge annoncée : 650 tr/min à 10 s, 600 à 50 s, 500 à 85-113 s, pendant que l'eau passe de 21 à 31 °C. **Sous 400 tr/min, le calculateur se croit en lancement** (`crankRPM = 400`) : enrichissement de lancement (injection 14,5 ms au lieu de 8,6, soit +70 %), avance de lancement 5°, et nouvel enrichissement après démarrage à chaque remontée. Cela dure de 114 à 152 s, puis calage à 155 s en remettant du régime. Odeur forte et légère fumée blanche (indiqué par le propriétaire). Sans lambda, impossible de dire si le ralenti baisse par excès ou par manque d'essence. |

## Ce qui restera à changer (relevé dans le tune et les journaux)

- **Capteur MAP** : figé à 25 kPa. Inutilisé par le tune (Alpha-N), à regarder ensuite.
- **Lambda** : figée à 25,5. Sans elle, impossible de régler la VE.
- **Axe des tables** : VE et avance s'arrêtent à 25 % de papillon. Au-delà, c'est la dernière ligne qui sert.
- **Vanne de ralenti** : `iacAlgorithm = None`, le Speeduino ne la pilote pas.
  - Sur le papillon : une **vanne chauffée par l'eau du moteur**, qui fonctionne seule, et une **électrovanne de ralenti non raccordée** (indiqué par le propriétaire le 4/11).
  - Une vanne de ce type, à cire ou à bilame, donne de l'air moteur froid et se ferme en chauffant ; Speeduino fonctionne avec ([wiki *Idle*](https://wiki.speeduino.com/en/configuration/Idle), § *Stand-Alone (Non-Electronic)*).
  - Hypothèse : c'est elle qui fait baisser le ralenti à 19 h 29 (650 → 500 tr/min pendant que l'eau passe de 21 à 31 °C). Ni l'essence ni l'avance ne remplacent l'air qui manque.
  - L'électrovanne s'ouvre selon le pourcentage de temps où elle est alimentée (PWM). Réglage : *Idle control* en PWM, puis la courbe « Idle - PWM Duty Cycle » (pourcentage selon la température d'eau) et « PWM Cranking Duty Cycle » (wiki *Idle*, § *PWM Settings*).
  - Une vanne à 2 fils utilise une sortie, une vanne à 3 fils en utilise deux (wiki *Idle*, § *2 wire vs 3 wire valves*). Sur la carte v0.4 : Idle 1 = IDC 37 (broche Arduino 5), Idle 2 = IDC 36 (broche 6) ([wiki v0.4](https://wiki.speeduino.com/en/boards/v0.4) ; `init.cpp` 202402, `pinIdle1` et `pinIdle2`).
  - Restent à trouver : le nombre de fils de la vanne, le fil qui va au 12 V et la fréquence qui convient à la vanne Honda.
  - Certaines vannes restent entrouvertes débranchées ou à 0 % : à tester (wiki *Idle*, § *PWM*).
- **Temps d'ouverture réel** des injecteurs Honda : à trouver (voir `fiches-techniques/README.md`).
- **Une étincelle à la mise sous tension** (impression du propriétaire, 4/11). Le firmware coupe toutes les bobines dès qu'il démarre, pour éviter justement ces étincelles parasites ([init.cpp l. 316](https://github.com/speeduino/speeduino/blob/202402/speeduino/init.cpp#L316)). Avant cela, au reset, les broches de l'ATmega2560 sont en haute impédance (fiche technique, *I/O-Ports*). Hypothèse : la bobine voit cette entrée flottante pendant l'amorçage de la carte. Essai pour trancher : USB branché d'abord (calculateur déjà démarré), puis contact. Étincelle quand même = côté bobines ; plus d'étincelle = démarrage du calculateur.

## Sources

- Wiki Speeduino : [Engine Constants](https://wiki.speeduino.com/en/configuration/Engine_Constants), [Injector Characteristics](https://wiki.speeduino.com/en/configuration/Injector_Characteristics), [Spark Settings](https://wiki.speeduino.com/en/configuration/Spark_Settings), [Honda D17](https://wiki.speeduino.com/en/decoders/Honda_D17)
- Firmware 202402 : [init.cpp](https://github.com/speeduino/speeduino/blob/202402/speeduino/init.cpp), [speeduino.ini](https://github.com/speeduino/speeduino/blob/202402/reference/speeduino.ini)
- Journal du 3 novembre 2026, 15 h 28 (`2026-11-03_15.27.59.mlg`)
