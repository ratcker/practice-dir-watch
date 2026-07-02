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
                meta.modification_time = std::chrono::duration_cast<std::chrono::seconds>(epoch).count();
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

std::unordered_map<std::string, FileMeta> load_snapshot(const std::string& db_path) {
    //функция для выгрузки данных из сохраненного ранее снапшота
    std::unordered_map<std::string, FileMeta> past_snapshot;
    std::ifstream in(db_path);
    if(!in.is_open()) {
        std::cerr << "Error: no such snaphot file on " << db_path << std::endl;
        return past_snapshot; //в случае если открыть файл снапшота не удалось выводим ошибку
    }
    std::string line;
    while (std::getline(in,line)) {//пройдемся по всем строкам файла
        if (line.empty()) {
            continue;//пропустим пустые строки
        }
        std::stringstream ss(line);//работать будем со строковым потоком
        std::string path_part, size_part, time_part;
        if (std::getline(ss, path_part, '|') && std::getline(ss, size_part, '|') && std::getline(ss, time_part, '|')) {//разобьем строку с метаданными на отдельные части метаданных файла
            FileMeta meta;
            meta.path = path_part;
            meta.size = std::stoll(size_part);
            meta.modification_time = std::stoll(time_part);
            past_snapshot[meta.path] = meta;//заполним хеш-таблицу набором метаданных файла по пути в качестве ключа
        }
    }
    in.close();
    std::cout << "succesfully Loaded files from snaphot. count: " << past_snapshot.size() << std::endl; //выве
    return past_snapshot; // вернем набор метаданных файлов
}

void compare_snapshots(const std::vector<FileMeta>& current, std::unordered_map<std::string,FileMeta>& past) {
    //Функция для сравнения текущего состояние директории со считанным состоянием из снапшота 
    std::cout << "Compare" << std::endl;
    int new_count = 0, modified_count = 0,deleted_count = 0;
    for (const auto& current_file : current) {//пройдемся по всем файлам
        auto past_file = past.find(current_file.path);
        if (past_file == past.end()) {//если файнд дошел до конца таблицы, значит ранее этого файла не было и => он новый 
            std::cout << "new file on " << current_file.path << std::endl;
            new_count++;
        }
        else {//иначе файл раньше был. если размер или время изменения разные, значит файл изменен
            if (current_file.size != past_file->second.size || current_file.modification_time != past_file->second.modification_time) {
                std::cout << "modified file on " << current_file.path << std::endl;
                modified_count++;
            }
            past.erase(past_file);//удалим файл из хеш-таблицы, чтобы в ней после последней итерации фора остались те файлы, которые в новом состоянии найти не получится
        }
        
    }
    for (const auto& pair : past) {//сейчас в хеш-таблице остались файлы, которых нет в текущем состоянии и => эти файлы были удалены
        std::cout << "deleted file on " << pair.first << std::endl;
        deleted_count++;
    }
    std::cout << "Summary: New: " << new_count << ", modified: " << modified_count << ", deleted: " << deleted_count << std::endl;//выведем сводку по сравнению директории со снапшотом
}

int main()
{
    /*std::cout << "тест снапшота" << std::endl;
    std::string target = "/app";
    std::string database_file = "/app/snapshot.db";
    std::cout << "scanning" << std::endl;
    std::vector<FileMeta> current_files = scan_directory(target);
    if (current_files.empty()) {
        std::cout << "directory is empty" << std::endl;
        return 1;
    }
    std::cout << "scanned" << std::endl;
    save_snapshot(database_file, current_files);
    std::cout << "Ready" << std::endl;
    return 0;*/
    std::string target_dir = "/app";
    std::string database_file = "/app/snapshot.db";

    std::cout << "тест сравнения" << std::endl;
    std::unordered_map<std::string, FileMeta> past_snapshot = load_snapshot(database_file);

    std::cout << "\nScanning directory for current state..." << std::endl;
    std::vector<FileMeta> current_state = scan_directory(target_dir);

    compare_snapshots(current_state, past_snapshot);

    std::cout << "финишед" << std::endl;
    return 0;
}