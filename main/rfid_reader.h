#pragma once
#ifndef RFID_READER_H
#define RFID_READER_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

// Inicializa o leitor RFID (modo Wiegand)
esp_err_t rfid_reader_init(void);

// Lê o UID do cartão (se disponível)
esp_err_t rfid_read_uid(char *uid_str, size_t uid_size);

// Verifica se o UID lido é de um usuário autorizado
bool rfid_is_user_authorized(const char *uid);

#endif // RFID_READER_H
