#include <sys/types.h>
#include <sys/stat.h>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <pwd.h>
#include <vector>
#include <sstream>
#include <cstring>
#include <filesystem>

struct file_or_dir{
    bool name_or_about= true;
    std::string type;
    std::string user_name;
    std::string access;
    std::string bytes;
    std::string last_modification_data_and_time;
    std::string file_name;
    std::string full_file_name;
};
std::string get_rwx(int desc){
    std::string res;
    if (desc & 4){
        res += "r";
    }
    else{
        res += "-";
    }
    if (desc & 2){
        res += "w";
    }
    else{
        res += "-";
    }
    if(desc & 1){
        res += "x";
    }
    else{
        res += "-";
    }
    return res;
}
std::vector<std::string> get_directories(const std::string& s)
{
    std::vector<std::string> r;
    for(auto& p : std::filesystem::recursive_directory_iterator(s))
        if (p.is_directory())
            r.push_back(p.path().string());
    return r;
}


void about_file(char* dir_name, std::vector<file_or_dir> &one_by_one, bool indir= false){

    file_or_dir cur;
    std::string name;
    for (int i = 0; i < strlen(dir_name); i++){
        if (dir_name[i] == '/'){
            name = "";
        }
        else{
            name += dir_name[i];
        }
    }
    cur.file_name = name;
    cur.full_file_name = dir_name;
    struct stat sb{};
    if (stat(dir_name, &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    switch (sb.st_mode & S_IFMT) {
        case S_IFDIR:
            cur.type = "/";
            break;
        case S_IFIFO:
            cur.type = "|";
            break;
        case S_IFLNK:
            cur.type = "@";
            break;
        case S_IFREG:
            break;
        case S_IFSOCK:
            cur.type = "=";
            break;
        default:
            cur.type = "unknown?";
            break;
    }
    if (!indir && cur.type == "/"){
        cur.name_or_about = false;
        one_by_one.emplace_back(cur);
        // NEEDED TO GET ALL FILES AND DIRECTORIES, SORT THEM AND RUN WITH "indir=true"
        // AFTER GET ALL SUBDIRECTORIES AND SORT THEM AND RUN WITH "indir=false"
    }
    else{
        struct tm * timeinfo = localtime(&sb.st_ctime);
        std::cout << asctime(timeinfo) << " " << sb.st_size << " " << sb.st_mode << '\n';
        int u = (sb.st_mode & S_IRWXU) >> 6;
        int g = (sb.st_mode & S_IRWXG) >> 3;
        int o = sb.st_mode & S_IRWXO;
        cur.access = get_rwx(u) + get_rwx(g) + get_rwx(o);
        cur.bytes = std::to_string(sb.st_size);
        cur.user_name = getpwuid(sb.st_uid)->pw_name;
        if (cur.type.empty() & (cur.access.find('x') != std::string::npos)){
            cur.type = "*";
        }
        std::string time;
        if (timeinfo->tm_hour < 10){
            time += "0";
        }
        time += std::to_string(timeinfo->tm_hour);
        time += ":";
        if (timeinfo->tm_min < 10){
            time += "0";
        }
        time += std::to_string(timeinfo->tm_min);
        time += ":";
        if (timeinfo->tm_sec < 10){
            time += "0";
        }
        time += std::to_string(timeinfo->tm_sec);
        std::vector<std::string> result;
        std::istringstream iss(asctime(timeinfo));
        for(std::string s; iss >> s; )
            result.push_back(s);
        std::string year = result[result.size() - 1];
        cur.last_modification_data_and_time = year + "-" + std::to_string(timeinfo->tm_mon) + "-" + std::to_string(timeinfo->tm_mday) + " " + time;
        one_by_one.emplace_back(cur);
    }

}
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pathname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    std::vector<file_or_dir> one_by_one;
    about_file(argv[1], one_by_one);
    // RUN FOR LOOP TO CKECK MAX SIZES OF EACH PART IN ABOUT FILE THAT NEEDED AND STANDARTIZE IT
    for(const auto& file:one_by_one){
        if (!file.name_or_about){
            std::cout << file.full_file_name << ":\n";
        }
        else{
            // STANDARTIZED OUTPUT FOR ALL, ALL PARTS THAT NEEDED ARE IN file
        }
    }
    return 0;
}