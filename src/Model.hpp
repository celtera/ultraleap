#pragma once
#include "DataTypes.hpp"
#include "Leap.hpp"
#include "TripleBuffer.hpp"

#include <avnd/introspection/input.hpp>
#include <halp/callback.hpp>
#include <halp/controls.hpp>
#include <halp/messages.hpp>
#include <halp/meta.hpp>
#include <halp/schedule.hpp>

#include <variant>
#include <vector>

namespace ul
{

using dump_type = std::variant<std::string, int, float>;
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
      halp_meta(description, "Data will be streamed when this is enabled.")
      halp_flag(class_attribute);
      void update(UltraLeap& obj) { obj.update_active(); }
    } active;

    struct : halp::val_port<"Device Index", int>
    {
      halp_meta(c_name, "device_index")
      halp_meta(description, "Index of the UltraLeap device to bind to.")
      halp_flag(class_attribute);
      void update(UltraLeap& obj) { obj.restart_tracking(); }
    } device_index;

    struct : halp::val_port<"Device Serial", std::string>
    {
      halp_meta(c_name, "device_serial")
      halp_meta(description, "Serial number of the UltraLeap device to bind to.")
      halp_flag(class_attribute);
      void update(UltraLeap& obj) {
        obj.restart_tracking();
      }
    } device_serial;

    struct : halp::val_port<"Connected", bool>
    {
      halp_meta(c_name, "connected")
      halp_meta(description, "True when connected to a device.")
      halp_flag(class_attribute);
    } connected;

    struct : halp::val_port<"Unit", std::string>
    {
      halp_meta(c_name, "unit")
      halp_meta(description, "Distance unit: m or mm.")
      halp_flag(class_attribute);
    } unit;
  } inputs;

  struct
  {
    struct : halp::callback<"End frame"> {
      halp_meta(description, "End frame : A bang is sent through this outlet when the frame has been fully processed.")
    } end_frame;
    struct : halp::callback<"Bones L", BoneInfo>    {
      halp_meta(description, "Left hand bones")
    } bone_l;
    struct : halp::callback<"Bones R", BoneInfo>    {
      halp_meta(description, "Right hand bones")
    } bone_r;
    struct : halp::callback<"Finger L", FingerInfo> {
      halp_meta(description, "Left hand fingers")
    } finger_l;
    struct : halp::callback<"Finger R", FingerInfo> {
      halp_meta(description, "Right hand fingers")
    } finger_r;
    struct : halp::callback<"Hand L", HandInfo>     {
      halp_meta(description, "Left hand")
    } hand_l;
    struct : halp::callback<"Hand R", HandInfo>     {
      halp_meta(description, "Right hand")
    } hand_r;
    struct : halp::callback<"Frame", FrameInfo>     {
      halp_meta(description, "Frame infos")
    } frame;
    struct : halp::callback<"Start frame"> {
      halp_meta(description, "Start frame : A bang is sent through this outlet when a new frame is going to be processed.")
    } start_frame;

    struct : halp::callback<"Dump", std::vector<dump_type>> {
      halp_meta(description, "Dump out.")
    } dump;
  } outputs;

  struct messages
  {
    struct dump
    {
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

  Fingers prev_left_hand;
  Fingers prev_right_hand;

  std::chrono::steady_clock::time_point prev_frame_time{};
  std::atomic_bool m_active{true};
};

}
