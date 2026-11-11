#include "../../runtime/native/gameplay.h"
#include "building.h"
#include "config.h"
int _fltused=0;
typedef struct {int n,in[4],out[4],next;} Junction;
static Junction junctions[32],draft;static int count,setup,kin,kout,kvariant,kclear,price;
static PTR conveyors,path,nearest,length,enter,route_point;
static int keys[2048],nkeys;static float reach;
static int aimed(void){
 DVec o,d;if(!baim(&o,&d))return 0;DVec point={o.x+d.x*3,o.y+d.y*3,o.z+d.z*3};int ok=1,result=0;float best=.5f;
 DRoot records=dhold(call(conveyors,dget(bm.world),0,&ok)),it=benum(dget(records),&ok);PTR rec;
 while((rec=bnext(it,&ok))){void *a[]={rec};DRoot route=dhold(call(path,dget(bm.world),a,&ok));void *b[]={dget(route),&point};PTR v=call(nearest,0,b,&ok);float along=v?*(float *)unbox(v):0;void *pa[]={&along};PTR sampled=call(route_point,dget(route),pa,&ok);DVec p={0};if(sampled)dcopy(&p,unbox(sampled),sizeof(p));float x=p.x-point.x,y=p.y-point.y,z=p.z-point.z,q=sampled?x*x+y*y+z*z:1e9f;if(q<best){best=q;result=bint(rec,"id");}ddrop(&route);}bend(&it);ddrop(&records);return ok?result:0;
}
static DRoot record(int id,int *ok){PTR dict=bfield(dget(bm.world),"conveyors");if(!dict)return 0;void *a[]={&id};PTR has=call(get_method(object_class(dict),"ContainsKey",1),dict,a,ok);return has&&*(unsigned char *)unbox(has)?dhold(call(get_method(object_class(dict),"get_Item",1),dict,a,ok)):0;}
static DVec point(PTR rec,const char *name){DVec v={0};PTR f=get_field(object_class(rec),name);if(f)field_value(rec,f,&v);return v;}
static float distance2(DVec a,DVec b){float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z;}
#include "model.h"
static void add(int output){
 int id=aimed();if(!id||count>=32)return;
 for(int j=0;j<count;j++)for(int i=0;i<junctions[j].n;i++)if(junctions[j].in[i]==id||junctions[j].out[i]==id)return;
 for(int i=0;i<4;i++)if(draft.in[i]==id||draft.out[i]==id)return;
 int *list=output?draft.out:draft.in;for(int i=0;i<draft.n;i++)if(!list[i]){list[i]=id;break;}
 for(int i=0;i<draft.n;i++)if(!draft.in[i]||!draft.out[i])return;
 /* Require a compact physical junction. This is a transfer across selected
  * existing belt endpoints, with native admission and cargo placement. */
 int ok=1;for(int i=0;i<draft.n;i++){DRoot a=record(draft.in[i],&ok);if(!a)return;DVec end=point(dget(a),"end");ddrop(&a);for(int j=0;j<draft.n;j++){DRoot b=record(draft.out[j],&ok);if(!b)return;DVec start=point(dget(b),"start");ddrop(&b);if(distance2(start,end)>reach*reach){setup=0;dzero(&draft,sizeof(draft));draft.n=2;bevent("junction-endpoints-too-far");return;}}}
 if(ok&&bcharge(price*draft.n)){junctions[count++]=draft;bevent("balancer-committed");mbm_update();}dzero(&draft,sizeof(draft));draft.n=2;setup=0;
}
static void balance(void){
 PTR riders=bfield(dget(bm.world),"riders");if(!riders)return;int ok=1;nkeys=0;PTR collection=call(get_method(object_class(riders),"get_Keys",0),riders,0,&ok);DRoot it=benum(collection,&ok);PTR item;
 while(nkeys<2048&&(item=bnext(it,&ok)))keys[nkeys++]=*(int *)unbox(item);bend(&it);
 PTR get=get_method(object_class(riders),"get_Item",1);
 for(int j=0;j<count&&ok;j++){
  Junction *junction=junctions+j;
  /* Visit inputs in rotation too, so a full first input cannot starve peers. */
  for(int ii=0;ii<junction->n&&ok;ii++){
   int input=junction->in[(ii+bm.last_frame)%junction->n];DRoot rec=record(input,&ok);if(!rec)continue;void *a[]={dget(rec)};DRoot route=dhold(call(path,dget(bm.world),a,&ok));float len=dnumber(length,dget(route),&ok);ddrop(&route);ddrop(&rec);if(!ok)break;
   for(int k=0;k<nkeys&&ok;k++){
    void *key[]={keys+k};DRoot rider=dhold(call(get,riders,key,&ok));PTR v=dget(rider);int belt=bint(v,"belt");float d=0;PTR f=v?get_field(object_class(v),"distance"):0;if(f)field_value(v,f,&d);ddrop(&rider);if(belt!=input||d<len-.35f)continue;
    for(int attempt=0;attempt<junction->n;attempt++){
     int index=(junction->next+attempt)%junction->n,output=junction->out[index];DRoot out=record(output,&ok);if(!out)continue;ddrop(&out);
     /* Enter(belt, capsule) owns spacing, rider record replacement and the
      * world-space capsule pose. A blocked output consumes no round-robin turn. */
     void *args[]={&output,keys+k};PTR accepted=call(enter,dget(bm.world),args,&ok);
     if(accepted&&*(unsigned char *)unbox(accepted)){junction->next=(index+1)%junction->n;break;}
    }
   }
  }
 }
 if(!ok){bm.disabled=1;bevent("balancer-native-transfer-failed");}
}
static int before(int hook,Context *c){
 (void)hook;int state=bframe(c),input=baction("multi-belt-balancer.input",kin),output=baction("multi-belt-balancer.output",kout),variant=baction("multi-belt-balancer.variant",kvariant)|moon_shop_poll(),clear=baction("multi-belt-balancer.cancel",kclear);
 if(state<0){mbm_drop();count=setup=0;dzero(&draft,sizeof(draft));draft.n=2;return 0;}if(state<=0)return 0;if(bm.host_ready){balance();if(bm.disabled){mbm_drop();return 0;}int time_ok=1;float dt=dnumber(bm.delta,0,&time_ok);if(time_ok&&dt>=0&&dt<1)mbm.clock-=dt;if(mbm.clock<=0){mbm.clock=1;mbm_update();}}if(!bm.eligible)return 0;
 if(variant){if(!setup){setup=1;draft.n=2;}else{int n=draft.n==4?2:draft.n+1;dzero(&draft,sizeof(draft));draft.n=n;}}
 if(setup&&input)add(0);if(setup&&output)add(1);if(clear&&setup){setup=0;dzero(&draft,sizeof(draft));draft.n=2;}
 if(setup)bstatus(draft.n==2?"Moon Sweepers | 2x2 balancer | I input / O output / F5 variant / Backspace cancel":draft.n==3?"Moon Sweepers | 3x3 balancer | I input / O output / F5 variant / Backspace cancel":"Moon Sweepers | 4x4 balancer | I input / O output / F5 variant / Backspace cancel");return 0;
}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
 if(!bstart(domain,game,"multi-belt-balancer")||!su_bind(game))return 0;cfg_start("MOONLAB_BALANCER_CONFIG","MoonLab\\mods\\multi-belt-balancer\\settings.ini");kin=cfg("InputKey",'I',1,255);kout=cfg("OutputKey",'O',1,255);kvariant=cfg("VariantKey",0x74,1,255);kclear=cfg("CancelKey",8,1,255);price=cfg("PricePerLane",40,0,10000);reach=(float)cfg("MaxJunctionMetres",12,1,30);
 conveyors=dmethod("MoonCleaners","MoonWorld","get_BuiltConveyors","");path=dmethod("MoonCleaners","MoonWorld","ConveyorPath","MoonCleaners.MoonConveyorSnapshot");nearest=dmethod("MoonCleaners","MoonWorld","NearestDistance","MoonCleaners.MoonConveyorPath;UnityEngine.Vector3");length=dmethod("MoonCleaners","MoonConveyorPath","get_Length","");enter=dmethod("MoonCleaners","MoonWorld","Enter","System.Int32;System.Int32");
 route_point=dmethod("MoonCleaners","MoonConveyorPath","Point","System.Single");if(!route_point||!conveyors||!path||!nearest||!length||!enter)return 0;if(!prepare_hook(method("MoonCleaners","MoonWalker","Update",0),100671733,0,game))return 0;before_hook=before;if(!install_hooks())return 0;bevent("ready");return 1;
}
