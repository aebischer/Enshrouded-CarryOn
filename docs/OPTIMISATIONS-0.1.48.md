# Mémoire et traitements — 0.1.48-dev

Les lots natifs de lecture restent transmis sans changement de descripteur,
d’ordre ni de découpage supplémentaire. Seuls les contenus privés séparent les
séquences natives adjacentes, comme en 0.1.47.

## Corrections

1. Les allocations des caches de placement sont protégées jusqu’à leur
   publication. Un échec temporaire de lecture ou d’allocation d’une ressource
   intégrée ne la marque plus définitivement invalide. L’indexation peut
   reprendre après une publication partielle sans transmettre un ID nul au moteur.
2. Les ressources absentes des portes et des puits sont retentées au plus une
   fois par seconde, par modèle. Les profils de pose conservent leur temporisation.
3. Le ciblage des objets déplaçables n’est plus exécuté lorsque le mode transport
   est inactif, sans focus, ou pendant une saisie. Les frontières et le diagnostic
   gardent leurs parcours propres. Un serveur sans saisie ni synchronisation en
   attente conserve son suivi de session sans réinitialiser la transaction à chaque pas.
4. L’indexation effectue au maximum 64 recherches natives par callback. Les
   recherches, lectures et calculs de profils s’effectuent sur leur thread natif,
   hors du mutex des caches. La préparation est sérialisée sans faire attendre
   un second callback sur le verrou de préparation. Les résultats déjà en cache
   restent immédiatement disponibles.
5. Le serveur effectue au maximum 32 contrôles de synchronisation par passage,
   avec rotation pour éviter la famine. Une synchronisation de plus de cinq
   secondes est réexaminée au plus une fois par seconde. Les contrôles frais du
   coffre ou décor saisi et la validation finale de pose restent indépendants.
6. Les profils réservent directement leur taille finale ; les références
   relatives et les allocations déjà publiées restent intactes. La liste des
   IDs temporairement absents est compactée sur place, sans nouveau vecteur à
   chaque reprise.
7. Les résultats privés sont rangés par demandeur. La copie des contenus est
   effectuée hors verrou ; réservation et publication restent synchronisées.
   Une file en préparation ne peut pas être supprimée par le consommateur.
   Jusqu’à 64 nœuds libérés sont réutilisés ; aucune liste temporaire n’alloue
   de sentinelle à chaque requête. Les files vides sans producteur sont retirées.

## Mesures et tests hors jeu

| Scénario déterministe | Avant | Après |
|---|---:|---:|
| Capacité des 5 769 profils du corpus | 29 688 691 octets | 24 491 262 octets |
| Ressource de porte absente, 100 demandes rapprochées | 100 lectures | 1 lecture |
| 32 synchronisations anciennes, 100 passages rapprochés | 3 200 lectures de composants | 32 lectures |
| Exceptions d’allocation échappant des deux insertions examinées | 2 cas reproduits | 0 |

Ces nombres ne représentent pas la RAM occupée ou les FPS d’une partie réelle.
Tous les profils ne sont pas construits simultanément en jeu. Les essais en jeu
restent effectués par Alain.

`tests/memory_runtime.cpp` vérifie également huit positions de panne pendant
l’indexation, sa reprise complète, la disponibilité du mutex des caches pendant
une recherche native, la reprise après temporisation, l’équité du parcours des
synchronisations et 2 000 résultats privés consommés concurremment dans l’ordre,
exactement une fois. Les compteurs de réservation doivent revenir à zéro.

La validation complète conserve les tests des 256 combinaisons de lectures,
du lot natif de 128 requêtes, des interruptions, du transport entre claims,
des onze puits, des traductions et du modèle 3D.
