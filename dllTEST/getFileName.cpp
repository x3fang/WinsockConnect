#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <fstream> //ifstream
#include <string>  //包含getline()
#include <cmath>
using namespace std;

int main(int argc, char **argv)
{
    string filePath = ".";
    vector<string> files;

    getFilesName(filePath, files);

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