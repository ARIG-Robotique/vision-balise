# Vision Balise C++ avec OpenCV
[![Build Status](https://travis-ci.org/ARIG-Robotique/vision-balise.svg?branch=master)](https://travis-ci.org/ARIG-Robotique/vision-balise)

## Description

Ce projet ouvre un socket TCP sur le port 9042 afin de recevoir des ordres
JSON pour piloter la récupération des information avec [OpenCV](https://opencv.org/)

Plusieurs clients peuvent se connecter en même temps.

Rendre la camera accessible par OpenCV :
```bash
$ sudo modprobe bcm2835-v4l2
```

## Options

- `--test` : lance le mode de test (fichiers en dur et affichage du résultat)
- `--debug` : actives les messages de debug et le stockage des images en sortie
- `--output-dir=output/` : dossier de stockage des images en sortie
- `--calibration` : lance la calibration de l'objectif
- `--calibration-dir=samples/calib/` : dossier contenant les photos source pour la calibration
- `--calibration-file=calibration.yml` : fichier de résultat de calibration
- `--config-file=config.yml` : fichier de configuration
- `--socket-type=inet` : type de socket pour la comm, `inet` ou `unix`
- `--socket-port=9042` : port pour le socket inet
- `--socket-file=/tmp/vision_balise.sock` : fichier pour le socket unix
- `--mock-photo=path/to/image.jpg` : fichier de mock de la caméra


## Messages JSON

### Récupérer les dernières photo prise

* Query
```json
{
  "action":"PHOTO",
  "cameras": ["cam1gauche", "cam2droite"]
}
```

> Le champ `cameras` est facultatif, si il est null ou vide, toutes les cameras sont retournées. 

* Réponse
```json
{
  "status": "OK",
  "action": "PHOTO",
  "data": {
    "cam1gauche": ".......... BASE64 ............",
    "cam2droite": ".......... BASE64 ............"
  } 
}
```

### Récupérer le statut

* Query
```json
{
  "action":"STATUS",
  "content": ["etalonnage", "pots" ]
}
```

Le champ content peut contenir les valeurs suivantes :

- `etalonnage` : Contient l'état de l'étalonnage.
- `pots` : Récupération de l'état des zones de stocks de pots.
- `horloge` : Récupération des plantes dans les zones au centre de la table.
- `plantes` : Liste des plantes isolés sur la table.
- `panneaux` : Etat des panneaux solaires
- `robots` : Position des robots sur la table.

> Le champ `content` vide ou null retourne toutes les infos

* Réponse
```json
{
  "status": "OK",
  "action": "STATUS",
  "data": {
    "etalonnage": true,
    "pots": { 
      "gaucheBleue": true,
      "gaucheJaune": true,
      "gauchePanneaux": true,
      "droitePanneaux": true,
      "droiteBleue": true,
      "droiteJaune": true
    },
    "horloge": {
      "0": [
        { "x": 1500 , "y":  1496, "type":  "F", "debout": true },
        { "x": 1520 , "y":  1446, "type":  "R", "debout": true }
      ],
      "2": [ ],
      "4": [ ],
      "6": [ ],
      "8": [ ],
      "10": [ ]
    },
    "plantes": [
      { "x": 800 , "y":  1138, "type":  "F", "debout": true },
      { "x": 599 , "y":  800, "type":  "R", "debout": false }
    ],
    "panneaux": [ // Index de gauche à droite
      "NEUTRE",
      "JAUNE",
      "BLEUE",
      "JAUNE_BLEU",
      "NEUTRE",
      "NEUTRE",
      "NEUTRE",
      "NEUTRE",
      "NEUTRE"
    ],
    "robots": {
      "nerell": { "x": 1500, "y": 1000 },
      "adversaire": { "x": 1000, "y": 1500 },
      "pamis": {
        "nomPami1": { "x": 1000, "y": 1500 },
        "nomPami2": { "x": 1000, "y": 1500 }
      }
    }
  }
}
```

> Les coordonnées sont dans le repère officiel (Y inversé pour le robot). `distribs` est dans l'ordre droite (JAUNE) / gauche (VIOLET).

### Lancer l'étalonnage

* Query
```json
{"action": "ETALONNAGE"}
```

* Réponse
```json
{
  "status": "OK",
  "action": "ETALONNAGE",
  "data": {
    "cam1gauche": ".......... BASE64 ............",
    "cam2droite": ".......... BASE64 ............"
  }
}
```

### Lancer la détection

* Query
```json
{"action": "DETECTION"}
```

* Réponse
```json
{
 "status": "OK",
 "action": "DETECTION"
}
```

### Demander l'équipe de match

* Query
```json
{"action": "EQUIPE"}
```

* Réponse
```json
{
 "status": "OK",
 "action": "EQUIPE",
 "data": "JAUNE"
}
```

> Data est une des valeurs parmis `JAUNE`, `BLEUE` ou `INCONNU`

### Revient dans un état d'attente

* Query
```json
{"action": "IDLE"}
```

* Réponse
```json
{
  "status": "OK",
  "action": "IDLE"
}
```

### Quitter le programme

* Query
```json
{"action": "EXIT"}
```

* Réponse
```json
{
  "status": "OK",
  "action": "EXIT"
}
```
