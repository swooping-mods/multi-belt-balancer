#ifndef MOON_BALANCER_MODEL_FIT_H
#define MOON_BALANCER_MODEL_FIT_H
/* Fit one rigid three-port housing. Endpoint order is user selection order,
 * so both ends require an independent one-to-one port assignment. */
static inline int mbm_finite(SPVec3 p){return p.x>=-1e7f&&p.x<=1e7f&&p.y>=-1e7f&&p.y<=1e7f&&p.z>=-1e7f&&p.z<=1e7f;}
static inline SPVec3 mbm_frame(SPVec3 p,SPVec3 right,SPVec3 up,SPVec3 forward){return su_add(su_mul(right,p.x),su_add(su_mul(up,p.y),su_mul(forward,p.z)));}
static inline float mbm_match(const SPVec3 actual[3],const SPVec3 local[3],SPVec3 origin,SPVec3 right,SPVec3 up,SPVec3 forward){
    static const unsigned order[6][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
    float best=1e30f;
    for(unsigned k=0;k<6;k++){
        float maximum=0;
        for(unsigned i=0;i<3;i++){
            SPVec3 delta=su_sub(actual[order[k][i]],su_add(origin,mbm_frame(local[i],right,up,forward)));
            float q=su_dot(delta,delta);if(q>maximum)maximum=q;
        }
        if(maximum<best)best=maximum;
    }
    return best;
}
static inline int mbm_fit(const SPVec3 input[3],const SPVec3 output[3],const SPVec3 local_input[3],const SPVec3 local_output[3],SPVec3 up,float tolerance,SPVec3 *origin,SPVec3 *forward){
    if(!mbm_finite(up)||su_dot(up,up)<.5f||!(tolerance>0&&tolerance<1))return 0;
    SPVec3 a={0},b={0},local={0};
    for(unsigned i=0;i<3;i++){
        if(!mbm_finite(input[i])||!mbm_finite(output[i])||!mbm_finite(local_input[i])||!mbm_finite(local_output[i]))return 0;
        a=su_add(a,input[i]);b=su_add(b,output[i]);local=su_add(local,su_add(local_input[i],local_output[i]));
    }
    a=su_mul(a,1.f/3);b=su_mul(b,1.f/3);local=su_mul(local,1.f/6);up=su_normal(up);
    SPVec3 direction=su_sub(b,a);direction=su_sub(direction,su_mul(up,su_dot(direction,up)));
    if(su_dot(direction,direction)<.25f)return 0;
    *forward=su_normal(direction);SPVec3 right=su_normal(su_cross(up,*forward));
    *origin=su_sub(su_mul(su_add(a,b),.5f),mbm_frame(local,right,up,*forward));
    float limit=tolerance*tolerance;
    return mbm_match(input,local_input,*origin,right,up,*forward)<=limit&&mbm_match(output,local_output,*origin,right,up,*forward)<=limit;
}
#endif
