//----------------------------------------------------------------
// Project              : Network Scanner
// Author               : Hanteville Nicolas
// Licence              : GPLv3
//----------------------------------------------------------------
#include "../resources.h"
//------------------------------------------------------------------------------
char* GetLocalPath(char *path, unsigned int sizeMax)
{
  char *c = path+GetModuleFileName(0, path,sizeMax);
  while(*c != '\\') c--;
  *c = 0;
  return path;
}
//------------------------------------------------------------------------------
BOOL Netbios_check_user(DWORD iitem, char *ip, char*username)
{
  char tmp[MAX_PATH];
  wchar_t serveur[MAX_PATH];
  wchar_t user[MAX_PATH];
  snprintf(tmp,MAX_PATH,"\\\\%s",ip);
  mbstowcs(serveur,tmp,MAX_PATH);
  mbstowcs(user,username,MAX_PATH);

  LPUSER_INFO_1 p1Buf =NULL;
  LPUSER_INFO_2 p2Buf =NULL;
  LPUSER_INFO_3 p3Buf =NULL;

  if (NetUserGetInfo(serveur,user,1,(LPBYTE*)&p1Buf)==NERR_Success)
  {
    if (p1Buf->usri1_flags&UF_ACCOUNTDISABLE)
    {
      if (p1Buf->usri1_flags&UF_PASSWORD_EXPIRED)
      {
        AddLSTVUpdateItem("ADMIN_ACCOUNT:OK (Exist but disable, password expired)", COL_CONFIG, iitem);
        AddMsg(h_main, (char*)"FOUND (Account policy)",ip,(char*)"ADMIN_ACCOUNT:OK (Exist but disable, password expired)");
      }else
      {
        AddLSTVUpdateItem("ADMIN_ACCOUNT:OK (Exist but disable)", COL_CONFIG, iitem);
        AddMsg(h_main, (char*)"FOUND (Account policy)",ip,(char*)"ADMIN_ACCOUNT:OK (Exist but disable)");
      }
    }else
    {
      AddLSTVUpdateItem("ADMIN_ACCOUNT:OK (Available)", COL_CONFIG, iitem);
      AddMsg(h_main, (char*)"FOUND (Account policy)",ip,(char*)"ADMIN_ACCOUNT:OK (Available)");
    }
    NetApiBufferFree(p1Buf);
    return TRUE;
  }else if (NetUserGetInfo(serveur,user,2,(LPBYTE*)&p2Buf)==NERR_Success)
  {
    if (p2Buf->usri2_flags&UF_ACCOUNTDISABLE)
    {
      if (p2Buf->usri2_flags&UF_PASSWORD_EXPIRED)
      {
        AddLSTVUpdateItem("ADMIN_ACCOUNT:OK (Exist but disable, password expired)", COL_CONFIG, iitem);
        AddMsg(h_main, (char*)"FOUND (Account policy)",ip,(char*)"ADMIN_ACCOUNT:OK (Exist but disable, password expired)");
      }else
      {
        AddLSTVUpdateItem("ADMIN_ACCOUNT:OK (Exist but disable)", COL_CONFIG, iitem);
        AddMsg(h_main, (char*)"FOUND (Account policy)",ip,(char*)"ADMIN_ACCOUNT:OK (Exist but disable)");
      }
    }else
    {
      AddLSTVUpdateItem("ADMIN_ACCOUNT:OK (Available)", COL_CONFIG, iitem);
      AddMsg(h_main, (char*)"FOUND (Account policy)",ip,(char*)"ADMIN_ACCOUNT:OK (Available)");
    }
    NetApiBufferFree(p2Buf);
    return TRUE;
  }else if (NetUserGetInfo(serveur,user,3,(LPBYTE*)&p3Buf)==NERR_Success)
  {
    if (p3Buf->usri3_flags&UF_ACCOUNTDISABLE)
    {
      if (p3Buf->usri3_flags&UF_PASSWORD_EXPIRED || p3Buf->usri3_password_expired)
      {
        AddLSTVUpdateItem("ADMIN_ACCOUNT:OK (Exist but disable, password expired)", COL_CONFIG, iitem);
        AddMsg(h_main, (char*)"FOUND (Account policy)",ip,(char*)"ADMIN_ACCOUNT:OK (Exist but disable, password expired)");
      }else
      {
        AddLSTVUpdateItem("ADMIN_ACCOUNT:OK (Exist but disable)", COL_CONFIG, iitem);
        AddMsg(h_main, (char*)"FOUND (Account policy)",ip,(char*)"ADMIN_ACCOUNT:OK (Exist but disable)");
      }
    }else
    {
      AddLSTVUpdateItem("ADMIN_ACCOUNT:OK (Available)", COL_CONFIG, iitem);
      AddMsg(h_main, (char*)"FOUND (Account policy)",ip,(char*)"ADMIN_ACCOUNT:OK (Available)");
    }
    NetApiBufferFree(p3Buf);
    return TRUE;
  }

  return FALSE;
}
//----------------------------------------------------------------
void RemoteConnexionScan_auto_scan(DWORD iitem, char*ip, BOOL windows_OS)
{
  char remote_name[MAX_PATH], tmp[MAX_PATH];
  HANDLE connect = NetConnexionAuthenticateTest(ip, remote_name,config, iitem);

  //account policy
  if (auto_scan_config.PASSWORD_POLICY && scan_start)
  {
    wchar_t server[MAX_PATH];
    snprintf(tmp,MAX_PATH,"\\\\%s",ip);

    WaitForSingleObject(hs_netbios,INFINITE);
    mbstowcs(server, tmp, MAX_PATH);

    char tmp_pUmI_0[MAX_PATH]="", tmp_pUmI_3[MAX_PATH]="";
    USER_MODALS_INFO_0 *pUmI_0 = NULL;
    USER_MODALS_INFO_3 *pUmI_3 = NULL;
    if (NERR_Success == NetUserModalsGet(server,0,(LPBYTE *)&pUmI_0))
    {
      if (pUmI_0 != NULL)
      {
        snprintf(tmp_pUmI_0,MAX_PATH,
                                     "min_passwd_age:%s(%d days)\r\n"
                                     "max_passwd_age:%s(%d days)\r\n"
                                     "min_passwd_len:%s(%d)\r\n"
                                     "passord_history:%s(%d)\r\n"
                                    ,(pUmI_0->usrmod0_min_passwd_age/86400)>=auto_scan_config.PASSWORD_POLICY_MIN_AGE?"OK":"NOK",pUmI_0->usrmod0_min_passwd_age/86400
                                    ,(pUmI_0->usrmod0_max_passwd_age/86400)<=auto_scan_config.PASSWORD_POLICY_MAX_AGE?"OK":"NOK",pUmI_0->usrmod0_max_passwd_age/86400
                                    ,pUmI_0->usrmod0_min_passwd_len>=auto_scan_config.PASSWORD_POLICY_MIN_LEN?"OK":"NOK",pUmI_0->usrmod0_min_passwd_len
                                    ,pUmI_0->usrmod0_password_hist_len>=auto_scan_config.PASSWORD_POLICY_HISTORY?"OK":"NOK",pUmI_0->usrmod0_password_hist_len);
        NetApiBufferFree(pUmI_0);
        AddLSTVUpdateItem(tmp_pUmI_0, COL_CONFIG, iitem);
        AddMsg(h_main, (char*)"FOUND (Account policy)",ip,tmp_pUmI_0);
      }
    }
    if (NERR_Success == NetUserModalsGet(server,3,(LPBYTE *)&pUmI_3))
    {
      if (pUmI_3 != NULL)
      {
        snprintf(tmp_pUmI_3,MAX_PATH,"bad_password_count_to_lock_account:%s(%d)"
                                    ,pUmI_3->usrmod3_lockout_threshold<=auto_scan_config.PASSWORD_POLICY_LOCKOUT_COUNT?"OK":"NOK",pUmI_3->usrmod3_lockout_threshold);
        NetApiBufferFree(pUmI_3);
        AddLSTVUpdateItem(tmp_pUmI_3, COL_CONFIG, iitem);
        AddMsg(h_main, (char*)"FOUND (Account policy)",ip,tmp_pUmI_3);
      }
    }
    ReleaseSemaphore(hs_netbios,1,NULL);
  }

  if (auto_scan_config.ADMIN_ACCOUNT && scan_start)
  {
    WaitForSingleObject(hs_netbios,INFINITE);
    if (!Netbios_check_user(iitem, ip, auto_scan_config.C_ADMIN_ACCOUNT))
    {
      if (TestReversSID(ip, auto_scan_config.C_ADMIN_ACCOUNT))
      {
        AddLSTVUpdateItem("ADMIN_ACCOUNT:OK (Available untested)", COL_CONFIG, iitem);
        AddMsg(h_main, (char*)"FOUND (Account policy)",ip,(char*)"ADMIN_ACCOUNT:OK (Available untested)");
      }else
      {
        if (connect != FALSE)
        {
          AddLSTVUpdateItem("ADMIN_ACCOUNT:NOK (Not available)", COL_CONFIG, iitem);
          AddMsg(h_main, (char*)"FOUND (Account policy)",ip,(char*)"ADMIN_ACCOUNT:NOK (Not available)");
        }
      }
    }
    ReleaseSemaphore(hs_netbios,1,NULL);
  }

  if (connect == FALSE) return;
  HKEY hkey;
  snprintf(tmp,MAX_PATH,"\\\\%s",ip);
  if (RegConnectRegistry(tmp,HKEY_LOCAL_MACHINE,&hkey)==ERROR_SUCCESS && scan_start)
  {
    DWORD DWORD_tmp=0;
    char value[MAX_PATH], tmp_value[MAX_PATH]="", msg[MAX_PATH];

    //get Exact OS !!!
    if (RegistryOS(iitem,hkey))
    {
      if (!windows_OS)nb_windows++;
    }

    if (auto_scan_config.PASSWORD_POLICY)
    {
      DWORD_tmp=0;
      if(ReadValue(hkey,"System\\CurrentControlSet\\Services\\Netlogon\\Parameters\\","RequireStrongKey",&DWORD_tmp, sizeof(DWORD)))
      {
        snprintf(value,MAX_PATH,"%s\\HKLM\\System\\CurrentControlSet\\Services\\Netlogon\\Parameters\\RequireStrongKey=%d",ip,DWORD_tmp);

        if (DWORD_tmp == 1)
        {
          AddLSTVUpdateItem("Password complexity:OK (Enable)", COL_CONFIG, iitem);
          AddMsg(h_main, (char*)"FOUND (Account policy)",(char*)"Password complexity:OK (Enable)",value);
        }else
        {
          AddLSTVUpdateItem("Password complexity:NOK (Disable)", COL_CONFIG, iitem);
          AddMsg(h_main, (char*)"FOUND (Account policy)",(char*)"Password complexity:NOK (Disable)",value);
        }
      }
    }

    if (auto_scan_config.AUTORUN)
    {
      DWORD_tmp=0;
      if(ReadValue(hkey,"Software\\Microsoft\\Windows\\CurrentVersion\\policies\\Explorer\\","NoDriveTypeAutorun",&DWORD_tmp, sizeof(DWORD)))
      {
        snprintf(value,MAX_PATH,"%s\\HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\policies\\Explorer\\NoDriveTypeAutorun=%d",ip,DWORD_tmp);
        if (DWORD_tmp == 0xFF)
        {
          AddLSTVUpdateItem("AUTORUN:OK (Disable)", COL_CONFIG, iitem);
          AddMsg(h_main, (char*)"FOUND (Registry)",(char*)"AUTORUN:OK (Disable)",value);
        }else
        {
          AddLSTVUpdateItem("AUTORUN:NOK (Enable)", COL_CONFIG, iitem);
          AddMsg(h_main, (char*)"FOUND (Registry)",(char*)"AUTORUN:NOK (Enable)",value);
        }
      }
    }

    if (auto_scan_config.M_SEC)
    {
      tmp_value[0]=0;
      if(ReadValue(hkey,auto_scan_config.MSEC_REG_PATH,auto_scan_config.MSEC_REG_VALUE,tmp_value, MAX_PATH))
      {
        snprintf(value,MAX_PATH,"%s\\HKLM\\%s%s=%s",ip,auto_scan_config.MSEC_REG_PATH,auto_scan_config.MSEC_REG_VALUE,tmp_value);
        AddLSTVUpdateItem("MSEC:OK (installed)", COL_CONFIG, iitem);
        AddMsg(h_main, (char*)"FOUND (Registry)",(char*)"MSEC:OK (installed)",value);
      }else
      {
        snprintf(value,MAX_PATH,"%s\\HKLM\\%s%s",ip,auto_scan_config.MSEC_REG_PATH,auto_scan_config.MSEC_REG_VALUE);
        AddLSTVUpdateItem("MSEC:NOK (not installed)", COL_CONFIG, iitem);
        AddMsg(h_main, (char*)"FOUND (Registry)",(char*)"MSEC:NOK (not installed)",value);
      }
    }

    //current time
    time_t current_time;
    time(&current_time);
    struct tm *today = localtime(&current_time);

    if (auto_scan_config.MCAFEE_INSTALLED)
    {
      tmp_value[0]=0;
      if(ReadValue(hkey,"SOFTWARE\\Mcafee\\AVEngine\\","AvDatDate",tmp_value, MAX_PATH))
      {
        snprintf(value,MAX_PATH,"%s\\HKLM\\SOFTWARE\\Mcafee\\AVEngine\\AvDatDate=%s",ip,tmp_value);
        AddLSTVUpdateItem("McAfee:OK (installed)", COL_CONFIG, iitem);
        AddMsg(h_main, (char*)"FOUND (Registry)",(char*)"McAfee:OK (installed)",value);

        //check version
        tmp[0] = tmp_value[0];
        tmp[1] = tmp_value[1];
        tmp[2] = tmp_value[2];
        tmp[3] = tmp_value[3];
        tmp[4] = 0;
        unsigned int year = atoi(tmp);

        tmp[0] = tmp_value[5];
        tmp[1] = tmp_value[6];
        tmp[2] = 0;
        unsigned int month = atoi(tmp);

        tmp[0] = tmp_value[8];
        tmp[1] = tmp_value[9];
        tmp[2] = 0;
        unsigned int day = atoi(tmp);
        if (today->tm_mday - day >= 0 && (today->tm_mon+1) == month && today->tm_year+1900 == year)
        {
          if (today->tm_mday - day <= auto_scan_config.MCAFEE_UPDATE_DAYS_INTERVAL)
          {
            snprintf(msg,MAX_PATH,"McAfee:OK (last update: %s)",tmp_value);

          }else
          {
            snprintf(msg,MAX_PATH,"McAfee:NOK (last update: %s)",tmp_value);
          }
        }else if (((today->tm_mon+1) - month == 1) && (27-day + today->tm_mday)<= auto_scan_config.MCAFEE_UPDATE_DAYS_INTERVAL)
        {
          //exact calcul
          unsigned int ref = 31;
          switch(today->tm_mon+1)
          {
            case 2:ref=27;break;

            case 4:
            case 6:
            case 9:
            case 11:ref=30;break;
          }

          if ((ref-day + today->tm_mday)<= auto_scan_config.MCAFEE_UPDATE_DAYS_INTERVAL)
          {
            snprintf(msg,MAX_PATH,"McAfee:OK (last update: %s)",tmp_value);

          }else
          {
            snprintf(msg,MAX_PATH,"McAfee:NOK (last update: %s)",tmp_value);
          }
        }else if (((today->tm_year+1900) - year == 1) && (today->tm_mon == 0 && month == 12))
        {
          if ((31-day + today->tm_mday) <= auto_scan_config.MCAFEE_UPDATE_DAYS_INTERVAL)
          {
            snprintf(msg,MAX_PATH,"McAfee:OK (last update: %s)",tmp_value);

          }else
          {
            snprintf(msg,MAX_PATH,"McAfee:NOK (last update: %s)",tmp_value);
          }
        }else
        {
          snprintf(msg,MAX_PATH,"McAfee:NOK (last update: %s)",tmp_value);
        }
        AddLSTVUpdateItem(msg, COL_CONFIG, iitem);
        AddMsg(h_main, (char*)"FOUND (Registry)",ip,(char*)msg);
      }else
      {
        snprintf(value,MAX_PATH,"%s\\HKLM\\SOFTWARE\\Mcafee\\AVEngine\\AvDatDate",ip);
        AddLSTVUpdateItem("McAfee:NOK (not installed)", COL_CONFIG, iitem);
        AddMsg(h_main, (char*)"FOUND (Registry)",(char*)"McAfee:NOK (not installed)",value);
      }
    }

    if (auto_scan_config.WSUS_WORKS)
    {
      tmp_value[0]=0;
      if(ReadValue(hkey,"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\","WUServer",tmp_value, MAX_PATH))
      {
        snprintf(msg,MAX_PATH,"WSUS:OK (server: %s)",tmp_value);

        //check if work
        DWORD_tmp=0;
        if(ReadValue(hkey,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Auto Update\\Results\\Detect\\","LastError",&DWORD_tmp, sizeof(DWORD)))
        {
          if (DWORD_tmp != 0)
          {
            snprintf(msg,MAX_PATH,"WSUS:NOK (error server: %s)",tmp_value);
          }
        }else snprintf(msg,MAX_PATH,"WSUS:NOK (not used, server: %s)",tmp_value);
        AddLSTVUpdateItem(msg, COL_CONFIG, iitem);
        AddMsg(h_main, (char*)"FOUND (Registry)",ip,(char*)msg);

        //patch update
        tmp_value[0]=0;
        if(ReadValue(hkey,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Auto Update\\Results\\Install\\","LastSuccessTime",tmp_value, MAX_PATH))
        {
          tmp[0] = tmp_value[0];
          tmp[1] = tmp_value[1];
          tmp[2] = tmp_value[2];
          tmp[3] = tmp_value[3];
          tmp[4] = 0;
          unsigned int year = atoi(tmp);

          tmp[0] = tmp_value[5];
          tmp[1] = tmp_value[6];
          tmp[2] = 0;
          unsigned int month = atoi(tmp);

          tmp[0] = tmp_value[8];
          tmp[1] = tmp_value[9];
          tmp[2] = 0;
          unsigned int day = atoi(tmp);

          if (today->tm_mday - day >= 0 && (today->tm_mon+1) == month && today->tm_year+1900 == year)
          {
            if (today->tm_mday - day <= 30)
            {
              snprintf(msg,MAX_PATH,"WSUS UPDATE:OK (last update: %s)",tmp_value);

            }else
            {
              snprintf(msg,MAX_PATH,"WSUS UPDATE:NOK (last update: %s)",tmp_value);
            }
          }else if (((today->tm_mon+1) - month == 1) && (27-day + today->tm_mday)<= 30)
          {
            //exact calcul
            unsigned int ref = 31;
            switch(today->tm_mon+1)
            {
              case 2:ref=27;break;

              case 4:
              case 6:
              case 9:
              case 11:ref=30;break;
            }

            if ((ref-day + today->tm_mday)<= 30)
            {
              snprintf(msg,MAX_PATH,"WSUS UPDATE:OK (last update: %s)",tmp_value);

            }else
            {
              snprintf(msg,MAX_PATH,"WSUS UPDATE:NOK (last update: %s)",tmp_value);
            }
          }else if (((today->tm_year+1900) - year == 1) && (today->tm_mon == 0 && month == 12))
          {
            if ((31-day + today->tm_mday) <= 30)
            {
              snprintf(msg,MAX_PATH,"WSUS UPDATE:OK (last update: %s)",tmp_value);

            }else
            {
              snprintf(msg,MAX_PATH,"WSUS UPDATE:NOK (last update: %s)",tmp_value);
            }
          }else
          {
            snprintf(msg,MAX_PATH,"WSUS UPDATE:NOK (last update: %s)",tmp_value);
          }
        }else snprintf(msg,MAX_PATH,"WSUS UPDATE:NOK (expire)",tmp_value);
        AddLSTVUpdateItem(msg, COL_CONFIG, iitem);
        AddMsg(h_main, (char*)"FOUND (Registry)",ip,(char*)msg);
      }else
      {
        AddMsg(h_main, (char*)"FOUND (Registry)",ip,"WSUS:NOK (unconfigured)");
        AddLSTVUpdateItem("WSUS:NOK (unconfigured)", COL_CONFIG, iitem);
      }
    }
    RegCloseKey(hkey);
  }

  if(connect)
  {
    WNetCancelConnection2(remote_name,CONNECT_UPDATE_PROFILE,1);
    CloseHandle(connect);
  }
}
//----------------------------------------------------------------
DWORD WINAPI ScanIp_auto_scan(LPVOID lParam)
{
  DWORD index = (DWORD)lParam;
  BOOL exist = FALSE, dnsok=FALSE;
  DWORD iitem = 0;
  char ip[MAX_PATH]="", ip_mac[MAX_PATH]="", dns[MAX_PATH]="", ttl_s[MAX_PATH]="", os_s[MAX_PATH]="",cfg[MAX_LINE_SIZE]="",test_title[MAX_PATH];
  if (SendDlgItemMessage(h_main, CB_IP, LB_GETTEXTLEN, (WPARAM)index,(LPARAM)NULL) > MAX_PATH)
  {
    ReleaseSemaphore(hs_threads,1,NULL);

    //tracking
    if (scan_start)
    {
      EnterCriticalSection(&Sync);
      snprintf(test_title,MAX_PATH,"%s %lu/%lu",TITLE,++nb_test_ip,nb_i);
      LeaveCriticalSection(&Sync);
      SetWindowText(h_main,test_title);
    }
    return 0;
  }
  SendDlgItemMessage(h_main, CB_IP, LB_GETTEXT, (WPARAM)index,(LPARAM)ip);

  if (ip[0]!=0 && scan_start)
  {
    WaitForSingleObject(hs_disco,INFINITE);

    if (ip[0]> '9' || ip[0]< '0' || ((ip[1]> '9' || ip[1]< '0') && ip[1] != '.'))
    {
      //resolution inverse
      strncpy(dns,ip,MAX_PATH);

      struct in_addr **a;
      struct hostent *host;

      if (host=gethostbyname(ip))
      {
        a = (struct in_addr **)host->h_addr_list;
        snprintf(ip,16,"%s",inet_ntoa(**a));
        exist = TRUE;
        dnsok = TRUE;

        iitem = AddLSTVItem(ip, dns, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
      }else
      {
        iitem = AddLSTVItem((char*)"[ERROR DNS]", ip, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, (char*)"OK");
        ReleaseSemaphore(hs_disco,1,NULL);
        ReleaseSemaphore(hs_threads,1,NULL);

        //tracking
        EnterCriticalSection(&Sync);
        snprintf(test_title,MAX_PATH,"%s %lu/%lu",TITLE,++nb_test_ip,nb_i);
        LeaveCriticalSection(&Sync);
        SetWindowText(h_main,test_title);
        return 0;
      }
    }

    //ICMP
    if (scan_start)
    {
      int ttl = Ping(ip);
      if (ttl > -1)
      {
        snprintf(ttl_s,MAX_PATH,"TTL:%d",ttl);

        if (!exist)
        {
          if (ttl <= MACH_LINUX)iitem = AddLSTVItem(ip, NULL, ttl_s, (char*)"Linux",NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
          else if (ttl <= MACH_WINDOWS)iitem = AddLSTVItem(ip, NULL, ttl_s, (char*)"Windows",NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
          else if (ttl <= MACH_WINDOWS)iitem = AddLSTVItem(ip, NULL, ttl_s, (char*)"Router",NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
          exist = TRUE;
        }else
        {
          ListView_SetItemText(GetDlgItem(h_main,LV_results),iitem,COL_TTL,ttl_s);

          if (ttl <= MACH_LINUX)ListView_SetItemText(GetDlgItem(h_main,LV_results),iitem,COL_OS,(LPSTR)"Linux")
          else if (ttl <= MACH_WINDOWS)ListView_SetItemText(GetDlgItem(h_main,LV_results),iitem,COL_OS,(LPSTR)"Windows")
          else if (ttl <= MACH_WINDOWS)ListView_SetItemText(GetDlgItem(h_main,LV_results),iitem,COL_OS,(LPSTR)"Router")
        }
      }
    }

    //DNS
    if (scan_start)
    {
      if(exist)ListView_SetItemText(GetDlgItem(h_main,LV_results),iitem,COL_STATE,(LPSTR)"DNS");
      if(ResDNS(ip, dns, MAX_PATH))
      {
        if (!exist)
        {
          iitem = AddLSTVItem(ip, dns, NULL, (char*)"Firewall", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
          exist = TRUE;
        }else
        {
          ListView_SetItemText(GetDlgItem(h_main,LV_results),iitem,COL_DNS,dns);
        }
        dnsok = TRUE;
      }
    }

    ReleaseSemaphore(hs_disco,1,NULL);

    BOOL windows_OS = FALSE;
    //tests !!!
    char tmp[MAX_LINE_SIZE] = "";
    if (exist && scan_start)
    {
      ListView_SetItemText(GetDlgItem(h_main,LV_results),iitem,COL_STATE,(LPSTR)"NetBIOS");
      WaitForSingleObject(hs_netbios,INFINITE);

      if (auto_scan_config.NULL_SESSION || auto_scan_config.REVERS_SID)
      {
        if(Netbios_NULLSession(ip, "IPC$"))
        {
          snprintf(cfg,MAX_LINE_SIZE,"NULL Session: Enable\r\n");
          if (auto_scan_config.REVERS_SID)
          {
            if(TestReversSID(ip,(char*)"invit�"))             snprintf(cfg+strlen(cfg),MAX_LINE_SIZE-strlen(cfg),"Revers SID: Enable (OK with \"invit�\" account)\r\n");
            else if(TestReversSID(ip,(char*)"guest"))         snprintf(cfg+strlen(cfg),MAX_LINE_SIZE-strlen(cfg),"Revers SID: Enable (OK with \"guest\" account)\r\n");
            else if(TestReversSID(ip,(char*)"gast"))          snprintf(cfg+strlen(cfg),MAX_LINE_SIZE-strlen(cfg),"Revers SID: Enable (OK with \"gast\" account)\r\n");
            else if(TestReversSID(ip,(char*)"invitado"))      snprintf(cfg+strlen(cfg),MAX_LINE_SIZE-strlen(cfg),"Revers SID: Enable (OK with \"invitado\" account)\r\n");
            else if(TestReversSID(ip,(char*)"HelpAssistant")) snprintf(cfg+strlen(cfg),MAX_LINE_SIZE-strlen(cfg),"Revers SID: Enable (OK with \"HelpAssistant\" account)\r\n");
            else if(TestReversSID(ip,(char*)"ASPNET"))        snprintf(cfg+strlen(cfg),MAX_LINE_SIZE-strlen(cfg),"Revers SID: Enable (OK with \"ASPNET\" account)\r\n");
            else if(TestReversSID(ip,(char*)"administrateur"))snprintf(cfg+strlen(cfg),MAX_LINE_SIZE-strlen(cfg),"Revers SID: Enable (OK with \"administrateur\" account)\r\n");
            else if(TestReversSID(ip,(char*)"administrator")) snprintf(cfg+strlen(cfg),MAX_LINE_SIZE-strlen(cfg),"Revers SID: Enable (OK with \"administrator\" account)\r\n");
          }
          ListView_SetItemText(GetDlgItem(h_main,LV_results),iitem,COL_CONFIG,cfg);
          AddMsg(h_main, (char*)"FOUND (Config)",ip,cfg);

          //Share
          if (scan_start)
          {
            wchar_t server[MAX_PATH];
            snprintf(tmp,MAX_PATH,"\\\\%s",ip);
            mbstowcs(server, tmp, MAX_PATH);

            char shares[MAX_LINE_SIZE+1]="";
            Netbios_Share(server, shares, MAX_LINE_SIZE, ip);
            if (shares[0] != 0)
            {
              AddLSTVUpdateItem(shares, COL_CONFIG, iitem);
              AddMsg(h_main, (char*)"FOUND (Config)",ip,shares);
            }
          }

          //get OS
          if (dns[0] == 0 && scan_start)
          {
            windows_OS = Netbios_OS(ip, os_s, dns, NULL, MAX_PATH);
            if (windows_OS)
            {
              ListView_SetItemText(GetDlgItem(h_main,LV_results),iitem,COL_DNS,dns);
              ListView_SetItemText(GetDlgItem(h_main,LV_results),iitem,COL_OS,os_s);
              nb_windows++;
            }
          }else
          {
            windows_OS = Netbios_OS(ip, os_s, NULL, NULL, MAX_PATH);
            if (windows_OS)
            {
              ListView_SetItemText(GetDlgItem(h_main,LV_results),iitem,COL_OS,os_s);
              nb_windows++;
            }
          }
        }
      }
      ReleaseSemaphore(hs_netbios,1,NULL);
    }

    if (exist && scan_start)
    {
      //registry
      ListView_SetItemText(GetDlgItem(h_main,LV_results),iitem,COL_STATE,(LPSTR)"Registry");
      WaitForSingleObject(hs_registry,INFINITE);
      RemoteConnexionScan_auto_scan(iitem, ip, windows_OS);
      ReleaseSemaphore(hs_registry,1,NULL);
    }

    //add file test !!!
    if (exist && scan_start && auto_scan_config.MCAFEE_SCAN)
    {
      ListView_SetItemText(GetDlgItem(h_main,LV_results),iitem,COL_STATE,(LPSTR)(LPSTR)"Files");
      WaitForSingleObject(hs_file,INFINITE);
      char msg[MAX_PATH];
      snprintf(msg,MAX_PATH,"McAfee last scan:NOK (no data)");

      //MCAFEE_UPDATE_DAYS_INTERVAL
      unsigned int i;
      char remote_name[LINE_SIZE],tmp_path[LINE_SIZE];
      char tmp_login[MAX_PATH];
      snprintf(remote_name,LINE_SIZE,"\\\\%s\\C$",ip);

      for (i=0; i<config.nb_accounts && scan_start;i++)
      {
        NETRESOURCE NetRes  = {0};
        NetRes.dwScope      = RESOURCE_GLOBALNET;
        NetRes.dwType	      = RESOURCETYPE_ANY;
        NetRes.lpLocalName  = (LPSTR)"";
        NetRes.lpProvider   = (LPSTR)"";
        NetRes.lpRemoteName	= remote_name;

        if (config.accounts[i].domain[0] != 0)
        {
          snprintf(tmp_login,MAX_PATH,"%s\\%s",config.accounts[i].domain,config.accounts[i].login);
        }else
        {
          snprintf(tmp_login,MAX_PATH,"%s\\%s",ip,config.accounts[i].login);
        }

        if (WNetAddConnection2(&NetRes,config.accounts[i].mdp,tmp_login,CONNECT_PROMPT)==NO_ERROR)
        {
          HANDLE hfic;
          WIN32_FIND_DATA data;
          FILETIME LocalFileTime;
          SYSTEMTIME SysTimeModification;
          snprintf(tmp_path,LINE_SIZE,"%s\\ProgramData\\McAfee\\DesktopProtection\\OnDemandScanLog.txt",remote_name);

          if (GetFileAttributes(tmp_path) != INVALID_FILE_ATTRIBUTES)
          {
            hfic = FindFirstFile(tmp_path, &data);
            if (hfic != INVALID_HANDLE_VALUE)
            {
              //current time
              time_t current_time;
              time(&current_time);
              struct tm *today = localtime(&current_time);

              FileTimeToLocalFileTime(&(data.ftLastWriteTime), &LocalFileTime);
              FileTimeToSystemTime(&LocalFileTime, &SysTimeModification);

              if (today->tm_mday - SysTimeModification.wDay >= 0 && (today->tm_mon+1) == SysTimeModification.wMonth && today->tm_year+1900 == SysTimeModification.wYear)
              {
                if (today->tm_mday - SysTimeModification.wDay <= auto_scan_config.MCAFEE_SCAN_DAYS_INTERVAL)
                {
                  snprintf(msg,MAX_PATH,"McAfee last scan:OK (last update: %02d/%02d/%02d)",SysTimeModification.wYear,SysTimeModification.wMonth,SysTimeModification.wDay);

                }else
                {
                  snprintf(msg,MAX_PATH,"McAfee last scan:NOK (last update: %02d/%02d/%02d)",SysTimeModification.wYear,SysTimeModification.wMonth,SysTimeModification.wDay);
                }
              }else if (((today->tm_mon+1) - SysTimeModification.wMonth == 1) && (27-SysTimeModification.wDay + today->tm_mday)<= auto_scan_config.MCAFEE_SCAN_DAYS_INTERVAL)
              {
                //exact calcul
                unsigned int ref = 31;
                switch(today->tm_mon+1)
                {
                  case 2:ref=27;break;

                  case 4:
                  case 6:
                  case 9:
                  case 11:ref=30;break;
                }

                if ((ref-SysTimeModification.wDay + today->tm_mday)<= auto_scan_config.MCAFEE_SCAN_DAYS_INTERVAL)
                {
                  snprintf(msg,MAX_PATH,"McAfee last scan:OK (last update: %02d/%02d/%02d)",SysTimeModification.wYear,SysTimeModification.wMonth,SysTimeModification.wDay);

                }else
                {
                  snprintf(msg,MAX_PATH,"McAfee last scan:NOK (last update: %02d/%02d/%02d)",SysTimeModification.wYear,SysTimeModification.wMonth,SysTimeModification.wDay);
                }
              }else if (((today->tm_year+1900) - SysTimeModification.wYear == 1) && (today->tm_mon == 0 && SysTimeModification.wMonth == 12))
              {
                if ((31-SysTimeModification.wDay + today->tm_mday) <= auto_scan_config.MCAFEE_SCAN_DAYS_INTERVAL)
                {
                  snprintf(msg,MAX_PATH,"McAfee last scan:OK (last update: %02d/%02d/%02d)",SysTimeModification.wYear,SysTimeModification.wMonth,SysTimeModification.wDay);

                }else
                {
                  snprintf(msg,MAX_PATH,"McAfee last scan:NOK (last update: %02d/%02d/%02d)",SysTimeModification.wYear,SysTimeModification.wMonth,SysTimeModification.wDay);
                }
              }else
              {
                snprintf(msg,MAX_PATH,"McAfee last scan:NOK (last update: %02d/%02d/%02d)",SysTimeModification.wYear,SysTimeModification.wMonth,SysTimeModification.wDay);
              }
              //AddLSTVUpdateItem(msg, COL_CONFIG, iitem);
              //AddMsg(h_main, (char*)"FOUND (File)",ip,msg);
            }
          }

          WNetCancelConnection2(remote_name,CONNECT_UPDATE_PROFILE,1);
          AddLSTVUpdateItem(msg, COL_CONFIG, iitem);
          AddMsg(h_main, (char*)"FOUND (File)",ip,msg);
          break;
        }
      }
      ReleaseSemaphore(hs_file,1,NULL);
    }

    if (exist)ListView_SetItemText(GetDlgItem(h_main,LV_results),iitem,COL_STATE,(LPSTR)"OK");
  }

  ReleaseSemaphore(hs_threads,1,NULL);

  //tracking
  if (scan_start)
  {
    EnterCriticalSection(&Sync);
    snprintf(test_title,MAX_PATH,"%s %lu/%lu",TITLE,++nb_test_ip,nb_i);
    LeaveCriticalSection(&Sync);
    SetWindowText(h_main,test_title);
  }
  return 0;
}
//----------------------------------------------------------------
DWORD WINAPI auto_scan(LPVOID lParam)
{
  //check if NS.ini exist !!!
  char ini_path[LINE_SIZE]="";
  scan_start = TRUE;
  strcat(GetLocalPath(ini_path, LINE_SIZE),AUTO_SCAN_FILE_INI);
  ListView_DeleteAllItems(GetDlgItem(h_main,LV_results));
  SendDlgItemMessage(h_main,CB_IP,LB_RESETCONTENT,(WPARAM)NULL,(LPARAM)NULL);
  SendDlgItemMessage(h_main,CB_infos,LB_RESETCONTENT,(WPARAM)NULL,(LPARAM)NULL);

  char tmp_check[LINE_SIZE]="";
  if(GetPrivateProfileString("SCAN","IP_FILE","",tmp_check,LINE_SIZE,ini_path))
  {
    //load ips
    loadFileIp(tmp_check);

    //loads accounts
    tmp_check[0] = 0;
    if(GetPrivateProfileString("SCAN","ACCOUNT_FILE","",tmp_check,LINE_SIZE,ini_path))
    {
      config.nb_accounts = 0;
      LoadAuthFile(tmp_check);
    }

    //check scan type :
    tmp_check[0] = 0;
    if(GetPrivateProfileString("SCAN","TYPE","",tmp_check,LINE_SIZE,ini_path))
    {
      if (tmp_check[0] == 'A' || tmp_check[0] == 'a') //AUTO mode
      {

      }else if (tmp_check[0] == 'S' || tmp_check[0] == 's') //SIMPLE mode
      {
        //check test if enable !!
        tmp_check[0] = 0;
        if(GetPrivateProfileString("SCAN","DISCO_ICMP","",tmp_check,LINE_SIZE,ini_path))
        {
          if (tmp_check[0] == 'Y' || tmp_check[0] == 'y')SendDlgItemMessage(h_main,CB_tests,LB_SETSEL,(WPARAM)TRUE,(LPARAM)0);
        }

        tmp_check[0] = 0;
        if(GetPrivateProfileString("SCAN","DISCO_DNS","",tmp_check,LINE_SIZE,ini_path))
        {
          if (tmp_check[0] == 'Y' || tmp_check[0] == 'y')SendDlgItemMessage(h_main,CB_tests,LB_SETSEL,(WPARAM)TRUE,(LPARAM)1);
        }

        tmp_check[0] = 0;
        if(GetPrivateProfileString("SCAN","DISCO_NETBIOS","",tmp_check,LINE_SIZE,ini_path))
        {
          if (tmp_check[0] == 'Y' || tmp_check[0] == 'y')SendDlgItemMessage(h_main,CB_tests,LB_SETSEL,(WPARAM)TRUE,(LPARAM)2);
        }

        tmp_check[0] = 0;
        if(GetPrivateProfileString("SCAN","DISCO_NETBIOS_POLICY","",tmp_check,LINE_SIZE,ini_path))
        {
          if (tmp_check[0] == 'Y' || tmp_check[0] == 'y')SendDlgItemMessage(h_main,CB_tests,LB_SETSEL,(WPARAM)TRUE,(LPARAM)3);
        }

        tmp_check[0] = 0;
        if(GetPrivateProfileString("SCAN","DISCO_CHECK_FILES","",tmp_check,LINE_SIZE,ini_path))
        {
          if (tmp_check[0] == 'Y' || tmp_check[0] == 'y')SendDlgItemMessage(h_main,CB_tests,LB_SETSEL,(WPARAM)TRUE,(LPARAM)5);
        }

        tmp_check[0] = 0;
        if(GetPrivateProfileString("SCAN","DISCO_CHECK_REGISTRY","",tmp_check,LINE_SIZE,ini_path))
        {
          if (tmp_check[0] == 'Y' || tmp_check[0] == 'y')SendDlgItemMessage(h_main,CB_tests,LB_SETSEL,(WPARAM)TRUE,(LPARAM)6);
        }

        tmp_check[0] = 0;
        if(GetPrivateProfileString("SCAN","DISCO_CHECK_SERVICES","",tmp_check,LINE_SIZE,ini_path))
        {
          if (tmp_check[0] == 'Y' || tmp_check[0] == 'y')SendDlgItemMessage(h_main,CB_tests,LB_SETSEL,(WPARAM)TRUE,(LPARAM)7);
        }

        tmp_check[0] = 0;
        if(GetPrivateProfileString("SCAN","DISCO_CHECK_SOFTWARE","",tmp_check,LINE_SIZE,ini_path))
        {
          if (tmp_check[0] == 'Y' || tmp_check[0] == 'y')SendDlgItemMessage(h_main,CB_tests,LB_SETSEL,(WPARAM)TRUE,(LPARAM)8);
        }

        tmp_check[0] = 0;
        if(GetPrivateProfileString("SCAN","DISCO_CHECK_USB","",tmp_check,LINE_SIZE,ini_path))
        {
          if (tmp_check[0] == 'Y' || tmp_check[0] == 'y')SendDlgItemMessage(h_main,CB_tests,LB_SETSEL,(WPARAM)TRUE,(LPARAM)9);
        }

        tmp_check[0] = 0;
        if(GetPrivateProfileString("SCAN","DISCO_CHECK_SSH","",tmp_check,LINE_SIZE,ini_path))
        {
          if (tmp_check[0] == 'Y' || tmp_check[0] == 'y')SendDlgItemMessage(h_main,CB_tests,LB_SETSEL,(WPARAM)TRUE,(LPARAM)10);
        }

        tmp_check[0] = 0;
        if(GetPrivateProfileString("SCAN","DISCO_WRITE_KEY","",tmp_check,LINE_SIZE,ini_path))
        {
          if (tmp_check[0] == 'Y' || tmp_check[0] == 'y')SendDlgItemMessage(h_main,CB_tests,LB_SETSEL,(WPARAM)TRUE,(LPARAM)12);
        }

        EnableWindow(GetDlgItem(h_main,ED_NET_DOMAIN),FALSE);
        EnableWindow(GetDlgItem(h_main,ED_NET_LOGIN),FALSE);
        EnableWindow(GetDlgItem(h_main,ED_NET_PASSWORD),FALSE);
        EnableWindow(GetDlgItem(h_main,CHK_NULL_SESSION),FALSE);
        EnableWindow(GetDlgItem(h_main,GRP_PERIMETER),FALSE);
        EnableWindow(GetDlgItem(h_main,IP1),FALSE);
        EnableWindow(GetDlgItem(h_main,BT_IP_CP),FALSE);
        EnableWindow(GetDlgItem(h_main,IP2),FALSE);
        EnableWindow(GetDlgItem(h_main,CHK_LOAD_IP_FILE),FALSE);
        EnableWindow(GetDlgItem(h_main,CHK_ALL_TEST),FALSE);
        EnableWindow(GetDlgItem(h_main,CB_tests),FALSE);
        EnableWindow(GetDlgItem(h_main,BT_LOAD_MDP_FILES),FALSE);

        SetWindowText(GetDlgItem(h_main,BT_START),"Stop");
        AddMsg(h_main, (char*)"INFORMATION",(char*)"Start scan",(char*)"");
        h_thread_scan = CreateThread(NULL,0,scan,0,0,0);
        return 0;
      }else if (tmp_check[0] == 'D' || tmp_check[0] == 'd')//DISABLE
      {
        scan_start = FALSE;
        return 0;
      }
    }

    //disable interface + init
    EnableWindow(GetDlgItem(h_main,ED_NET_DOMAIN),FALSE);
    EnableWindow(GetDlgItem(h_main,ED_NET_LOGIN),FALSE);
    EnableWindow(GetDlgItem(h_main,ED_NET_PASSWORD),FALSE);
    EnableWindow(GetDlgItem(h_main,CHK_NULL_SESSION),FALSE);
    EnableWindow(GetDlgItem(h_main,GRP_PERIMETER),FALSE);
    EnableWindow(GetDlgItem(h_main,IP1),FALSE);
    EnableWindow(GetDlgItem(h_main,BT_IP_CP),FALSE);
    EnableWindow(GetDlgItem(h_main,IP2),FALSE);
    EnableWindow(GetDlgItem(h_main,CHK_LOAD_IP_FILE),FALSE);
    EnableWindow(GetDlgItem(h_main,CHK_ALL_TEST),FALSE);
    EnableWindow(GetDlgItem(h_main,CB_tests),FALSE);
    EnableWindow(GetDlgItem(h_main,BT_LOAD_MDP_FILES),FALSE);

    SetWindowText(GetDlgItem(h_main,BT_START),"Stop");
    AddMsg(h_main, (char*)"INFORMATION",(char*)"Start scan",(char*)"");

    //check test minimals
    config.disco_icmp           = TRUE;
    config.disco_dns            = TRUE;
    config.disco_netbios        = TRUE;
    config.disco_netbios_policy = TRUE;
    config.check_files          = TRUE;
    config.check_registry       = TRUE;

    //get all parameters
    //SAVE
    tmp_check[0] = 0;
    if(GetPrivateProfileString("SAVE","CSV","",tmp_check,LINE_SIZE,ini_path))
    {
      if (tmp_check[0] == 'o' || tmp_check[0] == 'O')auto_scan_config.save_CSV = TRUE;
      else auto_scan_config.save_CSV = FALSE;
    }else auto_scan_config.save_CSV = FALSE;
    tmp_check[0] = 0;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("SAVE","XML","",tmp_check,LINE_SIZE,ini_path))
    {
      if (tmp_check[0] == 'o' || tmp_check[0] == 'O')auto_scan_config.save_XML = TRUE;
      else auto_scan_config.save_XML = FALSE;
    }else auto_scan_config.save_XML = FALSE;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("SAVE","HTML","",tmp_check,LINE_SIZE,ini_path))
    {
      if (tmp_check[0] == 'o' || tmp_check[0] == 'O')auto_scan_config.save_HTML = TRUE;
      else auto_scan_config.save_HTML = FALSE;
    }else auto_scan_config.save_HTML = FALSE;

    //CHECK
    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK","M_SEC","",tmp_check,LINE_SIZE,ini_path))
    {
      if (tmp_check[0] == 'o' || tmp_check[0] == 'O')auto_scan_config.M_SEC = TRUE;
      else auto_scan_config.M_SEC = FALSE;
    }else auto_scan_config.M_SEC = FALSE;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK","PATCH_UPDATED","",tmp_check,LINE_SIZE,ini_path))
    {
      if (tmp_check[0] == 'o' || tmp_check[0] == 'O')auto_scan_config.PATCH_UPDATED = TRUE;
      else auto_scan_config.PATCH_UPDATED = FALSE;
    }else auto_scan_config.PATCH_UPDATED = FALSE;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK","WSUS_WORKS","",tmp_check,LINE_SIZE,ini_path))
    {
      if (tmp_check[0] == 'o' || tmp_check[0] == 'O')auto_scan_config.WSUS_WORKS = TRUE;
      else auto_scan_config.WSUS_WORKS = FALSE;
    }else auto_scan_config.WSUS_WORKS = FALSE;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK","MCAFEE_INSTALLED","",tmp_check,LINE_SIZE,ini_path))
    {
      if (tmp_check[0] == 'o' || tmp_check[0] == 'O')auto_scan_config.MCAFEE_INSTALLED = TRUE;
      else auto_scan_config.MCAFEE_INSTALLED = FALSE;
    }else auto_scan_config.MCAFEE_INSTALLED = FALSE;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK","MCAFEE_UPDATED","",tmp_check,LINE_SIZE,ini_path))
    {
      if (tmp_check[0] == 'o' || tmp_check[0] == 'O')auto_scan_config.MCAFEE_UPDATED = TRUE;
      else auto_scan_config.MCAFEE_UPDATED = FALSE;
    }else auto_scan_config.MCAFEE_UPDATED = FALSE;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK","MCAFEE_SCAN","",tmp_check,LINE_SIZE,ini_path))
    {
      if (tmp_check[0] == 'o' || tmp_check[0] == 'O')auto_scan_config.MCAFEE_SCAN = TRUE;
      else auto_scan_config.MCAFEE_SCAN = FALSE;
    }else auto_scan_config.MCAFEE_SCAN = FALSE;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK","PASSWORD_POLICY","",tmp_check,LINE_SIZE,ini_path))
    {
      if (tmp_check[0] == 'o' || tmp_check[0] == 'O')auto_scan_config.PASSWORD_POLICY = TRUE;
      else auto_scan_config.PASSWORD_POLICY = FALSE;
    }else auto_scan_config.PASSWORD_POLICY = FALSE;

    tmp_check[0]                       = 0;
    auto_scan_config.C_ADMIN_ACCOUNT[0] = 0;
    if(GetPrivateProfileString("CHECK","ADMIN_ACOUNT","",tmp_check,LINE_SIZE,ini_path))
    {
      if (tmp_check[0] == 'o' || tmp_check[0] == 'O')
      {
        auto_scan_config.ADMIN_ACCOUNT = TRUE;
        GetPrivateProfileString("CHECK_OPTIONS","ADMIN_ACCOUNT","",auto_scan_config.C_ADMIN_ACCOUNT,MAX_PATH,ini_path);

      }else auto_scan_config.ADMIN_ACCOUNT = FALSE;
    }else auto_scan_config.ADMIN_ACCOUNT = FALSE;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK","NULL_SESSION","",tmp_check,LINE_SIZE,ini_path))
    {
      if (tmp_check[0] == 'o' || tmp_check[0] == 'O')auto_scan_config.NULL_SESSION = TRUE;
      else auto_scan_config.NULL_SESSION = FALSE;
    }else auto_scan_config.NULL_SESSION = FALSE;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK","REVERS_SID","",tmp_check,LINE_SIZE,ini_path))
    {
      if (tmp_check[0] == 'o' || tmp_check[0] == 'O')auto_scan_config.REVERS_SID = TRUE;
      else auto_scan_config.REVERS_SID = FALSE;
    }else auto_scan_config.REVERS_SID = FALSE;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK","AUTORUN","",tmp_check,LINE_SIZE,ini_path))
    {
      if (tmp_check[0] == 'o' || tmp_check[0] == 'O')auto_scan_config.AUTORUN = TRUE;
      else auto_scan_config.AUTORUN = FALSE;
    }else auto_scan_config.AUTORUN = FALSE;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK","SHARE_ACCESS","",tmp_check,LINE_SIZE,ini_path))
    {
      if (tmp_check[0] == 'o' || tmp_check[0] == 'O')auto_scan_config.SHARE_ACCESS = TRUE;
      else auto_scan_config.SHARE_ACCESS = FALSE;
    }else auto_scan_config.SHARE_ACCESS = FALSE;

    //Options
    auto_scan_config.MSEC_REG_PATH[0] = 0;
    GetPrivateProfileString("CHECK_OPTIONS","MSEC_REG_PATH","",auto_scan_config.MSEC_REG_PATH,MAX_PATH,ini_path);
    auto_scan_config.MSEC_REG_VALUE[0] = 0;
    GetPrivateProfileString("CHECK_OPTIONS","MSEC_REG_VALUE","",auto_scan_config.MSEC_REG_VALUE,MAX_PATH,ini_path);

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK_OPTIONS","PASSWORD_POLICY_COMPLEXITY_ENABLE","",tmp_check,LINE_SIZE,ini_path))
    {
      if (tmp_check[0] == 'o' || tmp_check[0] == 'O')auto_scan_config.PASSWORD_POLICY_COMPLEXITY_ENABLE = TRUE;
      else auto_scan_config.PASSWORD_POLICY_COMPLEXITY_ENABLE = FALSE;
    }else auto_scan_config.PASSWORD_POLICY_COMPLEXITY_ENABLE = FALSE;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK_OPTIONS","MCAFEE_SCAN_DAYS_INTERVAL","",tmp_check,LINE_SIZE,ini_path))
    {
      auto_scan_config.MCAFEE_SCAN_DAYS_INTERVAL = atoi(tmp_check);
    }else auto_scan_config.MCAFEE_SCAN_DAYS_INTERVAL = 0;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK_OPTIONS","MCAFEE_UPDATE_DAYS_INTERVAL","",tmp_check,LINE_SIZE,ini_path))
    {
      auto_scan_config.MCAFEE_UPDATE_DAYS_INTERVAL = atoi(tmp_check);
    }else auto_scan_config.MCAFEE_UPDATE_DAYS_INTERVAL = 0;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK_OPTIONS","PASSWORD_POLICY_MIN_AGE","",tmp_check,LINE_SIZE,ini_path))
    {
      auto_scan_config.PASSWORD_POLICY_MIN_AGE = atoi(tmp_check);
    }else auto_scan_config.PASSWORD_POLICY_MIN_AGE = 0;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK_OPTIONS","PASSWORD_POLICY_MAX_AGE","",tmp_check,LINE_SIZE,ini_path))
    {
      auto_scan_config.PASSWORD_POLICY_MAX_AGE = atoi(tmp_check);
    }else auto_scan_config.PASSWORD_POLICY_MAX_AGE = 0;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK_OPTIONS","PASSWORD_POLICY_MIN_LEN","",tmp_check,LINE_SIZE,ini_path))
    {
      auto_scan_config.PASSWORD_POLICY_MIN_LEN = atoi(tmp_check);
    }else auto_scan_config.PASSWORD_POLICY_MIN_LEN = 0;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK_OPTIONS","PASSWORD_POLICY_LOCKOUT_COUNT","",tmp_check,LINE_SIZE,ini_path))
    {
      auto_scan_config.PASSWORD_POLICY_LOCKOUT_COUNT = atoi(tmp_check);
    }else auto_scan_config.PASSWORD_POLICY_LOCKOUT_COUNT = 0;

    tmp_check[0] = 0;
    if(GetPrivateProfileString("CHECK_OPTIONS","PASSWORD_POLICY_HISTORY","",tmp_check,LINE_SIZE,ini_path))
    {
      auto_scan_config.PASSWORD_POLICY_HISTORY = atoi(tmp_check);
    }else auto_scan_config.PASSWORD_POLICY_HISTORY = 0;

    //start init scan
    time_t exec_time_start, exec_time_end;
    time(&exec_time_start);

    char tmp[MAX_PATH];
    nb_windows        = 0;

    //---------------------------------------------
    //scan_start
    DWORD i;
    nb_i = SendDlgItemMessage(h_main,CB_IP,LB_GETCOUNT,(WPARAM)NULL,(LPARAM)NULL);
    char ip_test[MAX_PATH];

    hs_threads  = CreateSemaphore(NULL,NB_MAX_THREAD,NB_MAX_THREAD,NULL);
    hs_disco    = CreateSemaphore(NULL,NB_MAX_DISCO_THREADS,NB_MAX_DISCO_THREADS,NULL);
    hs_netbios  = CreateSemaphore(NULL,NB_MAX_NETBIOS_THREADS,NB_MAX_NETBIOS_THREADS,NULL);
    hs_file     = CreateSemaphore(NULL,NB_MAX_FILE_THREADS,NB_MAX_FILE_THREADS,NULL);
    hs_registry = CreateSemaphore(NULL,NB_MAX_REGISTRY_THREADS,NB_MAX_REGISTRY_THREADS,NULL);
    hs_tcp      = CreateSemaphore(NULL,NB_MAX_TCP_TEST_THREADS,NB_MAX_TCP_TEST_THREADS,NULL);
    hs_ssh      = CreateSemaphore(NULL,NB_MAX_SSH_THREADS,NB_MAX_SSH_THREADS,NULL);

    //wsa init
    WSADATA WSAData;
    WSAStartup(0x02, &WSAData );
    nb_test_ip = 0;
    InitializeCriticalSection(&Sync);

    for (i=0;(i<nb_i) && scan_start;i++)
    {
      WaitForSingleObject(hs_threads,INFINITE);
      CreateThread(NULL,0,ScanIp_auto_scan,(PVOID)i,0,0);
    }

    //wait
    AddMsg(h_main,(char*)"INFORMATION",(char*)"Start waiting threads.",(char*)"");

    if (!scan_start)
    {
      while (nb_test_ip < i && scan_start)Sleep(100);
    }else
    {
      for(i=0;i<NB_MAX_THREAD;i++)WaitForSingleObject(hs_threads,INFINITE);

      WaitForSingleObject(hs_netbios,INFINITE);
      WaitForSingleObject(hs_file,INFINITE);
      WaitForSingleObject(hs_registry,INFINITE);
      WaitForSingleObject(hs_tcp,INFINITE);
      WaitForSingleObject(hs_ssh,INFINITE);
    }

    WSACleanup();

    //calcul run time
    time(&exec_time_end);

    AddMsg(h_main,(char*)"INFORMATION",(char*)"End of scan!",(char*)"");
    snprintf(tmp,MAX_PATH,"Ip view : %lu/%lu in %d.%0d minutes",ListView_GetItemCount(GetDlgItem(h_main,LV_results)),nb_i,(exec_time_end - exec_time_start)/60,(exec_time_end - exec_time_start)%60);
    AddMsg(h_main,(char*)"INFORMATION",(char*)tmp,(char*)"");
    snprintf(tmp,MAX_PATH,"Computer in Microsoft Windows OS : %lu/%lu",nb_windows,nb_i);
    AddMsg(h_main,(char*)"INFORMATION",(char*)tmp,(char*)"");

    //close
    DeleteCriticalSection(&Sync);
    CloseHandle(hs_threads);
    CloseHandle(hs_disco);
    CloseHandle(hs_netbios);
    CloseHandle(hs_file);
    CloseHandle(hs_registry);
    CloseHandle(hs_tcp);
    CloseHandle(hs_ssh);

    //save
    char file2[LINE_SIZE];
    time_t dateEtHMs;
    time(&dateEtHMs);
    struct tm *today = localtime(&dateEtHMs);

    char date[DATE_SIZE];
    strftime(date, DATE_SIZE,"%Y/%m/%d-%H:%M:%S",today);

    if (auto_scan_config.save_CSV)
    {
      snprintf(file2,LINE_SIZE,"[%s]_auto_scan_NS.csv",date);
      if(SaveLSTV(GetDlgItem(h_main,LV_results), file2, SAVE_TYPE_CSV, NB_COLUMN)) AddMsg(h_main, (char*)"INFORMATION",(char*)"Recorded data",file2);
      else AddMsg(h_main, (char*)"ERROR",(char*)"No data saved to!",file2);
    }

    if (auto_scan_config.save_XML)
    {
      snprintf(file2,LINE_SIZE,"[%s]_auto_scan_NS.xml",date);
      if(SaveLSTV(GetDlgItem(h_main,LV_results), file2, SAVE_TYPE_XML, NB_COLUMN)) AddMsg(h_main, (char*)"INFORMATION",(char*)"Recorded data",file2);
      else AddMsg(h_main, (char*)"ERROR",(char*)"No data saved to!",file2);
    }

    if (auto_scan_config.save_HTML)
    {
      snprintf(file2,LINE_SIZE,"[%s]_auto_scan_NS.html",date);
      if(SaveLSTV(GetDlgItem(h_main,LV_results), file2, SAVE_TYPE_HTML, NB_COLUMN)) AddMsg(h_main, (char*)"INFORMATION",(char*)"Recorded data",file2);
      else AddMsg(h_main, (char*)"ERROR",(char*)"No data saved to!",file2);
    }

    EnableWindow(GetDlgItem(h_main,ED_NET_DOMAIN),TRUE);
    EnableWindow(GetDlgItem(h_main,ED_NET_LOGIN),TRUE);
    EnableWindow(GetDlgItem(h_main,ED_NET_PASSWORD),TRUE);
    EnableWindow(GetDlgItem(h_main,CHK_NULL_SESSION),TRUE);
    EnableWindow(GetDlgItem(h_main,GRP_PERIMETER),TRUE);
    EnableWindow(GetDlgItem(h_main,IP1),TRUE);
    EnableWindow(GetDlgItem(h_main,BT_IP_CP),TRUE);
    EnableWindow(GetDlgItem(h_main,IP2),TRUE);
    EnableWindow(GetDlgItem(h_main,CHK_LOAD_IP_FILE),TRUE);
    EnableWindow(GetDlgItem(h_main,CHK_ALL_TEST),TRUE);
    EnableWindow(GetDlgItem(h_main,CB_tests),TRUE);
    EnableWindow(GetDlgItem(h_main,BT_LOAD_MDP_FILES),TRUE);
    scan_start = FALSE;

    SetWindowText(GetDlgItem(h_main,BT_START),"Start");
    SetWindowText(h_main,TITLE);
    return 0;
  }

  return 0;
}