# Vision Balise C++ avec OpenCV
[![Build Status](https://travis-ci.org/ARIG-Robotique/vision-balise.svg?branch=master)](https://travis-ci.org/ARIG-Robotique/vision-balise)

## Description

Ce projet ouvre un socket TCP sur le port 9042 afin de recevoir des ordres
JSON pour piloter la récupération des information avec [OpenCV](https://opencv.org/)

Jusqu'à deux clients peuvent se connecter en même temps.

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

### Récupérer la dernière photo prise

* Query
```json
{"action":"PHOTO"}
```

* Réponse
```json
{
  "status": "OK",
  "action": "PHOTO",
  "data": "......base64....."
}
```

### Récupérer le statut

* Query
```json
{"action":"STATUS"}
```

* Réponse
```json
{
  "status": "OK",
  "action": "STATUS",
  "data": {
    "etalonnageDone": true,
    "etalonnageConfirmed": true,
    "detection": {
      "distribs": ["PRESENT", "ABSENT"],
      "echantillons": [
        {"c": "RED", "x": 1500, "y": 850},
        {"c": "GREEN", "x": 1500, "y": 850},
        {"c": "BLUE", "x": 1500, "y": 850},
        {"c": "ROCK", "x": 1500, "y": 850}
      ]
    }
  }
}
```

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
  "data": "......base64....."
}
```

### Valider l'étalonnage

* Query
```json
{"action": "CONFIRM"}
```

* Réponse
```json
{
 "status": "OK",
 "action": "CONFIRM"
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
