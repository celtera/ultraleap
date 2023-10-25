#pragma once
#include "Leap.hpp"
#include "TripleBuffer.hpp"

#include <avnd/introspection/input.hpp>
#include <halp/callback.hpp>
#include <halp/controls.hpp>
#include <halp/meta.hpp>
#include <halp/messages.hpp>
#include <halp/schedule.hpp>

#include <vector>

namespace ul
{

struct BoneInfo
{
  int fid{};    //int finger_id{};
    int bid{}; //bone ID;

  float ppx{}, ppy{}, ppz{}; //bone prev_joint position

  float o1{}, o2{}, o3{}, o4{};    //quat
float pnx{}, pny{}, pnz{}; //bone next_joint position
    
    float w{};
    float l{};
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

struct UltraLeap
{
public:
  halp_meta(name, "UltraLeap")
  halp_meta(c_name, "ultraleap")
  halp_meta(author, "Jean-Michaël Celerier & Mathieu Chamagne")
  halp_meta(category, "Devices")
  halp_meta(description, "Process UltraLeap input")
  halp_meta(uuid, "52cf0135-9104-4c0b-85dc-1a95b54eda4b")

  struct
  {
    struct : halp::val_port<"Active", bool>
    {
      halp_meta(c_name, "active")
      halp_flag(class_attribute);
      void update(UltraLeap& obj) {
        obj.update_active();
      }
    } active;

    struct : halp::val_port<"Device Index", int>
    {
      halp_meta(c_name, "device_index")
      halp_flag(class_attribute);
      void update(UltraLeap& obj) {
        obj.restart_tracking();
      }
    } device_index;

    struct : halp::val_port<"Device Serial", std::string>
    {
      halp_meta(c_name, "device_serial")
      halp_flag(class_attribute);
      void update(UltraLeap& obj) {
        obj.restart_tracking();
      }
    } device_serial;

    struct : halp::val_port<"Connected", bool>
    {
      halp_meta(c_name, "connected")
      halp_flag(class_attribute);
    } connected;

    struct : halp::val_port<"Unit", std::string>
    {
      halp_meta(c_name, "unit")
      halp_flag(class_attribute);
    } unit;
  } inputs;

  struct
  {
    halp::callback<"End frame"> end_frame;
    halp::callback<"Bones L", BoneInfo> bone_l;
    halp::callback<"Bones R", BoneInfo> bone_r;
    halp::callback<"Finger L", FingerInfo> finger_l;
    halp::callback<"Finger R", FingerInfo> finger_r;
    halp::callback<"Hand L", HandInfo> hand_l;
    halp::callback<"Hand R", HandInfo> hand_r;
    halp::callback<"Frame", FrameInfo> frame;
    halp::callback<"Start frame"> start_frame;
  } outputs;

  struct messages {
    struct dump {
      halp_meta(name, "dump")
      void operator()(UltraLeap& self);
    } dump;
  };

  struct
  {
    halp::scheduler_fun<UltraLeap&> schedule_at;
  } schedule;

  UltraLeap();
  ~UltraLeap();

  void initialize(std::span<std::variant<float, std::string_view>>) noexcept;
  void restart_tracking();
  void update_active();
  void operator()() noexcept;
  void on_message(const tracking_message& msg) noexcept;

  std::shared_ptr<ul::leap_manager> m_instance;
  ul::subscriber_handle m_handle;

  triple_buffer<tracking_message> buf;

  std::atomic_bool m_active{true};
};

}
