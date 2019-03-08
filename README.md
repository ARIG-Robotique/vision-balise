# Vision Balise C++ avec OpenCV
[![Build Status](https://travis-ci.org/ARIG-Robotique/vision-balise.svg?branch=master)](https://travis-ci.org/ARIG-Robotique/vision-balise)

## Description

Ce projet ouvre un socket TCP sur le port 40000 afin de recevoir des ordres
JSON pour piloter la récupération des information avec [OpenCV](https://opencv.org/)

## Options

```bash
$ vision_balise --help
```

:-)

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
  "datas": "......base64....."
}
```

### Récupérer le statut (etallonage ok + résultat détection)

* Query
```json
{"action":"STATUS"}
```

* Réponse
```json
{
  "status": "OK",
  "action": "STATUS",
  "datas": {
    "foundGreen": [
      {"x": 100, "y": 100}
    ],
    "foundRed": [
      {"x": 100, "y": 100}
    ],
    "foundBlue": [
      {"x": 100, "y": 100}
    ],
    "verifiedGreen": [
      {"x": 100, "y": 100}
    ],
    "verifiedRed": [
      {"x": 100, "y": 100}
    ],
    "verifiedBlue": [
      {"x": 100, "y": 100}
    ]
  }
}
```

### Lancer l'étallonage

* Query
```json
{"action": "ETALLONAGE"}
```

* Réponse
```json
{
 "status": "OK",
 "action": "ETALLONAGE"
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
