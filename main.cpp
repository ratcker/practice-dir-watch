#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <getopt.h>

namespace fs = std::filesystem;

struct FileMeta {
    //Данные о файле - путь, размер, время изменения
    std::string path;
    long long size;
    long long modification_time;
};

std::vector<FileMeta> scan_directory(const std::string& target) {
    //функция для скана директории, возвращает вектор с данными о содержимом
    std::vector<FileMeta> checked;
    try {
        if (!fs::exists(target) || !fs::is_directory(target)) {
            std::cerr << "No such path " << target << std::endl;
            return checked; // В случае несуществования директории по такому пути возвращаем пустой вектор 
        }
        for (const auto& entry : fs::recursive_directory_iterator(target)) {
            if (fs::is_regular_file(entry.path())) {//в случае нахождения файла при рекурсивном сканипровании директории
                FileMeta meta;//записываем метаданные этого файла
                meta.path = entry.path().string();
                meta.size = fs::file_size(entry.path());
                auto last_mod_time = fs::last_write_time(entry.path());
                auto epoch = last_mod_time.time_since_epoch();
                meta.modification_time = std::chrono::duration_cast<chrono::seconds>(epoch).count();
                checked.push_back(meta);//и заносим их в вектор всех метаданных файлов
            }
        }
    } catch (const fs::filesystem_error& error) {
        std::cerr << "error: " << error.what() << std::endl;//в случае ошибки разворачиваем ошибку
    }
    return checked;
}

void save_snapshot(const std::string& db_path, const std::vector<FileMeta>& files) {
    //функция для сохранения снапшота в файловой системе. на вход получаем путь для сохранения снапшота и вектор с метаданными файлов
    std::ofstream out(db_path);
    if (!out.is_open()) {//в случае если не получится открыть файл
        std::cerr << "Error: couldn't open create snaphsot at " << db_path << std::endl; 
        return;//вывести шибку и выйти из функции
    }
    for (const auto& file : files) {
        out << file.path << "|" << file.size << "|" << file.modification_time << "\n";//метаданные о файле запишем в формате строки: "путь|размер|время изменения"
    }
    out.close();
    std::cout << "snapshot saved. Recorded files: " << files.size() << std::endl;
}

int main()
{
    std::cout << "Hello world" << std::endl;
    return 0;
}