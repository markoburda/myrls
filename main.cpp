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
#include <dirent.h>
#include <map>
std::map<const char*, struct stat> directories;
struct file_or_dir{
    bool name_or_about=true;
    std::string type;
    std::string user_name;
    std::string access;
    std::string bytes;
    std::string last_modified;
    const char *file_name;
    const char *full_file_name;
};

static const char *filterdir;

std::string get_mod(int desc){
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

static int is_dir(const struct dirent *dir)
{
    char buf[NAME_MAX];
    struct stat st{};
    if (directories.find(dir->d_name) != directories.end()){
        st = directories.at(dir->d_name);
    }
    else if (filterdir) {
        snprintf(buf, sizeof(buf), "%s/%s", filterdir, dir->d_name);
        stat(buf, &st);
        directories[dir->d_name] = st;
    } else {
        stat(dir->d_name, &st);
    }

    return st.st_mode & S_IFDIR;
}
static int is_file(const struct dirent *dir)
{
    return 1;
}
int get_dirs(const char* path, struct dirent ***filelist){
    filterdir = path;
    struct stat st{};
    int ndirs = scandir(path, filelist, is_dir, alphasort);
    filterdir = NULL;

    return ndirs;
}
int get_all_files(const char* path, struct dirent ***filelist){
    filterdir = path;
    struct stat st{};
    int ndirs = scandir(path, filelist, is_file, alphasort);
    filterdir = nullptr;
    return ndirs;
}

void about_file(char* dir_name, std::vector<file_or_dir> &one_by_one, bool indir=false){
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
    cur.file_name = name.c_str();
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
            //TODO change "unknown?" to "?" ?
            cur.type = "unknown?";
            break;
    }
    if (!indir && cur.type == "/"){
        cur.name_or_about = false;
        one_by_one.emplace_back(cur);
        struct dirent **dirlist;
        struct dirent **filelist;

        int n = get_all_files(cur.full_file_name, &filelist);
        int k = get_dirs(cur.full_file_name, &dirlist);
        //FOR EACH IN FILELIST RUN about_file WITH "indir=true"
        //FOR EACH IN DIRLIST RUN about_file
    }
    else{
        struct tm * timeinfo = localtime(&sb.st_ctime);
        std::cout << asctime(timeinfo) << " " << sb.st_size << " " << sb.st_mode << '\n';
        int u = (sb.st_mode & S_IRWXU) >> 6;
        int g = (sb.st_mode & S_IRWXG) >> 3;
        int o = sb.st_mode & S_IRWXO;
        cur.access = get_mod(u) + get_mod(g) + get_mod(o);
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
        cur.last_modified = year + "-" + std::to_string(timeinfo->tm_mon) + "-" + std::to_string(timeinfo->tm_mday) + " " + time;
        one_by_one.emplace_back(cur);
    }

}
int main(int argc, char *argv[]) {
    //TODO declare struct dirent** to store list of filenames when calling get_dirs();
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pathname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    struct stat sb;
    if (stat(argv[1], &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    directories[argv[1]] = sb;
    std::vector<file_or_dir> one_by_one;
    about_file(argv[1], one_by_one);
    size_t user_name=0, bytes=0, last_modified=0;
    for(const auto& file:one_by_one){
        if (file.name_or_about){
            user_name = std::max(user_name,file.user_name.size());
            bytes = std::max(bytes, file.bytes.size());
            last_modified = std::max(last_modified, file.last_modified.size());
        }
    }
    for(const auto& file:one_by_one){
        if (!file.name_or_about){
            std::cout << '\n' << file.full_file_name << ":\n";
        }
        else{
            std::cout << file.access << " " << std::string(user_name - file.user_name.size(), ' ') << file.user_name << " " << std::string(bytes - file.bytes.size(), ' ') << file.bytes << "  " <<std::string(last_modified - file.last_modified.size(), ' ') << file.last_modified << " " + file.type << file.file_name << '\n';

            //NOT NEEDED PART HERE, THIS IS JUST COUT
            //struct dirent **filelist;

            //file.full_file_name??
            //changed file_name type to const char*
            //int n = get_dirs(file.file_name, &filelist);

            //if (n < 0) {
            //    perror("scandir error");
            //} else {
            //    for (int i = 0; i < n; i++) {
            //        printf("%s\n", filelist[i]->d_name);
            //        free(filelist[i]);
            //    }
            //    free(filelist);
            //}
        }

    }
    return 0;
}