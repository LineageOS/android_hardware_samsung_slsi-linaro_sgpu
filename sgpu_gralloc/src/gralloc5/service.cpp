/*
 * Copyright (C) 2025 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "android.hardware.graphics.allocator-aidl-service-sgr"

#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "AllocatorAidl.h"
#include "util/util.h"
#include <android-base/logging.h>

using aidl::android::hardware::graphics::allocator::Allocator;

int main() {
    std::shared_ptr<Allocator> service = ndk::SharedRefBase::make<Allocator>();

    std::string instance = std::string() + Allocator::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(service->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    SGR_LOGI("Allocator AIDL HAL Ready.");
    ABinderProcess_joinThreadPool();
    /// Under normal cases, execution will not reach this line.
    SGR_LOGE("Allocator AIDL HAL failed to join thread pool.");
    return EXIT_FAILURE;
}