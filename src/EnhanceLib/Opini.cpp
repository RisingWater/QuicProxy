#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#else
#include "winpr/wtypes.h"
#include "winpr/tchar.h"
#include "winpr/file.h"
#include "winpr/synch.h"
#include "winpr/thread.h"
#include "winpr/interlocked.h"
#endif
#ifndef WIN32
#include <winpr/file.h>
#endif
#include "Opini.h"

#define BUFFER_LEN 32768
#define LINE_LENGTH 4096

/** < 前后去除空格 */
static CHAR *DeleteSpace(CHAR *sp)
{
    CHAR * tp;

    tp = sp + strlen(sp) - 1;

    while (*tp == ' ')
    {
        *tp = '\0';
        tp--;
    }

    while (*sp == ' ')
        sp++;

    return sp;
}

CFileIni::CFileIni(const CHAR* inifile)
{
    m_szIniFileName = strdup(inifile);
}

CFileIni::~CFileIni()
{
    free(m_szIniFileName);
}

BOOL CFileIni::ReadLineFromFile(FILE *f, CHAR *s)
{
    int c;

    do
    {
        c = fgetc(f);
        *(s++) = (CHAR)c;
    }
    while (c != EOF && c != '\xa' && c != '\xd');

    *(s - 1) = '\0';

    return (c == EOF);
}

BOOL CFileIni::ReadAllBufferFromFile(CHAR *s, DWORD len)
{
    FILE* file = fopen(m_szIniFileName, "rb");

    if (file)
    {
        int size = fread(s, 1, len, file);
        s[size] = '\0';
        fclose(file);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL CFileIni::WriteAllBufferToFile(const CHAR *s)
{
    FILE* file = fopen(m_szIniFileName, "wb");

    if (file)
    {
        int size = strlen(s);
        fwrite(s, size, 1, file);
        fclose(file);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

CHAR * CFileIni::GetLineFromBuffer(CHAR *sp, CHAR *dest, DWORD maxlen)
{
    while (maxlen--)
    {
        if (*sp && *sp != 13 && *sp != 10)
            * (dest++) = *(sp++);
        else
            break;
    }

    *dest = '\0';

    while ((*sp == 10 || *sp == 13) && *sp)
        sp++;

    return sp;
}

void CFileIni::DeleteLineFromBuffer(CHAR *sp)
{
    CHAR * cp;
    cp = sp;

    while (*sp && *sp != 10 && *sp != 13)
        sp++;

    while (*sp && (*sp == 10 || *sp == 13))
        sp++;

    while (*sp)
        *(cp++) = *(sp++);

    *cp = *sp;
}

void CFileIni::InsertLineFromBuffer(CHAR *sp, CHAR *line)
{
    int i, l, end;

    l = strlen(line);
    end = strlen(sp);

    for (i = end; i >= 0; i--)
        sp[i + l] = sp[i];

    for (i = 0; i < l; i++)
        sp[i] = line[i];
}

BOOL CFileIni::ReadOption(const CHAR* segment, const CHAR* name, CHAR* value)
{
    BOOL FindAllSegment = FALSE;
    FILE * file;
    BOOL found;
    int i;
    CHAR buf[LINE_LENGTH];
    CHAR bufname[MAX_PATH];
    CHAR seg[MAX_PATH];
    CHAR *s;

    //打开文件
    file = fopen(m_szIniFileName, "r");
    if (file == NULL)
    {
        return FALSE;
    }

    found = FALSE;
    if (segment != NULL)
    {
        sprintf(seg, "[%s]", segment);
        FindAllSegment = FALSE;
    }
    else
    {
        FindAllSegment = TRUE;
    }

    //查找片段
    while (!this->ReadLineFromFile(file, buf))
    {
        if (FindAllSegment || strcmp(seg, buf) == 0)
        {
            while (!this->ReadLineFromFile(file, buf) && buf[0] != '[')
            {
                //查找等于号后截断
                for (i = 0; buf[i] != '=' && buf[i]; i++)
                {
                    bufname[i] = buf[i];
                }

                bufname[i] = '\0';

                //回头去空格
                for (i--; i >= 0 && bufname[i] == ' '; i--)
                {
                    bufname[i] = '\0';
                }

                if (strcmp(bufname, name) == 0)
                {
                    s = buf;
                    //查找等于号后截断
                    while (*s != '=' && *s)
                        s++;

                    s++;
                    //向后删除空格
                    while (*s == ' ')
                        s++;


                    strcpy(value, s);

                    found = TRUE;

                    break;
                }

            }
            break;
        }
    }

    fclose(file);

    return found;
}

BOOL CFileIni::ReadOption(const CHAR* segment, const CHAR* name, DWORD* value)
{
    CHAR tmp[MAX_PATH];
    if (this->ReadOption(segment, name, tmp))
    {
        *value = atoi(tmp);
        return TRUE;
    }

    return FALSE;
}

BOOL CFileIni::DeleteOption(const CHAR* segment, const CHAR* name)
{
    CHAR WriteBuf[BUFFER_LEN];
    CHAR seg[MAX_PATH];
    CHAR buf[MAX_PATH];
    CHAR *sp, *np, *vp;

    if (!ReadAllBufferFromFile(WriteBuf, BUFFER_LEN))
    {
        return FALSE;
    }

    sprintf(seg, "[%s]", segment);

    sp = WriteBuf;

    while (*sp)
    {
        sp = GetLineFromBuffer(sp, buf, MAX_PATH);

        if (strcmp(seg, buf) == 0)
        {
            while (1)
            {
                np = GetLineFromBuffer(sp, buf, MAX_PATH);

                if (*sp && buf[0] != '[')
                {
                    vp = strchr(buf, '=');

                    if (vp)
                    {
                        *vp = '\0';

                        if (strcmp(DeleteSpace(buf), name) == 0)
                        {
                            DeleteLineFromBuffer(sp);
                            break;
                        }

                    }
                    else
                    {
                        DeleteLineFromBuffer(sp);
                    }

                }
                else
                {
                    break;
                }

                sp = np;
            }

            break;
        }

    }


    return WriteAllBufferToFile(WriteBuf);
}

BOOL CFileIni::WriteOption(const CHAR* segment, const CHAR* name, CHAR* value)
{
    CHAR WriteBuf[BUFFER_LEN];
    CHAR seg[MAX_PATH];
    CHAR buf[MAX_PATH];
    CHAR *sp, *np, *vp;
    BOOL found = FALSE;

    if (!ReadAllBufferFromFile(WriteBuf, BUFFER_LEN))
    {
        memset(WriteBuf, 0, BUFFER_LEN);
    }

    sprintf(seg, "[%s]", segment);

    sp = WriteBuf;

    while (*sp)
    {
        sp = GetLineFromBuffer(sp, buf, MAX_PATH);

        if (strcmp(seg, buf) == 0)
        {
            while (1)
            {
                np = GetLineFromBuffer(sp, buf, MAX_PATH);

                if (*sp && buf[0] != '[')
                {
                    vp = strchr(buf, '=');

                    if (vp)
                    {
                        *vp = '\0';

                        if (!strcmp(DeleteSpace(buf), name))
                        {
                            vp++;

                            if (strcmp(DeleteSpace(vp), value))
                            {
                                DeleteLineFromBuffer(sp);
                                sprintf(buf, "%s=%s\r\n", name, value);
                                InsertLineFromBuffer(sp, buf);
                            }

                            found = 1;

                            break;
                        }
                    }
                    else
                    {
                        DeleteLineFromBuffer(sp);
                    }

                }
                else
                {
                    break;
                }

                sp = np;
            }

            if (!found)
            {
                sprintf(buf, "%s=%s\r\n", name, value);
                InsertLineFromBuffer(sp, buf);
                found = TRUE;
            }

            break;
        }
    }

    if (!found)
    {
        sprintf(sp, "\r\n[%s]\r\n%s=%s\r\n", segment, name, value);
    }

    WriteAllBufferToFile(WriteBuf);
    return TRUE;
}

BOOL CFileIni::WriteOption(const CHAR* segment, const CHAR* name, DWORD value)
{
    CHAR tmp[MAX_PATH];
    sprintf(tmp, "%d", (int)value);
    return this->WriteOption(segment, name, tmp);
}


