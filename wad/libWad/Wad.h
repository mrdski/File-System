#ifndef WAD_H
#define WAD_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unistd.h>
#include <bits/stdc++.h>
#include <string>
#include <vector>
#include <string>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <bits/stdc++.h>
#include <utility>

using namespace std;

struct Tree{
    vector<Tree*> children = {};
    char* data = nullptr;
    bool isDir;
    string name = "/";
    int len = 0;
    int offset = 0;
};
std::pair<Tree*, bool> getNode(Tree* root, const std::string& path, bool parent);
Tree* add(string name, char* data, int len, bool isD);
class Wad{
    Tree* root = nullptr;
    std::string magic;
    int numD = 0;
    int offset = 0;
    public:
        static Wad* loadWad(const string &path);
        string getMagic();
        bool isContent(const string &path);
        bool isDirectory(const string &path);
        int getSize(const string &path);
        int getContents(const string &path, char *buffer, int length, int offset = 0);
        int getDirectory(const string &path, vector<string> *directory);
        void createDirectory(const std::string& path);
        void createFile(const std::string& path);
        int writeToFile(const std::string& path, const char* buffer, int length, int offset = 0);
        
        void descriptorForward(Tree* file, int length);
};
#endif