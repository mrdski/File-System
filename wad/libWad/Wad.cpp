#include "Wad.h"
#include <sstream>
#include <fcntl.h>
#include <stack>
#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utility>
#include <regex>

using namespace std;
Wad* Wad::loadWad(const string &path) {
    int f = open(path.c_str(),00);
    int desC = 0; 
    int size = 4;
    char* magic = new char[size];
    int offsetE = 0; 
    int lengthE = 0;
    char* nameE = new char[8];
    Wad* wad = new Wad();

    //magic, number of descriptors, and offset
    read(f, magic, size); 
    read(f, &wad->numD, size); 
    read(f, &wad->offset, size);
    wad->magic = (string)magic;

    //file offset to the beginning of the descriptors
    lseek(f, wad->offset, SEEK_SET);

    auto* root = new Tree();
    stack<Tree*> s;
    s.push(root);

    int offsetT = 0;
    std::regex startRegex("(.*)_START$"); 
    std::regex endRegex("(.*)_END$"); 
    std::regex emRegex("E[0-9]M[0-9]$");

    for(int i = 0; i < wad->numD; i++){
        read(f, &offsetE, 4); read(f, &lengthE, 4); read(f, nameE, 8);
        offsetT += 16;
        string newE = string(nameE);
        char* data = new char[lengthE];
        // Parse descriptors based on their type
        if(lengthE == 0 && std::regex_search(newE, startRegex)){// If it's a start descriptor, create a directory node
            Tree* temp = add(newE.substr(0,newE.length()-6),nullptr,0,true);
            root = s.top();
            root->children.push_back(temp);
            s.push(temp);
        } else if(newE.length()==4 && std::regex_search(newE, emRegex)){ // If it's a file descriptor, create a file node
            root=s.top();
            Tree* temp = add(newE,nullptr,0, true);
            root->children.push_back(temp);
            root = temp;
            desC = i+10;
        } else if(i <= desC){ // If it's a content descriptor, read its data and create node
            lseek(f, offsetE, SEEK_SET);
            read(f, data, lengthE);
            root->children.push_back(add(newE, data, lengthE, lengthE == 0));
            lseek(f, wad->offset + offsetT, SEEK_SET);
            if(i==desC) root = s.top();
        } else if(lengthE==0 && std::regex_search(newE, endRegex)){ // If it's an end descriptor, pop the stack to move up
            s.pop(); 
            root=s.top();
        } else{ // If it's a content descriptor in the middle, read its data and create node
            lseek(f, offsetE, SEEK_SET);
            read(f, data, lengthE);
            root = s.top();
            root->children.push_back(add(newE, data, lengthE, lengthE == 0));
            lseek(f, wad->offset + offsetT, SEEK_SET);
        }
    }
    close(f);
    wad->root = s.top();
    s.pop();
    return wad;
}

string Wad::getMagic(){
    return magic;
}

bool Wad::isContent(const string &path) {
    auto [node, isContent] = getNode(root, path, false);
    return ((node != nullptr && (node->children.empty() || node->data != nullptr)));
}

bool Wad::isDirectory(const std::string &path) {
    auto [node, isContent] = getNode(root, path, false);
    return ((node != nullptr && (!node->children.empty() || node->isDir || node->data == nullptr)));
}

int Wad::getSize(const std::string &path) {
    auto [node, isLastCharacterSlash] = getNode(root, path, false);
    return (node != nullptr && node->data != nullptr) ? node->len : -1;
}

// Reads the contents of the file at the given path into the buffer
int Wad::getContents(const std::string &path, char *buffer, int length, int offset) {
    auto [node, isLastCharacterSlash] = getNode(root, path, false);
    if (!isContent(path)) return -1;
    // Calculate the number of bytes to copy
    int bytes_to_copy = std::min(length, node->len - offset);
    if (bytes_to_copy <= 0) return 0;
    // Copy the content into the buffer
    memcpy(buffer, node->data + offset, bytes_to_copy);
    return bytes_to_copy;
}

// Retrieves the names of the children of the directory at the given path
int Wad::getDirectory(const string &path, vector<string> *directory) {
    auto [node, isLastCharacterSlash] = getNode(root, path, false);
    if (!isDirectory(path)) return -1;
    // Add the names of children nodes to the directory vector
    for (auto &child : node->children) {directory->push_back(child->name);}
    return (*directory).size();
}

// Creates a new directory at the given path
void Wad::createDirectory(const std::string& path) {
    // Extract directory name and parent path
    std::string directoryName = path.substr(path.find_last_of('/') + 1);
    std::string parentPath = path.substr(0, path.find_last_of('/'));
    auto [parent, isLastCharacterSlash] = getNode(root, parentPath, true);
    if (parent != nullptr) {
        // Check if the parent is not an end descriptor
        if (parent->name.find("_END") == std::string::npos) {
            // Create start and end descriptors for the directory
            Tree* startDesc = add(directoryName + "_START", nullptr, 0, true);
            Tree* endDesc = add(directoryName + "_END", nullptr, 0, true);
            auto it = parent->children.end();
            --it;
            // Insert start and end descriptors into the parent's children
            parent->children.insert(it, startDesc);
            parent->children.insert(it, endDesc);
            numD += 2;
        }
    }
}

// Creates a new file at the given path
void Wad::createFile(const std::string& path) {
    auto [parent, isLastCharacterSlash] = getNode(root, path, true);
    if (parent != nullptr) {
        // Extract file name
        std::string fileName = path.substr(path.find_last_of('/') + 1);
        // Add the file to the parent's children
        parent->children.push_back(add(fileName, nullptr, 0, false));
    }
}

// Writes data to a file at the given path
int Wad::writeToFile(const std::string& path, const char* buffer, int length, int offset) {
    if (isContent(path)) {
        auto [file, isLastCharacterSlash] = getNode(root, path, false);
        // If the file doesn't have any data yet
        if (file->data == nullptr) {
            file->data = new char[length];
            file->len = length;
        } 
        // If the length of the new data exceeds the current length
        else if (file->len < offset + length) {
            char* newData = new char[offset + length];
            memcpy(newData, file->data, file->len);
            delete[] file->data;
            file->data = newData;
            file->len = offset + length;
        } 
        // If the file already has data
        else if (file->len > 0) return 0;
        // Copy data into the file's buffer
        memcpy(file->data + offset, buffer, length);
        descriptorForward(file, length);
        offset += length;
        return length;
    }
    return -1;
}

// Updates offsets of descriptors after writing to a file
void Wad::descriptorForward(Tree* file, int length) {
    auto [parent, isLastCharacterSlash] = getNode(root, file->name, true);
    if (parent == nullptr) return;
    int index = -1;
    for (size_t i = 0; i < parent->children.size(); ++i) {
        if (parent->children[i] == file) {
            index = i;
            break;
        }
    }
    if (index == -1) return;
    for (size_t i = index + 1; i < parent->children.size(); ++i) {parent->children[i]->offset += length;}
}

// Adds a new tree node
Tree* add(string name, char* data, int len, bool isD){
    auto* temp = new Tree(); temp->name = name; temp->data = data; temp->len = len; temp->isDir = isD;
    return temp;
}

// Gets the node at the given path in the tree
std::pair<Tree*, bool> getNode(Tree* root, const std::string& path, bool parent) {
    if (path.empty() || path[0] != '/') return {nullptr, (path.find('.') != string::npos)};
    Tree* currentNode = root;
    Tree* parentNode = nullptr;
    std::istringstream pathStream(path);
    std::string directory;
    if (path == "/") return {currentNode, (path.find('.') != string::npos)};
    while (std::getline(pathStream, directory, '/')) {
        if (directory.empty()) continue;
        bool found = false;
        for (Tree* child : currentNode->children) {
            if (child->name == directory) {
                parentNode = currentNode;
                currentNode = child;
                found = true;
                break;
            }
        }
        if (!found) return {nullptr, false};
    }
    if (parent) return {parentNode, (path.find('.') != string::npos)};
    return {currentNode, (path.find('.') != string::npos)};
}

