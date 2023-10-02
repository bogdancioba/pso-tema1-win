#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <stddef.h>
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#define BUFFER_SIZE 4096 
#define SO_EOF -1

typedef struct {
    HANDLE fd; 
    char buffer[BUFFER_SIZE];
    int buf_pos;
    int buf_end;
    char mode[3];
    int eof;
    int error;
} SO_FILE;

SO_FILE *so_fopen(const char *pathname, const char *mode) {
    HANDLE hFile;
    DWORD dwDesiredAccess = 0;
    DWORD dwCreationDisposition = 0;

    if (strcmp(mode, "r") == 0) {
        dwDesiredAccess = GENERIC_READ;
        dwCreationDisposition = OPEN_EXISTING;
    } else if (strcmp(mode, "w") == 0) {
        dwDesiredAccess = GENERIC_WRITE;
        dwCreationDisposition = CREATE_ALWAYS;
    } else if (strcmp(mode, "a") == 0) {
        dwDesiredAccess = GENERIC_WRITE;
        dwCreationDisposition = OPEN_ALWAYS;
    }

    hFile = CreateFileA(pathname, dwDesiredAccess, 0, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    SO_FILE *soFile = (SO_FILE *)malloc(sizeof(SO_FILE));
    if (!soFile) {
        CloseHandle(hFile);
        return NULL;
    }

    soFile->fd = hFile;
    soFile->buf_pos = 0;
    soFile->buf_end = 0;
    strncpy_s(soFile->mode, sizeof(soFile->mode), mode, _TRUNCATE);
    soFile->mode[sizeof(soFile->mode) - 1] = '\0';
    soFile->eof = 0;
    soFile->error = 0;

    return soFile;
}

int so_fclose(SO_FILE *stream) {
    if (!stream) {
        return SO_EOF;
    }

    CloseHandle(stream->fd);
    free(stream);

    return 0;
}

int so_fflush(SO_FILE *stream) {
    if (!stream) {
        stream->eof = 1;
        return SO_EOF;
    }

    DWORD bytesToWrite = stream->buf_pos;
    DWORD bytesWritten;

    if (bytesToWrite > 0) {
        if (!WriteFile(stream->fd, stream->buffer, bytesToWrite, &bytesWritten, NULL) || bytesWritten < bytesToWrite) {
            stream->error = 1;
            return SO_EOF;
        }
        stream->buf_pos = 0;
    }

    return 0;
}

int so_fgetc(SO_FILE *stream) {
    if (stream->buf_pos >= stream->buf_end) {
        DWORD bytesRead;

        if (!ReadFile(stream->fd, stream->buffer, BUFFER_SIZE, &bytesRead, NULL)) {
            stream->error = 1;
            return SO_EOF;
        }

        if (bytesRead == 0) {
            stream->eof = 1;
            return SO_EOF;
        }

        stream->buf_pos = 0;
        stream->buf_end = bytesRead;
    }

    return (unsigned char) stream->buffer[stream->buf_pos++];
}

int so_fputc(int c, SO_FILE *stream) {
    if (!stream) {
        return SO_EOF;
    }

    stream->buffer[stream->buf_pos++] = (char)c;

    if (stream->buf_pos >= BUFFER_SIZE) {
        DWORD bytesWritten;
        
        if (!WriteFile(stream->fd, stream->buffer, BUFFER_SIZE, &bytesWritten, NULL) || bytesWritten != BUFFER_SIZE) {
            stream->error = 1;
            return SO_EOF;
        }
        
        stream->buf_pos = 0;
    }

    return (unsigned char) c;
}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream) {
    char *char_ptr = (char *)ptr;
    size_t totalBytesToRead = size * nmemb;
    DWORD bytesRead = 0;

    for (size_t i = 0; i < totalBytesToRead; i++) {
        int ch = so_fgetc(stream);
        
        if (ch == SO_EOF) {
            break;
        }
        
        char_ptr[i] = ch;
        bytesRead++;
    }

    return bytesRead / size;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream) {
    const char *char_ptr = (const char *)ptr;
    size_t totalBytesToWrite = size * nmemb;
    DWORD bytesWritten = 0;

    for (size_t i = 0; i < totalBytesToWrite; i++) {
        if (so_fputc(char_ptr[i], stream) == SO_EOF) {
            break;
        }
        bytesWritten++;
    }

    return bytesWritten / size;
}

int so_fseek(SO_FILE *stream, long offset, int whence) {
    DWORD dwMoveMethod;

    switch (whence) {
        case SEEK_SET:
            dwMoveMethod = FILE_BEGIN;
            break;
        case SEEK_CUR:
            dwMoveMethod = FILE_CURRENT;
            break;
        case SEEK_END:
            dwMoveMethod = FILE_END;
            break;
        default:
            return -1;
    }

    if (SetFilePointer(stream->fd, offset, NULL, dwMoveMethod) == INVALID_SET_FILE_POINTER) {
        stream->error = 1;
        return SO_EOF;
    }

    return 0;
}

long so_ftell(SO_FILE *stream) {
    DWORD result = SetFilePointer(stream->fd, 0, NULL, FILE_CURRENT);

    if (result == INVALID_SET_FILE_POINTER) {
        stream->error = 1;
        return SO_EOF;
    }

    return (long)result;
}

HANDLE so_fileno(SO_FILE *stream) {
    if (!stream) {
        return INVALID_HANDLE_VALUE;
    }
    return stream->fd;
}

int so_feof(SO_FILE *stream) {
    if (!stream) {
        return 0;
    }
    return stream->eof;
}

int so_ferror(SO_FILE *stream) {
    if (!stream) {
        return 0;
    }
    return stream->error;
}

SO_FILE *so_popen(const char *command, const char *type) {
    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES saAttr;
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    BOOL bSuccess = FALSE;

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hRead, &hWrite, &saAttr, 0)) {
        return NULL;
    }

    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    
    if (strcmp(type, "r") == 0) {
        siStartInfo.hStdOutput = hWrite;
        siStartInfo.hStdError = hWrite;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
    } else if (strcmp(type, "w") == 0) {
        siStartInfo.hStdInput = hRead;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
    } else {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return NULL;
    }

    bSuccess = CreateProcess(NULL, 
        command,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &siStartInfo,
        &piProcInfo);

    if (!bSuccess) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return NULL;
    } else {
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
    }

    SO_FILE *soFile = (SO_FILE *)malloc(sizeof(SO_FILE));
    if (!soFile) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return NULL;
    }

    if (strcmp(type, "r") == 0) {
        CloseHandle(hWrite);
        soFile->fd = hRead;
    } else {
        CloseHandle(hRead);
        soFile->fd = hWrite;
    }

    soFile->buf_pos = 0;
    soFile->buf_end = 0;

    return soFile;
}

int so_pclose(SO_FILE *stream) {
    if (!stream) {
        return -1;
    }

    CloseHandle(stream->fd);
    free(stream);
    return 0;
}

