# CarryOn pour Enshrouded

Déplacer les coffres avec leur contenu, puis les objets existants du jeu qui
ne peuvent actuellement qu'être détruits. Mod indépendant utilisant le
[SDK Enshrouded ModLoader](https://github.com/aebischer/Enshrouded-ModLoader).

## État : début de développement — 0.1.0-dev

Le projet compile en DLL Windows x64 et respecte le contrat de paquet ABI 1.
La configuration est reçue du chargeur, séparément pour chaque monde.

**Le déplacement en jeu n'est pas encore implémenté.** Cette première version
refuse explicitement son initialisation avec un message dans le journal.
Ne pas l'installer pour jouer : son activation bloque volontairement le
lancement du monde, puisque les fonctions natives ne sont pas encore validées.
Aucune fonction de jeu n'est appelée et aucun coffre n'est modifié.

## Compilation et contrôles

Installer Visual Studio avec les outils C++ x64 et le SDK Windows, puis lancer :

    build.cmd

Le script détecte Visual Studio, compile la DLL et exécute les tests
d'intégration dans un processus distinct. Le paquet est sous
'dist/Mods/carryon/carryon.dll', accompagné de 'carryon.dll.mod'.
Il ne fournit pas de dinput8.dll et ne remplace pas le chargeur.

Depuis le dépôt du chargeur, après build.cmd et build-sdk.cmd :

    build\validate_mod_package.exe CHEMIN_VERS_CARRYON\dist carryon/carryon.dll

Cette validation contrôle le format et le SDK, pas le gameplay.
Les deux options prévues, coffres et autres objets, utilisent le stockage par
monde du chargeur. Elles ne produiront d'effet qu'après intégration native.

## Développement

- [Périmètre et invariants](docs/CONCEPTION.md)
- [Recherche native et étapes restantes](docs/INTEGRATION_NATIVE.md)
- [Version du SDK](sdk/UPSTREAM.txt)

Le SDK fournit l'intégration au chargeur, pas une API de coffres.
Le support multijoueur et serveur dédié demande une intégration distincte ;
il n'est pas annoncé pour cette version.
