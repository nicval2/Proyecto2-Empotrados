import numpy as np
from scipy.io import wavfile
from scipy import signal

# --- ⚙️ PARÁMETROS DE CONFIGURACIÓN ---
INPUT_WAV = "./Songs/rosas.wav"          # Archivo de entrada
OUTPUT_LPF = "rosas_bajas.wav"     # Archivo de salida LPF
OUTPUT_HPF = "rosas_altas.wav"     # Archivo de salida HPF

# Frecuencia de corte (en Hz)
CUTOFF_FREQ = 3000.0

# Orden del filtro (Butterworth)
FILTER_ORDER = 5


def apply_filter(input_file, output_file, cutoff_freq, filter_type):
    """
    Carga un archivo WAV, aplica un filtro Butterworth (con filtfilt para fase cero)
    y guarda el resultado.

    filter_type: 'lowpass' o 'highpass'
    """
    try:
        # 1. Leer archivo WAV
        sample_rate, audio_data = wavfile.read(input_file)
        print(f"\n📂 Procesando: {input_file}")
        print(f"   Frecuencia de muestreo: {sample_rate} Hz")
        print(f"   Tipo de datos original: {audio_data.dtype}")
        print(f"   Dimensiones: {audio_data.shape}")

        # 2. Normalizar frecuencia de corte (Nyquist)
        nyquist_freq = 0.5 * sample_rate
        normalized_cutoff = cutoff_freq / nyquist_freq

        if not 0 < normalized_cutoff < 1:
            print(
                f"❌ Error: La frecuencia de corte ({cutoff_freq} Hz) "
                f"está fuera del rango válido para la Fs = {sample_rate} Hz."
            )
            return

        # 3. Diseñar filtro Butterworth
        b, a = signal.butter(
            FILTER_ORDER,
            normalized_cutoff,
            btype=filter_type,
            analog=False
        )

        # 4. Convertir audio a float32 en [-1, 1] para evitar overflow
        original_dtype = audio_data.dtype

        if np.issubdtype(original_dtype, np.integer):
            # Asumimos int16 típico de audio
            max_val = np.iinfo(original_dtype).max
            audio_float = audio_data.astype(np.float32) / max_val
        else:
            # Ya está en float, sólo casteamos
            audio_float = audio_data.astype(np.float32)

        # 5. Aplicar filtro con fase cero (filtfilt)
        if audio_float.ndim > 1:
            # Estéreo u otros canales: filtramos por columnas
            filtered_float = signal.filtfilt(b, a, audio_float, axis=0)
        else:
            # Mono
            filtered_float = signal.filtfilt(b, a, audio_float)

        # 6. Opcional: normalizar para evitar clipping, manteniendo dinámica
        max_abs = np.max(np.abs(filtered_float))
        if max_abs > 1.0:
            filtered_float = filtered_float / max_abs
            print("   ⚠ Se normalizó el audio filtrado para evitar clipping.")

        # 7. Convertir de vuelta al tipo original (típicamente int16)
        if np.issubdtype(original_dtype, np.integer):
            max_val = np.iinfo(original_dtype).max
            min_val = np.iinfo(original_dtype).min
            scaled = filtered_float * max_val
            # Clip para evitar overflow
            scaled = np.clip(scaled, min_val, max_val)
            final_data = scaled.astype(original_dtype)
        else:
            final_data = filtered_float.astype(original_dtype)

        # 8. Guardar archivo
        wavfile.write(output_file, sample_rate, final_data)
        print(
            f"✅ Filtro {filter_type} aplicado con éxito. "
            f"Archivo guardado en: {output_file}"
        )

    except FileNotFoundError:
        print(f"❌ Error: El archivo de entrada '{input_file}' no se encuentra.")
    except Exception as e:
        print(f"❌ Ocurrió un error durante el procesamiento: {e}")


# --- 🚀 EJECUCIÓN ---
print(f"Iniciando procesamiento con Frecuencia de Corte: {CUTOFF_FREQ} Hz")
print("-" * 50)

# Filtro pasa bajas (deja pasar graves, atenúa agudos)
apply_filter(INPUT_WAV, OUTPUT_LPF, CUTOFF_FREQ, 'lowpass')

# Filtro pasa altas (deja pasar agudos, atenúa graves)
apply_filter(INPUT_WAV, OUTPUT_HPF, CUTOFF_FREQ, 'highpass')
