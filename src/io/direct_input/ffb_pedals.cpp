#include "ffb_pedals.h"

FFBPedals::FFBPedals(const FFBConfig& config) :
    FFBDevice(config, config.GetString(L"base", L"pedal device"), true) {}
FFBPedals::~FFBPedals() {}
