//
// Created by David on 16/12/19.
//

#include "IYCWebRtcNSx.h"

int IYCWebRtcNsx_Create(NsxHandle** nsxInst) {
    NsxHandle* self = WebRtcNsx_Create();
    *nsxInst = self;

    if (self != NULL) {
        return 0;
    } else {
        return -1;
    }

}

int IYCWebRtcNsx_Process(NsxHandle* nsxInst, short* speechFrame,
                      short* speechFrameHB, short* outFrame,
                      short* outFrameHB) {

    WebRtcNsx_Process(nsxInst, &speechFrame, 0, &outFrame);
    return 0;
}

int IYCWebRtcNsx_Init(NsxHandle* nsxInst, uint32_t fs) {
    int ret = WebRtcNsx_Init(nsxInst, fs);
    return ret;
}

int IYCWebRtcNsx_set_policy(NsxHandle* nsxInst, int mode) {
    int ret = WebRtcNsx_set_policy(nsxInst, mode);
    return ret;
}

void IYCWebRtcNsx_Free(NsxHandle* nsxInst) {
    WebRtcNsx_Free(nsxInst) ;
}