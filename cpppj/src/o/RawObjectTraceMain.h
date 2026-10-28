// 자산 Main/Task 진단과 현재 원본 가상+0x70 분배를 제공한다.
#pragma once
#include "o/RawObjectTraceFields.h"

namespace netstorm::o { // 실제 로그 장치 하나가 공통/파생 진단을 같은 순서로 받는다.
// Main 로그에 전달되는 원문 float32 승격과 부호 확장의 최종 숫자 인자다.
struct ObjectTraceMainNumbers {
    double x{},y{}; // +0xe/+0x12 float32를 double로 승격한 값이다.
    std::int32_t highlight{}; // +0x21 signed BYTE다.
    std::uint32_t quantity{},region{}; // +0xc/+8의 unsigned WORD다.
    std::int32_t z{}; // +0x23 signed BYTE다.
};
// Main 줄일 때만 numbers를 빌리며 호출이 끝난 뒤 보관하면 안 된다. 다른 줄은 nullptr다.
using ObjectTraceMainLog=std::function<void(std::int32_t,std::string_view,const ObjectTraceMainNumbers*)>;
// 원본 CRT의17자리 가수·5자리 고정소수점·5이상 올림·음의0/특수값 표기를 float32에서 복원한다.
std::string FormatOriginalTraceCoordinate(float value);
// 004b0fb0/00450d60의 직접 본체다. task=true이면 Main 로그 복귀 뒤 현재 작업WORD를 다시 읽는다.
void TraceRawObjectMain(const SidPool& pool,std::span<const RiftTypeRecord> types,const std::uint32_t& suppress,
    std::uint32_t sid,bool task,const ObjectTraceMainLog& log);
// 현재 물리 가상 표에서 원문의+70 본체를 선택한다. 83개 자산과 네 내장Form 표를 지원한다.
void TraceRawObjectVirtual(const SidPool& pool,std::span<const RiftTypeRecord> types,const std::uint32_t& suppress,
    std::uint32_t sid,const ObjectTraceMainLog& log);
}
