# Cartographies Speeduino du D16Y3

| Fichier | Rôle |
|---|---|
| `2026-11-02-tune-actuel.msq` | Le tune actuel (`CurrentTune.msq` du 2 novembre 2026), copié tel quel. **Retour arrière** : recharger ce fichier. |
| `2026-11-03-test-demarrage.msq` | Carto de test du 3 novembre 2026 : démarrage, ralenti et calage, **au garage**. Ne pas rouler avec. |

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
3. **Lancer l'enregistrement du journal** avant de mettre le contact.
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

## Ce qui restera à changer (relevé dans le tune et les journaux)

- **Capteur MAP** : figé à 25 kPa. Inutilisé par le tune (Alpha-N), à regarder ensuite.
- **Lambda** : figée à 25,5. Sans elle, impossible de régler la VE.
- **Axe des tables** : VE et avance s'arrêtent à 25 % de papillon. Au-delà, c'est la dernière ligne qui sert.
- **Vanne de ralenti** : `iacAlgorithm = None`, le Speeduino ne la pilote pas.
- **Temps d'ouverture réel** des injecteurs Honda : à trouver (voir `fiches-techniques/README.md`).

## Sources

- Wiki Speeduino : [Engine Constants](https://wiki.speeduino.com/en/configuration/Engine_Constants), [Injector Characteristics](https://wiki.speeduino.com/en/configuration/Injector_Characteristics), [Spark Settings](https://wiki.speeduino.com/en/configuration/Spark_Settings), [Honda D17](https://wiki.speeduino.com/en/decoders/Honda_D17)
- Firmware 202402 : [init.cpp](https://github.com/speeduino/speeduino/blob/202402/speeduino/init.cpp), [speeduino.ini](https://github.com/speeduino/speeduino/blob/202402/reference/speeduino.ini)
- Journal du 3 novembre 2026, 15 h 28 (`2026-11-03_15.27.59.mlg`)
