# Migrarea Bibliotecii `so_stdio` de la Linux la Windows

## Modificări Generale

1. **Înlăturarea antetelor specifice Linux**: Unele anteturi precum `<fcntl.h>`, `<sys/types.h>`, și `<sys/stat.h>` nu sunt necesare sau disponibile în mediu Windows.
2. **Adăugarea antetelor specifice Windows**: Am inclus anteturi specifice Windows precum `<windows.h>` pentru a putea folosi API-urile Windows.

## Funcția `so_fopen`

1. **Modificarea modului de deschidere a fișierelor**: În loc de funcția `open()` am folosit funcția `CreateFileA()` pentru a deschide fișiere în Windows.
2. **Gestionarea modurilor de deschidere**: Modurile de deschidere (`r`, `w`, `a`, etc.) au fost mapate la constantele corespunzătoare din Windows.

## Funcția `so_fclose`

1. **Modificarea modului de închidere a fișierelor**: În loc de funcția `close()` am folosit funcția `CloseHandle()` pentru a închide fișiere în Windows.

## Funcțiile `so_fflush`, `so_fgetc`, `so_fputc`, `so_fread`, și `so_fwrite`

1. **Modificarea operațiilor de citire și scriere**: Am înlocuit funcțiile `read()` și `write()` cu `ReadFile()` și `WriteFile()` din Windows.

## Funcția `so_fseek` și `so_ftell`

1. **Modificarea modului de setare și obținere a poziției în fișier**: Am înlocuit funcțiile `lseek()` și `tell()` cu `SetFilePointer()` în Windows.

## Funcția `so_popen` și `so_pclose`

1. **Modificarea modului de creare a proceselor**: Am înlocuit mecanismul `popen()` și `pclose()` din Linux cu un mecanism bazat pe `CreateProcess()` și `CreatePipe()`. În Windows, nu există un echivalent direct al funcției `wait()`, așa că returnăm 0 pentru a indica succes. 


## Rulare cod

1. Pentru a face biblioteca dinamica am facut comanda: cl /c /W3 /TC main.c (cu ajutorl vs command line) si dupa am rulat : link /dll /out:so_stdio.dll main.obj