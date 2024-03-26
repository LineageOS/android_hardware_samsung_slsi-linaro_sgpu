/*
 * Copyright 2014 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * \file amdgpu_device.c
 *
 *  Implementation of functions for AMD GPU device
 */

 /*
 * @file amdgpu_device.c
 * @copyright 2020 Samsung Electronics
 */

#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef __ANDROID__
#include <hardware/exynos/ion.h>
#endif /* __ANDROID__ */
//#include "g_version.h"

#include "xf86drm.h"
#include "sgpu_drm.h"
#include "amdgpu_internal.h"
#include "util_math.h"

#define PTR_TO_UINT(x) ((unsigned)((intptr_t)(x)))

#define FIELD_GET(_mask, _reg) (((_reg) & (_mask)) >> (ffs(_mask) - 1))
#define GENMASK(h, l) \
	(((~0u) << (l)) & ((~0u) >> (32 - 1 - (h))))

static pthread_mutex_t dev_mutex = PTHREAD_MUTEX_INITIALIZER;
static amdgpu_device_handle dev_list;

static int fd_compare(int fd1, int fd2)
{
	char *name1 = drmGetPrimaryDeviceNameFromFd(fd1);
	char *name2 = drmGetPrimaryDeviceNameFromFd(fd2);
	int result;

	if (name1 == NULL || name2 == NULL) {
		free(name1);
		free(name2);
		return 0;
	}

	result = strcmp(name1, name2);
	free(name1);
	free(name2);

	return result;
}

/**
* Get the authenticated form fd,
*
* \param   fd   - \c [in]  File descriptor for AMD GPU device
* \param   auth - \c [out] Pointer to output the fd is authenticated or not
*                          A render node fd, output auth = 0
*                          A legacy fd, get the authenticated for compatibility root
*
* \return   0 on success\n
*          >0 - AMD specific error code\n
*          <0 - Negative POSIX Error code
*/
static int amdgpu_get_auth(int fd, int *auth)
{
	int r = 0;
	drm_client_t client = {};

	if (drmGetNodeTypeFromFd(fd) == DRM_NODE_RENDER)
		*auth = 0;
	else {
		client.idx = 0;
		r = drmIoctl(fd, DRM_IOCTL_GET_CLIENT, &client);
		if (!r)
			*auth = client.auth;
	}
	return r;
}

static void amdgpu_device_free_internal(amdgpu_device_handle dev)
{
	/* Remove dev from dev_list, if it was added there. */
	if (dev == dev_list) {
		dev_list = dev->next;
	} else {
		for (amdgpu_device_handle node = dev_list; node; node = node->next) {
			if (node->next == dev) {
				node->next = dev->next;
				break;
			}
		}
	}

	close(dev->fd);
	if ((dev->flink_fd >= 0) && (dev->fd != dev->flink_fd))
		close(dev->flink_fd);

	amdgpu_vamgr_deinit(&dev->va_mgr.vamgr_32);
	amdgpu_vamgr_deinit(&dev->va_mgr.vamgr_low);
	amdgpu_vamgr_deinit(&dev->va_mgr.vamgr_high_32);
	amdgpu_vamgr_deinit(&dev->va_mgr.vamgr_high);
	handle_table_fini(&dev->bo_handles);
	handle_table_fini(&dev->bo_flink_names);
	pthread_mutex_destroy(&dev->bo_table_mutex);
	free(dev->marketing_name);
	free(dev);
}

/**
 * Assignment between two amdgpu_device pointers with reference counting.
 *
 * Usage:
 *    struct amdgpu_device *dst = ... , *src = ...;
 *
 *    dst = src;
 *    // No reference counting. Only use this when you need to move
 *    // a reference from one pointer to another.
 *
 *    amdgpu_device_reference(&dst, src);
 *    // Reference counters are updated. dst is decremented and src is
 *    // incremented. dst is freed if its reference counter is 0.
 */
static void amdgpu_device_reference(struct amdgpu_device **dst,
				    struct amdgpu_device *src)
{
	if (update_references(&(*dst)->refcount, &src->refcount))
		amdgpu_device_free_internal(*dst);
	*dst = src;
}

drm_public int amdgpu_device_initialize(int fd,
					uint32_t *major_version,
					uint32_t *minor_version,
					amdgpu_device_handle *device_handle)
{
	struct amdgpu_device *dev;
	drmVersionPtr version;
	int r;
	int flag_auth = 0;
	int flag_authexist=0;
	uint32_t accel_working = 0;
	uint64_t start, max;

	*device_handle = NULL;

	pthread_mutex_lock(&dev_mutex);
#if 0
	/* @todo GFXSW-2476 - Remove the following print block */
	{
		printf("-------------------------------------------------------"
		       "\n");
		printf("libDRM version user name       = %s\n",
			LIBDRM_VERSION_USERNAME);
		printf("libDRM version merge SHA1      = %s\n",
			LIBDRM_VERSION_MERGE_SHA1);
		printf("libDRM version revision number = %s\n",
			LIBDRM_VERSION_REVISION_NUMBER);
		printf("-------------------------------------------------------"
		       "\n");
	}
#endif
	r = amdgpu_get_auth(fd, &flag_auth);
	if (r) {
		fprintf(stderr, "%s: amdgpu_get_auth (1) failed (%i)\n",
			__func__, r);
		pthread_mutex_unlock(&dev_mutex);
		return r;
	}

	for (dev = dev_list; dev; dev = dev->next)
		if (fd_compare(dev->fd, fd) == 0)
			break;

	if (dev) {
		r = amdgpu_get_auth(dev->fd, &flag_authexist);
		if (r) {
			fprintf(stderr, "%s: amdgpu_get_auth (2) failed (%i)\n",
				__func__, r);
			pthread_mutex_unlock(&dev_mutex);
			return r;
		}
		if ((flag_auth) && (!flag_authexist)) {
			dev->flink_fd = fcntl(fd, F_DUPFD_CLOEXEC, 0);
		}
		*major_version = dev->major_version;
		*minor_version = dev->minor_version;
		amdgpu_device_reference(device_handle, dev);
		pthread_mutex_unlock(&dev_mutex);
		return 0;
	}

	dev = calloc(1, sizeof(struct amdgpu_device));
	if (!dev) {
		fprintf(stderr, "%s: calloc failed\n", __func__);
		pthread_mutex_unlock(&dev_mutex);
		return -ENOMEM;
	}

	dev->fd = -1;
	dev->flink_fd = -1;

	atomic_set(&dev->refcount, 1);

	version = drmGetVersion(fd);
	if (version->version_major != 3) {
		fprintf(stderr, "%s: DRM version is %d.%d.%d but this driver is "
			"only compatible with 3.x.x.\n",
			__func__,
			version->version_major,
			version->version_minor,
			version->version_patchlevel);
		drmFreeVersion(version);
		r = -EBADF;
		goto cleanup;
	}

	dev->fd = fcntl(fd, F_DUPFD_CLOEXEC, 0);
	dev->flink_fd = dev->fd;
	dev->major_version = version->version_major;
	dev->minor_version = version->version_minor;
	drmFreeVersion(version);

	pthread_mutex_init(&dev->bo_table_mutex, NULL);

	/* Check if acceleration is working. */
	r = amdgpu_query_info(dev, AMDGPU_INFO_ACCEL_WORKING, 4, &accel_working);
	if (r) {
		fprintf(stderr, "%s: amdgpu_query_info(ACCEL_WORKING) failed (%i)\n",
			__func__, r);
		goto cleanup;
	}
	if (!accel_working) {
		fprintf(stderr, "%s: AMDGPU_INFO_ACCEL_WORKING = 0\n", __func__);
		r = -EBADF;
		goto cleanup;
	}

	r = amdgpu_query_gpu_info_init(dev);
	if (r) {
		fprintf(stderr, "%s: amdgpu_query_gpu_info_init failed\n", __func__);
		goto cleanup;
	}

	start = dev->dev_info.virtual_address_offset;
	max = MIN2(dev->dev_info.virtual_address_max, 0x100000000ULL);
	amdgpu_vamgr_init(&dev->va_mgr.vamgr_32, start, max,
			  dev->dev_info.virtual_address_alignment);

	start = max;
	max = MAX2(dev->dev_info.virtual_address_max, 0x100000000ULL);
	amdgpu_vamgr_init(&dev->va_mgr.vamgr_low, start, max,
			  dev->dev_info.virtual_address_alignment);

	start = dev->dev_info.high_va_offset;
	max = MIN2(dev->dev_info.high_va_max, (start & ~0xffffffffULL) +
		   0x100000000ULL);
	amdgpu_vamgr_init(&dev->va_mgr.vamgr_high_32, start, max,
			  dev->dev_info.virtual_address_alignment);

	start = max;
	max = MAX2(dev->dev_info.high_va_max, (start & ~0xffffffffULL) +
		   0x100000000ULL);
	amdgpu_vamgr_init(&dev->va_mgr.vamgr_high, start, max,
			  dev->dev_info.virtual_address_alignment);

	/* PCI device id */
	if (dev->info.asic_id == 0x73A0) {
		char name[128];
		const char *product = NULL;
		int gen = FIELD_GET(GENMASK(31, 24),
				    dev->info.chip_rev);

		/* Model ID */
		switch(FIELD_GET(GENMASK(23, 16), dev->info.chip_rev)) {
		case 0x60: /* Voyager, Viking */
			product = gen == 0 ? "920" :
				"930";
			break;
		case 0x40: /* Jupiter */
			if (gen == 0)
				product = "Jupiter";
			break;
		default:
			fprintf(stderr, "%s: cannot parse chip revision: chip_rev %x\n",
				__func__, dev->info.chip_rev);
			break;
		}
		if (product) {
			snprintf(name, sizeof(name), "Samsung Xclipse %s",
				 product);
			dev->marketing_name = strdup(name);
		}
	}

	*major_version = dev->major_version;
	*minor_version = dev->minor_version;
	*device_handle = dev;
	dev->next = dev_list;
	dev_list = dev;
	pthread_mutex_unlock(&dev_mutex);

	return 0;

cleanup:
	if (dev->fd >= 0)
		close(dev->fd);
	free(dev);
	pthread_mutex_unlock(&dev_mutex);
	return r;
}

drm_public int amdgpu_device_deinitialize(amdgpu_device_handle dev)
{
#ifdef __ANDROID__
	if (dev->ion_fd)
		exynos_ion_close(dev->ion_fd);
#endif /* __ANDROID__ */
	pthread_mutex_lock(&dev_mutex);
	amdgpu_device_reference(&dev, NULL);
	pthread_mutex_unlock(&dev_mutex);
	return 0;
}

drm_public int amdgpu_device_get_fd(amdgpu_device_handle device_handle)
{
	return device_handle->fd;
}

drm_public const char *amdgpu_get_marketing_name(amdgpu_device_handle dev)
{
	return dev->marketing_name;
}

drm_public int amdgpu_query_sw_info(amdgpu_device_handle dev,
				    enum amdgpu_sw_info info,
				    void *value)
{
	uint32_t *val32 = (uint32_t*)value;

	switch (info) {
	case amdgpu_sw_info_address32_hi:
		if (dev->va_mgr.vamgr_high_32.va_max)
			*val32 = (dev->va_mgr.vamgr_high_32.va_max - 1) >> 32;
		else
			*val32 = (dev->va_mgr.vamgr_32.va_max - 1) >> 32;
		return 0;
    default:
        break;
	}
	return -EINVAL;
}

static int sgpu_query_sw_info_version(amdgpu_device_handle dev,
				      enum amdgpu_sw_info info,
				      struct sgpu_param_query_sw_info *params)
{
	drmVersion drm_vers;
	drmVersionPtr drm_vers_handle;
	drm_vers_handle = &drm_vers;

	assert(info == sgpu_sw_info_version);

	drm_vers_handle = drmGetVersion(dev->fd);

	/** Pack Major Minor into the upper and lower bits of 32 bit value */
	params->drm_version = drm_vers_handle->version_major << 16;
	params->drm_version |= drm_vers_handle->version_minor & 0x0000FFFF;

	/** Max size is the size of array, 256 bytes */
	params->data_size = snprintf(params->data,
				     sizeof(params->data),
				     "KMD: <user> %s\n"
				     "KMD: <merge> %s\n"
				     "KMD: <revision> %s\n"
				     "libDRM_sgpu: <user> %s\n"
				     "libDRM_sgpu: <merge> %s\n"
				     "libDRM_sgpu: <revision> %s\n",
				     /** @todo GFXSW-3434 - Fill in values with real data
				      *  Requires extension to existing amdgpu_info_ioctl.
				      */
				     "NOT_IMPLEMENTED",
				     "NOT_IMPLEMENTED",
				     "NOT_IMPLEMENTED",
				     "","","");
//LIBDRM_VERSION_USERNAME,
//				     LIBDRM_VERSION_MERGE_SHA1,
//				     LIBDRM_VERSION_REVISION_NUMBER);

	return 0;
}

drm_public int sgpu_query_sw_info(amdgpu_device_handle dev,
				  const enum amdgpu_sw_info info,
				  struct sgpu_param_query_sw_info *params)
{
	int result;

	switch (info) {
	case amdgpu_sw_info_address32_hi:
		result = amdgpu_query_sw_info(dev, info, &params->value);
		params->data_size = 0;
		break;
	case sgpu_sw_info_version:
		result = sgpu_query_sw_info_version(dev, info, params);
		break;
	default:
		result = -EINVAL;
	}

	return result;
}
