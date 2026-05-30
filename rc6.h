#ifndef RC6_H
#define RC6_H

#include <string>

// mode: '0' - дешифрование, '1' - шифрование, '2' - генерация/сохранение раундовых ключей
void process_rc6(char mode, const std::string& key, const std::string& input, const std::string& output);

#endif // RC6_H

