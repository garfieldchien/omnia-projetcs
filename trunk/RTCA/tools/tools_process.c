//------------------------------------------------------------------------------
// Projet RtCA          : Read to Catch All
// Auteur               : Nicolas Hanteville
// Site                 : http://code.google.com/p/omnia-projetcs/
// Licence              : GPL V3
//------------------------------------------------------------------------------
#include "../RtCA.h"
//------------------------------------------------------------------------------
//extract icon from process
int __stdcall GetIconProcess(HANDLE hlv,char * szProcessPath)
{
    SHFILEINFO sfiInfo;
    HIMAGELIST hilPicture = (HIMAGELIST) SHGetFileInfo(szProcessPath,0,&sfiInfo,sizeof(SHFILEINFO),SHGFI_SMALLICON | SHGFI_SYSICONINDEX);
    ListView_SetImageList(hlv,hilPicture,LVSIL_SMALL);
    return sfiInfo.iIcon;
}
//------------------------------------------------------------------------------
//Functions for DLL injection from : http://www.cppfrance.com//code.aspx?ID=49419
#define W_MAX_PATH MAX_PATH*sizeof(WCHAR)
BOOL WINAPI DLLInjecteurW(DWORD dwPid,PWSTR szDLLPath)
{
	LPTHREAD_START_ROUTINE lpthThreadFunction; /* Pointeur de fonction. */
	/* Recherche de l'adresse de LoadLibraryW dans kernel32. */
    lpthThreadFunction = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryW"); /* GetProcAdress renvoie l'adresse de la fonction LoadLibrary. */
    if(lpthThreadFunction == NULL){return FALSE;}

	HANDLE hProcess;
	/* R�cup�ration du handle du processus cible. */
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE,dwPid);
    if(hProcess == NULL)return FALSE;

	/* Allocation m�moire dans le processus cible. */
    PWSTR szDllVariable;
    szDllVariable = (PWSTR) VirtualAllocEx(hProcess, NULL, (lstrlenW(szDLLPath)+1)*sizeof(WCHAR), MEM_COMMIT, PAGE_READWRITE);
    if(szDllVariable == NULL){ CloseHandle(hProcess); return FALSE; }

	/* Ecriture de l'adresse de la dll dans la m�moire du processus cible*/
    DWORD dwBytes;
    if(!WriteProcessMemory(hProcess, szDllVariable, szDLLPath,(lstrlenW(szDLLPath)+1)*sizeof(WCHAR), &dwBytes)){ CloseHandle(hProcess); return FALSE; }
    if(dwBytes != (lstrlenW(szDLLPath)+1)*sizeof(WCHAR))
	{
		VirtualFreeEx(hProcess,szDllVariable,0,MEM_RELEASE);
		return FALSE;
	}

	/* Cr�ation du thread dans le processus cible. */
    DWORD dwThreadID = 0;
    HANDLE hThread = NULL;
    hThread = CreateRemoteThread(hProcess, NULL, 0, lpthThreadFunction,szDllVariable, 0, &dwThreadID);
    if(hThread == NULL){
		VirtualFreeEx(hProcess,szDllVariable,0,MEM_RELEASE);
		return FALSE;
	}

    WaitForSingleObject(hThread, INFINITE); /* Attente de la fin du thread. */

    VirtualFreeEx(hProcess,szDllVariable,0,MEM_RELEASE);
    CloseHandle(hProcess);
    CloseHandle(hThread);

	return TRUE;
}
//------------------------------------------------------------------------------
BOOL WINAPI DLLEjecteurW(DWORD dwPid,PWSTR szDLLPath)
{
	/* Recherche de l'adresse du module dans le processus cible. */
	MODULEENTRY32W meModule;
	meModule.dwSize = sizeof(meModule);
	HANDLE hSnapshot = NULL;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPid);
	if(hSnapshot == NULL)return FALSE;

	/* Parcour de la liste des modules du processus cible. */
	Module32FirstW(hSnapshot, &meModule);
	do{
		if((lstrcmpiW(meModule.szModule,szDLLPath) == 0) || (lstrcmpiW(meModule.szExePath,szDLLPath) == 0))break;
	}while(Module32NextW(hSnapshot, &meModule));

	/* R�cup�ration du handle du processus. */
	HANDLE hProcess;

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE,dwPid);
    if(hProcess == NULL){
		CloseHandle(hSnapshot);
		return FALSE;
	}

    LPTHREAD_START_ROUTINE lpthThreadFunction; /* Pointeur de fonction. */
    /* R�cup�ration de l'adresse de FreeLibrary dans kernel32.dll . */
	lpthThreadFunction = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("kernel32.dll"), "FreeLibrary"); /* GetProcAdress renvoie l'adresse de la fonction LoadLibrary. */
    if(lpthThreadFunction == NULL){
		CloseHandle(hProcess);
		CloseHandle(hSnapshot);
		return FALSE;
	}
    /* Remarque : ici pas besoin de cr�e une variable contenant le path de la dll dans le processus cible
     * �tant donn� que la dll a �t� charg�e, il suffit de r�cup�rer l'adresse de celle-ci
     * dans les modules charg�s par le processus cible. (fait ci-dessus gr�ce � CreateToolhelp32Snapshot, ... )
     */

    /* Cr�ation du thread dans le processus cible. */
    DWORD dwThreadID = 0;
    HANDLE hThread = NULL;
    hThread = CreateRemoteThread(hProcess, NULL, 0, lpthThreadFunction,meModule.modBaseAddr, 0, &dwThreadID);
    if(hThread == NULL){
		CloseHandle(hSnapshot);
		CloseHandle(hProcess);
		return FALSE;
	}

	WaitForSingleObject(hThread,INFINITE);

	CloseHandle(hProcess);
	CloseHandle(hThread);

	return TRUE;
}
//------------------------------------------------------------------------------
BOOL WINAPI DllInjecteurA(DWORD dwPid,char * szDLLPath){

	WCHAR wszDllPath[W_MAX_PATH];

	MultiByteToWideChar((UINT)CP_ACP,(DWORD)MB_PRECOMPOSED,(LPSTR)szDLLPath,(int)-1,(LPWSTR)wszDllPath,(int)W_MAX_PATH);

	return (DLLInjecteurW(dwPid,wszDllPath));
}
//------------------------------------------------------------------------------
BOOL WINAPI DllEjecteurA(DWORD dwPid,char * szDLLPath){

	WCHAR wszDllPath[W_MAX_PATH];

	MultiByteToWideChar((UINT)CP_ACP,(DWORD)MB_PRECOMPOSED,(LPSTR)szDLLPath,(int)-1,(LPWSTR)wszDllPath,(int)W_MAX_PATH);

	return (DLLEjecteurW(dwPid,wszDllPath));
}
//------------------------------------------------------------------------------
//kill process
//------------------------------------------------------------------------------
BOOL KilllvProcess(DWORD pid)
{
  //kill
  HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
  if (hProcess != NULL)
  {
    if (TerminateProcess(hProcess, 0))
    {
      CloseHandle(hProcess);
      return TRUE;
    }
    CloseHandle(hProcess);
  }
  return FALSE;
}
//------------------------------------------------------------------------------
void EnumProcessAndThread_Current(HANDLE hlv, DWORD first_id, DWORD end_id)
{
  //enumerate all threads
  unsigned int img = 0;
  DWORD cbNeeded, i, k, j, ref_item;
  HANDLE hProcess;
  HMODULE hMod[MAX_PATH];
  FILETIME lpCreationTime, lpExitTime, lpKernelTime, lpUserTime;
  LINE_PROC_ITEM port_line[MAX_PATH];
  BOOL ok;
  char process[DEFAULT_TMP_SIZE],
       pid[DEFAULT_TMP_SIZE],
       path[MAX_PATH],
       cmd[MAX_PATH],
       owner[DEFAULT_TMP_SIZE],
       rid[DEFAULT_TMP_SIZE],
       sid[DEFAULT_TMP_SIZE],
       start_date[DATE_SIZE_MAX];
       /*parent_pid[DEFAULT_TMP_SIZE]="",
       parent_path[MAX_PATH]="";*/

  LVITEM lvi;
  lvi.mask     = LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE;
  lvi.iSubItem = 0;
  lvi.lParam  = LVM_SORTITEMS;
  lvi.pszText = "";

  for (i=first_id;i<end_id;i++) //32768 = 2gb/64k pour la gestion du 64bits
  {
    //process handle
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,0,i);
    if (hProcess!=INVALID_HANDLE_VALUE && hProcess!=NULL)
    {
      //get cmd line
      GetProcessArg(hProcess, cmd, MAX_PATH);
      if (cmd[0] == 0)continue;

      //search if exist or not
      ok = TRUE;
      path[0] = 0;
      j = ListView_GetItemCount(hlv);
      for (k=0;k<j;k++)
      {
        path[0] = 0;
        ListView_GetItemText(hlv,k,3,path,MAX_PATH);
        if (path[0] != 0)
        {
          if (!strcmp(path,cmd))
          {
            ok = FALSE;
            break;
          }
        }
      }

      //not hidden process !!!
      if (!ok)continue;

      //name
      process[0] = 0;
      GetModuleBaseName(hProcess,NULL,process,DEFAULT_TMP_SIZE);

      //pid
      snprintf(pid,DEFAULT_TMP_SIZE,"%05lu",i);

      //path
      path[0]=0;
      if (EnumProcessModules(hProcess,hMod, MAX_PATH,&cbNeeded))
      {
        GetModuleFileNameEx(hProcess,hMod[0],path,MAX_PATH);
      }

      //owner
      GetProcessOwner(i, owner, rid, sid, DEFAULT_TMP_SIZE);

      //start date process
      start_date[0] = 0;
      if (GetProcessTimes(hProcess, &lpCreationTime,&lpExitTime, &lpKernelTime, &lpUserTime))
      {
        //traitement de la date
        if (lpCreationTime.dwHighDateTime != 0 && lpCreationTime.dwLowDateTime != 0)
        {
         filetimeToString_GMT(lpCreationTime, start_date, DATE_SIZE_MAX);
        }
      }

      //ports !
      j=GetPortsFromPID(i, port_line, MAX_PATH, SIZE_ITEMS_PORT_MAX);

      //add items !
      if (j == 0)
      {
        //icon
        if (path[0]=='\\' && path[1]=='?' && path[2]=='?' && path[3]=='\\')img = GetIconProcess(hlv,path+4);
        else img = GetIconProcess(hlv,path);

        lvi.iImage = img;
        lvi.iItem  = ListView_GetItemCount(hlv);

        ref_item = ListView_InsertItem(hlv, &lvi);

        ListView_SetItemText(hlv,ref_item,0,process);
        ListView_SetItemText(hlv,ref_item,1,pid);
        ListView_SetItemText(hlv,ref_item,2,path);
        ListView_SetItemText(hlv,ref_item,3,cmd);
        ListView_SetItemText(hlv,ref_item,4,owner);
        ListView_SetItemText(hlv,ref_item,5,rid);
        ListView_SetItemText(hlv,ref_item,6,sid);
        ListView_SetItemText(hlv,ref_item,7,start_date);
        ListView_SetItemText(hlv,ref_item,8,"");
        ListView_SetItemText(hlv,ref_item,9,"");
        ListView_SetItemText(hlv,ref_item,10,"");
        ListView_SetItemText(hlv,ref_item,11,"");
        ListView_SetItemText(hlv,ref_item,12,"");
        ListView_SetItemText(hlv,ref_item,13,"");
        ListView_SetItemText(hlv,ref_item,14,"X");
        ListView_SetItemText(hlv,ref_item,15,"");
        ListView_SetItemText(hlv,ref_item,16,"");
        ListView_SetItemText(hlv,ref_item,17,"");
        ListView_SetItemText(hlv,ref_item,18,"");
      }else
      {
        for (k=0;k<j;k++)
        {
          //icon
          if (path[0]=='\\' && path[1]=='?' && path[2]=='?' && path[3]=='\\')img = GetIconProcess(hlv,path+4);
          else img = GetIconProcess(hlv,path);
          lvi.iImage = img;
          lvi.iItem  = ListView_GetItemCount(hlv);

          ref_item = ListView_InsertItem(hlv, &lvi);

          ListView_SetItemText(hlv,ref_item,0,process);
          ListView_SetItemText(hlv,ref_item,1,pid);
          ListView_SetItemText(hlv,ref_item,2,path);
          ListView_SetItemText(hlv,ref_item,3,cmd);
          ListView_SetItemText(hlv,ref_item,4,owner);
          ListView_SetItemText(hlv,ref_item,5,rid);
          ListView_SetItemText(hlv,ref_item,6,sid);
          ListView_SetItemText(hlv,ref_item,7,start_date);
          ListView_SetItemText(hlv,ref_item,8,port_line[k].protocol);
          ListView_SetItemText(hlv,ref_item,9,port_line[k].IP_src);
          ListView_SetItemText(hlv,ref_item,10,port_line[k].Port_src);
          ListView_SetItemText(hlv,ref_item,11,port_line[k].IP_dst);
          ListView_SetItemText(hlv,ref_item,12,port_line[k].Port_dst);
          ListView_SetItemText(hlv,ref_item,13,port_line[k].state);
          ListView_SetItemText(hlv,ref_item,14,"X");
          ListView_SetItemText(hlv,ref_item,15,"");
          ListView_SetItemText(hlv,ref_item,16,"");
          ListView_SetItemText(hlv,ref_item,17,"");
          ListView_SetItemText(hlv,ref_item,18,"");
        }
      }
      CloseHandle(hProcess);
    }
  }
}
//------------------------------------------------------------------------------
void LoadPRocessList(HWND hlv)
{
  ListView_DeleteAllItems(hlv);

  PROCESSENTRY32 pe = {sizeof(PROCESSENTRY32)};
  HANDLE hCT = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS|TH32CS_SNAPTHREAD, 0);
  if (hCT==INVALID_HANDLE_VALUE)return;

  unsigned int img = 0;
  DWORD cbNeeded, k, j, /*nb_process=0,*/ ref_item;
  HANDLE hProcess, parent_hProcess;
  HMODULE hMod[MAX_PATH];
  FILETIME lpCreationTime, lpExitTime, lpKernelTime, lpUserTime;
  LINE_PROC_ITEM port_line[MAX_PATH];
  char process[DEFAULT_TMP_SIZE],
       pid[DEFAULT_TMP_SIZE],
       path[MAX_PATH],
       cmd[MAX_PATH],
       owner[DEFAULT_TMP_SIZE],
       rid[DEFAULT_TMP_SIZE],
       sid[DEFAULT_TMP_SIZE],
       start_date[DATE_SIZE_MAX],
       parent_pid[DEFAULT_TMP_SIZE],
       parent_path[MAX_PATH];

  LVITEM lvi;
  lvi.mask     = LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE;
  lvi.iSubItem = 0;
  lvi.lParam   = LVM_SORTITEMS;
  lvi.pszText  = "";

  while(Process32Next(hCT, &pe))
  {
    //open process info
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,0,pe.th32ProcessID);
    if (hProcess == NULL)continue;

    //process
    process[0] = 0;
    strncpy(process,pe.szExeFile,DEFAULT_TMP_SIZE);

    //pid
    snprintf(pid,DEFAULT_TMP_SIZE,"%05lu",pe.th32ProcessID);

    //path
    path[0]=0;
    if (EnumProcessModules(hProcess,hMod, MAX_PATH,&cbNeeded))
    {
      if (GetModuleFileNameEx(hProcess,hMod[0],path,MAX_PATH) == 0)path[0] = 0;
    }

    //cmd
    cmd[0]=0;
    GetProcessArg(hProcess, cmd, MAX_PATH);

    //owner
    GetProcessOwner(pe.th32ProcessID, owner, rid, sid, DEFAULT_TMP_SIZE);

    //parent processID
    snprintf(parent_pid,DEFAULT_TMP_SIZE,"%05lu",pe.th32ParentProcessID);

    //parent name
    parent_path[0]=0;
    parent_hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,0,pe.th32ParentProcessID);
    if (parent_hProcess != NULL)
    {
      if (EnumProcessModules(parent_hProcess,hMod, MAX_PATH,&cbNeeded))
      {
        if (GetModuleFileNameEx(parent_hProcess,hMod[0],parent_path,MAX_PATH) == 0)parent_path[0] = 0;
      }
      CloseHandle(parent_hProcess);
    }

    //start date process
    start_date[0] = 0;
    if (GetProcessTimes(hProcess, &lpCreationTime,&lpExitTime, &lpKernelTime, &lpUserTime))
    {
      //traitement de la date
      if (lpCreationTime.dwHighDateTime != 0 && lpCreationTime.dwLowDateTime != 0)
      {
       filetimeToString_GMT(lpCreationTime, start_date, DATE_SIZE_MAX);
      }
    }

    //ports !
    j=GetPortsFromPID(pe.th32ProcessID, port_line, MAX_PATH, SIZE_ITEMS_PORT_MAX);

    //add items !
    if (j == 0)
    {
      //icon
      if (path[0]=='\\' && path[1]=='?' && path[2]=='?' && path[3]=='\\')img = GetIconProcess(hlv,path+4);
      else img = GetIconProcess(hlv,path);
      lvi.iImage = img;
      lvi.iItem  = ListView_GetItemCount(hlv);

      ref_item = ListView_InsertItem(hlv, &lvi);

      ListView_SetItemText(hlv,ref_item,0,process);
      ListView_SetItemText(hlv,ref_item,1,pid);
      ListView_SetItemText(hlv,ref_item,2,path);
      ListView_SetItemText(hlv,ref_item,3,cmd);
      ListView_SetItemText(hlv,ref_item,4,owner);
      ListView_SetItemText(hlv,ref_item,5,rid);
      ListView_SetItemText(hlv,ref_item,6,sid);
      ListView_SetItemText(hlv,ref_item,7,start_date);
      ListView_SetItemText(hlv,ref_item,8,"");
      ListView_SetItemText(hlv,ref_item,9,"");
      ListView_SetItemText(hlv,ref_item,10,"");
      ListView_SetItemText(hlv,ref_item,11,"");
      ListView_SetItemText(hlv,ref_item,12,"");
      ListView_SetItemText(hlv,ref_item,13,"");
      ListView_SetItemText(hlv,ref_item,14,"");
      ListView_SetItemText(hlv,ref_item,15,parent_path);
      ListView_SetItemText(hlv,ref_item,16,parent_pid);
      ListView_SetItemText(hlv,ref_item,17,"");
      ListView_SetItemText(hlv,ref_item,18,"");
    }else
    {
      for (k=0;k<j;k++)
      {
        //icon
        if (path[0]=='\\' && path[1]=='?' && path[2]=='?' && path[3]=='\\')img = GetIconProcess(hlv,path+4);
        else img = GetIconProcess(hlv,path);
        lvi.iImage = img;
        lvi.iItem  = ListView_GetItemCount(hlv);

        ref_item = ListView_InsertItem(hlv, &lvi);

        ListView_SetItemText(hlv,ref_item,0,process);
        ListView_SetItemText(hlv,ref_item,1,pid);
        ListView_SetItemText(hlv,ref_item,2,path);
        ListView_SetItemText(hlv,ref_item,3,cmd);
        ListView_SetItemText(hlv,ref_item,4,owner);
        ListView_SetItemText(hlv,ref_item,5,rid);
        ListView_SetItemText(hlv,ref_item,6,sid);
        ListView_SetItemText(hlv,ref_item,7,start_date);
        ListView_SetItemText(hlv,ref_item,8,port_line[k].protocol);
        ListView_SetItemText(hlv,ref_item,9,port_line[k].IP_src);
        ListView_SetItemText(hlv,ref_item,10,port_line[k].Port_src);
        ListView_SetItemText(hlv,ref_item,11,port_line[k].IP_dst);
        ListView_SetItemText(hlv,ref_item,12,port_line[k].Port_dst);
        ListView_SetItemText(hlv,ref_item,13,port_line[k].state);
        ListView_SetItemText(hlv,ref_item,14,"");
        ListView_SetItemText(hlv,ref_item,15,parent_path);
        ListView_SetItemText(hlv,ref_item,16,parent_pid);
        ListView_SetItemText(hlv,ref_item,17,"");
        ListView_SetItemText(hlv,ref_item,18,"");
      }
    }
    CloseHandle(hProcess);
  }

  //shadow process !!!
  EnumProcessAndThread_Current(hlv, 1, 32768);
  CloseHandle(hCT);
}
//------------------------------------------------------------------------------
BOOL GetFileInfos(char *file, char *size, unsigned int max_size, char *CreationTime, char *LastWriteTime, DWORD date_size)
{
  //File property
  size[0]         = 0;
  CreationTime[0] = 0;
  LastWriteTime[0]= 0;
  BOOL ret = FALSE;
  WIN32_FIND_DATA data;
  HANDLE hfic = FindFirstFile(file, &data);
  if (hfic != INVALID_HANDLE_VALUE)
  {
    //taille
    if (data.nFileSizeLow>(1024*1024*1024))snprintf(size,max_size,"%luGo (%luo)",data.nFileSizeLow/(1024*1024*1024),data.nFileSizeLow);
    else if (data.nFileSizeLow>(1024*1024))snprintf(size,max_size,"%luMo (%luo)",data.nFileSizeLow/(1024*1024),data.nFileSizeLow);
    else if (data.nFileSizeLow>(1024))snprintf(size,max_size,"%luKo (%luo)",data.nFileSizeLow/(1024),data.nFileSizeLow);
    else snprintf(size,max_size,"%luo",data.nFileSizeLow);

    filetimeToString_GMT(data.ftCreationTime, CreationTime, date_size);
    filetimeToString_GMT(data.ftLastWriteTime, LastWriteTime, date_size);
    ret = TRUE;
  }
  FindClose(hfic);
  return ret;
}
//----------------------------------------------------------------------
void FileInfoRead(char *file, char *ProductName, char *FileVersion, char *CompanyName, char *FileDescription, DWORD size_max)
{
  ProductName[0]=0;
  FileVersion[0]=0;
  CompanyName[0]=0;
  FileDescription[0]=0;

  HINSTANCE hDLL;
  if((hDLL = LoadLibrary("VERSION.DLL" ))!= NULL)
  {
    typedef BOOL (WINAPI * GETFILEVERSIONINFO)(LPCTSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);
    typedef BOOL (WINAPI * VERQUERYVALUE)(LPCVOID pBlock, LPCTSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen);

    GETFILEVERSIONINFO GetFileVersionInfo      = (GETFILEVERSIONINFO)   GetProcAddress(hDLL , "GetFileVersionInfoA");
    VERQUERYVALUE VerQueryValue                = (VERQUERYVALUE)   GetProcAddress(hDLL , "VerQueryValueA");
    if (GetFileVersionInfo && VerQueryValue)
    {
      //chargement des infos
      char buffer[MAX_LINE_SIZE];
      if (GetFileVersionInfo(file, 0, MAX_LINE_SIZE, (LPVOID) buffer) > 0 )
      {
        //lecture des infos du buffer
         WORD             *d_buffer;
         char             *c;
         unsigned int     size;

         //v�rification si bien une table + lecture de la langue
         if (VerQueryValue(buffer, "\\VarFileInfo\\Translation", (LPVOID *)&d_buffer, &size))
         {
           if (size>0 && d_buffer != NULL)
           {
            //g�n�ration du d�but de string :
            char v_string[MAX_PATH],t_string[MAX_PATH];
            /*char s_string[][]={"CompanyName"    , "FileDescription", "FileVersion",
                                 "InternalName"   , "LegalCopyright" , "OriginalFilename",
                                 "ProductName"    , "ProductVersion"};*/
            snprintf(v_string,MAX_PATH,"\\StringFileInfo\\%04x%04x\\", d_buffer[0], d_buffer[1]);

            //lecture de ProductName
            if (ProductName != NULL)
            {
              snprintf(t_string,MAX_PATH,"%sProductName",v_string);
              if (VerQueryValue(buffer, t_string, (LPVOID *)&c, &size))
              {
                if (size>0 && c!= NULL)strncpy(ProductName,c,size_max);
              }
            }

            //lecture de FileVersion
            if (FileVersion != NULL)
            {
              snprintf(t_string,MAX_PATH,"%sFileVersion",v_string);
              if (VerQueryValue(buffer, t_string, (LPVOID *)&c, &size))
              {
                if (size>0 && c!= NULL)strncpy(FileVersion,c,size_max);
              }
            }

            //lecture du CompanyName
            if (CompanyName != NULL)
            {
              snprintf(t_string,MAX_PATH,"%sCompanyName",v_string);
              if (VerQueryValue(buffer, t_string, (LPVOID *)&c, &size))
              {
                if (size>0 && c!= NULL)strncpy(CompanyName,c,size_max);
              }
            }

            //lecture du FileDescription
            if (FileDescription != NULL)
            {
              snprintf(t_string,MAX_PATH,"%sFileDescription",v_string);
              if (VerQueryValue(buffer, t_string, (LPVOID *)&c, &size))
              {
                if (size>0 && c!= NULL)strncpy(FileDescription,c,size_max);
              }
            }
           }
         }
      }
    }
    FreeLibrary(hDLL);
  }
}
//------------------------------------------------------------------------------
DWORD WINAPI ThreadGetProcessInfos(LPVOID lParam)
{
  long nitem = SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
  if (nitem > -1)
  {
    //allow 10mo memory (max data ok)
    char *buffer = (char *)LocalAlloc(LMEM_FIXED, DIXM);
    if (buffer !=0)
    {
      //get process simple infos
      char tmp[MAX_LINE_SIZE];
      char SEPARATOR[] = "---------------------------------------------\r\n";
      DWORD i;
      LVCOLUMN lvc;
      lvc.mask        = LVCF_TEXT;
      lvc.cchTextMax  = MAX_LINE_SIZE;
      lvc.pszText     = tmp;
      buffer[0]       = 0;

      for (i=0;i<NB_PROCESS_COLUMN;i++)
      {
        tmp[0] = 0;
        SendMessage(hlstv_process,LVM_GETCOLUMN,(WPARAM)i,(LPARAM)&lvc);
        strncat(buffer,tmp,DIXM);
        strncat(buffer,": ",DIXM);
        tmp[0] = 0;
        ListView_GetItemText(hlstv_process,nitem,i,tmp,MAX_LINE_SIZE);
        strncat(buffer,tmp,DIXM);
        strncat(buffer,"\r\n\0",DIXM);
      }
      strncat(buffer,SEPARATOR,DIXM);

      //get real path
      char path[MAX_PATH]="";
      ListView_GetItemText(hlstv_process,nitem,2,tmp,MAX_LINE_SIZE);
      if (tmp[0]=='\\' && tmp[1]=='?' && tmp[2]=='?' && tmp[3]=='\\')
      {
        strncpy(path,tmp+4,MAX_PATH);
      }else if (tmp[0]=='\\' && (tmp[1]=='S' || tmp[1]=='s') && tmp[2]=='y' && tmp[3]=='s' && tmp[4]=='t' && tmp[5]=='e' && tmp[6]=='m' && (tmp[7]=='R' || tmp[7]=='r')) //SystemRoot
      {
        strncpy(path,tmp+1,MAX_PATH); //passe le '\\'
        ReplaceEnv("SystemROOT", path, MAX_PATH);
      }else strncpy(path,tmp,MAX_PATH);

      //get pid
      tmp[0] = 0;
      ListView_GetItemText(hlstv_process,nitem,1,tmp,MAX_LINE_SIZE);
      DWORD pid = atol(tmp);

      //binary infos
      char tmp1[MAX_PATH], tmp2[MAX_PATH], tmp3[MAX_PATH];
      GetFileInfos(path, tmp, MAX_PATH, tmp1, tmp2, MAX_PATH);
      strncat(buffer,"Size: ",DIXM);
      strncat(buffer,tmp,DIXM);
      strncat(buffer,"\r\nFile create time: ",DIXM);
      strncat(buffer,tmp1,DIXM);
      strncat(buffer,"\r\nFile last update time: ",DIXM);
      strncat(buffer,tmp2,DIXM);

      GetACLS(path, tmp, tmp1, tmp2, tmp3, MAX_PATH);
      strncat(buffer,"\r\nACL Owner: ",DIXM);
      strncat(buffer,tmp1,DIXM);
      strncat(buffer," (",DIXM);
      strncat(buffer,tmp3,DIXM);
      strncat(buffer,")\r\nACLs: ",DIXM);
      strncat(buffer,tmp,DIXM);
      strncat(buffer,"\r\n\0",DIXM);
      strncat(buffer,SEPARATOR,DIXM);

      //file binary informations
      FileInfoRead(path, tmp, tmp1, tmp2, tmp3, MAX_PATH);
      strncat(buffer,"ProductName: ",DIXM);
      strncat(buffer,tmp,DIXM);
      strncat(buffer,"\r\nFileVersion: ",DIXM);
      strncat(buffer,tmp1,DIXM);
      strncat(buffer,"\r\nCompanyName: ",DIXM);
      strncat(buffer,tmp2,DIXM);
      strncat(buffer,"\r\nFileDescription: ",DIXM);
      strncat(buffer,tmp3,DIXM);
      strncat(buffer,"\r\n\0",DIXM);
      strncat(buffer,SEPARATOR,DIXM);

      //dll infos
      HMODULE hMod[MAX_LINE_SIZE];
      DWORD cbNeeded = 0, j;
      HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,0, pid);
      if (hProcess != NULL)
      {
        //chargement de la liste des dll du processus
        if (EnumProcessModules(hProcess, hMod, MAX_LINE_SIZE,&cbNeeded))
        {
          strncat(buffer,"DLL dependency:\r\n",DIXM);

          for ( j = 1; j < (cbNeeded / sizeof(HMODULE)) && j< MAX_LINE_SIZE; j++)
          {
            //emplacement de la dll
            tmp[0]=0;
            if (GetModuleFileNameEx(hProcess,hMod[j],tmp,MAX_LINE_SIZE)>0)
            {
              strncat(buffer,tmp,DIXM);
              strncat(buffer,"\r\n\0",DIXM);
            }
          }
        }
        CloseHandle(hProcess);
      }

      //set text
      strncat(buffer,"\0",DIXM);
      SetWindowText(hdbclk_info_process, buffer);
      ShowWindow (hdbclk_info_process, SW_SHOW);

      LocalFree((HLOCAL)buffer);
    }
  }
  return 0;
}
//------------------------------------------------------------------------------
BOOL CALLBACK DialogProc_info(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch(message)
  {
    case WM_SIZE:
    {
      unsigned int mWidth  = LOWORD(lParam);
      unsigned int mHeight = HIWORD(lParam);
      MoveWindow(hlstv_process,5,0,mWidth-10,mHeight-5,TRUE);

      //column resise
      if(nb_column_process_view)
      {
        //column resise
        unsigned int i;
        if (nb_column_process_view)
        {
          DWORD column_sz = (mWidth-40)/nb_column_process_view;
          for (i=0;i<nb_column_process_view;i++)
          {
            redimColumnH(hlstv_process,i,column_sz);
          }
        }
      }
    }
    break;
    case WM_COMMAND:
      switch (HIWORD(wParam))
      {
        case BN_CLICKED:
          switch(LOWORD(wParam))
          {
            case POPUP_H_00:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_01:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_02:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_03:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_04:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_05:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_06:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_07:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_08:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_09:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_10:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_11:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_12:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_13:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_14:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_15:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_16:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_17:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_18:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            case POPUP_H_19:if(ListView_GetColumnWidth(hlstv_process,LOWORD(wParam)-POPUP_H_00) > 20)redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,0);else redimColumnH(hlstv_process,LOWORD(wParam)-POPUP_H_00,50);break;
            //-----------------------------------------------------
            case POPUP_I_00:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 0);break;
            case POPUP_I_01:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 1);break;
            case POPUP_I_02:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 2);break;
            case POPUP_I_03:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 3);break;
            case POPUP_I_04:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 4);break;
            case POPUP_I_05:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 5);break;
            case POPUP_I_06:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 6);break;
            case POPUP_I_07:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 7);break;
            case POPUP_I_08:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 8);break;
            case POPUP_I_09:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 9);break;
            case POPUP_I_10:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 10);break;
            case POPUP_I_11:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 11);break;
            case POPUP_I_12:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 12);break;
            case POPUP_I_13:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 13);break;
            case POPUP_I_14:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 14);break;
            case POPUP_I_15:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 15);break;
            case POPUP_I_16:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 16);break;
            case POPUP_I_17:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 17);break;
            case POPUP_I_18:CopyDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), 18);break;
            case POPUP_CP_LINE:CopyAllDataToClipboard(hlstv_process, SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED), NB_PROCESS_COLUMN);break;
            //-----------------------------------------------------
            case POPUP_S_VIEW:
            {
              char file[MAX_PATH]="process_history";
              OPENFILENAME ofn;
              ZeroMemory(&ofn, sizeof(OPENFILENAME));
              ofn.lStructSize  = sizeof(OPENFILENAME);
              ofn.hwndOwner    = h_process;
              ofn.lpstrFile    = file;
              ofn.nMaxFile     = MAX_PATH;
              ofn.lpstrFilter  ="*.csv \0*.csv\0*.xml \0*.xml\0*.html \0*.html\0";
              ofn.nFilterIndex = 1;
              ofn.Flags        = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
              ofn.lpstrDefExt  =".csv\0";
              if (GetSaveFileName(&ofn)==TRUE)
              {
                SaveLSTV(hlstv_process, file, ofn.nFilterIndex, NB_PROCESS_COLUMN);
                SendMessage(hstatus_bar,SB_SETTEXT,0, (LPARAM)"Export done !!!");
              }
            }
            break;
            //-----------------------------------------------------
            case POPUP_OPEN_PATH:
            {
              char path[MAX_PATH]="";
              ListView_GetItemText(hlstv_process,SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED),2,path,MAX_PATH);
              if (path[0]!=0)
              {
                //get path of file
                char *c = path;
                while (*c++);
                while (*c != '\\' && *c != '/')c--;
                c++;
                *c=0;

                if (path[1]=='?')
                {
                  c = path;
                  c = c+4;
                  ShellExecute(h_process, "explore", c, NULL,NULL,SW_SHOW);
                }else if (path[0]=='\\' || path[0]=='/')
                {
                  path[0]='%';
                  char *c = path;
                  unsigned int i=0;
                  while (*c != '\\' && *c != '/' && *c){c++;i++;}
                  if (*c == '\\' || *c == '/')
                  {
                    char tmp_path[MAX_PATH]="";
                    strncpy(tmp_path,path,MAX_PATH);
                    tmp_path[i]= '%';
                    tmp_path[i+1]= 0;
                    strncat(tmp_path,c,MAX_PATH);
                    strncat(tmp_path,"\0",MAX_PATH);
                    ShellExecute(h_process, "explore", ReplaceEnv("systemroot", tmp_path, MAX_PATH), NULL,NULL,SW_SHOW);
                  }
                }else ShellExecute(h_process, "explore", path, NULL,NULL,SW_SHOW);
              }
            }
            break;
            //-----------------------------------------------------
            case POPUP_DUMP_MEMORY :
            {
              char cpid[MAX_PATH];
              ListView_GetItemText(hlstv_process,SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED),1,cpid,MAX_PATH);

              CreateThread(NULL,0,DumpProcessMemory,(PVOID)atoi(cpid),0,0);
            }
            break;
            case POPUP_KILL_PROCESS :
            {
              char cpid[MAX_PATH];
              DWORD item_id = SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
              ListView_GetItemText(hlstv_process,item_id,1,cpid,MAX_PATH);
              if (KilllvProcess(atol(cpid)))ListView_DeleteItem(hlstv_process,item_id);
            }
            break;
            //-----------------------------------------------------
            case POPUP_ADD_DLL_INJECT_REMOTE_THREAD:
            case POPUP_REM_DLL_INJECT_REMOTE_THREAD:
            {
              //lecture du pid du processus
              char tmp[MAX_PATH]="";
              ListView_GetItemText(hlstv_process,SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED),1,tmp,MAX_PATH);
              DWORD pid = atoi(tmp);

              //choix de la DLL
              tmp[0]=0;
              OPENFILENAME ofnFile;
              ZeroMemory(&ofnFile,sizeof(OPENFILENAME));
              ofnFile.lStructSize   = sizeof(OPENFILENAME);
              ofnFile.hwndOwner     = h_process;
              ofnFile.lpstrFile     = tmp;
              ofnFile.nMaxFile      = MAX_PATH;
              ofnFile.lpstrFilter   = "dll (*.dll)\0*.dll\0";
              ofnFile.nFilterIndex  = 1;
              ofnFile.lpstrTitle    = "DLL";
              ofnFile.Flags         = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
              if(GetOpenFileName(&ofnFile))
              {
                //injection or not ^^
                if (LOWORD(wParam) == POPUP_ADD_DLL_INJECT_REMOTE_THREAD)DllInjecteurA(pid,tmp);
                else DllEjecteurA(pid,tmp);
              }
            }
            break;
            case POPUP_PROCESS_REFRESH:LoadPRocessList(hlstv_process);break;
            case POPUP_VIRUSTOTAL_CHECK:
            {
              //get path !
              DWORD current_item = SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
              char path[MAX_PATH]="",ok_path[MAX_PATH]="";
              ListView_GetItemText(hlstv_process,current_item,2,path,MAX_PATH);
              if (path[0]!=0)
              {
                //get path of file
                char *c = path;
                if (path[1]=='?')
                {
                  c = path;
                  c = c+4;
                  strncpy(ok_path,c,MAX_PATH);
                }else if (path[0]=='\\' || path[0]=='/')
                {
                  path[0]='%';
                  char *c = path;
                  unsigned int i=0;
                  while (*c != '\\' && *c != '/' && *c){c++;i++;}
                  if (*c == '\\' || *c == '/')
                  {
                    char tmp_path[MAX_PATH]="";
                    strncpy(tmp_path,path,MAX_PATH);
                    tmp_path[i]= '%';
                    tmp_path[i+1]= 0;
                    strncat(tmp_path,c,MAX_PATH);
                    strncat(tmp_path,"\0",MAX_PATH);
                    strncpy(ok_path,ReplaceEnv("systemroot", tmp_path, MAX_PATH),MAX_PATH);
                  }
                }else strncpy(ok_path,path,MAX_PATH);

                //get sha256
                char s_sha[65]="";
                FileToSHA256(ok_path, s_sha);
                if (s_sha[0] != 0)
                {
                  ListView_SetItemText(hlstv_process,current_item,18,s_sha);

                  MessageBox(0,s_sha,ok_path,MB_OK|MB_TOPMOST);

                  //get VirusTotal Datas
                  CheckItemToVirusTotal(hlstv_process, current_item, 18, 18, NULL, FALSE);
                }
              }
            }
            break;
            case POPUP_VIRUSTOTAL_CHECK_ALL:CreateThread(NULL,0,CheckAllFileToVirusTotalProcess,0,0,0);break;
          }
        break;
      }
    break;
    case WM_CONTEXTMENU:
      if (ListView_GetItemCount(hlstv_process) > 0 && (HWND)wParam == hlstv_process)
      {
        if (disable_p_context)
        {
          disable_p_context = FALSE;
          break;
        }

        HMENU hmenu;
        if ((hmenu = LoadMenu(hinst, MAKEINTRESOURCE(POPUP_LSTV_PROCESS)))!= NULL)
        {
          //set text !!!

          ModifyMenu(hmenu,POPUP_PROCESS_REFRESH  ,MF_BYCOMMAND|MF_STRING,POPUP_PROCESS_REFRESH   ,cps[TXT_POPUP_REFRESH].c);
          ModifyMenu(hmenu,POPUP_S_VIEW           ,MF_BYCOMMAND|MF_STRING,POPUP_S_VIEW            ,cps[TXT_POPUP_S_VIEW].c);
          ModifyMenu(hmenu,POPUP_S_SELECTION      ,MF_BYCOMMAND|MF_STRING,POPUP_S_SELECTION       ,cps[TXT_POPUP_S_SELECTION].c);
          ModifyMenu(hmenu,POPUP_OPEN_PATH        ,MF_BYCOMMAND|MF_STRING,POPUP_OPEN_PATH         ,cps[TXT_OPEN_PATH].c);
          ModifyMenu(hmenu,POPUP_KILL_PROCESS     ,MF_BYCOMMAND|MF_STRING,POPUP_KILL_PROCESS      ,cps[TXT_KILL_PROCESS].c);
          ModifyMenu(hmenu,POPUP_DUMP_MEMORY      ,MF_BYCOMMAND|MF_STRING,POPUP_DUMP_MEMORY       ,cps[TXT_DUMP_PROC_MEM].c);

          ModifyMenu(hmenu,POPUP_ADD_DLL_INJECT_REMOTE_THREAD ,MF_BYCOMMAND|MF_STRING,POPUP_ADD_DLL_INJECT_REMOTE_THREAD ,cps[TXT_ADD_THREAD_INJECT_DLL].c);
          ModifyMenu(hmenu,POPUP_REM_DLL_INJECT_REMOTE_THREAD ,MF_BYCOMMAND|MF_STRING,POPUP_REM_DLL_INJECT_REMOTE_THREAD ,cps[TXT_REM_THREAD_INJECT_DLL].c);

          ModifyMenu(GetSubMenu(hmenu, 0),POPUP_DLL_INJECT ,MF_BYPOSITION|MF_STRING,POPUP_DLL_INJECT ,cps[TXT_POPUP_DLLINJECT].c);

          ModifyMenu(hmenu,POPUP_CP_LINE          ,MF_BYCOMMAND|MF_STRING ,POPUP_CP_LINE          ,cps[TXT_POPUP_CP_LINE].c);
          ModifyMenu(GetSubMenu(hmenu, 0),POPUP_PROCESS_COPY_TO_CLIPBORD ,MF_BYPOSITION|MF_STRING,POPUP_PROCESS_COPY_TO_CLIPBORD ,cps[TXT_POPUP_CLIPBORAD].c);

          //load column text
          char buffer[DEFAULT_TMP_SIZE]="";
          LVCOLUMN lvc;
          lvc.mask = LVCF_TEXT;
          lvc.cchTextMax = DEFAULT_TMP_SIZE;
          lvc.pszText = buffer;

          unsigned int i=0;
          while (SendMessage(hlstv_process,LVM_GETCOLUMN,(WPARAM)i,(LPARAM)&lvc))
          {
            ModifyMenu(hmenu,POPUP_I_00+i,MF_BYCOMMAND|MF_STRING,POPUP_I_00+i,buffer);

            //reinit
            buffer[0] = 0;
            lvc.mask = LVCF_TEXT;
            lvc.cchTextMax = DEFAULT_TMP_SIZE;
            lvc.pszText = buffer;
            i++;
          }

          //verify if path is empty
          char tmp[MAX_PATH]="";
          ListView_GetItemText(hlstv_process,SendMessage(hlstv_process,LVM_GETNEXTITEM,-1,LVNI_FOCUSED),2,tmp,MAX_PATH);
          if (tmp[0]==0)
          {
            RemoveMenu(hmenu,POPUP_VIRUSTOTAL_CHECK,MF_BYCOMMAND);
          }

          //affichage du popup menu
          TrackPopupMenuEx(GetSubMenu(hmenu, 0), 0, LOWORD(lParam), HIWORD(lParam), hwnd, NULL);
          DestroyMenu(hmenu);
        }
      }
    break;
    case WM_NOTIFY:
      switch(((LPNMHDR)lParam)->code)
      {
        // tri of columns
        case LVN_COLUMNCLICK:
          TRI_PROCESS_VIEW = !TRI_PROCESS_VIEW;
          c_Tri(hlstv_process,((LPNMLISTVIEW)lParam)->iSubItem,TRI_PROCESS_VIEW);
        break;
        //popup menu for view column !!
        case NM_RCLICK:
        {
          //src code : http://support.microsoft.com/kb/125694
          //get click pos
          DWORD dwPos = GetMessagePos();
          POINT pointScreen;
          pointScreen.x = LOWORD (dwPos);
          pointScreen.y = HIWORD (dwPos);

          //set to lstv pos
          ScreenToClient (hlstv_process, &pointScreen);
          HANDLE hChildWnd = ChildWindowFromPoint(hlstv_process, pointScreen);

          if (hChildWnd != hlstv_process) //header have been clicked
          {
            //view popup menu
            HMENU hmenu;
            if ((hmenu = LoadMenu(hinst, MAKEINTRESOURCE(POPUP_LSTV_HDR)))!= NULL)
            {
              //load column text
              char buffer[DEFAULT_TMP_SIZE]="";
              LVCOLUMN lvc;
              lvc.mask = LVCF_TEXT|LVCF_WIDTH;
              lvc.cchTextMax = DEFAULT_TMP_SIZE;
              lvc.pszText = buffer;
              lvc.cx = 0;

              unsigned int i=0;
              while (SendMessage(hlstv_process,LVM_GETCOLUMN,(WPARAM)i,(LPARAM)&lvc))
              {
                ModifyMenu(hmenu,POPUP_H_00+i,MF_BYCOMMAND|MF_STRING,POPUP_H_00+i,buffer);
                if(lvc.cx > 20 )CheckMenuItem(hmenu,POPUP_H_00+i,MF_BYCOMMAND|MF_CHECKED);
                else CheckMenuItem(hmenu,POPUP_H_00+i,MF_BYCOMMAND|MF_UNCHECKED);

                //reinit
                buffer[0] = 0;
                lvc.mask = LVCF_TEXT|LVCF_WIDTH;
                lvc.cchTextMax = DEFAULT_TMP_SIZE;
                lvc.pszText = buffer;
                lvc.cx = 0;
                i++;
              }

              //remove other items
              for (;i<NB_POPUP_I;i++)RemoveMenu(hmenu,POPUP_H_00+i,MF_BYCOMMAND);

              //view popup
              TrackPopupMenuEx(GetSubMenu(hmenu, 0), 0, LOWORD(dwPos), HIWORD(dwPos), hwnd, NULL);
              DestroyMenu(hmenu);
            }
            disable_p_context = TRUE;
          }
        }
        break;
        case NM_DBLCLK:
          if (LOWORD(wParam) == LV_VIEW)CreateThread(NULL,0,ThreadGetProcessInfos,NULL,0,0);
        break;

      }
    break;
    case WM_CLOSE : ShowWindow(hwnd, SW_HIDE);break;
  }
  return FALSE;
}
