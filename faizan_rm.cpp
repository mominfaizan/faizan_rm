#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <optional>
#include <map>
#include <set>
#include <regex>
#include <filesystem>
#include <thread>
using json = nlohmann::json;

void HelpFunction();
void FileAlreadyExistCheck(std::string filename);
void CreateFolderIfNotExist();
void ListItem(std::optional<std::string> foldername, std::optional<int> counter);
void DeleteFile(std::string filename, std::optional<bool> isFolder);
void DeleteFolder(std::string foldername);
void RestoreFiles(std::optional<std::string> filename);
std::string homeDir = std::getenv("HOME");

std::string FileStorePath = homeDir + "/.local/share/Faizan_rm/files";
std::string FileInfoPath = homeDir + "/.local/share/Faizan_rm/info";
int CleanUpDays = 90;
struct TrashEntry
{
    long long deleted_at;
    std::string original_name;
    std::string stored_name;
    bool is_folder;
};

std::map<int, TrashEntry> TimeStampMap;

void ProcessArguments(int argc, char *argv[]);
bool isFolderCreated()
{
    if (strcmp(homeDir.c_str(), "") == 0)
    {
        std::cerr << "Could not get HOME directory\n";
        return 1;
    }

    int ret = system(("ls " + FileStorePath + " > /dev/null 2>&1").c_str());

    if (ret == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}
std::string convertTimeStampToString(long long timestamp)
{
    std::time_t t = timestamp / 1000;
    std::tm *tm_ptr = std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void CreateFolderIfNotExist()
{
    if (!isFolderCreated())
    {

        int status = system(("mkdir -p " + homeDir + "/.local/share/Faizan_rm").c_str());
        if (status != 0)
        {
            std::cout << "Error creating directory " << homeDir << "/.local/share/Faizan_rm\n";
        }
        status = system(("mkdir -p " + FileStorePath).c_str());
        if (status != 0)
        {
            std::cout << "Error creating directory" << FileStorePath << "\n";
        }
        status = system(("mkdir -p " + FileInfoPath).c_str());
        if (status != 0)
        {
            std::cout << "Error creating directory " << FileInfoPath << "\n";
        }
        else
        {
            status = system("touch Faizan_rm_cleanupdays.json");
            if (status != 0)
            {
                std::cout << "Error creating file Faizan_rm_cleanupdays.json\n";
            }
            json clean_json;
            clean_json["cleanupdays"] = 90; // by default
            std::string cleanup_json_file = FileInfoPath + "Faizan_rm_cleanupdays.json";
            std::ofstream cleanupFile(cleanup_json_file);
            if (cleanupFile.is_open())
            {
                cleanupFile << clean_json.dump(4);
                cleanupFile.close();
            }
            else
            {
                std::cerr << "Could not open meta file for writing: " << cleanup_json_file << "\n";
            }
        }
    }
}
void ListItem(std::optional<std::string> foldername = std::nullopt, std::optional<int> counter = std::nullopt)
{
    std::string command = "ls /home/faizanmomin/.local/share/Faizan_rm/info";
    if (foldername && foldername.has_value() && !foldername.value().empty())
    {
        command = "ls /home/faizanmomin/.local/share/Faizan_rm/info | grep " + foldername.value();
    }
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe)
    {
        std::cerr << "Failed to run command\n";
        return;
    }
    char *line = nullptr;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, pipe)) != -1)
    {
        std::string metafilename(line);
        metafilename.erase(metafilename.find_last_not_of(" \n\r\t") + 1);
        std::string metaFilePath = FileInfoPath + "/" + metafilename;
        std::ifstream metaFile(metaFilePath);
        if (metaFile.is_open())
        {
            json meta;
            try
            {
                metaFile >> meta;
            }
            catch (const nlohmann::json::parse_error &e)
            {
                std::cerr << "Skipping invalid JSON file: " << metaFilePath << "\n";
                continue;
            }

            long long deleted_at = meta.value("deleted_at", 0LL);
            std::string original_name = meta.value("original_name", "");
            std::string store_name = meta.value("stored_name", "");
            bool is_folder = meta.value("Is_folder", false);

            if (deleted_at == 0 || original_name.empty() || store_name.empty())
                continue;
            if (counter && counter.has_value())
            {
                std::cout << counter.value() << " " << convertTimeStampToString(deleted_at) << " " << original_name << "\n";
                TrashEntry entry = {deleted_at, original_name, store_name, is_folder};
                TimeStampMap[counter.value()] = entry;
                counter.value()++;
            }
            else
            {
                std::cout << convertTimeStampToString(deleted_at) << " " << original_name << "\n";
            }
        }
        else
        {
            std::cerr << "Could not open meta file for reading: " << metaFilePath << "\n";
        }
    }
    pclose(pipe);
}

void DeleteFile(std::string filename, std::optional<bool> isFolder = false)
{
    if (isFolder.has_value() && isFolder.value() && filename.back() == '/')
    {
        filename = filename.substr(0, filename.size() - 1);
    }
    std::filesystem::path originalPath;
    try
    {
        std::filesystem::path input = filename;
        if (input.is_absolute())
        {
            originalPath = std::filesystem::canonical(input);
        }
        else
        {
            originalPath = std::filesystem::canonical(std::filesystem::current_path() / input);
        }
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        originalPath = (std::filesystem::current_path() / filename).lexically_normal();
    }

    std::chrono::system_clock::time_point timestamp;
    long long milliseconds;
    std::string newfilename;
    auto generateUniqueID = [&filename, &newfilename, &timestamp, &milliseconds]()
    {
        timestamp = std::chrono::system_clock::now();
        milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count();
        newfilename = std::filesystem::path(filename).filename().string() + "_" + std::to_string(milliseconds);
    };
    generateUniqueID();
    newfilename = FileStorePath + "/" + newfilename;
    try
    {

        std::filesystem::rename(filename, newfilename);
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        std::cerr << "Filesystem error: " << e.what() << "\n";
        return;
    }
    json meta;
    std::string res = std::filesystem::current_path().string();
    meta["original_name"] = originalPath.string();
    meta["deleted_at"] = milliseconds;
    meta["stored_name"] = newfilename;
    bool is_folder = std::filesystem::is_directory(meta["stored_name"]);
    meta["Is_folder"] = is_folder;
    std::string jsonFileName = FileInfoPath + "/" + newfilename.substr(newfilename.find_last_of("/") + 1);
    std::string metaFilePath = jsonFileName + ".json";
    std::ofstream metaFile(metaFilePath);

    if (metaFile.is_open())
    {
        metaFile << meta.dump(4);
        metaFile.close();
    }
    else
    {
        std::cerr << "Could not open meta file for writing: " << metaFilePath << "\n";
    }
}
void DeleteFolder(std::string foldername, bool isforce = false)
{
    if (isforce)
    {
        DeleteFile(foldername, true);
    }
    else
    {
        std::cout << "Are you sure you want to delete folder " << foldername << "? (y/n): ";
        char choice;
        std::cin >> choice;
        if (choice == 'y' || choice == 'Y')
        {
            DeleteFile(foldername, true);
        }
        else
        {
            std::cout << "Folder deletion cancelled\n";
        }
    }
}

void RestoreFiles(std::optional<std::string> filename = std::nullopt)
{
    std::set<std::string> IDs;
    if (filename.has_value())
    {
        ListItem(filename, 0);
    }
    else
    {
        ListItem(std::nullopt, 0);
    }
    std::cout << std::endl;

    std::string restoreIDs;
    std::cout << "Which file you want to restore you can Restore multiple files/folders by giving comma separated values(ex: -f ID_1,ID_2,ID_3) or range of values(ex: -f ID_1-ID_5):";
    std::cin >> restoreIDs;
    auto moveFiles = [](std::set<std::string> IDs)
    {
        for (const auto &id : IDs)
        {
            if (TimeStampMap.find(std::stoi(id)) != TimeStampMap.end())
            {
                TrashEntry entry = TimeStampMap[std::stoi(id)];
                long long timestamp = entry.deleted_at;
                std::string original_name = entry.original_name;
                std::string stored_name = entry.stored_name;
                int pos = stored_name.find_last_of("_");
                std::string filename = stored_name.substr(0, pos);
                if (std::filesystem::exists(original_name))
                {
                    std::cout << "File Already There can't replace original file:" << original_name << std::endl;
                    continue;
                }
                try
                {
                    std::filesystem::rename(stored_name, filename);
                }
                catch (const std::filesystem::filesystem_error &e)
                {
                    std::cerr << e.what() << "\n";
                    std::cout << "stored file not found: " << stored_name << "\n";
                    continue;
                }
                size_t pos3 = original_name.find_last_of("/");
                std::string parentDir = original_name.substr(0, pos3);
                if (!std::filesystem::exists(parentDir))
                {
                    try
                    {
                        std::filesystem::create_directories(parentDir);
                    }
                    catch (const std::filesystem::filesystem_error &e)
                    {
                        std::cerr << "Filesystem error: " << e.what() << "\n";
                        continue;
                    }
                }
                try
                {
                    std::filesystem::rename(filename, original_name);
                }
                catch (const std::filesystem::filesystem_error &e)
                {
                    std::cerr << "Filesystem error: " << e.what() << "\n";
                    continue;
                }

                std::string jsonFileName = stored_name;
                size_t pos2 = jsonFileName.find("/Faizan_rm/files/");
                if (pos2 != std::string::npos)
                {
                    jsonFileName.replace(pos2, std::string("/Faizan_rm/files/").length(), "/Faizan_rm/info/");
                }
                std::string metaFilePath = jsonFileName + ".json";
                try
                {

                    std::filesystem::remove(metaFilePath);
                }
                catch (const std::filesystem::filesystem_error &e)
                {
                    std::cerr << "Filesystem error: " << e.what() << "\n";
                    continue;
                }
                std::cout << "Restored file: " << original_name << "\n";
            }
            else
            {
                std::cout << "Invalid ID: " << id << "\n";
            }
        }
    };
    std::regex re(R"((\d+)(?:-(\d+))?)");
    std::smatch match;
    while (std::regex_search(restoreIDs, match, re))
    {
        if (match[2].matched)
        {
            int start = std::stoi(match[1]);
            int end = std::stoi(match[2]);
            for (int i = start; i <= end; ++i)
            {
                IDs.insert(std::to_string(i));
            }
        }
        else
        {
            IDs.insert(match[1]);
        }
        restoreIDs = match.suffix().str();
    }

    moveFiles(IDs);
}

void DeleteFilesFromTrash()
{
    ListItem(std::nullopt, 0);
    std::cout << std::endl;

    std::string IDs;
    std::cout << "What file you want to permanently delete you can delete multiple files/folders by giving comma separated values(ex: -f ID_1,ID_2,ID_3) or range of values(ex: -f ID_1-ID_5):";
    std::cin >> IDs;
    std::set<std::string> deleteIDs;
    auto removeFiles = [](std::set<std::string> IDs)
    {
        for (const auto &id : IDs)
        {
            if (TimeStampMap.find(std::stoi(id)) != TimeStampMap.end())
            {
                TrashEntry entry = TimeStampMap[std::stoi(id)];
                std::string stored_name = entry.stored_name;
                std::string original_name = entry.original_name;
                bool is_folder = entry.is_folder;
                std::string jsonFileName = stored_name;
                size_t pos2 = jsonFileName.find("/Faizan_rm/files/");
                if (pos2 != std::string::npos)
                {
                    jsonFileName.replace(pos2, std::string("/Faizan_rm/files/").length(), "/Faizan_rm/info/");
                }
                std::string metaFilePath = jsonFileName + ".json";
                int ret;

                try
                {
                    std::filesystem::remove(stored_name);
                }
                catch (const std::filesystem::filesystem_error &e)
                {
                    std::cerr << "Filesystem error: " << e.what() << "\n";
                    continue;
                }
                try
                {

                    std::filesystem::remove(metaFilePath);
                }
                catch (const std::filesystem::filesystem_error &e)
                {
                    std::cerr << "Filesystem error: " << e.what() << "\n";
                    continue;
                }
                std::cout << "Permanently deleted file: " << original_name << "\n";
            }
        }
    };
    std::regex re(R"((\d+)(?:-(\d+))?)");
    std::smatch match;
    while (std::regex_search(IDs, match, re))
    {
        if (match[2].matched)
        {
            int start = std::stoi(match[1]);
            int end = std::stoi(match[2]);
            for (int i = start; i <= end; ++i)
            {
                deleteIDs.insert(std::to_string(i));
            }
        }
        else
        {
            deleteIDs.insert(match[1]);
        }
        IDs = match.suffix().str();
    }
    removeFiles(deleteIDs);
}
void cleanUpFilesFromTrash(std::string TrashDir)
{
    if (!std::filesystem::exists(TrashDir))
        return;

    const auto now = std::chrono::system_clock::now();

    auto CLEANUP_AGE = std::chrono::seconds(CleanUpDays * 24 * 60 * 60);

    for (const auto &entry : std::filesystem::directory_iterator(TrashDir))
    {
        try
        {

            auto ftime = std::filesystem::last_write_time(entry);
            auto sctp =
                std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());

            auto age = now - sctp;
            if (age > CLEANUP_AGE)
            {
                std::filesystem::remove_all(entry.path()); // permanent delete
            }
        }
        catch (...)
        {
            // ignore files we cannot access
        }
    }
}
void AsyncCleanup()
{
    std::thread([]
                {
          cleanUpFilesFromTrash(FileInfoPath);
        cleanUpFilesFromTrash(FileStorePath); })
        .detach();
}
int main(int argc, char *argv[])
{

    CreateFolderIfNotExist();
    if (argc > 2 && strcmp(argv[1], "-cleandays") == 0)
    {
        CleanUpDays = atoi(argv[2]);
    }
    else
    {
        std::string cleanup_json_file = FileInfoPath + "/Faizan_rm_cleanupdays.json";
        if (!std::filesystem::exists(cleanup_json_file))
        {
            CleanUpDays = 90;
        }
        else
        {

            std::ifstream cleanupFile(cleanup_json_file);
            if (cleanupFile.is_open())
            {
                json clean_json;
                cleanupFile >> clean_json;
                CleanUpDays = clean_json["cleanupdays"];
                cleanupFile.close();
            }
            else
            {
                std::cerr << "Could not open meta file for writing: " << cleanup_json_file << "\n";
            }
        }
    }
    // clean up old files if any
    AsyncCleanup();
    // cleanUpFilesFromTrash();

    if (argc == 1)
    {
        printf("No arguments provided. please give appropriate argument\n");
        HelpFunction();
        return 0;
    }
    ProcessArguments(argc, argv);
    return 0;
}
void HelpFunction()
{
    printf("Usage: program [options]\n");
    printf("Options:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -list,-List          List all items\n");
    printf("  <file or folder name>     Delete the specified file\n");
    printf("  -rf,-RF,-R,-r <foldername>   Delete the specified folder\n");
    printf("  <foldername> -list,-List      List items in the specified folder\n");
    printf(" <folder name>  -Restore    Restore the file/folder by parent folder name\n");
    printf("  -Restore            Restore the  file or folder\n");
    printf("  -delete         Permanently delete the file or folder with the given ID\n");
    printf(" -empty       Empty the trash folder permanently delete all files trash\n");
    printf("  -cleandays <N>              Set number of days before files are auto-deleted from trash (default 90)\n");

    printf("you can Restore multiple files/folders by giving comma separated values(ex: -f ID_1,ID_2,ID_3) or range of values(ex: -f ID_1-ID_5)\n");
}
void ProcessArguments(int argc, char *argv[])
{
    if (argc == 2)
    {
        if ((strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-help") == 0))
        {
            HelpFunction(); // done
            return;
        }
        else if (strcmp(argv[1], "-list") == 0 || strcmp(argv[1], "-List") == 0)
        {
            ListItem();
            return;
        }
        else if (strcmp(argv[1], "-Restore") == 0 || strcmp(argv[1], "-restore") == 0)
        {
            RestoreFiles();
            return;
        }
        else if (strcmp(argv[1], "-empty") == 0)
        {
            std::cout << "Emptying trash folder permanently delete all files in trash\n";
            try
            {
                std::filesystem::remove_all("/home/faizanmomin/.local/share/Faizan_rm/files");
            }
            catch (const std::filesystem::filesystem_error &e)
            {
                std::cerr << "Filesystem error: " << e.what() << "\n";
                return;
            }
            try
            {
                std::filesystem::remove_all("/home/faizanmomin/.local/share/Faizan_rm/info");
            }
            catch (const std::filesystem::filesystem_error &e)
            {
                std::cerr << "Filesystem error: " << e.what() << "\n";
                return;
            }
            std::cout << "Trash folder emptied successfully\n";
            CreateFolderIfNotExist();

            return;
        }
        else if (strcmp(argv[1], "-delete") == 0)
        {

            DeleteFilesFromTrash();
            return;
        }
        else if (argv[1][0] != '-')
        {
            bool is_folder;
            try
            {
                is_folder = std::filesystem::is_directory(argv[1]);
            }
            catch (const std::filesystem::filesystem_error &e)
            {
                DeleteFile(argv[1], std::nullopt);
                return;
            }
            DeleteFile(argv[1], is_folder); // done
            return;
        }
        else
        {
            std::cout << "Invalid argument please choose correct options\n";
            HelpFunction();
            return;
        }
    }
    else if (argc == 3)
    {
        if (strcmp(argv[1], "-cleandays") == 0)
        {
            json clean_json;
            clean_json["cleanupdays"] = atoi(argv[2]);
            std::string cleanup_json_file = FileInfoPath + "/Faizan_rm_cleanupdays.json";
            std::ofstream cleanupFile(cleanup_json_file);
            if (cleanupFile.is_open())
            {
                cleanupFile << clean_json.dump(4);
                cleanupFile.close();
                std::cout << "Change clean up days to:" << atoi(argv[2]);
            }
            else
            {
                std::cerr << "Could not open meta file for writing: " << cleanup_json_file << "\n";
            }
        }
        else if (strcmp(argv[1], "-rf") == 0 || strcmp(argv[1], "-RF") == 0)
        {
            DeleteFolder(argv[2], true);
            return;
        }
        else if (strcmp(argv[1], "-r") == 0 || strcmp(argv[1], "-R") == 0)
        {
            DeleteFolder(argv[2], false); // done
            return;
        }
        else if (strcmp(argv[2], "-list") == 0 || strcmp(argv[2], "-List") == 0)
        {
            ListItem(argv[1], std::nullopt); // done
            return;
        }
        else if (strcmp(argv[2], "-Restore") == 0 || strcmp(argv[2], "-restore") == 0)
        {
            RestoreFiles(argv[1]);
            return;
        }
        else
        {
            std::cout << "Invalid argument please choose correct options\n";
            HelpFunction();
            return;
        }
    }
    // multiple file/folder delete case
    else
    {
        int start_index = 1;
        if (strcmp(argv[1], "-rf") == 0 || strcmp(argv[1], "-RF") == 0 || strcmp(argv[1], "-r") == 0 || strcmp(argv[1], "-R") == 0)
        {
            start_index = 2;
        }
        for (int i = start_index; i < argc; i++)
        {
            if (argv[i][0] == '-')
            {
                std::cout << "Invalid file/folder name: " << argv[i] << "\n";
                return;
            }
            bool is_folder;
            try
            {
                is_folder = std::filesystem::is_directory(argv[i]);
            }
            catch (const std::filesystem::filesystem_error &e)
            {
                DeleteFile(argv[i], std::nullopt);
                continue;
            }
            DeleteFile(argv[i], is_folder);
        }
        return;
    }
}