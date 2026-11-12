#ifndef MOON_BALANCER_STATE_H
#define MOON_BALANCER_STATE_H
/* Session identities deliberately include both endpoints. Native IDs alone can
 * outlive a removed/rebuilt belt. The native adapter additionally owns a GC root for exact reference identity.
 * No cross-save identity is inferred here. */
#define MB_MAX_RIDERS 8192
typedef struct {float x,y,z;} MBPoint;
typedef struct {int id;MBPoint start,end,start_direction,end_direction;unsigned long long root;} MBBelt;
typedef struct {int n;MBBelt in[4],out[4];int next,input_next;} Junction;
static inline int mb_point_same(MBPoint a,MBPoint b){
 float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;
 return x*x+y*y+z*z<=.000001f;
}
static inline int mb_belt_same(MBBelt a,MBBelt b,unsigned long long reference_a,unsigned long long reference_b){
 return reference_a&&reference_a==reference_b&&a.id==b.id&&mb_point_same(a.start,b.start)&&mb_point_same(a.end,b.end)&&mb_point_same(a.start_direction,b.start_direction)&&mb_point_same(a.end_direction,b.end_direction);
}
static inline int mb_transfer(Junction *j,int (*admit)(void *,int,int *),void *context,int *ok){
 for(int attempt=0;attempt<j->n&&*ok;attempt++){
  int index=(j->next+attempt)%j->n;
  if(admit(context,j->out[index].id,ok)&&*ok){j->next=(index+1)%j->n;return 1;}
 }
 return 0;
}
/* Advance once per balancing pass, independent of engine frame-count parity. */
static inline int mb_input_turn(Junction *j){int first=j->input_next;j->input_next=(first+1)%j->n;return first;}
/* A bounded page still visits every key in a stable oversized dictionary.
 * Snapshot enumeration always finishes before any native transfer. */
static inline int mb_page_next(int first,int total,int capacity){
 return total>capacity&&first+capacity<total?first+capacity:0;
}
#endif
