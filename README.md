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
sudo dd if=DE1_SoC_SD.img of=/dev/sdb bs=4M status=progress conv=fsync
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


