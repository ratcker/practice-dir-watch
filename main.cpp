#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <sstream>
#include <chrono>

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
                if (entry.path().filename().string() == ".snapshot" || entry.path().filename().string() == ".dir-watch-changes") {//в случае обнаружения системного файла dir-watch игнорируем его
                    continue;
                }
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

void save_snapshot(const std::string& snapshot_path, const std::vector<FileMeta>& files) {
    //функция для сохранения снапшота в файловой системе. на вход получаем путь для сохранения снапшота и вектор с метаданными файлов
    std::ofstream out(snapshot_path);
    if (!out.is_open()) {//в случае если не получится открыть файл
        std::cerr << "Error: couldn't open create snaphsot at " << snapshot_path << std::endl; 
        return;//вывести шибку и выйти из функции
    }
    for (const auto& file : files) {
        out << file.path << "|" << file.size << "|" << file.modification_time << "\n";//метаданные о файле запишем в формате строки: "путь|размер|время изменения"
    }
    out.close();
    std::cout << "snapshot saved. Recorded files: " << files.size() << std::endl;
}

std::unordered_map<std::string, FileMeta> load_snapshot(const std::string& snapshot_path) {
    //функция для выгрузки данных из сохраненного ранее снапшота
    std::unordered_map<std::string, FileMeta> past_snapshot;
    std::ifstream in(snapshot_path);
    if(!in.is_open()) {
        std::cerr << "Error: no such snaphot file on " << snapshot_path << std::endl;
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
    std::string report_data = "Detailed changes log\n";//Запишем все найденные изменения в строку
    for (const auto& current_file : current) {//пройдемся по всем файлам
        auto past_file = past.find(current_file.path);
        if (past_file == past.end()) {//если файнд дошел до конца таблицы, значит ранее этого файла не было и => он новый 
            std::cout << "new file on " << current_file.path << std::endl;
            report_data += "new file on " + current_file.path + "\n";
            new_count++;
        }
        else {//иначе файл раньше был. если размер или время изменения разные, значит файл изменен
            if (current_file.size != past_file->second.size || current_file.modification_time != past_file->second.modification_time) {
                std::cout << "modified file on " << current_file.path << std::endl;
                report_data += "modified file on " + current_file.path + "\n";
                modified_count++;
            }
            past.erase(past_file);//удалим файл из хеш-таблицы, чтобы в ней после последней итерации фора остались те файлы, которые в новом состоянии найти не получится
        }
        
    }
    for (const auto& pair : past) {//сейчас в хеш-таблице остались файлы, которых нет в текущем состоянии и => эти файлы были удалены
        std::cout << "deleted file on " << pair.first << std::endl;
        report_data += "deleted file on " + pair.first + "\n";
        deleted_count++;
    }
    std::cout << "Summary: New: " << new_count << ", modified: " << modified_count << ", deleted: " << deleted_count << std::endl;//выведем сводку по сравнению директории со снапшотом
    std::string summary = "New: " + std::to_string(new_count) + ", modified: " + std::to_string(modified_count) + ", deleted: " + std::to_string(deleted_count) + "\n";
    report_data += "------\n"+summary+"------\n";//запишем в конец строки с изменениями summary
    std::ofstream report_out("/tmp/.dir-watch-changes");//запишем изменения в файл /tmp/.dir-watch-changes
    if (report_out.is_open()) {
        report_out << report_data;
        report_out.close();
    }
}

void export_report(const std::string& export_path) {
    //функция для экспорта отчета по изменениям в директории. сами изменения будут взяты из /tmp/.dir-watch-changes
    std::string source_file = "/tmp/.dir-watch-changes"; 
    if (!fs::exists(source_file)) {//если файла /tmp/.dir-watch-changes не сущетвует, значит снимков состотяния директории еще не создавалось в этой сессии
        std::cerr << "Error: Nothing to export. Run --compare to create changes log in this session";
        return; 
    }
    std::ifstream in(source_file);
    std::ofstream out(export_path);//Откроем файл для записи и чтения - в out перепишем отчет из in
    if (!in.is_open() || !out.is_open()) {
        std::cerr << "Error: couldn't open files for exporting report." << std::endl;
        return;
    }
    std::string line;
    while (std::getline(in, line)) {//построчно переписываем
        out << line << "\n";
    }
    in.close();
    out.close();
    std::cout << "Report succesfully exported to " << export_path << std::endl;
}

int main(int argc, char* argv[])
{
    static struct option long_options[] = {//Опишем возможные флаги через getopt
        {"snapshot", required_argument, 0, 's'},
        {"compare", required_argument, 0, 'c'},
        {"export", required_argument, 0, 'e'},
        {"help", no_argument, 0, 'h'},
        {0,0,0,0}
    };
    std::string target = "";
    char mode = 0;
    int opt;
    std::string export_file = "";
    while ((opt = getopt_long(argc, argv, "s:c:e:h", long_options, nullptr)) != -1) {
        switch (opt) {//пройдемся по полученным аргументам и настроим режим программы
            case 's':
                mode = 's';
                target = optarg;
                break;
            case 'c':
                mode = 'c';
                target = optarg;
                break;
            case 'e':
                mode ='e';
                export_file = optarg;
                break;
            case 'h':
                std::cout << "for using: dir-watch [options]\nOptions:\n"
                << " -s, --snapshot <directory>     Create snaphot of directory status\n"
                << " -c, --compare <directory>      Compare directory with the previous snapshot\n"
                << " -e, --export <filename>        Create file with this name in ./<filename> with results of compare\n"
                << " -h, --help                     Show this help message\n";//вывод --help или -h
                return 0;
            default:
                return 1;
        }
    }
    if (mode == 0) {
        std::cerr << "Error: No mode specified. Use --help for usage instructions.\n";
        return 1;//если аргументов не получено
    }
    std::string snapshot_file = target + "/.snapshot";//будем держать снапшот сканируемой директории в .snapshot
    if (mode == 's') {//выполнение программы в режиме создания снапшота
        std::cout << "Creating snapshot\nScanned directory: " << target << std::endl;
        std::vector<FileMeta> current_files = scan_directory(target);
        if (current_files.empty()) {
            std::cerr << "Error: snapshot wasn't created - directory empty or doesn't exist." << std::endl;
            return 1;
        }
        save_snapshot(snapshot_file, current_files);
        std::cout << "Snaphsot created succesfully: " << snapshot_file << std::endl;
    }
    else if (mode =='c') {//выполнение программы в режиме сравнения состояния директории со снапшотом
        std::cout << "Compare " << target << " with snapshot" << std::endl;
        std::unordered_map<std::string, FileMeta> past_snapshot = load_snapshot(snapshot_file);
        std::cout << "Scanning.." << std::endl;
        std::vector<FileMeta> current_state = scan_directory(target);
        compare_snapshots(current_state, past_snapshot);
        std::cout << "comparison successfully complete." << std::endl;
    }
    else if (mode == 'e') {//выполнение программы в режиме экспорта отчета по изменениям в директории 
        std::cout << "Exporting report of changes.." << std::endl;
        export_report(export_file);
    }
    return 0;
}