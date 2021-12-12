#!/bin/sh

ROBOT_NAME=sauron
ROBOT_HOST=${1:-${ROBOT_NAME}}
INSTALL_DIR=/home/pi/${ROBOT_NAME}

mkdir -p output-${ROBOT_NAME}

echo "Récupération des fichiers ..."
scp ${ROBOT_HOST}:${INSTALL_DIR}/output/* ./output-${ROBOT_NAME}/

echo "Nettoyage ..."
ssh ${ROBOT_HOST} rm -vf ${INSTALL_DIR}/output/*
