# Fiches techniques — Honda D16Y3 sous Speeduino

Ce dossier rassemble la documentation des pièces du projet. Pour chaque pièce, on note sa source, et si elle est
**officielle** (constructeur, projet Speeduino, EFI Analytics) ou **non officielle** (forum, revendeur).

Ce qui manque est marqué **à ajouter**. Pour déposer un fichier : sur GitHub, **Add file → Upload files**.

## Matériel monté

| Pièce | Référence |
|---|---|
| Moteur | Honda D16Y3, compressions 9,5 bar, uniformes |
| Calculateur | Speeduino, carte v0.4, firmware 202402 |
| Logiciel | TunerStudio MS Lite 3.2.03 |
| Allumage | Kit Honed Developments D & B Series (HDP40), capteurs Hall de came Honda série K |
| Bobines | NGK U5014, marquage `105N R06 02 S 003 00 0025354724`, étincelle unique |
| Injecteurs | Honda d'origine, 240 cc/min |
| Régulateur de pression d'essence | AEM 25-304BK, réglable (pression réglée : à ajouter, mesurée au manomètre) |
| Capteur MAP | Denso 079800-3000, 5 V, marquage PS-30 (celui de la voiture, relié à l'admission) |
| Sonde lambda | Sonde étroite (référence à ajouter) |
| Ralenti | Pas de vanne de ralenti pilotée par la carte |

## Documents

| Pièce | Document | Source | Statut | Ce qu'on y trouve |
|---|---|---|---|---|
| Carte Speeduino v0.4 | [speeduino/speeduino-v0-4-4d-smd-schema.pdf](speeduino/speeduino-v0-4-4d-smd-schema.pdf) | [speeduino/Hardware](https://github.com/speeduino/Hardware/tree/master/v0.4/SMD/Latest) | officiel | Schéma complet, version CMS v0.4.4d (7 pages) |
| Carte Speeduino v0.4 | [speeduino/speeduino-v0-4-3d-tht-schema.pdf](speeduino/speeduino-v0-4-3d-tht-schema.pdf) | [speeduino/Hardware](https://github.com/speeduino/Hardware/tree/master/v0.4/THT/Latest) | officiel | Schéma complet, version traversante v0.4.3d (7 pages) |
| Carte Speeduino v0.4 | [Page wiki de la carte](https://wiki.speeduino.com/en/boards/v04) | wiki Speeduino | officiel | Contenu non relu |
| Kit Honed HDP40 | [Page produit](https://honeddevelopments.com/product/d-b-series-trigger-kit/) | Honed Developments | officiel | Capteurs Hall de came Honda série K, capteurs « Trigger » et « Home » ; un calage au strobo est nécessaire |
| Kit Honed HDP40 | [Manuel PDF](honed/HDP40-01-and-HDP40-06-DB-Series-Trigger-Kit-10APR26.pdf) | [Honed Developments](https://honeddevelopments.com/manuals/) | officiel | Montage, brochage p. 5 et configuration 8+1 sur arbre à cames p. 6 ; fichier fourni, révision interne 03 du 23DEC25 |
| Capteurs Hall du kit Honed | [Manuel Honed, p. 2 et 5](honed/HDP40-01-and-HDP40-06-DB-Series-Trigger-Kit-10APR26.pdf) | Honed Developments | officiel pour le kit | Référence 37510-PNB-003 ; broche 1 : signal, 2 : masse signal, 3 : 12 V. Respecter la vue du connecteur p. 5 ; cette alimentation ne signifie pas que l’entrée ECU accepte 12 V |
| Capteur MAP Denso 079800 | [speeduino.ini, préréglage « Denso 079800 »](https://github.com/speeduino/speeduino/blob/202402/reference/speeduino.ini#L3169) | code Speeduino 202402 | officiel | `mapMin=0, mapMax=173` (kPa) ; vise la famille 079800 en général |
| Capteur MAP Denso 079800 | [Fil de forum cité par speeduino.ini](http://speeduino.com/forum/viewtopic.php?f=18&t=510&p=7023#p7021) | forum Speeduino | non officiel | Parle du 079800-**1771**, pas du -3000 |
| Bobines NGK U5014 | [Catalogue NGK 2021](allumage/ngk-ignition-coils-leads-2021.pdf) | [NGK Part Finder](https://www.ngkpartfinder.co.uk/assets/Uploads/NGK_Ignition_coilsleads_Lowres_2021.pdf) | officiel | U5014 = stock 48041, 4 broches, 1 sortie, connecteur rouge. **À ajouter** : affectation des broches et courbe de dwell constructeur |
| Bobines (voisine U5015) | [HP Academy, Audi R8 ignition coil dwell](https://www.hpacademy.com/forum/general-tuning-discussion/show/audi-r8-ignition-coil-dwell-settings) | forum | non officiel | 1,7 ms pour la U5015, chiffre attribué à NGK ; autre référence que la U5014 |
| Injecteurs 240 cc/min | Pas de fiche trouvée | — | — | **À ajouter** : référence, impédance, temps d'ouverture, pression à laquelle le débit de 240 cc/min est donné |
| Régulateur AEM 25-304BK | [carburant/aem-25-304bk-regulateur-pression-instructions.pdf](carburant/aem-25-304bk-regulateur-pression-instructions.pdf) | [AEM, notice officielle](https://documents.aemelectronics.com/609dbef758b9e71e573f8a5a11771ed139bfd92a.pdf) | officiel | Réglable de 20 psi au maximum de la pompe ; prise de dépression 1:1 ; réglé à 40 psi en usine (p. 4) ; pression mesurée **au ralenti, durite de dépression débranchée et pincée** (p. 4) ; Civic 96-00 EX : 40-47 psi (tableau p. 6, le D16Y3 n'y figure pas) |
| Régulateur AEM 25-304BK | [Page produit](https://www.aemelectronics.com/products/fuel_delivery/adjustable_fuel_pressure_regulators/parts/25-304BK) | AEM | officiel | Donné pour B20B4, D16Y8, F22B1/B2 |
| Sondes d'eau et d'air Honda | Pas de table trouvée | — | — | **À ajouter** : table résistance / température |
| Capteur de papillon Honda | [Manuel Honda Civic 1999, connecteur ECM/PCM C, borne 27](https://content.instructables.com/FYT/UKSB/IKOAR3S2/FYTUKSBIKOAR3S2.pdf) | Manuel constructeur, copie hébergée par un tiers | référence comparative, D16Y7 | Environ 0,5 V fermé et 4,8 V ouvert ; **à confirmer sur le capteur du montage D16Y3**, pas une validation de son brochage |
| Sonde lambda étroite | Pas de fiche | — | — | **À ajouter** : référence |
| Sonde large bande Bosch LSU ADV / ADV pre Turbo | [Fiche PDF fournie](lambda/adv_pre_turbo.pdf) | Bosch Motorsport | document constructeur fourni | Sonde large bande nécessitant un contrôleur compatible LSU ADV ; présence sur la voiture non confirmée, ne remplace pas la référence de la sonde étroite |
| TunerStudio | [Aide en ligne](https://www.tunerstudio.com/index.php/support/manuals) | EFI Analytics | officiel | Pas de manuel utilisateur complet en PDF |

## Wiki Speeduino (officiel)

- [Injector Characteristics](https://wiki.speeduino.com/en/configuration/Injector_Characteristics) — temps d'ouverture typique : 0,9 à 1,5 ms.
- [Spark Settings](https://wiki.speeduino.com/en/configuration/Spark_Settings) — « Going Low » pour la grande majorité des montages ; Fixed Angle pour le calage.
- [Dwell](https://wiki.speeduino.com/en/configuration/Dwell)
- [Dual Wheel](https://wiki.speeduino.com/en/decoders/Dual_Wheel)
- [Trigger Setup](https://wiki.speeduino.com/en/configuration/Trigger_Setup)
- [Sensor Calibration](https://wiki.speeduino.com/en/configuration/Sensor_Calibration)
- [O2](https://wiki.speeduino.com/en/configuration/O2) — une sonde large bande est fortement recommandée pour régler.
- [Tuning](https://wiki.speeduino.com/en/tuning/Tuning)

## Code du firmware 202402 (officiel)

Le dépôt `noisymime/speeduino` redirige maintenant vers `speeduino/speeduino`.

- [init.cpp, lignes 438-441](https://github.com/speeduino/speeduino/blob/202402/speeduino/init.cpp#L438-L441) et [ligne 611](https://github.com/speeduino/speeduino/blob/202402/speeduino/init.cpp#L611) : le Required Fuel est le carburant d'un cycle complet à VE 100 %.
- [decoders.cpp, lignes 817-820](https://github.com/speeduino/speeduino/blob/202402/speeduino/decoders.cpp#L817-L820) : en Dual Wheel avec « Cam Speed », l'angle entre dents = 720° ÷ nombre de dents.

## À ajouter

- [x] Manuel Honed HDP40 (dans `honed/`)
- [x] Catalogue officiel NGK identifiant la U5014 (dans `allumage/`)
- [ ] Brochage et courbe de dwell constructeur NGK U5014
- [ ] Fiche des injecteurs d'origine
- [ ] Pression d'essence réglée sur le régulateur AEM (mesurée selon la notice, p. 4)
- [ ] Tables des sondes d'eau et d'air Honda
- [ ] Plage de tension du capteur de papillon du montage : comparaison Honda trouvée, mesure et référence à confirmer
- [ ] Référence de la sonde lambda étroite
- [ ] Schéma de câblage de la voiture

## Résultat des recherches du 3 novembre 2026

Les documents sont distingués des valeurs réellement relevées sur la voiture. Les cases restent ouvertes lorsqu’une référence voisine ne suffit pas à valider le montage.

| Élément restant | Résultat et prochaine information nécessaire |
|---|---|
| U5014 | Catalogue constructeur ajouté. Aucune courbe de dwell constructeur ni affectation certaine des broches trouvée lors de cette recherche. Ne pas reprendre le dwell de la U5015 comme valeur validée pour la U5014. |
| Injecteurs | Pas de fiche certaine sans la référence gravée sur les injecteurs. Relever les marquages, la résistance de chaque injecteur et la pression différentielle de référence du débit ; le temps mort doit aussi être caractérisé en fonction de la tension. |
| Pression AEM | La notice est déjà présente. La pression effectivement réglée doit être mesurée sur la voiture selon la p. 4 ; la valeur d’usine ne prouve pas le réglage actuel. |
| Sondes eau/air | Aucune table résistance/température vérifiée pour les références effectivement montées. Relever les références des deux sondes ; ne pas confondre sonde ECU, émetteur de jauge et thermocontact. |
| TPS | Repères 0,5 V fermé / 4,8 V ouvert trouvés dans le manuel Honda cité ci-dessus, pour une autre variante. Relever les valeurs du montage et calibrer ses butées dans TunerStudio. |
| Lambda étroite | Référence à lire sur la sonde ou sa facture. Le PDF LSU ADV concerne une autre technologie et ne permet pas d’identifier la sonde étroite. |
| Câblage voiture | Un schéma d’origine ne documente pas les modifications EJ9 / D16Y3 / Speeduino / Honed / bobines. Il faut relever les connecteurs, broches, masses, alimentations et protections du faisceau réel. |

### Précisions sur les deux PDF fournis

- **Honed** : p. 6, 8 dents Trigger + 1 Home par tour d’arbre à cames, capteurs Hall, front descendant et pull-up. Les angles suggérés aux pages Haltech et Link sont propres à ces configurations : ils ne constituent pas un angle Speeduino validé. Calage à contrôler à la lampe stroboscopique. Le nom du fichier indique 10APR26, mais la page de garde porte 23DEC25, révision 03.
- **Bosch LSU ADV** : la fiche fournie indique un fonctionnement avec une unité d’évaluation adaptée (LT4 ADV citée). La sonde seule ne fournit pas directement un signal large bande 0–5 V exploitable par Speeduino. [Fiche actuellement publiée par Bosch](https://www.bosch-motorsport.com/content/downloads/Raceparts/Resources/pdf/Data%20Sheet_69086859_Lambda_Sensor_LSU_ADV/ADV_pre_Turbo.pdf) : lien de provenance complémentaire, sans affirmer que cette édition est identique au fichier fourni.
