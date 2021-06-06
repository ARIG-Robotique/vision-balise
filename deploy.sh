#!/bin/sh

ROBOT_NAME=sauron
ROBOT_HOST=${1:-${ROBOT_NAME}}
INSTALL_DIR=/home/pi/$ROBOT_NAME

echo "Compilation ..."
./build-pi.sh

echo "Nettoyage des dossiers"
ssh $ROBOT_HOST mkdir -p $INSTALL_DIR
ssh $ROBOT_HOST rm -vf $INSTALL_DIR/*

echo "Déploiement Applicatif ..."
scp ./build-pi/bin/vision_balise $ROBOT_HOST:$INSTALL_DIR/
scp ./config.yml $ROBOT_HOST:$INSTALL_DIR/
scp ./calibration.yml $ROBOT_HOST:$INSTALL_DIR/
scp -r ./scripts/* $ROBOT_HOST:$INSTALL_DIR/
ssh $ROBOT_HOST mkdir -p $INSTALL_DIR/output

echo "Déploiement service ..."
ssh $ROBOT_HOST sudo mv $INSTALL_DIR/$ROBOT_NAME.service /lib/systemd/system/
ssh $ROBOT_HOST sudo mv $INSTALL_DIR/$ROBOT_NAME-shutdown.service /lib/systemd/system/
ssh $ROBOT_HOST sudo systemctl daemon-reload
ssh $ROBOT_HOST sudo systemctl enable $ROBOT_NAME.service
ssh $ROBOT_HOST sudo systemctl enable $ROBOT_NAME-shutdown.service
