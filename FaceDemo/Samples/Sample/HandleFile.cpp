#include "HandleFile.hpp"
#include "Samples.hpp"
HandleFile::HandleFile(char *FileName, char *rw)
{
#ifdef WIN32
    fopen_s(&m_fp, FileName, rw);
#else
    m_fp = fopen(FileName, rw);
#endif
}

HandleFile::~HandleFile() {
    if (m_fp)
    {
        fclose(m_fp);
    }
    if (m_DataPtr)
    {
        delete [] m_DataPtr;
    }
}

int HandleFile::ReadFile() {
    int readValue;
    int count = 0;

    if (!m_fp)
    {
        printf("File is not opened or open file error!");
        return FALSE;
    }

    m_Length = fileSize();
    m_DataPtr = new char[m_Length];
    if (NULL == m_DataPtr)
    {
        printf("Allocate memory error!");
        return FALSE;
    }
    unsigned short* readPtr = (unsigned short *)m_DataPtr;
    while (EOF != fscanf_s(m_fp, "%x", &readValue))
    {
        *readPtr++ = (unsigned short)readValue;
        count++;
    }
    m_Length = count * 2;
    return TRUE;
}

void HandleFile::WriteToFile(unsigned char *dataBuff, int dataLen)
{
    for (int i = 0; i < dataLen; i++)
    {
        fprintf(m_fp, "%02x ", dataBuff[i]);
#if 0
        if (i != 0 && ((i+1)%7) == 0)
        {

            fprintf(fp, "\n");
        }
        if (i != 0 && ((i+1)%49) == 0)
        {
            fprintf(fp, "\n");
        }
#else
        fprintf(m_fp, "\n");
#endif
    }
}

//=======================================================================
// Function name: int fileSize()
//   Gets file size.
//
// Input: None.
//
// Return: int - file size.
//=======================================================================
int HandleFile::fileSize()
{
    int size;

    if (NULL == m_fp)
    {
        return 0;
    }
    fseek(m_fp, 0L, SEEK_END);
    size = ftell(m_fp);
    rewind(m_fp);
    return size;
}
