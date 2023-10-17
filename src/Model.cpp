#include "Model.hpp"

namespace ul
{
UltraLeap::UltraLeap()
    : m_instance{ul::instance<ul::leap_manager>()}
    , buf{ {} }
{
}

UltraLeap::~UltraLeap()
{
  m_instance->unsubscribe(m_handle);
}

void UltraLeap::initialize() noexcept
{
  m_active = this->inputs.active;
  m_handle = m_instance->subscribe({.on_tracking_event = [this](const tracking_message& m) {
    this->buf.produce(m);

    if(this->m_active.load(std::memory_order_acquire)) {
      this->schedule.schedule_at(
          0, +[](UltraLeap& self) { self(); });
    }
  }});
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
  outputs.start_frame();

  const int Nhand = msg.hands.size();
  FrameInfo frameInfo{
            .frame = msg.frame_id
            //, .time = 0
            , .framerate = msg.framerate};

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

    oh.px = ih.palm.position.x;
    oh.py = ih.palm.position.y;
    oh.pz = ih.palm.position.z;

    oh.vx = ih.palm.velocity.x;
    oh.vy = ih.palm.velocity.y;
    oh.vz = ih.palm.velocity.z;

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

      of.px = finger.distal.next_joint.x;
      of.py = finger.distal.next_joint.y;
      of.pz = finger.distal.next_joint.z;

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
      for(auto& bone : finger.bones)
      {
        auto len = std::hypot(
            bone.next_joint.x - bone.prev_joint.x, bone.next_joint.y - bone.prev_joint.y,
            bone.next_joint.z - bone.prev_joint.z);
        of.length += len;
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
