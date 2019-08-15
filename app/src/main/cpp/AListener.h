/*
 *
 *     This file is part of AListener.
 *
 *  AListener is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  AListener is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with AListener.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef ALISTENER_ALISTENER_H
#define ALISTENER_ALISTENER_H

#include <string>
#include <array>
#include <vector>
#include <cstdint>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>

class AListener {
public:
    constexpr static auto FNAME = "/sdcard/android2.bin";
    // Signed 16 bit Little Endian, Rate 16000 Hz, Mono
    constexpr static auto RECORDER_FRAMES = 16000 * 5;
    struct record_buf_t {
        std::array<short, RECORDER_FRAMES> data;
        size_t filled{0};
        SLRecordItf rec;
        AListener *listener;
    };

    void main();

    void record_loop();

    static
    void log(std::string str);

    static void write(record_buf_t *data);

private:
    bool init();

    void start_record();

    void create_eng();

    SLObjectItf engineObj{nullptr};
    SLEngineItf engineEng;

    SLObjectItf recorderObj{nullptr};
    SLAndroidSimpleBufferQueueItf recorderBufferQueue;

    record_buf_t rbuf;
} ;


#endif //ALISTENER_ALISTENER_H
