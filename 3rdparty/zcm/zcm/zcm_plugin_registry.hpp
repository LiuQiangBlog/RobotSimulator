//
// Created by liuqiang on 25-5-12.
//
// zcm_plugin_registry.hpp

#pragma once

extern "C" void register_udp_plugin();
extern "C" void register_udpm_plugin();
extern "C" void register_ipc_plugin();
extern "C" void register_inproc_plugin();
extern "C" void register_serial_plugin();
extern "C" void register_ipcshm_plugin();
extern "C" void register_blockInproc_plugin();
extern "C" void register_nonBlockInproc_plugin();
extern "C" void register_file_plugin();
extern "C" void register_can_plugin();

namespace zcm
{
inline void RegisterAllPlugins()
{
    register_udp_plugin();
    register_udpm_plugin();
    register_ipc_plugin();
    register_inproc_plugin();
    register_serial_plugin();
    register_ipcshm_plugin();
    register_blockInproc_plugin();
    register_nonBlockInproc_plugin();
    register_file_plugin();
    register_can_plugin();
}
}