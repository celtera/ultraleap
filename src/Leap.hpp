#pragma once

#include <LeapC.h>
#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/variant2.hpp>

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace ul
{
struct device_info
{
  uint32_t status;
  uint32_t caps;
  eLeapDevicePID pid;
  uint32_t baseline;
  std::string serial;
  float h_fov;
  float v_fov;
  uint32_t range;
};

using head_message = LEAP_HEAD_POSE_EVENT;
using eye_message = LEAP_EYE_EVENT;

struct tracking_message
{
  int64_t frame_id;
  int64_t timestamp;

  std::vector<LEAP_HAND> hands;
  float framerate;
};

struct image_message
{
  LEAP_IMAGE_PROPERTIES properties{};
  std::shared_ptr<unsigned char> left;
  std::shared_ptr<unsigned char> right;
};

using message = boost::variant2::variant<tracking_message, head_message, eye_message>;

struct subscriber_options
{
  std::optional<uint32_t> tracked_device;
  std::optional<std::string> tracked_device_serial;

  std::function<void(tracking_message)> on_tracking_event;
  std::function<void(image_message)> on_image_event;
};

struct subscriber
{
  subscriber_options config;

  explicit subscriber(subscriber_options&& opts) noexcept
      : config{std::move(opts)}
  {
  }
};

using subscriber_handle = std::shared_ptr<subscriber>;

struct leap_manager
{
  static constexpr LEAP_ALLOCATOR allocator
      = {+[](uint32_t size, eLeapAllocatorType typeHint, void* state) {
    return malloc(size);
  },
         +[](void* ptr, void* state) { free(ptr); }, nullptr};

  leap_manager()
  {
    if(LeapCreateConnection(nullptr, &m_handle) != eLeapRS_Success)
      return;

    if(LeapOpenConnection(m_handle) != eLeapRS_Success)
      return;

    LeapSetAllocator(m_handle, &allocator);
    LeapSetPolicyFlags(
        m_handle,
        eLeapPolicyFlag_Images | eLeapPolicyFlag_MapPoints
            | eLeapPolicyFlag_BackgroundFrames,
        0);

    m_running = true;
    m_thread = std::thread([this] {
      while(m_running)
      {
        poll_one();
      }
    });
  }

  ~leap_manager()
  {
    m_running = false;

    LeapCloseConnection(m_handle);
    m_thread.join();

    LeapDestroyConnection(m_handle);
  }

  void register_device(const LEAP_DEVICE_EVENT* device_event)
  {
    LEAP_DEVICE hdl{};
    auto res = LeapOpenDevice(device_event->device, &hdl);
    if(res != eLeapRS_Success)
    {
      return;
    }

    auto props = LEAP_DEVICE_INFO{
        .size = sizeof(LEAP_DEVICE_INFO),
        .serial_length = 1,
        .serial = (char*)malloc(1)};

    // Most stupid little dance i've ever seen
    res = LeapGetDeviceInfo(hdl, &props);
    if(res == eLeapRS_InsufficientBuffer)
    {
      props.serial = (char*)realloc(props.serial, props.serial_length);
      res = LeapGetDeviceInfo(hdl, &props);
      if(res != eLeapRS_Success)
      {
        free(props.serial);
        return;
      }
    }

    {
      device_info d{};

      d.status = props.status;
      d.caps = props.caps;
      d.pid = props.pid;
      d.baseline = props.baseline;
      d.serial = std::string(props.serial, props.serial_length);
      d.h_fov = props.h_fov;
      d.v_fov = props.v_fov;
      d.range = props.range;

      std::lock_guard _{m_devices_lock};
      this->m_devices[device_event->device.id] = std::move(d);
    }

    free(props.serial);
    LeapCloseDevice(hdl);
  }

  void unregister_device(const LEAP_DEVICE_EVENT* device_event)
  {
    std::lock_guard _{m_devices_lock};
    this->m_devices.erase(device_event->device.id);
  }

  void unregister_all()
  {
    std::lock_guard _{m_devices_lock};
    m_devices.clear();
  }

  void poll_one()
  {
    LEAP_CONNECTION_MESSAGE msg{};
    auto result = LeapPollConnection(m_handle, 1000, &msg);

    if(result != eLeapRS_Success)
      return;

    switch(msg.type)
    {
      case eLeapEventType_Connection:
        m_connected = true;
        break;
      case eLeapEventType_ConnectionLost:
        m_connected = false;
        unregister_all();
        break;
      case eLeapEventType_Device:
        register_device(msg.device_event);
        break;
      case eLeapEventType_DeviceLost:
        unregister_device(msg.device_event);
        break;
      case eLeapEventType_DeviceFailure:
        // msg.device_failure_event
        break;
      case eLeapEventType_Tracking:
        on_tracking_event(msg, msg.tracking_event);
        break;
      case eLeapEventType_ImageComplete:
        break;
      case eLeapEventType_ImageRequestError:
        break;
      case eLeapEventType_LogEvent:
        // msg.log_event->severity, msg.log_event->timestamp, msg.log_event->message
        break;
      case eLeapEventType_Policy:
        // msg.policy_event->current_policy
        break;
      case eLeapEventType_ConfigChange:
        // msg.config_change_event->requestID, msg.config_change_event->status
        break;
      case eLeapEventType_ConfigResponse:
        // msg.config_response_event->requestID, msg.config_response_event->value
        break;
      case eLeapEventType_Image:
        on_image_event(msg.image_event);
        // msg.image_event
        break;
      case eLeapEventType_PointMappingChange:
        // msg.point_mapping_change_event
        break;
      case eLeapEventType_TrackingMode:
        // msg.tracking_mode_event
        break;
      case eLeapEventType_LogEvents:
        // for (int i = 0; i < (int)(msg.log_events->nEvents); i++) {
        //   const LEAP_LOG_EVENT* log_event = &msg.log_events->events[i];
        //   log_event->severity, log_event->timestamp, log_event->message
        // }
        break;
      case eLeapEventType_HeadPose:
        on_head_event(msg, msg.head_pose_event);
        break;
      case eLeapEventType_Eyes:
        on_eye_event(msg, msg.eye_event);
        break;
      case eLeapEventType_IMU:
        // msg.imu_event
        break;
      default:
        break;
    }
  }

  std::string device_serial(uint32_t id) const noexcept
  {
    std::lock_guard _{m_devices_lock};

    auto it = m_devices.find(id);
    if(it == m_devices.end())
      return {};
    return it->second.serial;
  }

  void for_each_subscriber(const LEAP_CONNECTION_MESSAGE& msg, auto func)
  {
    std::lock_guard _{m_subscribers_lock};
    for(auto& s : m_subscribers)
    {
      if(msg.device_id != 0) // 0: all devices
      {
        if(s->config.tracked_device && *s->config.tracked_device != msg.device_id)
          continue;

        if(auto& serial = s->config.tracked_device_serial)
        {
          if(*serial != device_serial(msg.device_id))
            continue;
        }
      }

      func(*s);
    }
  }

  void
  on_tracking_event(const LEAP_CONNECTION_MESSAGE& msg, const LEAP_TRACKING_EVENT* frame)
  {
    if(m_subscribers.empty() || m_devices.empty())
      return;

    //post("Tracking event:\n  ID: %d\n  SN: %s", msg.device_id, device_serial(msg.device_id).c_str());
    tracking_message m;
    m.frame_id = frame->tracking_frame_id;
    m.timestamp = frame->info.timestamp;
    if(frame->nHands > 0)
      m.hands.assign(frame->pHands, frame->pHands + frame->nHands);

    m.framerate = frame->framerate;

    for_each_subscriber(msg, [&](subscriber& s) {
      if(s.config.on_tracking_event)
        s.config.on_tracking_event(m);
    });
  }

  void
  on_head_event(const LEAP_CONNECTION_MESSAGE& msg, const LEAP_HEAD_POSE_EVENT* frame)
  {
    if(m_subscribers.empty() || m_devices.empty())
      return;

    // TODO
  }

  void on_eye_event(const LEAP_CONNECTION_MESSAGE& msg, const LEAP_EYE_EVENT* frame)
  {
    if(m_subscribers.empty() || m_devices.empty())
      return;

    // TODO
  }

  void on_image_event(const LEAP_IMAGE_EVENT* frame)
  {
    // TODO
    // post("%d %d", frame->image[0].properties.width, frame->image[0].properties.height);
  }

  subscriber_handle subscribe(subscriber_options opts)
  {
    std::lock_guard _{m_subscribers_lock};
    m_subscribers.push_back(std::make_shared<subscriber>(std::move(opts)));
    return m_subscribers.back();
  }

  void unsubscribe(subscriber_handle hdl)
  {
    std::lock_guard _{m_subscribers_lock};
    auto it = std::ranges::find(m_subscribers, hdl);
    if(it != m_subscribers.end())
      m_subscribers.erase(it);
  }

private:
  std::atomic<bool> m_connected{};
  std::atomic<bool> m_running{};

  LEAP_CONNECTION m_handle{};
  std::thread m_thread;

  mutable std::mutex m_subscribers_lock;
  std::vector<subscriber_handle> m_subscribers;

  mutable std::mutex m_devices_lock;
  boost::container::small_flat_map<uint32_t, device_info, 8> m_devices;
};

template <typename T>
std::shared_ptr<T> instance()
{
  static std::mutex mut;
  static std::weak_ptr<T> cache;

  std::lock_guard _{mut};

  if(auto ptr = cache.lock())
  {
    return ptr;
  }
  else
  {
    auto shared = std::make_shared<T>();
    cache = shared;
    return shared;
  }
}

}
