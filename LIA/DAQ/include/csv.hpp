#pragma once
#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <conio.h>
#include <filesystem>

class csvWriter
{
private:
	std::ofstream _file;
public:
	csvWriter()	{}
	csvWriter(const char* filename)
	{
		this->open(filename, this->_file);
	}
	bool open(const char* filename)
	{
		return this->open(filename, this->_file);
	}
	template <typename ... Args>
	void printf(const std::string& fmt, Args ... args)
	{
		size_t len = std::snprintf(nullptr, 0, fmt.c_str(), args ...);
		std::vector<char> buf(len + 1);
		std::snprintf(&buf[0], len + 1, fmt.c_str(), args ...);
		this->write(std::string(&buf[0], &buf[0] + len));
	}
	static bool open(const char* filename, std::ofstream& file)
	{
		//std::ifstream ifs(filename);
		//if (ifs.is_open())
		//{
		//	std::cerr << "--- Warnning ------------------------------------------\n";
		//	std::cerr << "There is '" << filename << "' in this PC.\n";
		//	std::cerr << "Push 'ESC' key to finish. If push any key, overwrite the file.\n";
		//	char key = _getch();
		//	if (key == 0x1B)
		//	{
		//		std::cerr << "It was canceled to overwire '" << filename << "'.\n";
		//		std::cerr << "-------------------------------------------------------\n";
		//		return false;
		//	}
		//	else
		//	{
		//		std::cerr << "'" << filename << "' was overwire.\n";
		//		std::cerr << "-------------------------------------------------------\n";
		////	}
		//}
		//ifs.close();

		std::filesystem::create_directories(std::filesystem::path(filename).parent_path());

		if (!file.is_open())
		{
			while (true)
			{
				file.open(filename);
				if (!file.fail()) return true;
				std::cerr << "--- Error ---------------------------------------------\n";
				std::cerr << " '" << filename << "' couldn't be opened.\n";
				std::cerr << " Push 'ESC' key to finish, or push another key to retry.\n";
				std::cerr << "-------------------------------------------------------\n";
				if (_getch() == 0x1B) return false;
			}
		}
		return false;
	}
	static void close(std::ofstream& file)
	{
		if (file.is_open())
		{
			file.close();
		}
	}
	void close()
	{
		this->close(this->_file);
	}
	template < typename T >
	auto write(T arg)
		//ŒÄ‚Ño‚·‚²‚Æ‚É•Û‘¶B
	{
		this->_file << arg;
	}
	//template < typename... Args >
	//auto write(Args const& ... args)
	//{
	//	for (auto const& e : { args... })
	//	{
	//		this->file << e << "\n";
	//	}
	//	this->file << "\n";
	//}
	static bool write(const char* filename, const int length, const double dt, const double* arr, const char comment[])
	{
		std::ofstream file;
		if (!open(filename, file)) return false;
		file << comment << "\n";
		for (int i=0;i<length;i++)
		{
			file << dt*i << "," << arr[i] << "\n";
		}
		close(file);
		return true;
	}
	static bool write(const char* filename, const int length, const int colm, const double dt, const double* arr, const char comment[])
	{
		std::ofstream file;
		if (!open(filename, file)) return false;
		file << comment << "\n";
		for (int i = 0; i < length; i++)
		{
			file << dt * i;
			for (int j = 0; j < colm; j++)
			{
				file << "," << arr[i * colm + j];
			}
			file << "\n";
		}
		close(file);
		return true;
	}
	static bool write(const char* filename, const std::vector<double>& arr, const char comment[])
	{
		std::ofstream file;
		if (!open(filename, file)) return false;
		file << comment << "\n";
		for (auto e : arr)
		{
			file << e << "\n";
		}
		close(file);
		return true;
	}
	static bool write(const char* filename, const std::vector<double>& arr)
	{
		std::ofstream file;
		if (!open(filename, file)) return false;
		for (auto e : arr)
		{
			file << e << "\n";
		}
		close(file);
		return true;
	}
	static bool write(const char* filename, const std::vector<double>& arr1, const std::vector<double>& arr2, const char comment[])
	{
		std::ofstream file;
		if (!open(filename, file)) return false;
		file << comment << "\n";
		for (size_t i = 0; i < arr1.size(); i++)
		{
			file << arr1[i] << ',' << arr2[i] << '\n';
		}
		close(file);
		return true;
	}
	static bool write(const char* filename, const std::vector<double>& arr1, const std::vector<double>& arr2)
	{
		std::ofstream file;
		if (!open(filename, file)) return false;
		for (size_t i = 0; i < arr1.size(); i++)
		{
			file << arr1[i] << ',' << arr2[i] << '\n';
		}
		close(file);
		return true;
	}
	static bool write(const char* filename, const std::vector<double>& arr1, const std::vector<double>& arr2, const std::vector<double>& arr3, const char comment[])
	{
		std::ofstream file;
		if (!open(filename, file)) return false;
		file << comment << "\n";
		for (size_t i = 0; i < arr1.size(); i++)
		{
			file << arr1[i] << ',' << arr2[i] << ',' << arr3[i] << '\n';
		}
		close(file);
		return true;
	}
	static bool write(const char* filename, const std::vector<std::vector<double>>& arr, const char comment[])
	{
		std::ofstream file;
		if (!open(filename, file)) return false;
		file << comment << "\n";
		for (const auto& e : arr)
		{
			for (size_t i = 0; i < e.size() - 1; i++)
			{
				file << e[i] << ',';
			}
			file << e[e.size() - 1] << '\n';
		}
		close(file);
		return true;
	}
	static bool write(const char* filename, const std::vector<std::vector<double>>& arr)
	{
		std::ofstream file;
		if (!open(filename, file)) return false;
		for (const auto& e : arr)
		{
			for (size_t i = 0; i < e.size() - 1;i++)
			{
				file << e[i] << ',';
			}
			file << e[e.size() - 1] << '\n';
		}
		close(file);
		return true;
	}
};

class csvReader
{
private:
	static bool open(const char* filename, std::ifstream& file)
	{
		if (!file.is_open())
		{
			while (true)
			{
				file.open(filename);
				if (!file.fail()) return true;
				std::cerr << "--- Error ---------------------------------------------\n";
				std::cerr << " '" << filename << "' couldn't be opened.\n";
				std::cerr << " Push 'ESC' key to finish, or push another key to retry.\n";
				std::cerr << "-------------------------------------------------------\n";
				if (_getch() == 0x1B) return false;
			}
		}
		return false;
	}
	static void close(std::ifstream& file)
	{
		if (file.is_open())
		{
			file.close();
		}
	}
	static std::vector<std::string> split(std::string& input, char delimiter)
	{
		std::istringstream stream(input);
		std::string field;
		std::vector<std::string> result;
		while (std::getline(stream, field, delimiter))
		{
			result.push_back(field);
		}
		return result;
	}
public:
	static bool read(const char* filename, std::vector<std::string>& arr, static bool flagContinue = true)
	{
		std::ifstream file;
		size_t count = 0;
		if (flagContinue)
		{
			if (!open(filename, file)) return false;
		}
		else
		{
			file.open(filename);
			if (file.fail()) return false;
		}
		std::string line;
		while (std::getline(file, line))
		{
			if (line[0] == '#')continue;
			arr.push_back(line);
			count++;
		}
		close(file);
		return true;
	}
	static bool read(const char* filename, std::vector<double>& arr, static bool flagContinue = true)
	{
		std::ifstream file;
		size_t count = 0;
		if (flagContinue)
		{
			if (!open(filename, file)) return false;
		}
		else
		{
			file.open(filename);
			if (file.fail()) return false;
		}
		std::string line;
		while (std::getline(file, line))
		{
			if (line[0] == '#')continue;
			arr.push_back(stod(line));
			count++;
		}
		close(file);
		return true;
	}
	static bool read(const char* filename, std::vector<double>& arr1, std::vector<double>& arr2)
	{
		std::ifstream file;
		size_t count = 0;
		if (!open(filename, file)) return false;
		std::string line;
		while (std::getline(file, line))
		{
			if (line[0] == '#')continue;
			std::vector<std::string> strvec = split(line, ',');
			arr1.push_back(stod(strvec.at(0)));
			arr2.push_back(stod(strvec.at(1)));
			count++;
		}
		close(file);
		return true;
	}
	static bool read(const char* filename, std::vector<double>& arr1, std::vector<double>& arr2, std::vector<double>& arr3)
	{
		std::ifstream file;
		size_t count = 0;
		if (!open(filename, file)) return false;
		std::string line;
		while (std::getline(file, line))
		{
			if (line[0] == '#')continue;
			std::vector<std::string> strvec = split(line, ',');
			arr1.push_back(stod(strvec.at(0)));
			arr2.push_back(stod(strvec.at(1)));
			arr3.push_back(stod(strvec.at(2)));
			count++;
		}
		close(file);
		return true;
	}
};
