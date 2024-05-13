#include "Model.hpp"

#include <fmt/format.h>
#include <halp/attributes.hpp>

namespace ul
{

static double distance_conversion_factor(std::string_view unit)
{
  if(unit == "m")
  {
    return 0.001;
  }
  else if(unit == "mm")
  {
    return 1.;
  }
  else
  {
    return 1.;
  }
}

static std::string_view product_name(eLeapDevicePID pid)
{
  switch(pid)
  {
    case eLeapDevicePID_Unknown:
      return "Unknown";
    case eLeapDevicePID_Peripheral:
      return "Leap Motion Controller";
    case eLeapDevicePID_Dragonfly:
      return "Dragonfly";
    case eLeapDevicePID_Nightcrawler:
      return "Nightcrawler";
    case eLeapDevicePID_Rigel:
      return "Rigel";
    case eLeapDevicePID_SIR170:
      return "Ultraleap Stereo IR 170 ";
    case eLeapDevicePID_3Di:
      return "Ultraleap 3Di";
    case eLeapDevicePID_LMC2:
      return "Leap Motion Controller 2";
    case eLeapDevicePID_Invalid:
      return "Invalid";
    default:
      return "Unknown?";
  }
}

void UltraLeap::messages::dump::operator()(UltraLeap& self)
{
  using strings = std::vector<dump_type>;
  const auto& devs = self.m_instance->devices();

  auto& outputs = self.outputs;
  outputs.dump(strings{"dump.begin"});
  int k = 0;
  for(auto& [id, dev] : devs)
  {
    outputs.dump(strings{"device", dump_type{(int)id}});
    outputs.dump(strings{"index", dump_type{k++}});
    outputs.dump(strings{"serial", dump_type{dev.serial}});
    outputs.dump(strings{"product", dump_type{(std::string)product_name(dev.pid)}});
    outputs.dump(strings{"status", dump_type{(int)dev.status}});
    outputs.dump(strings{"caps", dump_type{(int)dev.caps}});
    outputs.dump(strings{"baseline", dump_type{(int)dev.baseline}});
    outputs.dump(strings{"h_fov", dump_type{(float)dev.h_fov}});
    outputs.dump(strings{"v_fov", dump_type{(float)dev.v_fov}});
    outputs.dump(strings{"range", dump_type{(int)dev.range}});
  }
  outputs.dump(strings{"dump.end"});
}

UltraLeap::UltraLeap()
    : m_instance{ul::instance<ul::leap_manager>()}
    , buf{{}}
{
  this->inputs.device_index.value = -1;
}

UltraLeap::~UltraLeap()
{
  m_instance->unsubscribe(m_handle);
}

void UltraLeap::initialize(
    std::span<std::variant<float, std::string_view>> range) noexcept
{
  halp::parse_attributes(*this, range);
  restart_tracking();
}

void UltraLeap::restart_tracking()
{
  m_active = this->inputs.active;
  if(m_handle)
    m_instance->unsubscribe(m_handle);

  subscriber_options opt;
  opt.on_tracking_event = [this](const tracking_message& m) {
    this->buf.produce(m);

    if(this->m_active.load(std::memory_order_acquire))
    {
      this->schedule.schedule_at(
          0, +[](UltraLeap& self) { self(); });
    }
  };

  if(!this->inputs.device_serial.value.empty())
  {
    opt.tracked_device_serial = this->inputs.device_serial.value;
  }
  else if(this->inputs.device_index.value >= 0)
  {
    opt.tracked_device = this->inputs.device_index.value;
  }
  m_handle = m_instance->subscribe(std::move(opt));
}

void UltraLeap::operator()() noexcept
{
  if(tracking_message m; this->buf.consume(m))
    this->on_message(m);
}

void UltraLeap::update_active()
{
  m_active = this->inputs.active;
}

void UltraLeap::on_message(const tracking_message& msg) noexcept
{
  const auto frame_time = std::chrono::steady_clock::now();
  const auto frame_diff = std::chrono::duration_cast<std::chrono::nanoseconds>(frame_time - prev_frame_time).count();
  const auto inv_frame_rate = (frame_diff > 1e9 || frame_diff <= 1e3) ? 0. : 1e9 / frame_diff;
  const auto factor = distance_conversion_factor(this->inputs.unit.value);

  outputs.start_frame();

  const int Nhand = msg.hands.size();
  FrameInfo frameInfo{
      .frame = msg.frame_id
      //, .time = 0
      ,
      .framerate = msg.framerate};

  for(int i = 0; i < Nhand; i++)
  {
    const auto& ih = msg.hands[i];
    if(ih.type == eLeapHandType_Left)
      frameInfo.leftHandTracked = true;
    else
      frameInfo.rightHandTracked = true;
  }

  if(!frameInfo.leftHandTracked)
    prev_left_hand.hand_id = -1;
  if(!frameInfo.rightHandTracked)
    prev_right_hand.hand_id = -1;

  outputs.frame(frameInfo);

  auto update_speed = [=] (Fingers& prev, int hand, int finger, FingerInfo& of)
  {
    if(prev.hand_id == hand && inv_frame_rate > 0.)
    {
      const auto& pf = prev.fingers[finger];

      of.vx = (of.px - pf.px) * inv_frame_rate;
      of.vy = (of.py - pf.py) * inv_frame_rate;
      of.vz = (of.pz - pf.pz) * inv_frame_rate;
    }
    prev.fingers[finger] = of;
  };

  for(int i = 0; i < Nhand; i++)
  {
    const auto& ih = msg.hands[i];
    HandInfo oh;
    oh.id = ih.id;

    oh.px = ih.palm.position.x * factor;
    oh.py = ih.palm.position.y * factor;
    oh.pz = ih.palm.position.z * factor;

    oh.vx = ih.palm.velocity.x * factor;
    oh.vy = ih.palm.velocity.y * factor;
    oh.vz = ih.palm.velocity.z * factor;

    oh.o1 = ih.palm.orientation.x;
    oh.o2 = ih.palm.orientation.y;
    oh.o3 = ih.palm.orientation.z;
    oh.o4 = ih.palm.orientation.w;

    //oh.radius = ih.palm.width;
    oh.pinch = ih.pinch_strength;
    oh.grab = ih.grab_strength;

    oh.time = ih.visible_time * 0.000001;

    if(ih.type == eLeapHandType::eLeapHandType_Left)
    {
      outputs.hand_l(oh);
    }
    else
    {
      outputs.hand_r(oh);
    }

    int finger_index = 0;
    for(const auto& finger : ih.digits)
    {
      FingerInfo of;

      of.id = finger.finger_id;
      //of.hand_id = ih.id;
      //of.id = 10 * of.hand_id + finger.finger_id;

      of.px = finger.distal.next_joint.x * factor;
      of.py = finger.distal.next_joint.y * factor;
      of.pz = finger.distal.next_joint.z * factor;

      of.o1 = finger.distal.rotation.x;
      of.o2 = finger.distal.rotation.y;
      of.o3 = finger.distal.rotation.z;
      of.o4 = finger.distal.rotation.w;

      //of.dx = finger.distal.next_joint.x - finger.distal.prev_joint.x;
      //of.dy = finger.distal.next_joint.y - finger.distal.prev_joint.y;
      //of.dz = finger.distal.next_joint.z - finger.distal.prev_joint.z;

      // Updated later
      // of.vx = 0.; // ih.palm.velocity.x;
      // of.vy = 0.; // ih.palm.velocity.y;
      // of.vz = 0.; // ih.palm.velocity.z;

      //of.width = 0.;
      of.length = 0.;

      BoneInfo ob;
      int boneID = 0;

      for(auto& bone : finger.bones)
      {
        auto len = std::hypot(
            bone.next_joint.x - bone.prev_joint.x, bone.next_joint.y - bone.prev_joint.y,
            bone.next_joint.z - bone.prev_joint.z);
        of.length += len * factor;

        ob.fid = of.id;

        ob.bid = boneID;
        boneID += 1;

        ob.ppx = bone.prev_joint.x * factor;
        ob.ppy = bone.prev_joint.y * factor;
        ob.ppz = bone.prev_joint.z * factor;

        ob.o1 = bone.rotation.x;
        ob.o2 = bone.rotation.y;
        ob.o3 = bone.rotation.z;
        ob.o4 = bone.rotation.w;

        ob.pnx = bone.next_joint.x * factor;
        ob.pny = bone.next_joint.y * factor;
        ob.pnz = bone.next_joint.z * factor;

        ob.w = bone.width * factor;
        ob.l = len * factor;

        if(ih.type == eLeapHandType::eLeapHandType_Left)
          outputs.bone_l(ob);
        else
          outputs.bone_r(ob);
      }

      of.extended = finger.is_extended;

      if(ih.type == eLeapHandType::eLeapHandType_Left)
      {
        update_speed(prev_left_hand, oh.id, finger_index, of);
        outputs.finger_l(of);
      }
      else
      {
        update_speed(prev_right_hand, oh.id, finger_index, of);
        outputs.finger_r(of);
      }
      ++finger_index;
    }

    // For speed computation on the next turn
    if(ih.type == eLeapHandType::eLeapHandType_Left)
      prev_left_hand.hand_id = ih.id;
    else
      prev_right_hand.hand_id = ih.id;
  }

  outputs.end_frame();
  prev_frame_time = frame_time;
}
}
