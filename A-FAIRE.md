# À faire — D16Y3 / Speeduino

Dernière mise à jour : 15 novembre 2026 (essai de la carto 4). Le détail et les sources sont dans `cartographies/README.md` et `fiches-techniques/README.md` (§ « Recherche du 5 novembre 2026 »).

## 0. Mesures à prendre au prochain passage, avant la carto « tout en un »

Ce qu'aucune source publique n'a donné ; une mesure au multimètre le remplace.

- [x] **Injecteurs** : 12 Ω (propriétaire, 15/11) = haute impédance ; pas de référence gravée, seulement « Keihin, Japon ».
- [x] **Électrovanne de ralenti** : 11,3 Ω entre ses 2 fils (propriétaire, 15/11), soit ≈1,2 A à 14 V. Reste : marquage (référence attendue 36450-P2J-J01) et câblage, voir § 1.
- [ ] **Sonde d'eau et sonde d'air** : résistance à 3 températures (eau glacée 0 °C, ambiante avec thermomètre, eau chaude 80-90 °C avec thermomètre). Speeduino n'a pas de préréglage Honda : ces 3 points servent à le calibrer.
- [ ] **MAP** : tension sur la broche A3 de l'Arduino, moteur arrêté puis au ralenti (§ 2).
- [ ] **Lambda** : les 4 mesures (§ 2).
- [ ] **Batterie** : chargée, puis tension à l'arrêt, au démarreur, moteur à ~2 000 tr/min.
- [ ] **Compressions** : confirmer l'unité de la mesure (bar ?) et les conditions (moteur chaud ? papillon ouvert ? batterie chargée ?). Valeur nominale D16Y3 non trouvée ; pour un autre D16 (VTEC), 1 700 kPa nominal et 950 kPa minimum.

## 1. Électrovanne de ralenti (en premier)

Pourquoi : pédale relâchée, le moteur cale dès 37 °C d'eau, faute d'air (journal du 4/11, 19 h 54). La vanne à eau se ferme en chauffant ; l'électrovanne n'est pas pilotée (`iacAlgorithm = None`).

- [x] Vanne testée en 12 V direct : elle clique, elle n'est pas grippée (propriétaire, 15/11).
- [x] Un fil raccordé à la broche **IDC 37** (Idle 1) (propriétaire, 15/11).
- [ ] Confirmer que l'**autre fil** reçoit le 12 V **après contact** (pas un 12 V permanent).
- [x] Carto 4 essayée le 15/11 (journal 18 h 11) : **le ralenti tient** jusqu'à 39 °C, sens Normal correct.
- [x] Carto 5 essayée le 15/11 (19 h 24) : bien jusqu'à 45 °C, trop bas une fois chaud (≈650 tr/min, 2 calages).
- [ ] **Essayer la carto 6** (`cartographies/2026-11-15-test-6-ralenti-chaud-plus.msq`, +3 points à partir de 53 °C), jusqu'au ventilateur.
- [ ] **Sauts de la température d'eau avec la tension** (journal du 15/11, 19 h 24) : contrôler les masses (moteur-caisse, batterie, Speeduino) et la masse capteurs de la sonde d'eau ; mesurer au multimètre la tension du signal de la sonde moteur chaud au ralenti, et voir si elle bouge quand la tension batterie bouge. À régler avant d'affiner le ralenti : la courbe de l'électrovanne suit la température lue.
- [ ] Trouver la fréquence qui convient à la vanne Honda (160 Hz dans le tune actuel).

## 2. Ensuite

- [ ] **Étincelle à la mise sous tension** : essai USB branché d'abord, puis contact (vient-elle des bobines ou du démarrage du calculateur ?).
- [ ] **Lambda** : la sonde ne donne rien (figée à 25,5, valeur maximale affichable = pas de signal exploitable, hypothèse). Sans elle, le mélange reste inconnu (odeur d'essence et légère fumée blanche le 4/11). Sonde étroite neuve montée, 4 broches : la garder, chercher d'abord la panne dans le câblage ou le réglage.
  - [ ] **Repérer les fils**, prise débranchée, ohmmètre côté sonde, fil par fil : la paire de faible résistance (quelques ohms, à noter) est le chauffage ; les deux autres sont le signal et sa masse.
  - [ ] **Chauffage** : 12 V sur un fil du chauffage, contact mis ? L'autre fil va-t-il bien à la masse ?
  - [ ] **Signal côté sonde**, moteur chaud, prise rebranchée : tension entre le fil signal et sa masse. Attendu : elle bouge entre ~0,1 et ~0,9 V ; si le mélange est riche, elle reste haute (~0,8–0,9 V), ce qui est déjà une information.
  - [ ] **Signal sur la carte** : même mesure sur la broche O2 de la Speeduino. Bon côté sonde et mauvais sur la carte = câblage. Bon aux deux = réglage TunerStudio (`egoType`, aujourd'hui Disabled, voir `cartographies/README.md`).
  - [ ] Ensuite seulement : choisir entre sonde étroite (réglage limité : riche ou pauvre) et large bande avec boîtier (Bosch LSU 4.9 en premier ; [wiki Speeduino O2](https://wiki.speeduino.com/en/configuration/O2) : « fortement recommandée » pour régler).
  - Pièce de secours : ancienne sonde Denso **065500-2730** (marquage « NS6 … 01E28 »), 4 broches, d'aspect proche de la Denso DOX-1459 (selon le propriétaire). DOX-1459 d'après les revendeurs : sonde étroite chauffée « finger-type », M18×1,5, Honda 1990–2005 (OE cités : 36531-P1H-E01, 36531-P28-A01…) ; absente de la liste Denso pour le D16Y3 (DOX-1409 / 0109 / 0125). Même technologie que la sonde neuve : n'apporte rien de plus. État inconnu, à ne tester que si la sonde neuve est en cause.
- [ ] **Calage réel au stroboscope** : la pince ne marche pas sur les bobines crayon. Méthode la plus citée : une **rallonge haute tension** entre la bobine du cylindre 1 et sa bougie (bobine posée à côté, sortie du puits de bougie), la pince sur la rallonge. Rallonge « COP extension lead » du commerce ou faite avec un fil de bougie. Sources : forums [Link ECU](https://forums.linkecu.com/topic/24252-cop-timing-light), [HP Academy](https://www.hpacademy.com/forum/general-tuning-discussion/show/timing-light-on-cop-effected-by-dwell). Éviter de pincer les fils basse tension de la bobine (lecture peu fiable selon ces fils).
- [ ] **Capteur MAP** figé à 25-26 kPa, alors que les tensions au capteur ont été vérifiées (propriétaire, 5/11). Avec la calibration du tune (0-174 kPa), 26 kPa = 0,75 V vus par le calculateur. Mesurer moteur arrêté, contact mis, **directement sur la broche A3 de l'Arduino Mega** (entrée MAP de la carte v0.4, `init.cpp` 202402) : ~2,9 V = problème de lecture ou de réglage ; ~0,75 V = le signal se perd entre le connecteur et l'Arduino.
- [ ] **Température d'air** : jamais vérifiée. Résistance de la sonde à température connue, et comparer avec la valeur lue dans TunerStudio.
- [ ] **Batterie** : la charger, puis noter la tension moteur arrêté, au démarreur et moteur tournant à ~2 000 tr/min (charge de l'alternateur).
- [x] Papillon calibré (propriétaire, 5/11).
- [x] Compressions : 9 à 9,5 sur les 4 cylindres, uniformes (propriétaire, 5/11 ; unité à confirmer, probablement bar). À comparer à la valeur du manuel Honda.
- [ ] **Tables VE et avance** : l'axe s'arrête à 25 % de papillon.
- [ ] **Pression d'essence** : jamais mesurée (régulateur AEM, voir `fiches-techniques/README.md`).
- [ ] **Temps d'ouverture réel** des injecteurs Honda.
