#ifndef __HANDLEFILE_HPP
#define __HANDLEFILE_HPP
#include <stdio.h>
class HandleFile
{
public:
    HandleFile(char *FileName, char *rw);
    ~HandleFile();

    int ReadFile();
    void WriteToFile(unsigned char *dataBuff, int dataLen);
    char *GetPtr() { return m_DataPtr; }
    int GetLength() { return m_Length; }
private:
    FILE *m_fp = NULL;
    char *m_DataPtr = NULL;
    int m_Length;

    int fileSize();
};

#endif