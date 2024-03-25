#pragma once
#include <android/log.h>
#include <string.h>
#include <errno.h>

#ifndef NDEBUG
static constexpr auto TAG = "ZygiskAssistant/JNI";
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#else
#define LOGD(...)
#define LOGI(...)
#define LOGE(...)
#endif
