#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct {
    char     id[4];
    uint32_t size;
} ChunkHeader;

typedef struct {
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
} FmtChunk;
#pragma pack(pop)

static void clean_string(char *s) {
    int len = (int)strlen(s);
    while (len > 0 &&
          (s[len-1] == '\r' ||
           s[len-1] == '\n' ||
           s[len-1] == ' '  ||
           s[len-1] == '\t')) {
        s[len-1] = '\0';
        len--;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s archivo.wav\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("No se pudo abrir el archivo");
        return 1;
    }

    char riff[4], wave[4];
    uint32_t riffSize;

    if (fread(riff, 1, 4, f) != 4 ||
        fread(&riffSize, 4, 1, f) != 1 ||
        fread(wave, 1, 4, f) != 4) {
        printf("Archivo demasiado pequeño\n");
        fclose(f);
        return 1;
    }

    if (memcmp(riff, "RIFF", 4) != 0 || memcmp(wave, "WAVE", 4) != 0) {
        printf("No es un archivo WAV válido\n");
        fclose(f);
        return 1;
    }

    FmtChunk fmt;
    uint32_t dataSize   = 0;
    int fmtFound        = 0;
    int dataFound       = 0;

    char title[64]  = "";
    char artist[64] = "";
    char album[64]  = "";

    // Recorremos todos los chunks después de RIFF/WAVE
    while (1) {
        ChunkHeader chunk;
        if (fread(&chunk, sizeof(chunk), 1, f) != 1) {
            break; // fin de archivo
        }

        // "fmt "
        if (memcmp(chunk.id, "fmt ", 4) == 0) {
            if (fread(&fmt, sizeof(FmtChunk), 1, f) != 1) {
                printf("Error leyendo chunk fmt\n");
                fclose(f);
                return 1;
            }
            if (chunk.size > sizeof(FmtChunk)) {
                fseek(f, (long)(chunk.size - sizeof(FmtChunk)), SEEK_CUR);
            }
            fmtFound = 1;
        }
        // "data"
        else if (memcmp(chunk.id, "data", 4) == 0) {
            dataSize = chunk.size;
            dataFound = 1;
            fseek(f, (long)chunk.size, SEEK_CUR);
        }
        // "LIST" (posible INFO)
        else if (memcmp(chunk.id, "LIST", 4) == 0) {
            uint32_t listSize = chunk.size;

            if (listSize < 4) {
                fseek(f, (long)listSize, SEEK_CUR);
                continue;
            }

            char listType[4];
            if (fread(listType, 1, 4, f) != 4) {
                printf("Error leyendo tipo de LIST\n");
                fclose(f);
                return 1;
            }
            listSize -= 4;

            if (memcmp(listType, "INFO", 4) != 0) {
                fseek(f, (long)listSize, SEEK_CUR);
            } else {
                // Subchunks INFO
                while (listSize > sizeof(ChunkHeader)) {
                    ChunkHeader infoChunk;
                    if (fread(&infoChunk, sizeof(infoChunk), 1, f) != 1) {
                        break;
                    }
                    listSize -= sizeof(ChunkHeader);

                    if (infoChunk.size > listSize) {
                        fseek(f, (long)listSize, SEEK_CUR);
                        listSize = 0;
                        break;
                    }

                    uint32_t toRead = infoChunk.size;
                    char buf[256];
                    if (toRead >= sizeof(buf)) {
                        toRead = sizeof(buf) - 1;
                    }

                    if (fread(buf, 1, toRead, f) != toRead) {
                        break;
                    }
                    buf[toRead] = '\0';
                    listSize -= infoChunk.size;

                    clean_string(buf);

                    if (memcmp(infoChunk.id, "INAM", 4) == 0 && title[0] == '\0') {
                        strncpy(title, buf, sizeof(title) - 1);
                        title[sizeof(title) - 1] = '\0';
                    } else if (memcmp(infoChunk.id, "IART", 4) == 0 && artist[0] == '\0') {
                        strncpy(artist, buf, sizeof(artist) - 1);
                        artist[sizeof(artist) - 1] = '\0';
                    } else if (memcmp(infoChunk.id, "IPRD", 4) == 0 && album[0] == '\0') {
                        strncpy(album, buf, sizeof(album) - 1);
                        album[sizeof(album) - 1] = '\0';
                    }

                    if (infoChunk.size > toRead) {
                        uint32_t skip = infoChunk.size - toRead;
                        fseek(f, (long)skip, SEEK_CUR);
                        listSize -= skip;
                    }

                    if (infoChunk.size & 1) {
                        if (listSize > 0) {
                            fseek(f, 1, SEEK_CUR);
                            listSize--;
                        }
                    }

                    if (listSize <= 0) {
                        break;
                    }
                }

                if (listSize > 0) {
                    fseek(f, (long)listSize, SEEK_CUR);
                    listSize = 0;
                }
            }
        }
        // otros chunks: saltar
        else {
            fseek(f, (long)chunk.size, SEEK_CUR);
        }
    }

    if (!fmtFound) {
        printf("Error: no se encontró chunk fmt\n");
        fclose(f);
        return 1;
    }
    if (!dataFound) {
        printf("Error: no se encontró chunk data\n");
        fclose(f);
        return 1;
    }
    if (fmt.audioFormat != 1) {
        printf("No es PCM (audioFormat=%d)\n", fmt.audioFormat);
        fclose(f);
        return 1;
    }

    printf("==== METADATA WAV ====\n");
    printf("Archivo        : %s\n", argv[1]);
    printf("Canales        : %d\n", fmt.numChannels);
    printf("Sample Rate    : %d Hz\n", fmt.sampleRate);
    printf("Bits por sample: %d\n", fmt.bitsPerSample);
    printf("Data Size      : %u bytes\n", dataSize);

    double duracion = (double)dataSize /
                      (fmt.sampleRate * fmt.numChannels * (fmt.bitsPerSample / 8.0));
    printf("Duracion aprox : %.2f segundos\n", duracion);

    printf("\n---- INFO (LIST/INFO) ----\n");
    printf("Titulo (INAM): %s\n", title[0]  ? title  : "(no disponible)");
    printf("Artista(IART): %s\n", artist[0] ? artist : "(no disponible)");
    printf("Album  (IPRD): %s\n", album[0]  ? album  : "(no disponible)");

    fclose(f);
    return 0;
}