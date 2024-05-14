#pragma once
#include <cstdint>

namespace ul
{

struct BoneInfo
{
  int fid{}; //int finger_id{};
  int bid{}; //bone ID;

  float ppx{}, ppy{}, ppz{}; //bone prev_joint position

  float o1{}, o2{}, o3{}, o4{}; //quat
  float pnx{}, pny{}, pnz{};    //bone next_joint position

  float w{}; //bone width
  float l{}; //bone length
};

struct FingerInfo
{
  int id{};
  //int hand_id{};
  float px{}, py{}, pz{}; //finger distal next_joint position

  //quat
  float o1{}, o2{}, o3{}, o4{};

  //float dx{}, dy{}, dz{}; // direction

  float vx{}, vy{}, vz{};
  int extended{};
  float length{};
  //float width{};
  //int type{};
};

struct Fingers
{
  int hand_id{-1};
  FingerInfo fingers[5];
};

struct HandInfo
{
  //who needs hands IDs ??
  int id{};
  float px{}, py{}, pz{};

  //quat
  float o1{}, o2{}, o3{}, o4{};

  //velocity
  float vx{}, vy{}, vz{};

  float pinch{};
  float grab{};

  //float radius{};
  float time{};
};

struct FrameInfo
{
  int64_t frame{};
  //int64_t time{};
  bool leftHandTracked{};
  bool rightHandTracked{};
  float framerate{};
};

}
