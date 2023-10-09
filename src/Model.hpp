#pragma once
#include "Leap.hpp"

#include <concurrentqueue.h>

#include <halp/callback.hpp>
#include <halp/controls.hpp>
#include <halp/schedule.hpp>
#include <halp/meta.hpp>

#include <vector>

#include <avnd/introspection/input.hpp>

namespace ul
{
struct FingerInfo
{
    
  int id{};
  //int hand_id{};
  float px{}, py{}, pz{}; //finger distal next_joint position
  
//quat
 //   float o1{}, o2{}, o3{}, o4{};
    
  float dx{}, dy{}, dz{}; // direction
  
  float vx{}, vy{}, vz{};
  float width{};
  float length{};
  int extended{};
  int type{};
};

struct HandInfo
{
  //who needs IDs ??
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
  int hands{};
  float framerate{};
};

struct UltraLeap
{
public:
  halp_meta(name, "UltraLeap")
  halp_meta(c_name, "avnd_ultraleap")
  halp_meta(author, "Jean-Michaël Celerier")
  halp_meta(category, "Devices")
  halp_meta(description, "Process UltraLeap input")
  halp_meta(uuid, "52cf0135-9104-4c0b-85dc-1a95b54eda4b")

  struct
  {
    struct : halp::val_port<"active", bool> {
      halp_flag(class_attribute);
    } active;

    struct : halp::val_port<"Device Index", int> {
      halp_flag(class_attribute);
    } device_index;

    struct: halp::val_port<"Device Serial", std::string> {
      halp_flag(class_attribute);
    } device_serial;

    struct: halp::val_port<"Connected", bool> {
      halp_flag(class_attribute);
    } connected;
      
    struct: halp::val_port<"unit", std::string> {
    halp_flag(class_attribute);
    } unit;
  } inputs;

  struct
  {
    halp::callback<"End frame"> end_frame;
    halp::callback<"Finger L", FingerInfo> finger_l;
    halp::callback<"Finger R", FingerInfo> finger_r;
    halp::callback<"Hand L", HandInfo> hand_l;
    halp::callback<"Hand R", HandInfo> hand_r;
    halp::callback<"Frame", FrameInfo> frame;
    halp::callback<"Start frame"> start_frame;
  } outputs;

  struct {
    halp::scheduler_fun<UltraLeap&> schedule_at;
  } schedule;

  UltraLeap();
  ~UltraLeap();

  void initialize() noexcept;
  void operator()() noexcept;
  void on_message(const tracking_message& msg) noexcept;
  void on_message(const head_message& msg) noexcept;
  void on_message(const eye_message& msg) noexcept;

  std::shared_ptr<ul::leap_manager> m_instance;
  ul::subscriber_handle m_handle;

  moodycamel::ConcurrentQueue<message> msg;

};

}
