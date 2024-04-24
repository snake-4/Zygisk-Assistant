#pragma once
#include <android/log.h>
#include <cstring>
#include <cerrno>
#include <cinttypes>

#ifndef NDEBUG
static constexpr auto TAG = "ZygiskAssistant/JNI";
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#else
#define LOGD(...)
#define LOGW(...)
#define LOGE(...)
#endif
