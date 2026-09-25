/*
 * SPDX-FileCopyrightText: Copyright (c) 2019 NVIDIA CORPORATION & AFFILIATES.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>

#include "utils/utils.h"
#include "utils/channel.hpp"
#include "common.h"

extern "C" __device__ __noinline__
void instrument_mem(
    int pred,
    int /* opcode_id */,
    uint64_t addr,
    uint64_t /* grid_launch_id */,
    uint64_t pchannel_dev) {

    // Only lanes that actually perform the memory reference contribute.
    if (!pred) {
        return;
    }

    const unsigned mask = __ballot_sync(__activemask(), 1);
    const int leader = __ffs(mask) - 1;
    const int lane = get_laneid();

    mem_access_t record = {};

    const int4 cta = get_ctaid();

    // Flatten a three-dimensional CTA index.
    record.cta_id =
        (uint64_t)cta.x +
        (uint64_t)gridDim.x *
            ((uint64_t)cta.y +
             (uint64_t)gridDim.y * (uint64_t)cta.z);

    record.active_mask = mask;

    // All participating lanes execute these shuffles.
    for (int i = 0; i < 32; ++i) {
        if (mask & (1u << i)) {
            record.addrs[i] = __shfl_sync(mask, addr, i);
        }
    }

    // Emit one record for this warp memory reference.
    if (lane == leader) {
        ChannelDev *channel = (ChannelDev *)pchannel_dev;
        channel->push(&record, sizeof(record));
    }
}