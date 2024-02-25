#include "Model.hpp"

#include <ext.h>
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
  const auto& devs = self.m_instance->devices();

  outputs.dump("dump.begin");
  int k = 0;
  for(auto& [id, dev] : devs)
  {
    outputs.dump(fmt::format("device {}", id));
    outputs.dump(fmt::format("index {}", k++));
    outputs.dump(fmt::format("serial {}", dev.serial));
    outputs.dump(fmt::format("product {}", product_name(dev.pid)));
    outputs.dump(fmt::format("status {}", dev.status));
    outputs.dump(fmt::format("caps {}", dev.caps));
    outputs.dump(fmt::format("baseline {}", dev.baseline));
    outputs.dump(fmt::format("h_fov {}", dev.h_fov));
    outputs.dump(fmt::format("v_fov {}", dev.v_fov));
    outputs.dump(fmt::format("range {}", dev.range));
  }
  outputs.dump("dump.end");
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

  auto factor = distance_conversion_factor(this->inputs.unit.value);

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

  outputs.frame(frameInfo);

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

    int k = 0;
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
        {
          outputs.bone_l(ob);
        }
        else
        {
          outputs.bone_r(ob);
        }
      }

      of.extended = finger.is_extended;

      //of.type = k++;

      if(ih.type == eLeapHandType::eLeapHandType_Left)
      {
        outputs.finger_l(of);
      }
      else
      {
        outputs.finger_r(of);
      }
    }
  }

  outputs.end_frame();
}
}
