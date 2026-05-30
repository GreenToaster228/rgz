#include "rc6.h"
#include <fstream>
#include <cstdint>
#include <vector>
#include <cstring>
#include <iomanip>
#include <random>
#include <iostream>
#include <ctime>
#include <filesystem>

namespace
{
	//циклические сдвиги
	inline uint32_t rotl(uint32_t x, uint32_t y)
	{
		return (x << (y & 31)) | (x >> (32 - (y & 31)));
	}

	inline uint32_t rotr(uint32_t x, uint32_t y)
	{
		return (x >> (y & 31)) | (x << (32 - (y & 31)));
	}

	//развёртка ключа
	void rc6_key_setup(const std::string& key_str, uint32_t* S)
	{
		uint8_t K[16] = {0}; //инициализация 16-байтового ключа
		std::memcpy(K, key_str.data(), std::min(key_str.size(), size_t(16)));

		
		uint32_t L[4]; //Разбитие ключа на 4 части
		for (int i = 0; i < 4; ++i)
		{
			L[i] = K[i*4] | (K[i*4+1] << 8) | (K[i*4+2] << 16) | (K[i*4+3] << 24);
		}

		S[0] = 0xB7E15163; //P32
		for (int i = 1; i < 44; i++)
		{
			S[i] = S[i-1] + 0x9E3779B9; //Q32
		}


		//перемешивание
		uint32_t A = 0, B = 0;
		int i = 0, j = 0;
		for (int k = 0; k < 132; ++k) //132 = (2 * 20 + 4) * 3
		{
			A = S[i] = rotl(S[i] + A + B, 3);
			B = L[j] = rotl(L[j] + A + B, A + B);
			i = (i + 1) % 44;
			j = (j + 1) % 4;
		}
	}



	void rc6_encrypt(uint32_t* block, const uint32_t* S)
	{
		uint32_t A = block[0], B = block[1], C = block[2], D = block[3];
		//отбеливание
		B += S[0];
		D += S[1];

		for (int i = 1; i <= 20; i++)
		{
			uint32_t t = rotl(B * (2 * B + 1), 5);
			uint32_t u = rotl(D * (2 * D + 1), 5);
			A = rotl(A ^ t, u) + S[2 * i];
			C = rotl(C ^ u, t) + S[2 * i + 1];
			
			uint32_t temp = A; A = B; B = C; C = D; D = temp;
		}
		block[0] = A + S[42]; block[1] = B; block[2] = C + S[43]; block[3] = D;
	}



	void rc6_decrypt(uint32_t* block, const uint32_t* S)
	{
		uint32_t A = block[0], B = block[1], C = block[2], D = block[3];
		A -= S[42];
		C -= S[43];

		for (int i = 20; i >= 1; i--)
		{
			uint32_t temp = D; D = C; C = B; B = A; A = temp;

			uint32_t t = rotl(B * (2 * B + 1), 5);
			uint32_t u = rotl(D * (2 * D + 1), 5);
			C = rotr(C - S[2 * i + 1], t) ^ u;
			A = rotr(A - S[2 * i], u) ^ t;
		}
		D -= S[1];
		B -= S[0];
		block[0] = A; block[1] = B; block[2] = C; block[3] = D;
	}
}

void process_rc6(char action, const std::string& key, const std::string& input, const std::string& output)
{
	//генерация случайного пароля
	if (action == 2)
	{
		std::srand(static_cast<unsigned int>(std::time(nullptr)));
		std::string password(16, '\0');

		for (int i = 0; i < 16; ++i)
		{
			password[i] = (std::rand() % (126-48+1)) + 48;
		}
		std::cout << password << '\n';
		return;
	}


	uint32_t S[44];
	rc6_key_setup(key, S);

	std::ifstream in_file(input, std::ios::binary | std::ios::ate);
	
	std::streamsize file_size = in_file.tellg();
	in_file.seekg(0, std::ios::beg);

	std::vector<char> input_buffer(file_size);
	in_file.read(input_buffer.data(), file_size);
	in_file.close();

	std::vector<char> output_buffer;
	uint32_t block[4] = {0};
	std::streamsize bytes_processed = 0;

	if (action == 1)
	{
		output_buffer.reserve(((file_size + 15) / 16) * 16); //выделение памяти
		while (bytes_processed < file_size)
		{
			std::streamsize chunk_size = std::min(file_size - bytes_processed, std::streamsize(16));
			
			std::memcpy(block, input_buffer.data() + bytes_processed, chunk_size);
			
			if (chunk_size < 16) //добивание неполноценного блока нулями
			{
				std::memset(reinterpret_cast<char*>(block) + chunk_size, 0, 16 - chunk_size);
			}

			rc6_encrypt(block, S);

			const char* cipher_bytes = reinterpret_cast<const char*>(block);
			output_buffer.insert(output_buffer.end(), cipher_bytes, cipher_bytes + 16);

			bytes_processed += chunk_size;
		}
	} 
	else if (action == 0)
	{
		output_buffer.reserve(file_size);
		std::vector<char> temp_decrypted;
		temp_decrypted.reserve(file_size);

		while (bytes_processed < file_size)
		{
			std::memcpy(block, input_buffer.data() + bytes_processed, 16);

			rc6_decrypt(block, S);

			const char* plain_bytes = reinterpret_cast<const char*>(block);
			temp_decrypted.insert(temp_decrypted.end(), plain_bytes, plain_bytes + 16);

			bytes_processed += 16;
		}

		std::streamsize real_size = temp_decrypted.size();
		while (real_size > 0 && temp_decrypted[real_size - 1] == '\0') {
			real_size--;
		}

		output_buffer.insert(output_buffer.end(), temp_decrypted.begin(), temp_decrypted.begin() + real_size);
	}

	//запись
	try
	{
		std::string new_output = (input == output) ? (input + ".tmp") : output;
		std::ofstream out_file(new_output, std::ios::binary);
		out_file.write(output_buffer.data(), output_buffer.size());
		out_file.close();
		if (input == output) std::filesystem::rename(new_output, input);
	}
	catch(...)
	{
		std::cout << "Ошибка: Не удалось перезаписать файл";
		return;
	}


}

