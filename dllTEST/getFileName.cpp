#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <fstream> //ifstream
#include <string>  //包含getline()
#include <cmath>
using namespace std;

void getFilesName(string path, vector<string> &files)
{
    // 文件句柄
    intptr_t hFile = 0;
    // 文件信息
    struct _finddata_t fileinfo;
    string p;
    if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
    {
        do
        {
            // 如果是目录,迭代之
            // 如果不是,加入列表
            if ((fileinfo.attrib & _A_SUBDIR))
            {
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
                    getFiles(p.assign(path).append("\\").append(fileinfo.name), files);
            }
            else
            {
                files.push_back(string(fileinfo.name).substr(0, string(fileinfo.name).find_last_of('.')));
            }
        } while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
}

int main(int argc, char **argv)
{
    string filePath = "..\\";
    vector<string> files;

    getFiles(filePath, files);

    char str[30];
    int size = files.size();
    int nullSize = 0;
    for (int i = 0; i < size; i++)
    {
        cout << files[i] << endl;

        /*
                string s;
                ifstream inf;
                inf.open(files[i].c_str());
                while (getline(inf, s)){
                    if (s == "null"){
                        cout << files[i].c_str() << ":NULL" << endl;
                        nullSize++;
                    }
                }
        */
    }
    //	cout << nullSize << ": " << size << endl;
    //	cout << (float)nullSize / size << endl;

    return 0;
}