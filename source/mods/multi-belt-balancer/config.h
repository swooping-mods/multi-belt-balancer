/* Load private per-mod settings; absent values retain bounded defaults. */
static char cfg_path[1200];
static unsigned (__stdcall *cfg_read)(const char *,const char *,int,const char *);
static void cfg_start(const char *environment,const char *fallback){
 PTR k=GetModuleHandleA("kernel32.dll");cfg_read=(unsigned(__stdcall *)(const char *,const char *,int,const char *))GetProcAddress(k,"GetPrivateProfileIntA");
 DWORD n=GetEnvironmentVariableA(environment,cfg_path,sizeof(cfg_path));if(!n||n>=sizeof(cfg_path))cat(cfg_path,fallback);
}
static int cfg(const char *name,int fallback,int low,int high){int v=cfg_read?(int)cfg_read("settings",name,fallback,cfg_path):fallback;return v<low?low:(v>high?high:v);}
