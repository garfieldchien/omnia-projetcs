//------------------------------------------------------------------------------
// Projet RtCA          : Read to Catch All
// Auteur               : Nicolas Hanteville
// Site                 : http://code.google.com/p/omnia-projetcs/
// Licence              : GPL V3
//------------------------------------------------------------------------------
#include "RtCA.h"
//------------------------------------------------------------------------------
void ReviewWOW64Redirect(PVOID OldValue_W64b)
{
  typedef BOOL (WINAPI *WOW64DISABLEREDIRECT)(PVOID *OldValue);
  WOW64DISABLEREDIRECT Wow64DisableWow64FsRedirect;

  HMODULE hDLL = LoadLibrary( "KERNEL32.dll");
  if (hDLL != NULL)
  {
    Wow64DisableWow64FsRedirect = (WOW64DISABLEREDIRECT) GetProcAddress(hDLL,"Wow64DisableWow64FsRedirection");
    if (Wow64DisableWow64FsRedirect)Wow64DisableWow64FsRedirect(&OldValue_W64b);
    FreeLibrary(hDLL);
  }
}
//------------------------------------------------------------------------------
void replace_to_char(char *buffer, unsigned long int taille, char a)
{
  char *c = buffer;

  while (c != buffer+taille-4)
  {
    if (*c==0x00 && *(c+1)==0x00 && *(c+2)==0x00&& *(c+3)==0x00)break;
    else if (*c==0x00 && *(c+1)==0x00) *c = a;
    c++;
  }
}
//------------------------------------------------------------------------------
char *convertStringToSQL(char *data, unsigned int size_max)
{
  char tmp[MAX_PATH];
  char *s = data;
  char *d = tmp;
  unsigned int i=0, j=0;
  for (;j<size_max && i<MAX_PATH;i++,j++,d++,s++)
  {
    if (*s == '"')
    {
      if (j+2 < MAX_PATH)
      {
        *d++ = '"';j++;
        *d = '"';
      }else *d = '\'';
    }else *d = *s;
  }
  *d = 0;

  snprintf(data,size_max,"%s",tmp);
  return data;
}
//------------------------------------------------------------------------------
char *ReplaceEnv(char *var, char *path, unsigned int size_max)
{
  //get var
  if (getenv(var) && path[0]=='%')
  {
    char *c = path;
    while (*c && *c!='\\')c++;
    if (*c == '\\')
    {
      //c++;
      char tmp[MAX_PATH];
      snprintf(tmp,MAX_PATH,"%s%s",getenv(var),c);
      strncpy(path,tmp,size_max);
    }
  }
  return path;
}
//------------------------------------------------------------------------------
char *GetTextFromTrv(HTREEITEM hitem, char *txt, DWORD item_size_max)
{
  TVITEM tvitem;
  txt[0]=0;
  tvitem.hItem = hitem;
  tvitem.mask = TVIF_TEXT;
  tvitem.pszText = txt;
  tvitem.cchTextMax = item_size_max;
  SendDlgItemMessage(h_conf,TRV_FILES, TVM_GETITEM,(WPARAM)0, (LPARAM)&tvitem);
  return txt;
}
//------------------------------------------------------------------------------
void SetDebugPrivilege(BOOL enable)
{
    TOKEN_PRIVILEGES privilege;
    LUID Luid;
    HANDLE handle1;
    HANDLE handle2;
    handle1 = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
    OpenProcessToken(handle1,TOKEN_ALL_ACCESS, &handle2);
    LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &Luid);
    privilege.PrivilegeCount = 1;
    privilege.Privileges[0].Luid = Luid;
    privilege.Privileges[0].Attributes = enable?SE_PRIVILEGE_ENABLED:0x04/*SE_PRIVILEGE_REMOVED*/;
    AdjustTokenPrivileges(handle2, FALSE, &privilege, sizeof(privilege), NULL, NULL);
    CloseHandle(handle2);
    CloseHandle(handle1);
}
//------------------------------------------------------------------------------
char *filetimeToString(FILETIME FileTime, char *str, unsigned int string_size)
{
  str[0] = 0;
  SYSTEMTIME SysTime;
  if (FileTimeToSystemTime(&FileTime, &SysTime) != 0)//traitement de l'affichage de la date
    snprintf(str,string_size,"%02d/%02d/%02d %02d:%02d:%02d",SysTime.wYear,SysTime.wMonth,SysTime.wDay,SysTime.wHour,SysTime.wMinute,SysTime.wSecond);

  return str;
}
//------------------------------------------------------------------------------
char *timeToString(DWORD time, char *str, unsigned int string_size)
{
  FILETIME FileTime,LocalFileTime;
  LONGLONG lgTemp = Int32x32To64(time,10000000) + 116444736000000000;

  FileTime.dwLowDateTime  = (DWORD) lgTemp;
  FileTime.dwHighDateTime = (DWORD)(lgTemp >> 32);
  if (FileTimeToLocalFileTime(&FileTime, &LocalFileTime))
  {
    filetimeToString(LocalFileTime, str, string_size);
  }
  return str;
}
//------------------------------------------------------------------------------
char *convertUTF8toUTF16(char *src, DWORD size)
{
  if (src == NULL)return src;

  wchar_t *buffer;
  buffer = malloc(sizeof(wchar_t)*size);
  if (buffer)
  {
    MultiByteToWideChar(CP_UTF8, 0, src, size, buffer, size);
    snprintf(src,size,"%S",buffer);
  }
  free(buffer);
  return src;
}
//------------------------------------------------------------------------------
char *charToLowChar(char *src)
{
  unsigned int i;
  for (i=0;i<strlen(src);i++)
  {
    if (src[i]>64 && src[i]<91)src[i] = src[i]+32;
  }
  return src;
}
//------------------------------------------------------------------------------
char *DataToHexaChar(char *data, unsigned int data_size, char *hexa_char, unsigned int hexa_char_size)
{
  if (data_size >0 && hexa_char_size > 0 && data!=NULL && hexa_char!=NULL)
  {
    unsigned int i;
    char tmp[11];
    for (i=0;i<data_size && i<(hexa_char_size/2);i++)
    {
      snprintf(tmp,10,"%02X",data[i]&0xff);
      strncat(hexa_char,tmp,hexa_char_size);
    }
    strncat(hexa_char,"\0",hexa_char_size);
    return hexa_char;
  }else return NULL;
}
//------------------------------------------------------------------------------
char *extractFileFromPath(char *path, char *file, unsigned int file_size_max)
{
  char *c = path;
  file[0] = 0;

  while(*c++);
  while(*c!='\\' && c>path)c--;
  c++;
  strncpy(file,c,file_size_max);
  return file;
}
//------------------------------------------------------------------------------
char *extractExtFromFile(char *file, char *ext, unsigned int ext_size_max)
{
  char *c = file;
  ext[0]  = 0;

  while(*c++);
  while(*c!='.' && c>file)c--;
  if (*c == '.')
  {
    c++;
    strncpy(ext,c,ext_size_max);
    return ext;
  }else return NULL;
}
//------------------------------------------------------------------------------
BOOL isDirectory(char *path)
{
  DWORD attr = GetFileAttributes(path);
  if (attr & FILE_ATTRIBUTE_DIRECTORY) return TRUE;
  else return FALSE;
}
//------------------------------------------------------------------------------
BOOL startWith(char* txt, char *search)
{
  char *t = txt, *s = search;

  while (*t && *s && *t == *s){t++;s++;}
  if (*t == *s || *s == 0)return TRUE;
  return FALSE;
}