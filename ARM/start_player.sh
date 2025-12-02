#!/bin/sh
# start_player.sh
cd /home/root/proyecto_audio
WAV_FILES=$(ls -1 *.wav 2>/dev/null | tr '\n' ' ')
if [ -z "$WAV_FILES" ]; then
    echo "Error: No hay archivos .wav"
    exit 1
fi
exec ./wav_player $WAV_FILES
