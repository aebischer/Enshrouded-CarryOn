# Transport entre claims — 0.1.46-dev

Objectif : trouver un décor, poser un autel sur place, saisir avec la pince,
se téléporter vers un autre claim puis valider la pose.

## Invariants

L’original reste intact à son emplacement jusqu’à la pose validée. Une ouverture
de carte suspend les commandes et l’aperçu, sans perdre la sélection de transport.
Fermer la partie ou annuler laisse l’original à sa place. Aucun objet d’inventaire
intermédiaire et aucune reconstruction générique de composants ne sont prévus.

La destination doit être validée dans son propre claim côté serveur, avec les
limites complètes de l’objet et les permissions/collisions natives. L’identité
de ce claim doit accompagner la demande : un autre autel qui recouvre la même
position ne doit pas remplacer silencieusement celui qui a été validé.

Le claim d’origine et l’identité de l’objet restent contrôlés pendant le trajet.
Les pointeurs de composant ne survivent pas à un déchargement. Le relevé ci-dessous
a confirmé que l’entité serveur reste chargée alors que sa copie client disparaît.

## Contrat natif et état actuel

- HudScreenType : InGame=1, Map=5, NoUi=51 (table native 0x1bac180, 54 valeurs).
- Map suspend une saisie ; NoUi prolonge une suspension déjà engagée.
- La requête et le commit vérifient le claim de destination, distinct du claim source.
- TeleportCount contient un compteur à +0 et lastTeleportTime à +8.
- Le changement de monde/serveur doit continuer à invalider toute transaction.

## Relevé et implémentation

Relevé du 17 novembre : table de marché Prop_Deco_Villageprops_Markettable_03,
entité 1897, template 04178d57450c134d9b6dc5c97c4777e9. Après téléportation
sur environ 1,27 km, elle quitte le monde client mais reste dans le même monde
serveur, même identité, transformation et handle de collision (486).
Témoins : work/teleport-source-*-before.json, teleport-source-server-after.json,
teleport-presence.jsonl. Aucune modification effectuée pendant ces relevés.

La transaction conserve l’original côté serveur. Map (5) suspend une saisie
en cours ; NoUi (51) prolonge uniquement une suspension déjà engagée. Le client publie
un snapshot protégé par mutex (écran, date UI, date du ciblage, monde, génération,
activité et éligibilité). Le serveur lit une copie cohérente, sans pointeur UI.
Les clics, échantillons d’aperçu et demandes de pose antérieurs sont invalidés.
Une nouvelle entrée en jeu exige un aperçu frais. Les autres menus, la perte
de focus, le changement d’outil ou de monde et l’invalidation de la source
continuent d’annuler. Fermer la carte sans téléportation reprend aussi l’aperçu.

Chaque requête contient l’identité et l’entité de l’autel de destination. La
validation serveur et le commit exigent toujours ce même autel actif, contenant
tous les coins de l’objet, ainsi que les contrôles natifs de pose. Le claim source
reste contrôlé par son GUID et son entité d’autel, choisis à la saisie. Un autre
claim superposé ne le remplace pas si l’ordre des zones change. Retirer ou réduire
ce claim au point d’exclure l’origine invalide la saisie.

Après commit, la synchronisation attend l’apparition de l’objet côté client.
Une entité absente ne signifie plus identité incorrecte. Une identité existante
mais différente reste refusée, et la destruction du monde efface les attentes.
Le serveur retire les attentes dont l’objet a disparu, changé d’identité ou de
position (avec la tolérance déjà utilisée pour l’arrondi réseau). Une destination
encore valable n’expire pas : après cinq secondes, le rendu réessaie au maximum
une fois par seconde et journalise l’attente, même si le descripteur client manque.

## Validation

Tests hors jeu : carte et chargement prolongés, reprise sans Tab, refus des autres
menus, focus et changement d’outil, invalidation d’une ancienne pose, changement
d’autel entre validation et commit, pose entre claims pour coffre/décor/porte/puits,
et réplication client retardée. La campagne complète conserve les régressions.

Confirmé par Alain en 0.1.45 : saisie, carte/téléportation, pose dans un autre
claim et position conservée après rechargement. En 0.1.46, vérifier aussi
la fermeture de la carte sans trajet et l’annulation après arrivée. Aucun test
hors jeu ne certifie le cycle réel de téléportation ou de sauvegarde du moteur.
Les instances de scène non encore prises en charge restent refusées comme avant.


## Correction de suspension — 0.1.45

Retour Alain : ouvrir la carte annule la saisie en 0.1.44. Le relevé de mémoire
confirme que le global UI à 0x283f4a8 est généralement nul hors des appels de
rendu. Le watch observe ponctuellement Map=5 entre des lectures nulles.
Le serveur interprétait cette absence de contexte comme écran Invalid=0 et
annulait la transaction après que le client avait suspendu son mode actif.

Le serveur ne déréférence plus ce contexte graphique. transportFrame publie
le HudScreenType réel du curseur, et le serveur lit cette copie atomique.
Si le client publie Map entre la lecture serveur et la perte d’éligibilité,
la suspension est revérifiée avant toute annulation. La première frame de
carte après une longue pause préserve aussi la sélection interne déjà armée.

Tests : accessor sans contexte graphique, entrelacement Map/éligibilité,
première frame Map retardée, suspension NoUi, reprise, puis campagne complète.
Le parcours a ensuite été confirmé en jeu par Alain, y compris après rechargement.

## Protections des interruptions — 0.1.46

- Le destructeur ECS natif `0x95d9c0` détruit notamment le stockage dynamique
  `world+0x7bc030`. Appels observés depuis les destructions des mondes serveur
  et client (`0x94e0f6`, `0x94e571`). Le detour invalide uniquement l’état du mod,
  avant de transmettre l’appel natif, hors de ses mutex. Il incrémente une génération
  indépendante de l’adresse mémoire. Aucun handle de rendu n’est libéré sur ce thread.
- Le passage UI `0xd5bb50`, appelé à `0xbd1127` avec le contexte UI vivant, publie
  aussi l’écran pendant l’absence de ciblage. Chaîne vérifiée : contexte+0xd0 →
  services+0 → game+0x38 → client+0x1cd8. Il transmet toujours l’appel natif.
- Une carte ou un chargement avec heartbeat UI frais peut durer indéfiniment.
  Si le serveur continue mais qu’aucune publication UI ne survient pendant cinq
  secondes, la saisie est annulée. Si le serveur lui-même a été suspendu plus de
  cinq secondes, deux secondes de grâce permettent la première frame UI de reprise.
  Après un retour InGame, le ciblage dispose de deux secondes pour reprendre ;
  son absence n’entretient plus une suspension éternelle.
- `keen::ecs::NetworkHealth` (8 octets : santé à +0, maximum à +4) est publié
  depuis `Health` par `network_health_attribute` (`0x27a230`). Santé nulle,
  maximum nul ou composant absent interdisent saisie et commit, y compris sur la carte.
- Tests : 144 combinaisons famille/phase/interruption, santé réelle du mock,
  menus, focus, outil, source/claim invalidés, ancien snapshot de session,
  voyages répétés, chargement vivant prolongé, serveur suspendu, reprise avortée,
  fermeture avant pose et réutilisation d’adresse, claims superposés,
  réplication tardive, disparition serveur et limitation des reprises de rendu.

Les nouveaux hooks sont couverts par la restriction SHA-256 de l’exécutable.
Les tests hors jeu n’exécutent pas le moteur ni sa sauvegarde ; les essais de la
0.1.46 en jeu restent à effectuer par Alain.
