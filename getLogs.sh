#!/bin/sh

ROBOT_NAME=sauron
INSTALL_DIR=/home/pi/${ROBOT_NAME}

mkdir -p output-${ROBOT_NAME}

echo "Récupération des infos ..."
scp ${ROBOT_NAME}:${INSTALL_DIR}/output/* ./output-${ROBOT_NAME}/

echo "Nétoyage ..."
ssh ${ROBOT_NAME} sudo rm -Rvf ${INSTALL_DIR}/${ROBOT_NAME}/output/*
