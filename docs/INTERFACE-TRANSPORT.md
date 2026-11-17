# Interface transport — 0.1.47-dev

La pince équipée affiche les limites natives de construction. Tab active ou

quitte le mode transport ; Échap en jeu, les menus autres que la carte, la perte du premier plan et le

changement d’outil annulent l’opération en cours.

Le bandeau propose Saisir, Poser et Terminer. Saisir est disponible pour un objet autorisé

ciblé dans une zone autorisée. Poser dépend de l’aperçu et de la validation native.

Pendant la saisie, les aides natives de rotation, rotation fine, décalage et

accrochage sont affichées. Le réglage de masquage des aides est respecté.

Toutes les commandes suivent les actions natives, dont entrée/sortie du mode
et annulation (129/126). Le contexte `0x3c47b0` capture leurs états évalués ;
le filtre `0xe85800` maintient leur éligibilité uniquement pour la pince en jeu,
avec focus et observation fraîche. Les états consommés ne déclenchent pas
l’action native correspondante. Les bindings, modificateurs et états physiques
restent au moteur : aucune interception de Tab/Échap par code Windows.
Les essais hors jeu vérifient les fronts, le maintien d’une touche, les menus,
les drapeaux de blocage et l’absence de modification d’autres actions.
Clavier réaffecté et manette restent à valider en jeu par Alain.

Le surlignage et l’aperçu utilisent les composants graphiques du jeu. La pose

réelle n’intervient qu’après validation, entièrement dans le claim de destination.
Pendant une saisie, la carte et le chargement de téléportation suspendent les
commandes et l’aperçu ; ceux-ci reprennent au retour en jeu.

Les textes propres au mod sont traduits ; le refus hors zone reprend le texte

et l’affichage natifs du marteau. L’orbe orange reste une ressource native.

Le getter 0x3aa980 fournit l’état visuel des frontières. La copie locale au

thread modifie uniquement son indicateur de visibilité, sans modifier les

permissions, l’autel ou l’état partagé du jeu.

## Aides de placement natives (0.1.42)

Le parent natif `0xb6bb80` est conservé intégralement. Il organise le panneau

objet `0xb6a560` (nom, rotation à 90°, angle courant, accrochage), puis le

panneau Ajustement `0xc6e1d0` au-dessus. Les trois arguments sont transmis :

contexte d’entrée et deux rectangles de sortie. L’ancrage, l’échelle et les

réglages de visibilité restent ceux du jeu.

Pendant cet appel seulement, un pointeur local au thread désigne l’ItemInfo

de l’objet saisi. Le lookup `0xe94470` le renvoie exclusivement au site de

retour `0xb6bc42`, où le parent demande l’objet équipé. Tous les autres appels

conservent leur résultat natif. Aucun objet d’inventaire n’est remplacé.

Le constructeur natif gère les contextes UI, leurs copies et leur destruction.

Le prédicat `0xd8e320` autorise ces contextes uniquement pendant ce rendu et

si leur ItemInfo à +0 correspond à l’objet emprunté. Aucun état partagé de

construction, permission ou mécanisme de déplacement n’est modifié.

L’affichage exige un aperçu frais, le focus et le réglage de la catégorie

active. Sans ces conditions, le parent s’exécute sans substitution. Le mutex

n’est pas gardé pendant son exécution. Les tests couvrent coffres, décors,

portes, catégories désactivées, péremption, copies de contexte, délégation des

autres lookups, rectangles de sortie et absence d’écriture sur l’entité.

L’aspect à l’écran reste à valider en jeu par Alain.

