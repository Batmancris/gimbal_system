#ifndef __ODOM_H__ 
#define __ODOM_H__
typedef struct Position
{
  float v_x_m;
  float v_y_m;
  float position_x_m;
  float position_y_m;
  float yaw;
  float wz;
} Pos_t;

extern Pos_t pose;

#endif

