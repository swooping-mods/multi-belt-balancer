#ifndef MOON_BALANCER_MODEL_H
#define MOON_BALANCER_MODEL_H
#include "../../runtime/sdk/scene/static_model.h"
#include "assets/generated/model_data.h"
#include "model_fit.h"
static struct {SMResources resources;SURoot roots[32];SPVec3 up[32];unsigned state[32];float clock;} mbm;
static void mbm_hide(unsigned i){su_destroy(su_get(mbm.roots[i]));su_drop(mbm.roots+i);}
static void mbm_drop(void){for(unsigned i=0;i<32;i++)mbm_hide(i);sm_release(&mbm.resources);su_zero(&mbm,sizeof(mbm));}
static SPVec3 mbm_vec(DVec p){return su_v(p.x,p.y,p.z);}
static void mbm_update(void){
    for(int j=0;j<count;j++){
        if(junctions[j].n!=3)continue;
        SPVec3 input[3],output[3];int ok=1;
        for(unsigned i=0;i<3;i++){
            DRoot a=record(junctions[j].in[i],&ok),b=record(junctions[j].out[i],&ok);
            if(!a||!b)ok=0;
            input[i]=a?mbm_vec(point(dget(a),"end")):su_v(0,0,0);output[i]=b?mbm_vec(point(dget(b),"start")):su_v(0,0,0);ddrop(&a);ddrop(&b);
        }
        /* Capture local up near configuration, rather than rotating a distant
         * housing whenever the player walks around the spherical world. */
        if(su_dot(mbm.up[j],mbm.up[j])<.5f)mbm.up[j]=su_vector(su.tr_up,call(bm.transform,dget(bm.walker),0,&ok));
        SPVec3 origin,forward;
        if(!ok||!mbm_fit(input,output,mbm_input,mbm_output,mbm.up[j],MBM_PORT_TOLERANCE,&origin,&forward)){
            mbm_hide((unsigned)j);if(mbm.state[j]!=1)bevent("housing-layout-unmatched");mbm.state[j]=1;continue;
        }
        if(!mbm.roots[j]){
            PTR root=sm_attach(&mbm_model,&mbm.resources,0,2,"Three lane balancer housing");mbm.roots[j]=su_hold(root);
            if(!mbm.roots[j]){su_destroy(root);if(mbm.state[j]!=3)bevent("housing-create-pending");mbm.state[j]=3;continue;}
        }
        PTR transform=su_transform(su_get(mbm.roots[j]));su_set_vector(su.tr_position,transform,origin);su_set_quat(su.tr_rotation,transform,su_rotation(forward,mbm.up[j]));
        if(mbm.state[j]!=2)bevent("housing-placed");mbm.state[j]=2;
    }
}
#endif
