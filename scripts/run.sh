#!/bin/sh

echo "Nettoyage des vielles images"
find output/*.jpg -mmin +120 -exec rm -f {} \;

./vision_balise --debug
