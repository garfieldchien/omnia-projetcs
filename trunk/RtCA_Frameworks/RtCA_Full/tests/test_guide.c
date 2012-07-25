//------------------------------------------------------------------------------
// Projet RtCA          : Read to Catch All
// Auteur               : Nicolas Hanteville
// Site                 : http://code.google.com/p/omnia-projetcs/
// Licence              : GPL V3
//------------------------------------------------------------------------------
#include "../RtCA.h"
//------------------------------------------------------------------------------
void addGuidetoDB(char *file, char *hk,char *key,char *value,char*data,char *data_read, char*title_id, char*description_id, unsigned int ok_id, unsigned int session_id, sqlite3 *db)
{
  char request[REQUEST_MAX_SIZE];
  snprintf(request,REQUEST_MAX_SIZE,
           "INSERT INTO extract_guide (file,hk,key,value,data,data_read,title_id,description_id,ok_id,session_id) "
           "VALUES(\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",%s,%s,%d,%d);",
           file,hk,key,value,data,data_read,title_id,description_id,ok_id,session_id);
  if (!CONSOL_ONLY || DEBUG_CMD_MODE)AddDebugMessage("test_guide", request, "-", MSG_INFO);
  sqlite3_exec(db,request, NULL, NULL, NULL);
}
//------------------------------------------------------------------------------
//test if the os match with os of the test
BOOL OsTypeValide(char *Os)
{
  //all tests
  if (strcmp(Os,GUIDE_REG_OS_ALL) == 0)return TRUE;
  if (strcmp(Os,GUIDE_REG_OS_ALL_ONLY_32b) == 0 && current_OS_BE_64b == FALSE)return TRUE;
  if (strcmp(Os,GUIDE_REG_OS_ALL_ONLY_64b) == 0 && current_OS_BE_64b == TRUE)return TRUE;

  //specific test
  if (strcmp(Os,current_OS) == 0)return TRUE;
  return FALSE;
}
//------------------------------------------------------------------------------
void CheckGuideValue(char *file, BOOL read, char *test, char *hk,char *key,char *value, char *data, char *data_read, char*title_id, char*description_id, unsigned int session_id, sqlite3 *db)
{
  if (read == TRUE && data_read!=0)convertStringToSQL(data_read, REQUEST_MAX_SIZE);

  switch(atoi(test))
  {
    case GUIDE_REG_TEST_IDENTIQUE:
      if (strcmp(data,data_read) == FALSE)addGuidetoDB(file,hk,key,value,data,data_read, title_id, description_id, GUIDE_REG_TEST_OK, session_id, db_scan);
      else addGuidetoDB(file,hk,key,value,data,data_read, title_id, description_id, GUIDE_REG_TEST_NOK, session_id, db_scan);
    break;
    case GUIDE_REG_TEST_CONTIENT:
      if (Contient(data_read,data) == FALSE)addGuidetoDB(file,hk,key,value,data,data_read, title_id, description_id, GUIDE_REG_TEST_OK, session_id, db_scan);
      else addGuidetoDB(file,hk,key,value,data,data_read, title_id, description_id, GUIDE_REG_TEST_NOK, session_id, db_scan);
    break;
    case GUIDE_REG_TEST_EXIST:
      if (read == TRUE)addGuidetoDB(file,hk,key,value,data,data_read, title_id, description_id, GUIDE_REG_TEST_OK, session_id, db_scan);
      else addGuidetoDB(file,hk,key,value,data,data_read, title_id, description_id, GUIDE_REG_TEST_NOK, session_id, db_scan);
    break;
    case GUIDE_REG_TEST_NEXISTPAS:
      if (read == FALSE)addGuidetoDB(file,hk,key,value,data,data_read, title_id, description_id, GUIDE_REG_TEST_OK, session_id, db_scan);
      else addGuidetoDB(file,hk,key,value,data,data_read, title_id, description_id, GUIDE_REG_TEST_NOK, session_id, db_scan);
    break;
  }
}
//------------------------------------------------------------------------------
//local function part !!!
//------------------------------------------------------------------------------
//read current OS
BOOL ReadCurrentOs(char *data)
{
  if (data != NULL)
  {
    if (data[0]== 0)return FALSE;

    if (Contient(data,"64"))
    {
      current_OS_BE_64b = TRUE;
           if (Contient(data,GUIDE_REG_OS_2003_64b))strncpy(current_OS,GUIDE_REG_OS_2003_32b  ,DEFAULT_TMP_SIZE);
      else if (Contient(data,GUIDE_REG_OS_2008_64b))strncpy(current_OS,GUIDE_REG_OS_2008_32b  ,DEFAULT_TMP_SIZE);
      else if (Contient(data,GUIDE_REG_OS_XP_64b))  strncpy(current_OS,GUIDE_REG_OS_XP_32b    ,DEFAULT_TMP_SIZE);
      else if (Contient(data,GUIDE_REG_OS_8_64b))   strncpy(current_OS,GUIDE_REG_OS_8_32b     ,DEFAULT_TMP_SIZE);
      else if (Contient(data,GUIDE_REG_OS_7_64b))   strncpy(current_OS,GUIDE_REG_OS_7_32b     ,DEFAULT_TMP_SIZE);
      else return FALSE; //unknow
    }else
    {
      current_OS_BE_64b = FALSE;
           if (Contient(data,GUIDE_REG_OS_2000))    strncpy(current_OS,GUIDE_REG_OS_2000      ,DEFAULT_TMP_SIZE);
      else if (Contient(data,GUIDE_REG_OS_2003_32b))strncpy(current_OS,GUIDE_REG_OS_2003_32b  ,DEFAULT_TMP_SIZE);
      else if (Contient(data,GUIDE_REG_OS_2008_32b))strncpy(current_OS,GUIDE_REG_OS_2008_32b  ,DEFAULT_TMP_SIZE);
      else if (Contient(data,GUIDE_REG_OS_XP_32b))  strncpy(current_OS,GUIDE_REG_OS_XP_32b    ,DEFAULT_TMP_SIZE);
      else if (Contient(data,GUIDE_REG_OS_8_32b))   strncpy(current_OS,GUIDE_REG_OS_8_32b     ,DEFAULT_TMP_SIZE);
      else if (Contient(data,GUIDE_REG_OS_7_32b))   strncpy(current_OS,GUIDE_REG_OS_7_32b     ,DEFAULT_TMP_SIZE);
      else return FALSE; //unknow
    }

    AddDebugMessage("ReadCurrentOs", current_OS, "OK", MSG_INFO);
    return TRUE;
  }
  return FALSE;
}
//------------------------------------------------------------------------------
int callback_sqlite_guide_local(void *datas, int argc, char **argv, char **azColName)
{
  FORMAT_CALBAK_TYPE *type = datas;
  unsigned int session_id = current_session_id;
  switch(type->type)
  {
    case SQLITE_GUIDE:
    {
      //read record and test it !!!!
      if (OsTypeValide(argv[5]))
      {
        HKEY hk = hkStringtohkey(argv[0]);
        char data_read[REQUEST_MAX_SIZE]="";

        switch(atoi(argv[4])) //data type
        {
          case TYPE_VALUE_STRING:
            if (ReadValue(hk,argv[1],argv[3],data_read, REQUEST_MAX_SIZE))
              CheckGuideValue("",TRUE,argv[6],argv[0],argv[1],argv[3],argv[5],data_read, argv[8], argv[9], session_id, db_scan);
            else CheckGuideValue("",FALSE,argv[6],argv[0],argv[1],argv[3],argv[5],data_read, argv[8], argv[9], session_id, db_scan);
          break;
          case TYPE_VALUE_DWORD:
          {
            long int v = ReadDwordValue(hk,argv[1],argv[3]);
            if (v == -1)CheckGuideValue("",FALSE,argv[6],argv[0],argv[1],argv[3],argv[5],data_read, argv[8], argv[9], session_id, db_scan);
            else
            {
              snprintf(data_read,REQUEST_MAX_SIZE,"%lu",v);
              CheckGuideValue("",TRUE,argv[6],argv[0],argv[1],argv[3],argv[5],data_read, argv[8], argv[9], session_id, db_scan);
            }
          }
          break;
          case TYPE_VALUE_MULTI_STRING:
          {
            long int i, tmp_size = ReadValue(hk,argv[1],argv[3],data_read, REQUEST_MAX_SIZE);
            if (tmp_size>0)
            {
              for (i=0;i<tmp_size;i++)
              {
                if (data_read[i] == 0)data_read[i]=';';
              }
              CheckGuideValue("",TRUE,argv[6],argv[0],argv[1],argv[3],argv[5],data_read, argv[8], argv[9], session_id, db_scan);
            }else CheckGuideValue("",FALSE,argv[6],argv[0],argv[1],argv[3],argv[5],data_read, argv[8], argv[9], session_id, db_scan);
          }
          break;
          case TYPE_VALUE_WSTRING:
          {
            char tmp[REQUEST_MAX_SIZE]="";
            if (ReadValue(hk,argv[1],argv[3],tmp, REQUEST_MAX_SIZE))
            {
              snprintf(data_read,REQUEST_MAX_SIZE,"%S",tmp);
              CheckGuideValue("",TRUE,argv[6],argv[0],argv[1],argv[3],argv[5],data_read, argv[8], argv[9], session_id, db_scan);
            }else CheckGuideValue("",FALSE,argv[6],argv[0],argv[1],argv[3],argv[5],data_read, argv[8], argv[9], session_id, db_scan);
          }
          break;
        }
      }
    }break;
  }
  return 0;
}
//------------------------------------------------------------------------------
//file registry part
//------------------------------------------------------------------------------
HK_F_OPEN guide_hks;
//------------------------------------------------------------------------------
int callback_sqlite_guide_file(void *datas, int argc, char **argv, char **azColName)
{
  FORMAT_CALBAK_TYPE *type = datas;
  unsigned int session_id = current_session_id;
  char data_read[MAX_PATH]="",tmp[MAX_PATH]="";
  switch(type->type)
  {
    case SQLITE_GUIDE:
    {
      //read record and test it !!!!
      if (OsTypeValide(argv[5]))
      {
        switch(atoi(argv[4])) //data type
        {
          case TYPE_VALUE_STRING:
          case TYPE_VALUE_MULTI_STRING:
          case TYPE_VALUE_WSTRING:
          {
            if(Readnk_Value(guide_hks.buffer, guide_hks.taille_fic, (guide_hks.pos_fhbin)+HBIN_HEADER_SIZE, guide_hks.position, argv[2], NULL, argv[3], data_read, MAX_PATH))
              CheckGuideValue(guide_hks.file, TRUE,argv[6],"",argv[2],argv[3],argv[5],data_read, argv[8], argv[9], session_id, db_scan);
            else CheckGuideValue(guide_hks.file,FALSE,argv[6],"",argv[2],argv[3],argv[5],"", argv[8], argv[9], session_id, db_scan);
          }
          break;
          case TYPE_VALUE_DWORD:
          {
            if(Readnk_Value(guide_hks.buffer, guide_hks.taille_fic, (guide_hks.pos_fhbin)+HBIN_HEADER_SIZE, guide_hks.position, argv[2], NULL, argv[3], data_read, MAX_PATH))
            {
              if(strlen(data_read)>2)
              {
                strncpy(tmp,data_read+2,MAX_PATH);
                snprintf(data_read,MAX_PATH,"%lu",atol(tmp));
                CheckGuideValue(guide_hks.file, TRUE,argv[6],"",argv[2],argv[3],argv[5],data_read, argv[8], argv[9], session_id, db_scan);
              }else CheckGuideValue(guide_hks.file, TRUE,argv[6],"",argv[2],argv[3],argv[5],"", argv[8], argv[9], session_id, db_scan);
            }else CheckGuideValue(guide_hks.file,FALSE,argv[6],"",argv[2],argv[3],argv[5],"", argv[8], argv[9], session_id, db_scan);
          }
          break;
        }
      }
    }break;
  }
  return 0;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DWORD WINAPI Scan_guide(LPVOID lParam)
{
  //init
  char file[MAX_PATH];
  char tmp_msg[MAX_PATH];
  char data[MAX_PATH]="";

  FORMAT_CALBAK_READ_INFO fcri;
  fcri.type = SQLITE_GUIDE;
  WaitForSingleObject(hsemaphore,INFINITE);
  AddDebugMessage("test_guide", "Scan guide  - START", "OK", MSG_INFO);

  //files or local
  HTREEITEM hitem = (HTREEITEM)SendDlgItemMessage((HWND)h_conf,TRV_FILES, TVM_GETNEXTITEM,(WPARAM)TVGN_CHILD, (LPARAM)TRV_HTREEITEM_CONF[FILES_TITLE_REGISTRY]);
  if (hitem!=NULL) //files
  {
    while(hitem!=NULL)
    {
      file[0] = 0;
      GetTextFromTrv(hitem, file, MAX_PATH);
      if (file[0] != 0)
      {
        //info
        snprintf(tmp_msg,MAX_PATH,"Scan Registry file : %s",file);
        AddDebugMessage("test_guide", tmp_msg, "OK", MSG_INFO);

        //verify
        if(OpenRegFiletoMem(&guide_hks, file))
        {
          //get OS
          if(Readnk_Value(guide_hks.buffer, guide_hks.taille_fic, (guide_hks.pos_fhbin)+HBIN_HEADER_SIZE, guide_hks.position, "microsoft\\windows nt\\currentversion", NULL,"ProductName", data, MAX_PATH))
            ReadCurrentOs(data);

          sqlite3_exec(db_scan, "SELECT hk,key,search_key,value,value_type,data,test,OS,title_id,description_id FROM extract_guide_request;", callback_sqlite_guide_file, &fcri, NULL);

          CloseRegFiletoMem(&guide_hks);
        }
      }
      hitem = (HTREEITEM)SendDlgItemMessage((HWND)h_conf,TRV_FILES, TVM_GETNEXTITEM,(WPARAM)TVGN_NEXT, (LPARAM)hitem);
    }
  }else
  {

    //read actual OS
    if (ReadValue(HKEY_LOCAL_MACHINE,"software\\microsoft\\windows nt\\currentversion","ProductName",data, MAX_PATH))
      ReadCurrentOs(data);

    sqlite3_exec(db_scan, "SELECT hk,key,search_key,value,value_type,data,test,OS,title_id,description_id FROM extract_guide_request;", callback_sqlite_guide_local, &fcri, NULL);
  }

  AddDebugMessage("test_guide", "Scan guide  - DONE", "OK", MSG_INFO);
  check_treeview(GetDlgItem(h_conf,TRV_TEST), H_tests[(unsigned int)lParam], TRV_STATE_UNCHECK);//db_scan
  ReleaseSemaphore(hsemaphore,1,NULL);
  return 0;
}