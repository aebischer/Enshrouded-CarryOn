Périmètre CarryOn — 13 novembre 2026

Demandé : prendre et redéposer les coffres avec leur contenu, puis les autres
objets existants du jeu qui ne proposent actuellement que la destruction.

Premier jalon jouable : un coffre ordinaire dans une copie de monde solo.
Ensuite : autres variantes de coffres (dont magiques), puis familles d'objets.
Les objets de décor et les objets construits par le joueur peuvent suivre des
mécanismes différents : vérifier chaque famille avant de l'autoriser.

Invariants à démontrer dans le jeu
- Même objet logique, même inventaire, mêmes quantités, propriétés et liens.
- Une action de déplacement ne crée ni copie ni butin et ne détruit pas l'objet.
- Un dépôt refusé laisse l'état récupérable ; aucune consommation partielle.
- Vérifier distance, accès, collision, support et limites de construction.
- Pas de mutation d'un coffre simultanément ouvert/utilisé sans arbitrage.
- Mort, fermeture du monde et interruption du transport ne perdent rien.
- Sauvegarder puis recharger retrouve exactement un objet et le contenu attendu.
- Les objets de quête et de progression nécessitent une étude de leurs liens.
  Ne pas assimiler un template instanciable à un objet librement déplaçable.

Choix d'implémentation encore à valider
Chercher d'abord un déplacement de l'entité persistante existante. Ne pas
remplacer un coffre par un objet d'inventaire générique : cette conversion
pourrait perdre son contenu et ses métadonnées. Le mode de transport, l'aperçu
de dépôt et les raccourcis seront intégrés après identification des API natives.
Aucune adresse, structure mémoire ou transaction de jeu n'est encore validée.

Multijoueur à conserver dans l'architecture
Le serveur valide et applique les prises/dépôts ; le client exprime une demande.
La configuration appartient au monde du serveur. Verrouiller l'objet pour éviter
deux prises simultanées, vérifier droits et versions, synchroniser l'état.
Tester déconnexion, mort et redémarrage. ABI 1 n'offre actuellement ni réseau
ni événements de cycle de monde : ne pas prétendre qu'une DLL chargée côté
client fournit déjà ce support.
