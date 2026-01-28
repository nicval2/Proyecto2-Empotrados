# Proyecto2-Empotrados

```bash
lsblk
```

```bash
sudo umount /dev/sdb1
sudo umount /dev/sdb2
sudo umount /dev/sdb3
```

```bash
sudo dd if=de10_standard_linux_console.img of=/dev/sdb bs=4M status=progress conv=fsync
```

```bash
sudo pkill -9 screen
```

```bash
lsof /dev/ttyUSB0
```


```bash
sudo screen /dev/ttyUSB0 115200
```


```bash
sudo apt update
sudo apt install gcc-arm-linux-gnueabihf
```

```bash
arm-linux-gnueabihf-gcc --version
```

Por tu lsblk ya vimos que está en:
```bash
/dev/sdb2  →  /media/kun/ac4e5a85-5e37-401f-8474-20d4316e4dee
```

Tomemos eso como SYSROOT:
```bash
export SYSROOT=/media/kun/ac4e5a85-5e37-401f-8474-20d4316e4dee
```

### Recompilar hola.c usando ese sysroot
Usa las headers y librerías que están dentro del rootfs de la SD.

Por lo tanto el binario dependerá de la glibc vieja correcta (la que sí existe en la DE1-SoC).
```bash
arm-linux-gnueabihf-gcc \
  --sysroot=$SYSROOT \
  -o hola_arm_sysroot \
  hola.c
```

### Copiar al rootfs del board
```bash
sudo cp hola_arm_sysroot $SYSROOT/home/root/
chmod +x $SYSROOT/home/root/hola_arm_sysroot
sync
```


Sincronizar y desmontar (MUY IMPORTANTE) - para no dañar la SD
```bash
umount /media/kun/ac4e5a85-5e37-401f-8474-20d4316e4dee
umount /media/kun/F725-1429
```


```bash
sudo cp We_Didnt_Start_the_Fire.wav /media/kun/847f4797-311c-4286-8370-9d5573b201d7/home/root/proyecto_audio/
```

```bash
sync
umount /media/kun/847f4797-311c-4286-8370-9d5573b201d7
umount /media/kun/B817-2299
```

```bash
ffmpeg -i We_Didnt_Start_the_Fire.wav \
  -c:a copy \
  -metadata title="We Didn't Start the Fire" \
  -metadata artist="Billy Joel" \
  -metadata album="Storm Front" \
  We_Didnt_Start_the_Fire_tagged.wav
```

```bash
gcc wav_info.c -o wav_info
```

```bash
./wav_info We_Didnt_Start_the_Fire.wav
```

Angstrom linux distribution
```bash
https://www.terasic.com.tw/cgi-bin/page/archive.pl?Language=English&CategoryNo=205&No=1081&PartNo=4
```

```bash
cat > wav_info.c << 'EOF'
cat > wav_player_arm.c << 'EOF'
```
EOF

vi wav_info.c

```bash
cd /media/kun/847f4797-311c-4286-8370-9d5573b201d7
cd home/root/proyecto_audio
ls
cp wav_player /home/kun/Descargas/Proyecto2-Empotrados/ARM/
```

```bash
sudo vi /etc/systemd/system/wav-player.service
```

```bash
# 1. Recargar la configuración de systemd
sudo systemctl daemon-reload

# 2. Habilitar el servicio (para que inicie en el arranque)
sudo systemctl enable wav-player.service

# 3. Iniciar el servicio inmediatamente
sudo systemctl start wav-player.service

# 4. Verificar el estado
sudo systemctl status wav-player.service
```

Detener el servicio 
```bash
systemctl stop wav-player.service
```

Deshabilitar el servicio
```bash
systemctl disable wav-player.service
```

Eliminar archivo de definición 
```bash
rm /etc/systemd/system/wav-player.service
cat > /etc/systemd/system/wav-player.service << 'EOF'
```

Recargar la configuración de systemd
```bash
systemctl daemon-reload
```

```bash
gcc wav_player_arm.c -o wav_player -O2 -std=c99
```
