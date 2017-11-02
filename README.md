# Vision Balise C++ avec OpenCV
[![Build Status](https://travis-ci.org/ARIG-Robotique/vision-balise.svg?branch=master)](https://travis-ci.org/ARIG-Robotique/vision-balise)

## Description

Ce projet ouvre un socket TCP sur le port 40000 afin de recevoir des ordres
JSON pour piloter la récupération des information avec [OpenCV](https://opencv.org/)

## Messages JSON

### Récupérer les infos du pattern de construction du batiment

* Query
```json
{
  "action": "PATTERN"
}
```

* Réponse
```json
{
  "status": "OK",
  "action": "PATTERN",
  "datas": {
    "colorLeft": "FFFFFF",
    "colorCenter": "12AA43",
    "colorRight": "EFEFEF"
  },
  "errorMessage": null
}
```

### Récupérer l'état des stations d'épurations

* Query
```json
{
  "action": "STATIONS"
}
```

* Réponse
```json
{
  "status": "OK",
  "action": "STATIONS",
  "datas": {
    "stationEquipe": {
      "eauxPropres" : 3,
      "eauxSales" : 2
    },
    "stationAdverse": {
      "eauxPropres" : 3,
      "eauxSales" : 2
    }
  },
  "errorMessage": null
}
```

### Quitter le programme

* Query
```json
{
  "action": "EXIT"
}
```

* Réponse
```json
{
  "status": "OK",
  "action": "EXIT",
  "errorMessage": null
}
```
