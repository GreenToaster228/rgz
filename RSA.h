#ifndef RSA_H
#define RSA_H

#include <string>

// mode: '0' - дешифрование, '1' - шифрование, '2' - генерация ключей
void process_rsa(char action, const std::string& key, const std::string& input, const std::string& output);

#endif
