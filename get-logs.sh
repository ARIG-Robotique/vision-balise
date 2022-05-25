#!/bin/sh

ROBOT_NAME=sauron
ROBOT_HOST=${1:-${ROBOT_NAME}}
INSTALL_DIR=/home/pi/${ROBOT_NAME}

mkdir -p logs-${ROBOT_NAME}

echo "Récupération des fichiers ..."
scp ${ROBOT_HOST}:${INSTALL_DIR}/logs/* ./output-${ROBOT_NAME}/

echo "Nettoyage ..."
ssh ${ROBOT_HOST} rm -vf ${INSTALL_DIR}/logs/*
