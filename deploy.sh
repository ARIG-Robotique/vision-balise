#!/bin/sh

ROBOT_NAME=sauron
INSTALL_DIR=/home/pi/$ROBOT_NAME

echo "Nettoyage des dossiers"
ssh $ROBOT_NAME mkdir -p $INSTALL_DIR
ssh $ROBOT_NAME rm -vf $INSTALL_DIR/*

echo "Compilation ..."
./build-pi-new.sh

echo "Déploiement Applicatif ..."
scp ./build-pi-new/bin/vision_balise $ROBOT_NAME:$INSTALL_DIR/
scp ./config.yml $ROBOT_NAME:$INSTALL_DIR/
scp ./calibration.yml $ROBOT_NAME:$INSTALL_DIR/
scp ./etallonage.yml $ROBOT_NAME:$INSTALL_DIR/
scp -r ./scripts/* $ROBOT_NAME:$INSTALL_DIR/
ssh $ROBOT_NAME mkdir -p $INSTALL_DIR/output

echo "Déploiement service ..."
ssh $ROBOT_NAME sudo mv $INSTALL_DIR/$ROBOT_NAME.service /lib/systemd/system/
ssh $ROBOT_NAME sudo mv $INSTALL_DIR/$ROBOT_NAME-shutdown.service /lib/systemd/system/
ssh $ROBOT_NAME sudo systemctl daemon-reload
#ssh $ROBOT_NAME sudo systemctl enable $ROBOT_NAME.service
ssh $ROBOT_NAME sudo systemctl enable $ROBOT_NAME-shutdown.service
