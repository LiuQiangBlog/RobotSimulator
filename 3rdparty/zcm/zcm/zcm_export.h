//
// Created by liuqiang on 25-5-12.
//

#ifndef MUJOCO_ZCM_EXPORT_H
#define MUJOCO_ZCM_EXPORT_H

// zcm_export.h
#pragma once

#if defined _WIN32 || defined __CYGWIN__
#define ZCM_EXPORT __declspec(dllexport)
#else
#define ZCM_EXPORT __attribute__((visibility("default")))
#endif

#endif // MUJOCO_ZCM_EXPORT_H
