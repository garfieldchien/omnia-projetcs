//------------------------------------------------------------------------------
// Projet RtCA          : Read to Catch All
// Auteur               : Nicolas Hanteville
// Site                 : http://code.google.com/p/omnia-projetcs/
// Licence              : GPL V3
//------------------------------------------------------------------------------
#include "../RtCA.h"
char tmp_file_firefox[MAX_PATH];
//------------------------------------------------------------------------------
void addFirefoxtoDB(char *file, char *parameter, char *data, char *date, DWORD id_language_description, unsigned int session_id, sqlite3 *db)
{
  char request[REQUEST_MAX_SIZE];
  snprintf(request,REQUEST_MAX_SIZE,
           "INSERT INTO extract_firefox (file,parameter,data,date,id_language_description,session_id) "
           "VALUES(\"%s\",\"%s\",\"%s\",\"%s\",\"%lu\",%d);",
           file,parameter,data,date,id_language_description,session_id);
  if (!CONSOL_ONLY || DEBUG_CMD_MODE)AddDebugMessage("test_firefox", request, "-", MSG_INFO);
  sqlite3_exec(db,request, NULL, NULL, NULL);
}
//------------------------------------------------------------------------------
int callback_sqlite_firefox(void *datas, int argc, char **argv, char **azColName)
{
  FORMAT_CALBAK_READ_INFO *type = datas;
  char tmp[MAX_PATH]="";
  char date[DATE_SIZE_MAX]="";
  unsigned int i,size=0;
  if (type->type > 0 && type->type < nb_sql_FIREFOX)
  {
    //copy datas
    for (i=0;i<argc && MAX_PATH-size > 0;i++)
    {
      if (argv[i] == NULL)continue;

      //date or not ?
      if (strlen(argv[i]) == DATE_SIZE_MAX-1)
      {
        if (argv[i][4] == '/' && argv[i][13] == ':')
        {
          if (strcmp("1970/01/01 01:00:00",argv[i])!=0)strncpy(date,argv[i],DATE_SIZE_MAX);
          continue;
        }
      }
      if (i>0)snprintf(tmp+size,MAX_PATH-size,", %s",convertUTF8toUTF16(argv[i], strlen(argv[i])+1));
      else snprintf(tmp+size,MAX_PATH-size,"%s",convertUTF8toUTF16(argv[i], strlen(argv[i])+1));
      size = strlen(tmp);
    }
    //get datas and write it
    convertStringToSQL(tmp, MAX_PATH);
    addFirefoxtoDB(tmp_file_firefox,sql_FIREFOX[type->type].params,tmp,date,sql_FIREFOX[type->type].test_string_id,current_session_id,db_scan);
  }
  return 0;
}
//------------------------------------------------------------------------------
DWORD WINAPI Scan_firefox_history(LPVOID lParam)
{
  WaitForSingleObject(hsemaphore,INFINITE);
  AddDebugMessage("test_firefox", "Scan Firefox history - START", "OK", MSG_INFO);

  FORMAT_CALBAK_READ_INFO data;
  char tmp_msg[MAX_PATH];

  //get child
  HTREEITEM hitem = (HTREEITEM)SendDlgItemMessage(h_conf,TRV_FILES, TVM_GETNEXTITEM,(WPARAM)TVGN_CHILD, (LPARAM)TRV_HTREEITEM_CONF[FILES_TITLE_APPLI]);
  if (hitem == NULL) //local
  {
    //get path of all profils users
    //HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList
    HKEY CleTmp   = 0;
    if (RegOpenKey(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\",&CleTmp)==ERROR_SUCCESS)
    {
      DWORD i, nbSubKey=0, key_size;
      sqlite3 *db_tmp;
      char tmp_key[MAX_PATH], tmp_key_path[MAX_PATH];
      if (RegQueryInfoKey (CleTmp,0,0,0,&nbSubKey,0,0,0,0,0,0,0)==ERROR_SUCCESS)
      {
        //get subkey
        for(i=0;i<nbSubKey;i++)
        {
          key_size    = MAX_PATH;
          tmp_key[0]  = 0;
          if (RegEnumKeyEx (CleTmp,i,tmp_key,&key_size,0,0,0,0)==ERROR_SUCCESS)
          {
            //generate the key path
            snprintf(tmp_key_path,MAX_PATH,"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\%s\\",tmp_key);

            //get profil path
            if (ReadValue(HKEY_LOCAL_MACHINE,tmp_key_path,"ProfileImagePath",tmp_key, MAX_PATH))
            {
              //verify the path if %systemdrive%
              ReplaceEnv("SYSTEMDRIVE",tmp_key,MAX_PATH);

              //search profil directory
              snprintf(tmp_key_path,MAX_PATH,"%s\\Application Data\\Mozilla\\Firefox\\Profiles\\*.default",tmp_key);
              WIN32_FIND_DATA wfd0;
              HANDLE hfic = FindFirstFile(tmp_key_path, &wfd0);
              if (hfic != INVALID_HANDLE_VALUE)
              {
                if (wfd0.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                  //path to search
                  snprintf(tmp_key_path,MAX_PATH,"%s\\Application Data\\Mozilla\\Firefox\\Profiles\\%s\\*.sqlite",tmp_key,wfd0.cFileName);

                  WIN32_FIND_DATA wfd;
                  HANDLE hfic2 = FindFirstFile(tmp_key_path, &wfd);
                  if (hfic2 != INVALID_HANDLE_VALUE)
                  {
                    do
                    {
                      if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){}else
                      {
                        if(wfd.cFileName[0] == '.' && (wfd.cFileName[1] == 0 || wfd.cFileName[1] == '.')){}
                        else
                        {
                          //test all files
                          snprintf(tmp_file_firefox,MAX_PATH,"%s\\Application Data\\Mozilla\\Firefox\\Profiles\\%s\\%s",tmp_key,wfd0.cFileName,wfd.cFileName);

                          //test to open file
                          if (sqlite3_open(tmp_file_firefox, &db_tmp) == SQLITE_OK)
                          {
                            for (data.type =0;data.type <nb_sql_FIREFOX && start_scan;data.type = data.type+1)
                            {
                              sqlite3_exec(db_tmp, sql_FIREFOX[data.type].sql, callback_sqlite_firefox, &data, NULL);
                            }
                            sqlite3_close(db_tmp);
                            snprintf(tmp_msg,MAX_PATH,"Scan local Firefox file : %s",tmp_file_firefox);

                            AddDebugMessage("test_firefox", tmp_msg, "OK", MSG_INFO);
                          }
                        }
                      }
                    }while(FindNextFile (hfic2,&wfd) && start_scan);
                  }
                }
              }
            }
          }
        }
      }
      RegCloseKey(CleTmp);
    }
  }else
  {
    sqlite3 *db_tmp;

    while(hitem!=NULL)
    {
      //get item txt
      GetTextFromTrv(hitem, tmp_file_firefox, MAX_PATH);
      //test to open file
      if (sqlite3_open(tmp_file_firefox, &db_tmp) == SQLITE_OK)
      {
        for (data.type =0;data.type <nb_sql_FIREFOX && start_scan;data.type = data.type+1)
        {
          sqlite3_exec(db_tmp, sql_FIREFOX[data.type].sql, callback_sqlite_firefox, &data, NULL);
        }
        sqlite3_close(db_tmp);
        snprintf(tmp_msg,MAX_PATH,"Scan Firefox file : %s",tmp_file_firefox);
        AddDebugMessage("test_firefox", tmp_msg, "OK", MSG_INFO);
      }
      hitem = (HTREEITEM)SendDlgItemMessage(h_conf,TRV_FILES, TVM_GETNEXTITEM,(WPARAM)TVGN_NEXT, (LPARAM)hitem);
    }
  }

  AddDebugMessage("test_firefox", "Scan Firefox history - DONE", "OK", MSG_INFO);
  check_treeview(GetDlgItem(h_conf,TRV_TEST), H_tests[(unsigned int)lParam], TRV_STATE_UNCHECK);//db_scan
  ReleaseSemaphore(hsemaphore,1,NULL);
  return 0;
}
