#pragma once

#include <LeapC.h>
#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/variant2.hpp>
#include <ossia/detail/dylib_loader.hpp>

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>
#include <thread>
#include <vector>

namespace ul
{

#define UL_SYMBOL_NAME_S(prefix, name) #prefix #name
#define UL_SYMBOL_NAME(prefix, name) prefix##name
#define UL_SYMBOL_DEF(prefix, name) decltype(&::UL_SYMBOL_NAME(prefix, name)) name{};
#define UL_SYMBOL_INIT(prefix, name)                                  \
  {                                                                   \
    name = library.symbol<decltype(&::UL_SYMBOL_NAME(prefix, name))>( \
        UL_SYMBOL_NAME_S(prefix, name));                              \
    if(!name)                                                         \
    {                                                                 \
      available = false;                                              \
      return;                                                         \
    }                                                                 \
  }

struct libleapc
{
  explicit libleapc()
      : library{{
#if defined(__linux__)
          "libLeapC.so.5", "libLeapC.so.6",
          "/usr/lib/ultraleap-hand-tracking-service/libLeapC.so.5",
          "/usr/lib/ultraleap-hand-tracking-service/libLeapC.so.6",
          "/opt/ultraleap/LeapSDK/lib/libLeaC.so.5",
          "/opt/ultraleap/LeapSDK/lib/libLeaC.so.6"
#elif defined(__APPLE__)
          "libLeapC.5.dylib", "libLeapC.6.dylib",
          "/Applications/Ultraleap Hand Tracking "
          "Service.app/Contents/LeapSDK/lib/libLeapC.5.dylib",
          "/Applications/Ultraleap Hand Tracking "
          "Service.app/Contents/LeapSDK/lib/libLeapC.6.dylib"
#else
          "LeapC.dll", "C:\\Program Files\\Ultraleap\\LeapSDK\\lib\\x64\\LeapC.dll"
#endif
      }}
  {
    UL_SYMBOL_INIT(Leap, CreateConnection);
    UL_SYMBOL_INIT(Leap, OpenConnection);
    UL_SYMBOL_INIT(Leap, SetAllocator);
    UL_SYMBOL_INIT(Leap, SetPolicyFlags);
    UL_SYMBOL_INIT(Leap, CloseConnection);
    UL_SYMBOL_INIT(Leap, DestroyConnection);
    UL_SYMBOL_INIT(Leap, OpenDevice);
    UL_SYMBOL_INIT(Leap, GetDeviceInfo);
    UL_SYMBOL_INIT(Leap, SubscribeEvents);
    UL_SYMBOL_INIT(Leap, CloseDevice);
    UL_SYMBOL_INIT(Leap, PollConnection);
  }

  static const libleapc& instance()
  {
    static const libleapc self;
    return self;
  }

  ossia::dylib_loader library;
  bool available{true};

  UL_SYMBOL_DEF(Leap, CreateConnection);
  UL_SYMBOL_DEF(Leap, OpenConnection);
  UL_SYMBOL_DEF(Leap, SetAllocator);
  UL_SYMBOL_DEF(Leap, SetPolicyFlags);
  UL_SYMBOL_DEF(Leap, CloseConnection);
  UL_SYMBOL_DEF(Leap, DestroyConnection);
  UL_SYMBOL_DEF(Leap, OpenDevice);
  UL_SYMBOL_DEF(Leap, GetDeviceInfo);
  UL_SYMBOL_DEF(Leap, SubscribeEvents);
  UL_SYMBOL_DEF(Leap, CloseDevice);
  UL_SYMBOL_DEF(Leap, PollConnection);
};

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

  boost::container::static_vector<LEAP_HAND, 2> hands;
  float framerate;
};

struct image_message
{
  LEAP_IMAGE_PROPERTIES properties{};
  std::shared_ptr<unsigned char> left;
  std::shared_ptr<unsigned char> right;
};

struct subscriber_options
{
  std::optional<uint32_t> tracked_device;
  std::optional<std::string> tracked_device_serial;

  std::function<void(const tracking_message&)> on_tracking_event;
  std::function<void(const image_message&)> on_image_event;
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
  const ul::libleapc& Leap = ul::libleapc::instance();
  static constexpr LEAP_ALLOCATOR allocator
      = {+[](uint32_t size, eLeapAllocatorType typeHint, void* state) {
           return malloc(size);
         },
         +[](void* ptr, void* state) { free(ptr); }, nullptr};

  leap_manager()
  {
    LEAP_CONNECTION_CONFIG config;

    // Set connection to multi-device aware
    memset(&config, 0, sizeof(config));
    config.size = sizeof(config);
    config.flags = eLeapConnectionConfig_MultiDeviceAware;

    if(Leap.CreateConnection(&config, &m_handle) != eLeapRS_Success)
      return;

    if(Leap.OpenConnection(m_handle) != eLeapRS_Success)
      return;

    Leap.SetAllocator(m_handle, &allocator);
    Leap.SetPolicyFlags(
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

    Leap.CloseConnection(m_handle);
    m_thread.join();

    Leap.DestroyConnection(m_handle);
  }

  void register_device(const LEAP_DEVICE_EVENT* device_event)
  {
    LEAP_DEVICE hdl{};
    auto res = Leap.OpenDevice(device_event->device, &hdl);
    if(res != eLeapRS_Success)
    {
      return;
    }

    auto props = LEAP_DEVICE_INFO{
        .size = sizeof(LEAP_DEVICE_INFO),
        .serial_length = 1,
        .serial = (char*)malloc(1)};

    // Most stupid little dance i've ever seen
    res = Leap.GetDeviceInfo(hdl, &props);
    if(res == eLeapRS_InsufficientBuffer)
    {
      props.serial = (char*)realloc(props.serial, props.serial_length);
      res = Leap.GetDeviceInfo(hdl, &props);
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
      d.serial = std::string(props.serial, props.serial_length - 1);
      d.h_fov = props.h_fov;
      d.v_fov = props.v_fov;
      d.range = props.range;

      Leap.SubscribeEvents(m_handle, hdl);
      std::lock_guard _{m_devices_lock};
      this->m_devices[device_event->device.id] = std::move(d);
    }

    free(props.serial);
    // Leap.CloseDevice(hdl);
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
    auto result = Leap.PollConnection(m_handle, 1000, &msg);

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
        break;
      case eLeapEventType_Eyes:
        break;
      case eLeapEventType_IMU:
        // msg.imu_event
        break;
      default:
        break;
    }
  }

  auto devices() const noexcept
  {
    std::lock_guard _{m_devices_lock};
    return m_devices;
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
