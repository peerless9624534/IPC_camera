#!/bin/sh
##check ota.zip 
#!/bin/sh
##check ota.zip
OTA_FILE=/tmp/ota.zip
UNZIP_PATH=/tmp
if [ -s $OTA_FILE ]; then
    echo "ota upgrading ..."
    unzip -o $OTA_FILE -d $UNZIP_PATH
    chmod 775 -R $UNZIP_PATH/ota;
    sh $UNZIP_PATH/ota/ota.sh
    echo "ota done, rebooting..."
    rm -rf $UNZIP_PATH/ota
    rm -f $OTA_FILE
    sync
    reboot
fi

