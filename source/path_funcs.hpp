#pragma once
#include <sys/stat.h>
#include <dirent.h>

// Function to create a directory if it doesn't exist
void createDirectory(const std::string& directoryPath) {
    struct stat st;
    if (stat(directoryPath.c_str(), &st) != 0) {
        mkdir(directoryPath.c_str(), 0777);
    }
}

// Function to create a text file with the specified content
void createTextFile(const std::string& filePath, const std::string& content) {
    FILE* file = std::fopen(filePath.c_str(), "w");
    if (file != nullptr) {
        std::fwrite(content.c_str(), 1, content.length(), file);
        std::fclose(file);
    }
}


void removeEntryFromList(const std::string& entry, std::vector<std::string>& fileList) {
    fileList.erase(std::remove_if(fileList.begin(), fileList.end(), [&](const std::string& filePath) {
        return filePath.compare(0, entry.length(), entry) == 0;
    }), fileList.end());
}


// Delete functions
void deleteFileOrDirectory(const std::string& pathToDelete) {
    struct stat pathStat;
    if (stat(pathToDelete.c_str(), &pathStat) == 0) {
        if (S_ISREG(pathStat.st_mode)) {
            if (std::remove(pathToDelete.c_str()) == 0) {
                // Deletion successful
            }
        } else if (S_ISDIR(pathStat.st_mode)) {
            // Delete all files in the directory
            DIR* directory = opendir(pathToDelete.c_str());
            if (directory != nullptr) {
                dirent* entry;
                while ((entry = readdir(directory)) != nullptr) {
                    std::string fileName = entry->d_name;
                    if (fileName != "." && fileName != "..") {
                        std::string filePath = pathToDelete + "/" + fileName;
                        deleteFileOrDirectory(filePath);
                    }
                }
                closedir(directory);
            }

            // Remove the directory itself
            if (rmdir(pathToDelete.c_str()) == 0) {
                // Deletion successful
            }
        }
    }
}

void deleteFileOrDirectoryByPattern(const std::string& pathPattern) {
    //logMessage("pathPattern: "+pathPattern);
    std::vector<std::string> fileList = getFilesListByWildcards(pathPattern);

    for (const auto& path : fileList) {
        //logMessage("path: "+path);
        deleteFileOrDirectory(path);
    }
}

void mirrorDeleteFiles(const std::string& sourcePath, const std::string& targetPath="sdmc:/") {
    std::vector<std::string> fileList = getFilesListFromDirectory(sourcePath);

    for (const auto& path : fileList) {
        // Generate the corresponding path in the target directory by replacing the source path
        std::string updatedPath = targetPath + path.substr(sourcePath.size());
        //logMessage("mirror-delete: "+path+" "+updatedPath);
        deleteFileOrDirectory(updatedPath);
    }
}


// Move functions
void moveFileOrDirectory(const std::string& sourcePath, const std::string& destinationPath) {
    struct stat sourceInfo;
    struct stat destinationInfo;
    
    //logMessage("sourcePath: "+sourcePath);
    //logMessage("destinationPath: "+destinationPath);
    
    if (stat(sourcePath.c_str(), &sourceInfo) == 0) {
        // Source file or directory exists

        // Check if the destination path exists
        bool destinationExists = (stat(getParentDirFromPath(destinationPath).c_str(), &destinationInfo) == 0);
        if (!destinationExists) {
            // Create the destination directory
            createDirectory(getParentDirFromPath(destinationPath).c_str());
        }

        if (S_ISDIR(sourceInfo.st_mode)) {
            // Source path is a directory
            DIR* dir = opendir(sourcePath.c_str());
            if (!dir) {
                //logMessage("Failed to open source directory: "+sourcePath);
                //printf("Failed to open source directory: %s\n", sourcePath.c_str());
                return;
            }

            struct dirent* entry;
            while ((entry = readdir(dir)) != NULL) {
                const std::string fileOrFolderName = entry->d_name;

                if (fileOrFolderName != "." && fileOrFolderName != "..") {
                    std::string sourceFilePath = sourcePath + fileOrFolderName;
                    std::string destinationFilePath = destinationPath + fileOrFolderName;

                    if (entry->d_type == DT_DIR) {
                        // Append trailing slash to destination path for folders
                        destinationFilePath += "/";
                        sourceFilePath += "/";
                    }

                    moveFileOrDirectory(sourceFilePath, destinationFilePath);
                }
            }

            closedir(dir);

            // Delete the source directory
            deleteFileOrDirectory(sourcePath);

            return;
        } else {
            // Source path is a regular file
            std::string filename = getNameFromPath(sourcePath.c_str());

            std::string destinationFilePath = destinationPath;

            if (destinationPath[destinationPath.length() - 1] == '/') {
                destinationFilePath += filename;
            }
            
            
            //logMessage("sourcePath: "+sourcePath);
            //logMessage("destinationFilePath: "+destinationFilePath);
            
            deleteFileOrDirectory(destinationFilePath); // delete destiantion file for overwriting
            if (rename(sourcePath.c_str(), destinationFilePath.c_str()) == -1) {
                //printf("Failed to move file: %s\n", sourcePath.c_str());
                //logMessage("Failed to move file: "+sourcePath);
                return;
            }

            return;
        }
    }

    // Move unsuccessful or source file/directory doesn't exist
    return;
}

void moveFilesOrDirectoriesByPattern(const std::string& sourcePathPattern, const std::string& destinationPath) {
    std::vector<std::string> fileList = getFilesListByWildcards(sourcePathPattern);
    
    std::string fileListAsString;
    for (const std::string& filePath : fileList) {
        fileListAsString += filePath + "\n";
    }
    //logMessage("File List:\n" + fileListAsString);
    
    //logMessage("pre loop");
    // Iterate through the file list
    for (const std::string& sourceFileOrDirectory : fileList) {
        //logMessage("sourceFileOrDirectory: "+sourceFileOrDirectory);
        // if sourceFile is a file (Needs condition handling)
        if (!isDirectory(sourceFileOrDirectory)) {
            //logMessage("destinationPath: "+destinationPath);
            moveFileOrDirectory(sourceFileOrDirectory.c_str(), destinationPath.c_str());
        } else if (isDirectory(sourceFileOrDirectory)) {
            // if sourceFile is a directory (needs conditoin handling)
            std::string folderName = getNameFromPath(sourceFileOrDirectory);
            std::string fixedDestinationPath = destinationPath + folderName + "/";
        
            //logMessage("fixedDestinationPath: "+fixedDestinationPath);
        
            moveFileOrDirectory(sourceFileOrDirectory.c_str(), fixedDestinationPath.c_str());
        }

    }
    //logMessage("post loop");
}


// Copy functions
void copySingleFile(const std::string& fromFile, const std::string& toFile) {
    FILE* srcFile = fopen(fromFile.c_str(), "rb");
    FILE* destFile = fopen(toFile.c_str(), "wb");
    if (srcFile && destFile) {
        const size_t bufferSize = 131072; // Increase buffer size to 128 KB
        char buffer[bufferSize];
        size_t bytesRead;

        while ((bytesRead = fread(buffer, 1, bufferSize, srcFile)) > 0) {
            fwrite(buffer, 1, bytesRead, destFile);
        }

        fclose(srcFile);
        fclose(destFile);
    } else {
        // Error opening files or performing copy action.
        // Handle the error accordingly.
    }
}

void copyFileOrDirectory(const std::string& fromFileOrDirectory, const std::string& toFileOrDirectory) {
    struct stat fromFileOrDirectoryInfo;
    if (stat(fromFileOrDirectory.c_str(), &fromFileOrDirectoryInfo) == 0) {
        if (S_ISREG(fromFileOrDirectoryInfo.st_mode)) {
            // Source is a regular file
            std::string fromFile = fromFileOrDirectory;
            
            struct stat toFileOrDirectoryInfo;
            
            if (stat(toFileOrDirectory.c_str(), &toFileOrDirectoryInfo) == 0 && S_ISDIR(toFileOrDirectoryInfo.st_mode)) {
                // Destination is a directory
                std::string toDirectory = toFileOrDirectory;
                std::string fileName = fromFile.substr(fromFile.find_last_of('/') + 1);
                std::string toFilePath = toDirectory + fileName;

                // Create the destination directory if it doesn't exist
                createDirectory(toDirectory);

                // Check if the destination file exists and remove it
                if (stat(toFilePath.c_str(), &toFileOrDirectoryInfo) == 0 && S_ISREG(toFileOrDirectoryInfo.st_mode)) {
                    std::remove(toFilePath.c_str());
                }

                copySingleFile(fromFile, toFilePath);
            } else {
                std::string toFile = toFileOrDirectory;
                // Destination is a file or doesn't exist
                std::string toDirectory = toFile.substr(0, toFile.find_last_of('/'));

                // Create the destination directory if it doesn't exist
                createDirectory(toDirectory);
                
                // Destination is a file or doesn't exist
                // Check if the destination file exists and remove it
                if (stat(toFile.c_str(), &toFileOrDirectoryInfo) == 0 && S_ISREG(toFileOrDirectoryInfo.st_mode)) {
                    std::remove(toFile.c_str());
                }

                copySingleFile(fromFile, toFile);
            }
        } else if (S_ISDIR(fromFileOrDirectoryInfo.st_mode)) {
            // Source is a directory
            std::string fromDirectory = fromFileOrDirectory;
            //logMessage("fromDirectory: "+fromDirectory);
            
            struct stat toFileOrDirectoryInfo;
            if (stat(toFileOrDirectory.c_str(), &toFileOrDirectoryInfo) == 0 && S_ISDIR(toFileOrDirectoryInfo.st_mode)) {
                // Destination is a directory
                std::string toDirectory = toFileOrDirectory;
                std::string dirName = getNameFromPath(fromDirectory);
                if (dirName != "") {
                    std::string toDirPath = toDirectory + dirName +"/";
                    //logMessage("toDirectory: "+toDirectory);
                    //logMessage("dirName: "+dirName);
                    //logMessage("toDirPath: "+toDirPath);

                    // Create the destination directory
                    createDirectory(toDirPath);
                    //mkdir(toDirPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

                    // Open the source directory
                    DIR* dir = opendir(fromDirectory.c_str());
                    if (dir != nullptr) {
                        dirent* entry;
                        while ((entry = readdir(dir)) != nullptr) {
                            std::string fileOrFolderName = entry->d_name;
                            
                            // handle cade for files
                            if (fileOrFolderName != "." && fileOrFolderName != "..") {
                                std::string fromFilePath = fromDirectory + fileOrFolderName;
                                copyFileOrDirectory(fromFilePath, toDirPath);
                            }
                            // handle case for subfolders within the from file path
                            if (entry->d_type == DT_DIR && fileOrFolderName != "." && fileOrFolderName != "..") {
                                std::string subFolderPath = fromDirectory + fileOrFolderName + "/";
                                
                                
                                copyFileOrDirectory(subFolderPath, toDirPath);
                            }
                            
                        }
                        closedir(dir);
                    }
                }
            }
        }
    }
}

void copyFileOrDirectoryByPattern(const std::string& sourcePathPattern, const std::string& toDirectory) {
    std::vector<std::string> fileList = getFilesListByWildcards(sourcePathPattern);

    for (const std::string& sourcePath : fileList) {
        //logMessage("sourcePath: "+sourcePath);
        //logMessage("toDirectory: "+toDirectory);
        if (sourcePath != toDirectory){
            copyFileOrDirectory(sourcePath, toDirectory);
        }
        
    }
}

void mirrorCopyFiles(const std::string& sourcePath, const std::string& targetPath="sdmc:/") {
    std::vector<std::string> fileList = getFilesListFromDirectory(sourcePath);

    for (const auto& path : fileList) {
        // Generate the corresponding path in the target directory by replacing the source path
        std::string updatedPath = targetPath + path.substr(sourcePath.size());
        if (path != updatedPath){
            //logMessage("mirror-copy: "+path+" "+updatedPath);
            copyFileOrDirectory(path, updatedPath);
        }
    }
}
