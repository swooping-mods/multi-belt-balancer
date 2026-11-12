/* Local source copy: each mod is independently loadable. */
#include "api.h"
#include "../../runtime/native/input.h"
#include "../../runtime/native/local_actor.h"
#include "../build-menu-framework/sdk/construction_unity.h"
#include "shop_signal.h"
#include "../../runtime/sdk/wallet.h"
static struct {
    PTR log,world_field,alive,restoring,focused,typing,blocked,first,frame,delta;
    PTR joined,host,local,player,player_equal,wallet,decimal_number,decimal_int,decimal_add,decimal_compare,charge;
    PTR camera,transform,position,forward;
    void (*set_field)(PTR,PTR,void *);
    int (*value_type)(PTR);
    short (*key)(int);PTR (*foreground)(void);DWORD (*pid)(PTR,DWORD *);
    BOOL (*title)(PTR,const char *);
    DRoot world,walker;int last_frame,eligible,host_ready,disabled;DWORD thread;
    unsigned keys[4];
} bm;
static void bevent(const char *s){char b[512],*p=cat(b,"{\"event\":\"");p=cat(p,s);p=cat(p,"\"}\n");DWORD n=0;WriteFile(bm.log,b,(DWORD)(p-b),&n,0);FlushFileBuffers(bm.log);}
static PTR bfield(PTR p,const char *name){return moon_build_field(p,name);}
static int bint(PTR p,const char *name){return moon_build_int(p,name);}
static int bhost(void){int ok=1;if(!boolean(bm.joined,0,&ok))return ok;U64 a=0,b=0;PTR h=call(bm.host,0,0,&ok);if(h)dcopy(&a,unbox(h),8);PTR l=call(bm.local,0,0,&ok);if(l)dcopy(&b,unbox(l),8);return ok&&a&&a==b;}
static int baction(const char *name,int key){return moon_input_key_action(name,(unsigned)key,bm.eligible).pressed;}
static int bcontext(PTR world,PTR walker){
 if(!world||!walker||bm.disabled||!moon_local_actor_admit(walker)||!bhost())return 0;
 int ok=1;void *args[]={world};PTR live=call(bm.alive,0,args,&ok);
 if(!ok||!live||!*(unsigned char *)unbox(live))return 0;
 args[0]=walker;live=call(bm.alive,0,args,&ok);
 if(!ok||!live||!*(unsigned char *)unbox(live)||boolean(bm.restoring,world,&ok)||!ok)return 0;
 PTR actual=0;field_value(walker,bm.world_field,&actual);
 return actual==world&&dget(bm.world)==world&&dget(bm.walker)==walker;
}
typedef struct {DRoot player,expected,world,walker;} MBWalletContext;
static int mb_wallet_context(MBWalletContext *context){
 if(!bcontext(dget(context->world),dget(context->walker)))return 0;
 int ok=1;DRoot current=dhold(call(bm.player,0,0,&ok));
 void *args[]={dget(context->player),dget(current)};
 PTR same=ok&&current?call(bm.player_equal,0,args,&ok):0;
 int admitted=ok&&same&&*(unsigned char *)unbox(same);ddrop(&current);
 return admitted&&bcontext(dget(context->world),dget(context->walker));
}
static int mb_wallet_balance(void *raw,long long *balance){
 MBWalletContext *context=(MBWalletContext *)raw;int ok=1;
 if(!mb_wallet_context(context))return 0;
 void *args[]={dget(context->player)};DRoot money=dhold(call(bm.wallet,dget(context->world),args,&ok));
 if(!money||!ok){ddrop(&money);return 0;}
 void *value[]={unbox(dget(money))};PTR amount=call(bm.decimal_number,0,value,&ok);
 if(amount&&ok)*balance=*(int *)unbox(amount);else ok=0;
 if(ok&&context->expected){void *compare[]={unbox(dget(money)),unbox(dget(context->expected))};PTR same=call(bm.decimal_compare,0,compare,&ok);if(!same||!ok||*(int *)unbox(same)!=0)ok=0;}
 if(ok&&!context->expected){context->expected=money;money=0;}
 ddrop(&money);return ok&&mb_wallet_context(context);
}
static int mb_wallet_adjust(void *raw,int delta){
 MBWalletContext *context=(MBWalletContext *)raw;int ok=1;unsigned char value[16]={0};
 void *ctor[]={&delta};call(bm.decimal_int,value,ctor,&ok);
 void *sum[]={unbox(dget(context->expected)),value};DRoot expected=ok?dhold(call(bm.decimal_add,0,sum,&ok)):0;
 if(!ok||!expected){ddrop(&expected);return 0;}
 ddrop(&context->expected);context->expected=expected;
 if(!mb_wallet_context(context))return 0;
 void *args[]={dget(context->player),value};if(ok)call(bm.charge,dget(context->world),args,&ok);
 return ok;
}
static int bcharge(int cost){
 if(bm.disabled||!bm.host_ready)return 0;
 int ok=1;MBWalletContext context={0};context.world=dhold(dget(bm.world));context.walker=dhold(dget(bm.walker));context.player=dhold(call(bm.player,0,0,&ok));
 if(!ok||!context.player||!context.world||!context.walker){ddrop(&context.player);ddrop(&context.world);ddrop(&context.walker);return 0;}
 const MoonWallet wallet={mb_wallet_balance,mb_wallet_adjust};
 int result=mw_debit(&wallet,&context,cost,0);ddrop(&context.player);ddrop(&context.expected);ddrop(&context.world);ddrop(&context.walker);
 /* A native exception may follow a mutation. Never retry an uncertain debit. */
 if(result==MW_DEBIT_UNCONFIRMED||result==MW_DEBIT_REFUSED){bm.disabled=1;bevent("balancer-payment-uncertain");}
 return result==MW_PAID;
}
static int bframe(Context *context){
    if(bm.disabled)return 0;PTR actor=0;if(!moon_local_actor_read(&actor))return 0;if(!actor){ddrop(&bm.world);ddrop(&bm.walker);bm.host_ready=bm.eligible=0;return -1;}if(actor!=(PTR)context->rcx)return 0;if(!bm.thread)bm.thread=GetCurrentThreadId();if(bm.thread!=GetCurrentThreadId())return 0;
    int ok=1,frame=integer(bm.frame,0,&ok);if(!ok||frame==bm.last_frame)return 0;bm.last_frame=frame;
    PTR walker=(PTR)context->rcx,world=0;field_value(walker,bm.world_field,&world);
    if(world!=dget(bm.world)||walker!=dget(bm.walker)){ddrop(&bm.world);ddrop(&bm.walker);bm.world=dhold(world);bm.walker=dhold(walker);bm.eligible=0;return -1;}
    DWORD pid=0;PTR window=bm.foreground();if(window)bm.pid(window,&pid);
    bm.host_ready=bcontext(world,walker);
    bm.eligible=bm.host_ready&&pid==GetCurrentProcessId()&&boolean(bm.focused,0,&ok)&&!boolean(bm.typing,0,&ok)&&!boolean(bm.blocked,walker,&ok)&&boolean(bm.first,walker,&ok)&&ok;
    return 1;
}
static void bstatus(const char *s){DWORD pid=0;PTR w=bm.foreground();if(w)bm.pid(w,&pid);if(pid==GetCurrentProcessId())bm.title(w,s);}
static int baim(DVec *origin,DVec *direction){int ok=1;DRoot camera=dhold(call(bm.camera,0,0,&ok)),tr=camera?dhold(call(bm.transform,dget(camera),0,&ok)):0;if(tr){*origin=dvector(bm.position,dget(tr),&ok);*direction=dvector(bm.forward,dget(tr),&ok);}else ok=0;ddrop(&tr);ddrop(&camera);return ok;}
static float bscore(DVec origin,DVec direction,DVec point){return moon_build_target_score((MoonBuildPoint){origin.x,origin.y,origin.z},(MoonBuildPoint){direction.x,direction.y,direction.z},(MoonBuildPoint){point.x,point.y,point.z},8,.8f,.005f,1e8f);}
static DRoot benum(PTR collection,int *ok){return moon_build_enumerate(collection,ok);}
static PTR bnext(DRoot iterator,int *ok){return moon_build_next(iterator,bm.value_type,ok);}
static void bend(DRoot *it){moon_build_end(it,bm.value_type);}
static int bstart(PTR domain,PTR game,const char *id){
    if(!bind_runtime(domain,game)||!dapi(game)||!moon_local_actor_bind())return 0;
    char folder[1000],path[1200];DWORD n=GetEnvironmentVariableA("MOONLAB_EVIDENCE",folder,sizeof(folder));if(!n||n>=sizeof(folder))return 0;char *p=cat(path,folder);p=cat(p,"\\");p=cat(p,id);cat(p,".jsonl");bm.log=CreateFileA(path,0x40000000,1,0,1,0x80,0);if(bm.log==(PTR)(U64)-1)return 0;
#define BM(F,NS,T,N,S) bm.F=dmethod(NS,T,N,S);if(!bm.F){bevent(T "::" N);return 0;}
    BM(alive,"UnityEngine","Object","op_Implicit","UnityEngine.Object");BM(restoring,"MoonCleaners","MoonWorld","get_IsRestoringRendering","");BM(focused,"UnityEngine","Application","get_isFocused","");BM(typing,"MoonCleaners","MoonInput","get_Typing","");BM(blocked,"MoonCleaners","MoonWalker","get_MenuBlocked","");BM(first,"MoonCleaners","MoonWalker","get_IsFirstPerson","");BM(frame,"UnityEngine","Time","get_frameCount","");BM(delta,"UnityEngine","Time","get_deltaTime","");
    BM(joined,"MoonCleaners","MoonCrew","get_Joined","");BM(host,"MoonCleaners","MoonCrew","get_HostId","");BM(local,"Steamworks","SteamUser","GetSteamID","");BM(player,"MoonCleaners","MoonCrew","get_LocalPlayerId","");BM(player_equal,"System","String","op_Equality","System.String;System.String");BM(wallet,"MoonCleaners","MoonWorld","WalletDollars","System.String");
    /* Decimal has several return-only overloads: resolve the integer converter below. */
#undef BM
    bm.decimal_add=dmethod("System","Decimal","Add","System.Decimal;System.Decimal");bm.decimal_compare=dmethod("System","Decimal","Compare","System.Decimal;System.Decimal");
    bm.decimal_number=dmethod("System","Decimal","ToInt32","System.Decimal");
    bm.decimal_int=dmethod("System","Decimal",".ctor","System.Int32");bm.charge=dmethod("MoonCleaners","MoonWorld","ChargeBuild","System.String;System.Decimal");bm.camera=dmethod("UnityEngine","Camera","get_main","");bm.transform=dmethod("UnityEngine","Component","get_transform","");bm.position=dmethod("UnityEngine","Transform","get_position","");bm.forward=dmethod("UnityEngine","Transform","get_forward","");
    bm.value_type=(int(*)(PTR))GetProcAddress(game,"il2cpp_class_is_valuetype");bm.world_field=get_field(find_class("MoonCleaners","MoonWalker"),"world");bm.set_field=(void(*)(PTR,PTR,void *))GetProcAddress(game,"il2cpp_field_set_value");PTR user=LoadLibraryA("user32.dll");bm.key=(short(*)(int))GetProcAddress(user,"GetAsyncKeyState");bm.foreground=(PTR(*)(void))GetProcAddress(user,"GetForegroundWindow");bm.pid=(DWORD(*)(PTR,DWORD *))GetProcAddress(user,"GetWindowThreadProcessId");bm.title=(BOOL(*)(PTR,const char *))GetProcAddress(user,"SetWindowTextA");bm.last_frame=-1;
    (void)moon_shop_unlisten;(void)baction;(void)dzero;(void)dtype;(void)darray;(void)dnumber;(void)bint;(void)bfield;(void)bcharge;(void)bstatus;(void)baim;(void)bscore;(void)benum;(void)bnext;(void)bend;
    return bm.value_type&&bm.world_field&&bm.set_field&&bm.key&&bm.foreground&&bm.pid&&bm.title&&bm.decimal_int&&bm.decimal_number&&bm.decimal_add&&bm.decimal_compare&&bm.charge&&bm.camera&&bm.transform&&bm.position&&bm.forward&&moon_shop_listen(id);
}
