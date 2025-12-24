#pragma once

#include <stdint.h>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <format>
#include <chrono>
#include <algorithm>

#include <RE/Fallout.h>
#include <F4SE/F4SE.h>

#include <REL/Trampoline.h>
#include <REX/REX/INI.h>
#include <xbyak/xbyak.h>
#include <spdlog/spdlog.h>


using namespace std::literals;

using AddressT = REL::Relocation<std::uintptr_t>;
