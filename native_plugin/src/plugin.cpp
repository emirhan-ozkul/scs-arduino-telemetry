#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

#include "logger.h"

#include "scssdk_telemetry.h"
#include "eurotrucks2/scssdk_eut2.h"
#include "eurotrucks2/scssdk_telemetry_eut2.h"
#include "amtrucks/scssdk_ats.h"
#include "amtrucks/scssdk_telemetry_ats.h"
#include "common/scssdk_telemetry_truck_common_channels.h"

namespace {

struct FloatSlot {
    float* target;
    bool* valid;
};

struct GearSlot {
    scs_s32_t* target;
    bool* valid;
};

struct TelemetryState {
    float speed = 0.0f;
    float rpm = 0.0f;
    scs_s32_t gear = 0;
    float fuel = 0.0f;
    float fuel_consumption = 0.0f;
    float odometer = 0.0f;
    float remaining_distance = 0.0f;
    float water_temperature = 0.0f;

    bool speed_valid = false;
    bool rpm_valid = false;
    bool gear_valid = false;
    bool fuel_valid = false;
    bool fuel_consumption_valid = false;
    bool odometer_valid = false;
    bool remaining_distance_valid = false;
    bool water_temperature_valid = false;

    bool running = false;
    char game_id[16] = "unknown";
    ULONGLONG last_log_ms = 0;
    ULONGLONG last_udp_ms = 0;
};

TelemetryState g_state;
telemetry_plugin::Logger g_logger;
scs_log_t g_game_log = nullptr;

SOCKET g_udp_socket = INVALID_SOCKET;
sockaddr_in g_udp_destination{};
int g_udp_port = 12345;

void Log(const char* message) {
    g_logger.Log(message);

    if (g_game_log != nullptr && message != nullptr) {
        g_game_log(SCS_LOG_TYPE_message, message);
    }
}

void LogError(const char* message) {
    g_logger.Log(message);
    if (g_game_log != nullptr && message != nullptr) {
        g_game_log(SCS_LOG_TYPE_error, message);
    }
}

const char* ResultName(scs_result_t result) {
    if (result == SCS_RESULT_ok) return "SCS_RESULT_ok";
    if (result == SCS_RESULT_unsupported) return "SCS_RESULT_unsupported";
    if (result == SCS_RESULT_invalid_parameter) return "SCS_RESULT_invalid_parameter";
    if (result == SCS_RESULT_already_registered) return "SCS_RESULT_already_registered";
    if (result == SCS_RESULT_not_found) return "SCS_RESULT_not_found";
    if (result == SCS_RESULT_unsupported_type) return "SCS_RESULT_unsupported_type";
    if (result == SCS_RESULT_not_now) return "SCS_RESULT_not_now";
    return "SCS_RESULT_generic_error";
}

void SendUdpPacket();
bool InitializeUdp(const char* port_text);
void ShutdownUdp();

const char* UDPPortEnvironment();

SCSAPI_VOID FrameEnd(
    const scs_event_t /*event*/,
    const void* const /*event_info*/,
    const scs_context_t /*context*/)
{
    const ULONGLONG now = GetTickCount64();

    if (g_state.last_udp_ms == 0 || now - g_state.last_udp_ms >= 100) {
        g_state.last_udp_ms = now;
        SendUdpPacket();
    }

    if (!g_state.running) {
        return;
    }

    if (g_state.last_log_ms != 0 && now - g_state.last_log_ms < 1000) {
        return;
    }
    g_state.last_log_ms = now;

    char line[512];
    std::snprintf(
        line,
        sizeof(line),
        "Telemetry speed=%s%0.2f rpm=%s%0.1f gear=%s%d fuel_l=%s%0.2f water_c=%s%0.2f fuel_c=%s%0.2f odometer=%s%0.2f remaining_distance=%s%0.2f",
        g_state.speed_valid ? "" : "invalid:", g_state.speed,
        g_state.rpm_valid ? "" : "invalid:", g_state.rpm,
        g_state.gear_valid ? "" : "invalid:", g_state.gear,
        g_state.fuel_valid ? "" : "invalid:", g_state.fuel,
        g_state.water_temperature_valid ? "" : "invalid:", g_state.water_temperature,
        g_state.fuel_consumption_valid ? "" : "invalid:", g_state.fuel_consumption,
        g_state.odometer_valid ? "" : "invalid:", g_state.odometer,
        g_state.remaining_distance_valid ? "" : "invalid:", g_state.remaining_distance);
    Log(line);
}

SCSAPI_VOID TelemetryPause(
    const scs_event_t event,
    const void* const /*event_info*/,
    const scs_context_t /*context*/)
{
    g_state.running = (event == SCS_TELEMETRY_EVENT_started);
    char line[128];
    std::snprintf(
        line,
        sizeof(line),
        "Telemetry state: %s",
        g_state.running ? "started/driving" : "paused");
    Log(line);
}

SCSAPI_VOID StoreFloat(
    const scs_string_t /*name*/,
    const scs_u32_t /*index*/,
    const scs_value_t* const value,
    const scs_context_t context)
{
    if (context == nullptr) {
        return;
    }

    float* target = nullptr;
    bool* valid = nullptr;
    auto* slot = static_cast<struct FloatSlot*>(context);
    target = slot->target;
    valid = slot->valid;

    if (value == nullptr) {
        if (valid != nullptr) {
            *valid = false;
        }
        return;
    }
    if (value->type != SCS_VALUE_TYPE_float) {
        if (valid != nullptr) {
            *valid = false;
        }
        return;
    }

    if (target != nullptr) {
        *target = value->value_float.value;
    }
    if (valid != nullptr) {
        *valid = true;
    }
}

SCSAPI_VOID StoreS32(
    const scs_string_t /*name*/,
    const scs_u32_t /*index*/,
    const scs_value_t* const value,
    const scs_context_t context)
{
    if (context == nullptr) {
        return;
    }

    if (value == nullptr) {
        GearSlot* slot = static_cast<GearSlot*>(context);
        *slot->valid = false;
        return;
    }

    if (value->type != SCS_VALUE_TYPE_s32) {
        GearSlot* slot = static_cast<GearSlot*>(context);
        *slot->valid = false;
        return;
    }

    GearSlot* slot = static_cast<GearSlot*>(context);
    *slot->target = value->value_s32.value;
    *slot->valid = true;
}

FloatSlot g_speed_slot{&g_state.speed, &g_state.speed_valid};
FloatSlot g_rpm_slot{&g_state.rpm, &g_state.rpm_valid};
FloatSlot g_fuel_slot{&g_state.fuel, &g_state.fuel_valid};
FloatSlot g_water_slot{&g_state.water_temperature, &g_state.water_temperature_valid};
FloatSlot g_fuel_consumption_slot{&g_state.fuel_consumption, &g_state.fuel_consumption_valid};
FloatSlot g_odometer_slot{&g_state.odometer, &g_state.odometer_valid};
FloatSlot g_remaining_distance_slot{&g_state.remaining_distance, &g_state.remaining_distance_valid};
GearSlot g_gear_slot{&g_state.gear, &g_state.gear_valid};

void RegisterFloatChannel(
    const scs_telemetry_register_for_channel_t register_channel,
    const char* channel_name,
    FloatSlot* slot)
{
    const scs_result_t result = register_channel(
        channel_name,
        SCS_U32_NIL,
        SCS_VALUE_TYPE_float,
        SCS_TELEMETRY_CHANNEL_FLAG_no_value,
        StoreFloat,
        slot);

    char line[256];
    std::snprintf(line, sizeof(line), "Channel %s registration: %s", channel_name, ResultName(result));
    if (result == SCS_RESULT_ok) {
        Log(line);
    }
    else {
        LogError(line);
    }
}

void RegisterGearChannel(const scs_telemetry_register_for_channel_t register_channel) {
    const scs_result_t result = register_channel(
        SCS_TELEMETRY_TRUCK_CHANNEL_engine_gear,
        SCS_U32_NIL,
        SCS_VALUE_TYPE_s32,
        SCS_TELEMETRY_CHANNEL_FLAG_no_value,
        StoreS32,
        &g_gear_slot);

    char line[256];
    std::snprintf(line, sizeof(line), "Channel %s registration: %s", SCS_TELEMETRY_TRUCK_CHANNEL_engine_gear, ResultName(result));
    if (result == SCS_RESULT_ok) {
        Log(line);
    }
    else {
        LogError(line);
    }
}

SCSAPI_RESULT scs_telemetry_init_impl(
    const scs_u32_t version,
    const scs_telemetry_init_params_t* const params)
{
    if (params == nullptr) {
        return SCS_RESULT_invalid_parameter;
    }

    if (version != SCS_TELEMETRY_VERSION_1_01) {
        return SCS_RESULT_unsupported;
    }

    const scs_telemetry_init_params_v101_t* const version_params =
        static_cast<const scs_telemetry_init_params_v101_t*>(params);

    g_logger.Open();
    g_game_log = version_params->common.log;

    char line[512];
    std::snprintf(
        line,
        sizeof(line),
        "Initializing SCS telemetry API 1.01. game_id=%s game_name=%s game_version=%u.%u",
        version_params->common.game_id,
        version_params->common.game_name,
        SCS_GET_MAJOR_VERSION(version_params->common.game_version),
        SCS_GET_MINOR_VERSION(version_params->common.game_version));
    Log(line);

    if (std::strcmp(version_params->common.game_id, SCS_GAME_ID_EUT2) == 0) {
        Log("Detected ETS2 game.");

        if (SCS_GET_MAJOR_VERSION(version_params->common.game_version) >
            SCS_GET_MAJOR_VERSION(SCS_TELEMETRY_EUT2_GAME_VERSION_CURRENT)) {
            LogError("Compatibility warning: ETS2 game major telemetry version is newer than SDK 1.16's current ETS2 declaration.");
        }
    }
    else if (std::strcmp(version_params->common.game_id, SCS_GAME_ID_ATS) == 0) {
        Log("Detected ATS game.");

        if (SCS_GET_MAJOR_VERSION(version_params->common.game_version) >
            SCS_GET_MAJOR_VERSION(SCS_TELEMETRY_ATS_GAME_VERSION_CURRENT)) {
            LogError("Compatibility warning: ATS game major telemetry version is newer than SDK 1.16's current ATS declaration.");
        }
    }
    else {
        LogError("Compatibility warning: unsupported or unknown SCS game_id.");
    }

    g_state = TelemetryState{};
    std::snprintf(g_state.game_id, sizeof(g_state.game_id), "%s", version_params->common.game_id);

    InitializeUdp(UDPPortEnvironment());

    scs_result_t result;

    result = version_params->register_for_event(SCS_TELEMETRY_EVENT_frame_end, FrameEnd, nullptr);
    std::snprintf(line, sizeof(line), "Event frame_end registration: %s", ResultName(result));
    result == SCS_RESULT_ok ? Log(line) : LogError(line);
    if (result != SCS_RESULT_ok) return SCS_RESULT_generic_error;

    result = version_params->register_for_event(SCS_TELEMETRY_EVENT_paused, TelemetryPause, nullptr);
    std::snprintf(line, sizeof(line), "Event paused registration: %s", ResultName(result));
    result == SCS_RESULT_ok ? Log(line) : LogError(line);
    if (result != SCS_RESULT_ok) return SCS_RESULT_generic_error;

    result = version_params->register_for_event(SCS_TELEMETRY_EVENT_started, TelemetryPause, nullptr);
    std::snprintf(line, sizeof(line), "Event started registration: %s", ResultName(result));
    result == SCS_RESULT_ok ? Log(line) : LogError(line);
    if (result != SCS_RESULT_ok) return SCS_RESULT_generic_error;

    RegisterFloatChannel(version_params->register_for_channel, SCS_TELEMETRY_TRUCK_CHANNEL_speed, &g_speed_slot);
    RegisterFloatChannel(version_params->register_for_channel, SCS_TELEMETRY_TRUCK_CHANNEL_engine_rpm, &g_rpm_slot);
    RegisterGearChannel(version_params->register_for_channel);
    RegisterFloatChannel(version_params->register_for_channel, SCS_TELEMETRY_TRUCK_CHANNEL_fuel, &g_fuel_slot);
    RegisterFloatChannel(version_params->register_for_channel, SCS_TELEMETRY_TRUCK_CHANNEL_water_temperature, &g_water_slot);
    RegisterFloatChannel(version_params->register_for_channel, SCS_TELEMETRY_TRUCK_CHANNEL_fuel_average_consumption, &g_fuel_consumption_slot);
    RegisterFloatChannel(version_params->register_for_channel, SCS_TELEMETRY_TRUCK_CHANNEL_odometer, &g_odometer_slot);
    RegisterFloatChannel(version_params->register_for_channel, SCS_TELEMETRY_TRUCK_CHANNEL_navigation_distance, &g_remaining_distance_slot);

    Log("SCS telemetry plugin initialized.");
    char path_line[512];
    std::snprintf(path_line, sizeof(path_line), "Log file: %s", telemetry_plugin::Logger::LogPath().c_str());
    Log(path_line);
    return SCS_RESULT_ok;
}

const char* UDPPortEnvironment() {
    static char value[16] = {};
    char* buffer = nullptr;
    size_t size = 0;
    if (_dupenv_s(&buffer, &size, "ETS2_TELEMETRY_UDP_PORT") == 0 && buffer != nullptr) {
        std::snprintf(value, sizeof(value), "%s", buffer);
        free(buffer);
        return value;
    }
    return nullptr;
}

bool InitializeUdp(const char* port_text) {
    if (port_text != nullptr) {
        char* end = nullptr;
        const long parsed = std::strtol(port_text, &end, 10);
        if (end != port_text && *end == '\0' && parsed >= 1 && parsed <= 65535) {
            g_udp_port = static_cast<int>(parsed);
        }
    }

    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        return false;
    }

    g_udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (g_udp_socket == INVALID_SOCKET) {
        return false;
    }

    u_long nonblocking = 1;
    if (ioctlsocket(g_udp_socket, FIONBIO, &nonblocking) != 0) {
        closesocket(g_udp_socket);
        g_udp_socket = INVALID_SOCKET;
        return false;
    }

    g_udp_destination.sin_family = AF_INET;
    g_udp_destination.sin_port = htons(static_cast<u_short>(g_udp_port));
    InetPtonA(AF_INET, "127.0.0.1", &g_udp_destination.sin_addr);

    char line[160];
    std::snprintf(line, sizeof(line), "UDP telemetry target: 127.0.0.1:%d", g_udp_port);
    Log(line);
    return true;
}

void ShutdownUdp() {
    if (g_udp_socket != INVALID_SOCKET) {
        closesocket(g_udp_socket);
        g_udp_socket = INVALID_SOCKET;
    }
    WSACleanup();
}

void SendUdpPacket() {
    if (g_udp_socket == INVALID_SOCKET) {
        return;
    }

    char packet[1024];
    const auto value = [](bool valid, float value) -> std::string {
        if (!valid) return "null";
        char text[32];
        std::snprintf(text, sizeof(text), "%.3f", value);
        return text;
    };

    std::snprintf(
        packet,
        sizeof(packet),
        "{\"protocol\":\"scs-telemetry-v1\",\"game_id\":\"%s\",\"connected\":%s,\"speed_mps\":%s,\"engine_rpm\":%s,\"gear\":%s,\"fuel_l\":%s,\"fuel_consumption_l_km\":%s,\"odometer_km\":%s,\"remaining_distance_m\":%s,\"water_temperature_c\":%s}",
        g_state.game_id,
        g_state.running ? "true" : "false",
        value(g_state.speed_valid, g_state.speed).c_str(),
        value(g_state.rpm_valid, g_state.rpm).c_str(),
        g_state.gear_valid ? std::to_string(g_state.gear).c_str() : "null",
        value(g_state.fuel_valid, g_state.fuel).c_str(),
        value(g_state.fuel_consumption_valid, g_state.fuel_consumption).c_str(),
        value(g_state.odometer_valid, g_state.odometer).c_str(),
        value(g_state.remaining_distance_valid, g_state.remaining_distance).c_str(),
        value(g_state.water_temperature_valid, g_state.water_temperature).c_str());

    sendto(g_udp_socket, packet, static_cast<int>(std::strlen(packet)), 0,
           reinterpret_cast<const sockaddr*>(&g_udp_destination), sizeof(g_udp_destination));
}

} // namespace

extern "C" {

SCSAPI_RESULT scs_telemetry_init(
    const scs_u32_t version,
    const scs_telemetry_init_params_t* const params)
{
    try {
        return scs_telemetry_init_impl(version, params);
    }
    catch (...) {
        LogError("Fatal C++ exception escaped from scs_telemetry_init; initialization rejected.");
        return SCS_RESULT_generic_error;
    }
}

SCSAPI_VOID scs_telemetry_shutdown(void)
{
    try {
        g_state.running = false;
        g_state = TelemetryState{};
        Log("SCS telemetry plugin shutting down.");
        g_game_log = nullptr;
        g_logger.Close();
        ShutdownUdp();
    }
    catch (...) {
    }
}

}

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE /*module*/, DWORD reason, LPVOID /*reserved*/) {
    if (reason == DLL_PROCESS_DETACH) {
        g_logger.Close();
    }
    return TRUE;
}
#endif
