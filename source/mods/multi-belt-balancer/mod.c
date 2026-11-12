#include "../../runtime/native/gameplay.h"
#include "building.h"
#include "config.h"
int _fltused=0;
#include "state.h"
static Junction junctions[32],draft;static int count,setup,kin,kout,kvariant,kclear,kremove,price;
static PTR conveyors,path,nearest,length,enter,route_point;
static int keys[2048],nkeys,key_page,was_host,rider_limit_reported;static float reach;
static int aimed(void){
 DVec o,d;if(!baim(&o,&d))return 0;DVec point={o.x+d.x*3,o.y+d.y*3,o.z+d.z*3};int ok=1,result=0,scanned=0;float best=.5f;
 DRoot records=dhold(call(conveyors,dget(bm.world),0,&ok)),it=benum(dget(records),&ok);PTR rec;
 while(scanned<4096&&(rec=bnext(it,&ok))){++scanned;DRoot selected=dhold(rec);void *a[]={dget(selected)};DRoot route=dhold(call(path,dget(bm.world),a,&ok));void *b[]={dget(route),&point};PTR v=call(nearest,0,b,&ok);float along=v?*(float *)unbox(v):0;void *pa[]={&along};PTR sampled=call(route_point,dget(route),pa,&ok);DVec p={0};if(sampled)dcopy(&p,unbox(sampled),sizeof(p));float x=p.x-point.x,y=p.y-point.y,z=p.z-point.z,q=sampled?x*x+y*y+z*z:1e9f;if(q<best){best=q;result=bint(dget(selected),"id");}ddrop(&route);ddrop(&selected);}if(scanned==4096&&bnext(it,&ok))result=0;bend(&it);ddrop(&records);return ok?result:0;
}
static DRoot record(int id,int *ok){
 DRoot dict=dhold(bfield(dget(bm.world),"conveyors"));if(!dict)return 0;
 void *a[]={&id};PTR has=call(get_method(object_class(dget(dict)),"ContainsKey",1),dget(dict),a,ok);
 DRoot result=has&&*ok&&*(unsigned char *)unbox(has)?dhold(call(get_method(object_class(dget(dict)),"get_Item",1),dget(dict),a,ok)):0;
 ddrop(&dict);return result;
}
static DVec point(PTR rec,const char *name){DVec v={0};PTR f=get_field(object_class(rec),name);if(f)field_value(rec,f,&v);return v;}
static void drop_belt(MBBelt *belt){ddrop(&belt->root);dzero(belt,sizeof(*belt));}
static void clear_draft(void){for(int i=0;i<4;i++){drop_belt(draft.in+i);drop_belt(draft.out+i);}dzero(&draft,sizeof(draft));draft.n=2;}
static int capture(int id,MBBelt *belt,int *ok){
 DRoot rec=record(id,ok);if(!rec)return 0;
 PTR klass=object_class(dget(rec)),start=get_field(klass,"start"),end=get_field(klass,"end"),start_direction=get_field(klass,"startDirection"),end_direction=get_field(klass,"endDirection");
 if(!start||!end||!start_direction||!end_direction){*ok=0;ddrop(&rec);return 0;}
 belt->id=id;field_value(dget(rec),start,&belt->start);field_value(dget(rec),end,&belt->end);field_value(dget(rec),start_direction,&belt->start_direction);field_value(dget(rec),end_direction,&belt->end_direction);belt->root=rec;return *ok;
}
static int valid(Junction *j,int *ok){
 for(int i=0;i<j->n;i++){
  MBBelt a={0},b={0};int same=capture(j->in[i].id,&a,ok)&&capture(j->out[i].id,&b,ok)&&mb_belt_same(a,j->in[i],(U64)dget(a.root),(U64)dget(j->in[i].root))&&mb_belt_same(b,j->out[i],(U64)dget(b.root),(U64)dget(j->out[i].root));
  drop_belt(&a);drop_belt(&b);if(!same)return 0;
 }
 return *ok;
}
#include "model.h"
static void remove_junction(int index){
 mbm_hide((unsigned)index);
 for(int i=0;i<junctions[index].n;i++){drop_belt(junctions[index].in+i);drop_belt(junctions[index].out+i);}
 for(int j=index;j<count-1;j++){
  junctions[j]=junctions[j+1];mbm.roots[j]=mbm.roots[j+1];mbm.up[j]=mbm.up[j+1];mbm.state[j]=mbm.state[j+1];
 }
 --count;dzero(junctions+count,sizeof(*junctions));mbm.roots[count]=0;mbm.up[count]=su_v(0,0,0);mbm.state[count]=0;
}
static void reset_session(void){
 mbm_drop();for(int j=0;j<count;j++)for(int i=0;i<junctions[j].n;i++){drop_belt(junctions[j].in+i);drop_belt(junctions[j].out+i);}
 dzero(junctions,sizeof(junctions));count=setup=nkeys=key_page=was_host=rider_limit_reported=0;clear_draft();if(bm.disabled){ddrop(&bm.world);ddrop(&bm.walker);}
}
static void add(int output){
 int id=aimed();if(!id||count>=32)return;
 for(int j=0;j<count;j++)for(int i=0;i<junctions[j].n;i++)if(junctions[j].in[i].id==id||junctions[j].out[i].id==id)return;
 for(int i=0;i<4;i++)if(draft.in[i].id==id||draft.out[i].id==id)return;
 MBBelt *list=output?draft.out:draft.in;int ok=1;
 for(int i=0;i<draft.n;i++)if(!list[i].id){if(!capture(id,list+i,&ok))return;break;}
 for(int i=0;i<draft.n;i++)if(!draft.in[i].id||!draft.out[i].id)return;
 /* Recheck the complete staged identities before the only wallet mutation. */
 if(!valid(&draft,&ok)){bevent("balancer-draft-invalidated");goto done;}
 for(int i=0;i<draft.n;i++)for(int j=0;j<draft.n;j++){
  MBPoint a=draft.in[i].end,b=draft.out[j].start;float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;
  if(!(x*x+y*y+z*z<=reach*reach)){bevent("junction-endpoints-too-far");goto done;}
 }
 if(bcharge(price*draft.n)){junctions[count++]=draft;dzero(&draft,sizeof(draft));bevent("balancer-committed");mbm_update();}
 done:clear_draft();setup=0;
 if(bm.disabled)reset_session();
}
typedef struct {DRoot world,walker;int capsule;} MBTransferContext;
static int transfer(void *raw,int output,int *ok){
 MBTransferContext *context=(MBTransferContext *)raw;
 if(!bcontext(dget(context->world),dget(context->walker))){*ok=0;return 0;}
 void *args[]={&output,&context->capsule};PTR accepted=call(enter,dget(context->world),args,ok);
 return *ok&&accepted&&*(unsigned char *)unbox(accepted);
}
static void balance(void){
 if(!count)return;
 MBTransferContext context={dhold(dget(bm.world)),dhold(dget(bm.walker)),0};
 DRoot riders=dhold(bfield(dget(bm.world),"riders"));if(!riders||!context.world||!context.walker){ddrop(&riders);ddrop(&context.world);ddrop(&context.walker);return;}
 int ok=1,total=0;nkeys=0;
 int expected=integer(get_method(object_class(dget(riders)),"get_Count",0),dget(riders),&ok);
 if(ok&&expected>MB_MAX_RIDERS){if(!rider_limit_reported)bevent("balancer-rider-limit");rider_limit_reported=1;ddrop(&riders);ddrop(&context.world);ddrop(&context.walker);return;}
 rider_limit_reported=0;
 DRoot collection=dhold(call(get_method(object_class(dget(riders)),"get_Keys",0),dget(riders),0,&ok));
 DRoot it=benum(dget(collection),&ok);PTR item;
 /* Finish enumeration before Enter mutates the rider dictionary. Paging avoids
  * permanently starving IDs beyond the fixed-capacity snapshot. */
 while(total<=MB_MAX_RIDERS&&(item=bnext(it,&ok))){if(total>=key_page&&nkeys<2048)keys[nkeys++]=*(int *)unbox(item);++total;}
 bend(&it);ddrop(&collection);if(total>MB_MAX_RIDERS||total!=expected)ok=0;key_page=mb_page_next(key_page,total,2048);
 PTR get=get_method(object_class(dget(riders)),"get_Item",1),has=get_method(object_class(dget(riders)),"ContainsKey",1);
 for(int j=0;j<count&&ok;){
  Junction *junction=junctions+j;
  if(!valid(junction,&ok)){if(!ok)break;remove_junction(j);bevent("balancer-endpoint-invalidated");continue;}
  int first=mb_input_turn(junction);
  for(int ii=0;ii<junction->n&&ok;ii++){
   int input=junction->in[(ii+first)%junction->n].id;DRoot rec=record(input,&ok);
   if(!rec)continue;
   void *a[]={dget(rec)};DRoot route=dhold(call(path,dget(bm.world),a,&ok));float len=dnumber(length,dget(route),&ok);ddrop(&route);ddrop(&rec);if(!ok)break;
   for(int k=0;k<nkeys&&ok;k++){
    void *key[]={keys+k};PTR present=call(has,dget(riders),key,&ok);if(!ok||!present||!*(unsigned char *)unbox(present))continue;
    DRoot rider=dhold(call(get,dget(riders),key,&ok));PTR v=dget(rider);int belt=bint(v,"belt");float d=0;PTR f=v?get_field(object_class(v),"distance"):0;if(f)field_value(v,f,&d);ddrop(&rider);if(!ok||!f||belt!=input||!(d>=len-.35f))continue;
    context.capsule=keys[k];mb_transfer(junction,transfer,&context,&ok);
   }
  }
  ++j;
 }
 ddrop(&riders);ddrop(&context.world);ddrop(&context.walker);
 if(!ok){bm.disabled=1;reset_session();bevent("balancer-native-transfer-failed");}
}
static int before(int hook,Context *c){
 (void)hook;int state=bframe(c);
 if(state<0){reset_session();return 0;}if(state<=0)return 0;if(was_host&&!bm.host_ready){reset_session();bevent("balancer-authority-lost");}was_host=bm.host_ready;if(bm.host_ready){balance();if(bm.disabled){mbm_drop();return 0;}int time_ok=1;float dt=dnumber(bm.delta,0,&time_ok);if(time_ok&&dt>=0&&dt<1)mbm.clock-=dt;if(mbm.clock<=0){mbm.clock=1;mbm_update();}}
 int input=baction("multi-belt-balancer.input",kin),output=baction("multi-belt-balancer.output",kout),variant=baction("multi-belt-balancer.variant",kvariant),clear=baction("multi-belt-balancer.cancel",kclear),remove=baction("multi-belt-balancer.remove",kremove);
 int shop=moon_shop_poll();if(!bm.eligible)return 0;variant|=shop;
 if(variant){if(!setup){setup=1;draft.n=2;}else{int n=draft.n==4?2:draft.n+1;clear_draft();draft.n=n;}}
 if(remove&&!setup){int id=aimed();for(int j=0;j<count;j++){int found=0;for(int i=0;i<junctions[j].n;i++)if(junctions[j].in[i].id==id||junctions[j].out[i].id==id)found=1;if(found){remove_junction(j);bevent("balancer-removed");break;}}}
 if(setup&&input)add(0);if(setup&&output)add(1);if(clear&&setup){setup=0;clear_draft();}
 if(setup)bstatus(draft.n==2?"Moon Sweepers | 2x2 balancer | I input / O output / F5 variant / Backspace cancel":draft.n==3?"Moon Sweepers | 3x3 balancer | I input / O output / F5 variant / Backspace cancel":"Moon Sweepers | 4x4 balancer | I input / O output / F5 variant / Backspace cancel");return 0;
}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
 if(!bstart(domain,game,"multi-belt-balancer")||!su_bind(game))return 0;cfg_start("MOONLAB_BALANCER_CONFIG","MoonLab\\mods\\multi-belt-balancer\\settings.ini");kin=cfg("InputKey",'I',1,255);kout=cfg("OutputKey",'O',1,255);kvariant=cfg("VariantKey",0x74,1,255);kclear=cfg("CancelKey",8,1,255);kremove=cfg("RemoveKey",0x2e,1,255);price=cfg("PricePerLane",40,0,10000);reach=(float)cfg("MaxJunctionMetres",12,1,30);
 conveyors=dmethod("MoonCleaners","MoonWorld","get_BuiltConveyors","");path=dmethod("MoonCleaners","MoonWorld","ConveyorPath","MoonCleaners.MoonConveyorSnapshot");nearest=dmethod("MoonCleaners","MoonWorld","NearestDistance","MoonCleaners.MoonConveyorPath;UnityEngine.Vector3");length=dmethod("MoonCleaners","MoonConveyorPath","get_Length","");enter=dmethod("MoonCleaners","MoonWorld","Enter","System.Int32;System.Int32");
 PTR snapshot=find_class("MoonCleaners","MoonConveyorSnapshot");route_point=dmethod("MoonCleaners","MoonConveyorPath","Point","System.Single");if(!snapshot||!route_point||!conveyors||!path||!nearest||!length||!enter||bm.value_type(snapshot))return 0;if(!prepare_hook(method("MoonCleaners","MoonWalker","Update",0),100671733,0,game))return 0;before_hook=before;if(!install_hooks())return 0;bevent("ready");return 1;
}
