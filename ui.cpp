#include "rc6.h"
#include "RSA.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

//help-сообщение
void print_usage()
{
    std::cout << "Использование:\n"
    << "  rgz [шифр] [действие] [ключ] [if] [of]\n"
    << "  rgz [шифр] keygen\n\n"
    << "Параметры:\n"
    << "  шифр : RSA, RC6, Enigma\n"
    << "  действие : decrypt, encrypt, keygen\n"
    << "  ключ : ключ шифрования\n"
    << "  if : путь к входному файлу\n"
    << "  of : путь к выходному файлу. если не указан - перезаписывается входной\n"
    << "Пример использования: rgz RSA encrypt 123456789,17 ./file \n";

}

std::string to_lower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c)
    {
        return std::tolower(c);
    });
    return str;
}

//Функции шифрования
void rsa(char action, const std::string& key, const fs::path& input, const fs::path& output)
{
    process_rsa(action, key, input.string(), output.string());
}

void rc6(char action, const std::string& key, const fs::path& input, const fs::path& output)
{
    process_rc6(action, key, input.string(), output.string());
}

void enigma(char action, const std::string& key, const fs::path& input, const fs::path& output)
{
    // TODO: реализовать Enigma
    std::cout << "Enigma пока не реализован" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 3 || ((argc < 5 && to_lower(argv[2]) != "keygen") || (to_lower(argv[2]) == "keygen" && argc != 3)) || argc > 6)
    {
        std::cout << "Неверное число аргументов\n";
        print_usage();
        return 1;
    }

    std::string cipher_str = to_lower(argv[1]);
    char cipher;
    if (cipher_str == "rsa") cipher = 0;
    else if (cipher_str == "rc6") cipher = 1;
    else if (cipher_str == "enigma") cipher = 2;
    else
    {
        std::cout << "Неизвестный шифр\n";
        print_usage();
        return 1;
    }

    std::string action_str = to_lower(argv[2]);
    char action;
    if (action_str == "decrypt") action = 0;
    else if (action_str == "encrypt") action = 1;
    else if (action_str == "keygen") action = 2;
    else
    {
        std::cout << "Неизвестное действие\n";
        print_usage();
        return 1;
    }

    std::string key = "";
    fs::path input_path;
    fs::path output_path;

    if (action != 2)
    {
        key = argv[3];
        input_path = argv[4];

        try
        {
            if (!fs::exists(input_path))
            {
                std::cerr << "Ошибка: Входной файл '" << input_path.string() << "' не существует.\n";
                return 1;
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "Ошибка: Невозможно открыть файл '" << input_path.string() << "': " << e.what() << '\n';
            return 1;
        }

        try
        {
            if (argc == 6)
            {
                output_path = argv[5];
                if (fs::exists(output_path))
                {
                    std::cout << "Файл '" << output_path.string() << "' существует. Перезаписать? [y/N]: ";
                    std::string answer;
                    std::getline(std::cin, answer);
                    answer = to_lower(answer);

                    if (answer != "y" && answer != "yes")
                    {
                        std::cout << "Отмена шифрования\n";
                        return 0;
                    }
                }
            }
            else
            {
                output_path = input_path;
            }

            std::ofstream test_file;
            test_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
            test_file.open(output_path, std::ios::binary | std::ios::app);
            test_file.close();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Ошибка: Не удалось создать или открыть файл '" << output_path << "': " << e.what() << '\n';
            return 1;
        }
    }

    switch (cipher)
    {
        case 0:
            rsa(action, key, input_path, output_path);
            break;
        case 1:
            rc6(action, key, input_path, output_path);
            break;
        case 2:
            enigma(action, key, input_path, output_path);
            break;
    }

    return 0;
}
