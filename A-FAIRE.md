# À faire — D16Y3 / Speeduino

Dernière mise à jour : 5 novembre 2026. Le détail et les sources sont dans `cartographies/README.md`.

## 1. Électrovanne de ralenti (en premier)

Pourquoi : pédale relâchée, le moteur cale dès 37 °C d'eau, faute d'air (journal du 4/11, 19 h 54). La vanne à eau se ferme en chauffant ; l'électrovanne n'est pas pilotée (`iacAlgorithm = None`).

- [ ] **Multimètre** sur les 2 fils de l'électrovanne, prise débranchée : continuité vers la broche 37 du connecteur IDC (Idle 1) ? Tension 12 V sur l'autre fil, contact mis ?
- [ ] Si le fil n'arrive pas sur IDC 37 : le raccorder (montage à confirmer sur le schéma de la carte v0.4).
- [ ] Carto avec le ralenti en **PWM Open loop** (courbes déjà remplies dans le tune) : au ralenti, le régime doit monter. S'il baisse, mettre le sens sur Reverse.
- [ ] Trouver la fréquence qui convient à la vanne Honda (160 Hz dans le tune actuel).

## 2. Ensuite

- [ ] **Étincelle à la mise sous tension** : essai USB branché d'abord, puis contact (vient-elle des bobines ou du démarrage du calculateur ?).
- [ ] **Lambda** : la sonde ne donne rien. Choisir : sonde d'origine ou large bande avec boîtier. Sans elle, le mélange reste inconnu (odeur d'essence et légère fumée blanche le 4/11).
- [ ] **Calage réel au stroboscope** : la pince ne marche pas sur les bobines crayon. Trouver une autre prise de signal.
- [ ] **Capteur MAP** figé à 25 kPa.
- [ ] **Tables VE et avance** : l'axe s'arrête à 25 % de papillon.
- [ ] **Pression d'essence** : jamais mesurée (régulateur AEM, voir `fiches-techniques/README.md`).
- [ ] **Temps d'ouverture réel** des injecteurs Honda.
