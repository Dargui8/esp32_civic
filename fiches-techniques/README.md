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
| Injecteurs | Honda d’origine D16Y3 : ensemble **06164-P06-A02** (identification catalogue). 240 cc/min : donnée du projet, pression de référence non documentée |
| Régulateur de pression d'essence | AEM 25-304BK, réglable. Vis de réglage non touchée depuis la réception (indiqué par le propriétaire le 3 novembre 2026) : réglage d'usine 40 psi (notice p. 4), **pas encore mesuré** sur la voiture |
| Capteur MAP | Denso 079800-3000, 5 V, marquage PS-30 (celui de la voiture, relié à l'admission) |
| Sonde d’eau ECU | **37870-PJ7-003**, TW5 Matsushita — référence du montage d’origine D16Y3 |
| Sonde de température d’air | **37880-P05-A00** — référence du montage d’origine D16Y3 |
| Corps de papillon / TPS | **16400-P1G-E01** (GRS3A), retenu sur correspondance visuelle signalée par le propriétaire le 3 novembre 2026 ; TPS non référencé séparément |
| Sonde lambda | Sonde étroite actuellement indiquée ; remplacement futur traité à part dans la section « Lambda — évolution ultérieure » |
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
| Injecteurs 240 cc/min (donnée du projet) | Voir les références d’origine ci-dessous | Catalogue Honda 17ST301 | constructeur reproduit | 06164-P06-A02 identifié ; **à ajouter** : impédance, temps mort et débit avec pression de référence |
| Régulateur AEM 25-304BK | [carburant/aem-25-304bk-regulateur-pression-instructions.pdf](carburant/aem-25-304bk-regulateur-pression-instructions.pdf) | [AEM, notice officielle](https://documents.aemelectronics.com/609dbef758b9e71e573f8a5a11771ed139bfd92a.pdf) | officiel | Réglable de 20 psi au maximum de la pompe ; prise de dépression 1:1 ; réglé à 40 psi en usine (p. 4) ; pression mesurée **au ralenti, durite de dépression débranchée et pincée** (p. 4) ; Civic 96-00 EX : 40-47 psi (tableau p. 6, le D16Y3 n'y figure pas) |
| Régulateur AEM 25-304BK | [Page produit](https://www.aemelectronics.com/products/fuel_delivery/adjustable_fuel_pressure_regulators/parts/25-304BK) | AEM | officiel | Donné pour B20B4, D16Y8, F22B1/B2 |
| Sondes d’eau et d’air Honda | Voir les références d’origine ci-dessous | Catalogue Honda 17ST301 | constructeur reproduit | Eau 37870-PJ7-003 ; air 37880-P05-A00 ; **à ajouter** : tables résistance/température |
| Capteur de papillon Honda | [Manuel Honda Civic 1999, connecteur ECM/PCM C, borne 27](https://content.instructables.com/FYT/UKSB/IKOAR3S2/FYTUKSBIKOAR3S2.pdf) | Manuel constructeur, copie hébergée par un tiers | référence comparative, D16Y7 | Environ 0,5 V fermé et 4,8 V ouvert ; **à confirmer sur le capteur du montage D16Y3**, pas une validation de son brochage |
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
- [x] Référence catalogue de l’ensemble injecteur d’origine
- [ ] Caractéristiques électriques et hydrauliques des injecteurs d’origine
- [ ] Pression d'essence réglée sur le régulateur AEM (mesurée selon la notice, p. 4)
- [ ] Tables des sondes d'eau et d'air Honda
- [x] Références d’origine des sondes eau/air intégrées au matériel du projet
- [x] Papillon 16400-P1G-E01 retenu sur correspondance visuelle du propriétaire
- [ ] Plage de tension et calibration TPS du papillon retenu : valeurs comparatives disponibles, mesures à confirmer
- [ ] Schéma de câblage de la voiture

## Résultat des recherches du 3 novembre 2026

Les documents sont distingués des valeurs réellement relevées sur la voiture. Les cases restent ouvertes lorsqu’une référence voisine ne suffit pas à valider le montage.

| Élément restant | Résultat et prochaine information nécessaire |
|---|---|
| U5014 | Catalogue constructeur ajouté. Aucune courbe de dwell constructeur ni affectation certaine des broches trouvée lors de cette recherche. Ne pas reprendre le dwell de la U5015 comme valeur validée pour la U5014. |
| Injecteurs | Référence catalogue 06164-P06-A02 identifiée ; voir les sources et limites ci-dessous. Impédance, pression de référence du débit et temps mort restent à documenter. |
| Pression AEM | La notice est déjà présente. La pression effectivement réglée doit être mesurée sur la voiture selon la p. 4 ; la valeur d’usine ne prouve pas le réglage actuel. |
| Sondes eau/air | Références d’origine identifiées : TW5 37870-PJ7-003 et IAT 37880-P05-A00 ; tables résistance/température encore à documenter. |
| TPS | Repères 0,5 V fermé / 4,8 V ouvert trouvés dans le manuel Honda cité ci-dessus, pour une autre variante. Relever les valeurs du montage et calibrer ses butées dans TunerStudio. |
| Câblage voiture | Un schéma d’origine ne documente pas les modifications EJ9 / D16Y3 / Speeduino / Honed / bobines. Il faut relever les connecteurs, broches, masses, alimentations et protections du faisceau réel. |

### Précisions sur le manuel Honed

- **Honed** : p. 6, 8 dents Trigger + 1 Home par tour d’arbre à cames, capteurs Hall, front descendant et pull-up. Tune actuel (`CurrentTune.msq`) : `TrigEdge` et `TrigEdgeSec` = FALLING, conforme à la p. 6. Les angles suggérés aux pages Haltech et Link sont propres à ces configurations : ils ne constituent pas un angle Speeduino validé. Calage à contrôler à la lampe stroboscopique. Le nom du fichier indique 10APR26, mais la page de garde porte 23DEC25, révision 03.

### Mesure de la pression d'essence

- **Outil** : un manomètre d'essence gradué jusqu'à environ 100 psi (7 bar), avec un raccord Honda. Exemple de kit vendu pour les Civic 1988-2000 : [B&M 46054](https://888murrays.com/b-m-46054-fuel-pressure-gauge) (source revendeur, non officielle).
- **Où le brancher** : à la place de la vis de service de 6 mm, sur le banjo du filtre à essence ([manuel d'atelier Civic 1995, 1.5 L](https://workshop-manuals.com/honda/civic_sedan/l4-1.5l_sohc_(16_valve)/powertrain_management/fuel_delivery_and_air_induction/fuel_pressure_release/system_information/service_and_repair/), autre moteur que le D16Y3). Sans vis de 6 mm : raccord à la place du banjo de 12 mm, M12 x 1,25 vers 1/8 NPT ([exemple d'adaptateur](https://www.vmsracing.com/products/honda-acura-fuel-pressure-gauge-adapter-banjo-bolt-fitting-12mm-bolt-kit-1-8-npt-filter), revendeur). **À vérifier sur la voiture** avant d'acheter.
- **Conditions** (notice AEM) : moteur au ralenti, durite de dépression du régulateur débranchée et pincée (p. 4) ; moteur refroidi s'il a tourné dans les deux heures, rondelle de la vis de service remplacée à chaque desserrage (p. 3).
- **Cible** : 40-47 psi pour toutes les Civic 96-00 du tableau AEM (p. 6) ; le D16Y3 n'y figure pas.
- **Plus tard, en option** : le firmware a une entrée pour un capteur de pression d'essence (`fuelPressureEnable`, désactivée dans le tune actuel, broche par défaut A14). Broche libre sur la carte v0.4 : non vérifiée.

## Références d’origine D16Y3 et variantes de catalogue

Recherche approfondie du 3 novembre 2026, sur la base du matériel d’origine D16Y3 indiquée par le propriétaire, sans exiger de marquage lisible. Comparaison des fiches Honda Civic 1.6 iLS **5 portes**, catalogue 17ST301 : fiche 18196 (1995, boîte automatique) et fiche 18290 (1996, boîte manuelle). Cette comparaison documente des montages précis, pas toutes les destinations commerciales ; elle ne permet pas d’attribuer chaque différence uniquement à l’année ou uniquement à la boîte.

| Pièce | Référence relevée | Variantes / portée |
|---|---|---|
| Sonde d’eau ECU, TW5 Matsushita | **37870-PJ7-003** | Identique dans les deux fiches, repère 13 du groupe pompe à eau. |
| Sonde de température d’air | **37880-P05-A00** | Identique dans les deux fiches, repère 11 du collecteur d’admission. |
| Ensemble injecteur, repère 1 | **06164-P06-A02** | Identique dans les deux fiches. Le catalogue européen le nomme « PACKING SET, INJECTOR » ; le catalogue Honda US le nomme « Fuel Injector Set ». Ne pas le confondre avec les joints séparés 16472-PD6-000 et 16473-PD6-000. |
| Corps de papillon complet | **16400-P1G-E01** (GRS3A), retenu pour le projet | Correspondance visuelle signalée par le propriétaire le 3 novembre 2026. Variante de catalogue : 16400-P1K-E51 (GR52A). Le TPS n’est pas détaillé séparément ; la ressemblance ne valide pas les caractéristiques électriques. |
| MAP d’origine | **37830-P0G-S00**, remplacement affiché **37830-PAA-S00** | Identique dans les deux listes de papillon ; cela ne démontre pas une équivalence avec le Denso -3000 conservé sur le projet. |

Les références **37750-PH2-014** (« Thermo unit », jauge) et **37760-P00-003** (« Thermo switch A-93 », thermocontact) figurent à côté de la TW5 : ce sont des composants distincts, pas deux autres calibrations de sonde ECU.

### Sources Honda des références

Catalogues constructeur reproduits par le réseau de vente de pièces Honda :

- [Admission 1995 automatique](https://www.parts-honda.uk/honda-cars/CIVIC/1995/16ILS/ENGINE/INTAKE-MANIFOLD-SOHC-16L-VTEC-95-96-/17ST301/E__0301/1/18196) ; [admission 1996 manuelle](https://www.honda-car-parts.com/honda-cars/CIVIC/1996/16ILS/ENGINE/INTAKE-MANIFOLD-SOHC-16L-VTEC-95-96-/17ST301/E__0301/1/18290).
- [Pompe à eau et sondes 1995 automatique](https://www.parts-honda.uk/honda-cars/CIVIC/1995/16ILS/ENGINE/WATER-PUMP-SOHC-/17ST301/E__1500/1/18196) ; [1996 manuelle](https://www.honda-car-parts.com/honda-cars/CIVIC/1996/16ILS/ENGINE/WATER-PUMP-SOHC-/17ST301/E__1500/1/18290).
- [Papillon 1995 automatique](https://www.parts-honda.uk/honda-cars/CIVIC/1995/16ILS/ENGINE/THROTTLE-BODY-SOHC-16L-VTEC-95-96-/17ST301/E__0101/1/18196) ; [papillon 1996 manuelle](https://www.honda-car-parts.com/honda-cars/CIVIC/1996/16ILS/ENGINE/THROTTLE-BODY-SOHC-16L-VTEC-95-96-/17ST301/E__0101/1/18290).
- [Dénomination Honda US de 06164-P06-A02](https://www.hondapartsnow.com/genuine/honda~injector~set~fuel~06164-p06-a02.html).

### Ce que l’identification résout et ce qui reste à documenter

- Les références des sondes eau/air et de l’ensemble injecteur sont désormais identifiées par catalogue ; leur recherche ne dépend plus de marquages sur les pièces.
- Les courbes résistance/température des sondes, l’impédance documentée et le temps mort des injecteurs restent à trouver dans une source technique applicable. La référence commerciale seule ne fournit pas ces paramètres.
- Le débit **240 cc/min** reste une donnée du projet, non confirmé par les catalogues consultés avec sa pression différentielle de référence.
- Le papillon 16400-P1G-E01 est retenu sur correspondance visuelle du propriétaire. La recherche d’identification de l’ancienne lambda est mise de côté, son remplacement étant prévu.


## Lambda — évolution ultérieure

Le propriétaire prévoit d’installer plus tard une sonde plus récente. La lambda est traitée séparément du matériel d’origine retenu pour le projet ; aucune référence de remplacement n’est choisie à ce stade. Les recherches sur l’ancienne sonde sont conservées ci-dessous à titre documentaire, sans tâche active pour identifier sa variante.

- [ ] Choisir ultérieurement la sonde et, si nécessaire, son contrôleur compatible.
- [ ] Documenter alors le câblage et la calibration correspondants dans Speeduino.

### Documentation disponible

| Pièce | Document | Source | Statut | Ce qu’on y trouve |
|---|---|---|---|---|
| Sonde large bande Bosch LSU ADV / ADV pre Turbo | [Fiche PDF fournie](lambda/adv_pre_turbo.pdf) | Bosch Motorsport | document constructeur fourni | Sonde large bande nécessitant un contrôleur compatible LSU ADV ; document conservé pour étude, ne constitue pas le choix de la future sonde |

- **Bosch LSU ADV** : la fiche fournie indique un fonctionnement avec une unité d’évaluation adaptée (LT4 ADV citée). La sonde seule ne fournit pas directement un signal large bande 0–5 V exploitable par Speeduino. [Fiche actuellement publiée par Bosch](https://www.bosch-motorsport.com/content/downloads/Raceparts/Resources/pdf/Data%20Sheet_69086859_Lambda_Sensor_LSU_ADV/ADV_pre_Turbo.pdf) : lien de provenance complémentaire, sans affirmer que cette édition est identique au fichier fourni.

### Anciennes références d’origine — pour mémoire

| Pièce | Référence relevée | Portée |
|---|---|---|
| Lambda | **36531-P1K-E01** | Fiche 1995 automatique consultée. |
| Lambda avant | **36531-P1K-E11**, remplacement affiché **36531-P2R-A01** | Fiche 1996 manuelle consultée. Il existe donc plusieurs références de catalogue ; ne pas présenter une seule comme universelle au D16Y3. |

- [Lambda 1995 automatique](https://www.parts-honda.uk/honda-cars/CIVIC/1995/16ILS/ENGINE/EXHAUST-MANIFOLD-SOHC-/17ST301/E__0400/1/18196) ; [lambda 1996 manuelle](https://www.honda-car-parts.com/honda-cars/CIVIC/1996/16ILS/ENGINE/EXHAUST-MANIFOLD-SOHC-/17ST301/E__0400/1/18290).

### Variantes lambda explicitement listées par Denso

Le [catalogue officiel Denso 2024, page imprimée 66](https://assets.denso-am.eu/production/attachments/DENSO-Lambda-Sensors-Product-Catalogue-2024.pdf) comporte les lignes suivantes pour le **D16Y3**. Ce sont des références de remplacement Denso, pas des numéros de pièces Honda d’origine.

| Période / condition imprimée | Montage direct avant catalyseur | Universelle avant catalyseur |
|---|---|---|
| 01/1995–06/1996, sans condition de boîte sur cette ligne | — | DOX-0125 |
| 07/1996–01/1997, ATM (automatique) | — | DOX-0109 |
| 07/1995–06/1996, M (manuelle) | DOX-1409 | DOX-0109 |

Les périodes se chevauchent : conserver les conditions de boîte et vérifier le connecteur/nombre de fils pour choisir la variante. Ce tableau prouve plusieurs possibilités et ne constitue pas une correspondance univoque de chaque numéro Honda ci-dessus.
