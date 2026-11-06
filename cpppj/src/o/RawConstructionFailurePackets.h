// 원본 비용 환불6ch/배치 취소2fh 쓰기와004332c0의 예측 객체 정리다.
#pragma once
#include "o/RawConstructionPlace.h"
#include "o/RawConstructionClear.h"
namespace netstorm::o { // 기존 실제 Pool/공간/Decoder/삭제 가상을 재사용한다.
std::array<std::uint8_t,11> WriteConstructionMoneyPacket(const ConstructionMoneyNotice& notice); //0040af90/442a40의 실제11BYTE를패딩없이쓴다.
std::array<std::uint8_t,15> WriteConstructionRejectPacket(const ConstructionRejectNotice& notice); //0040a620/442af0의 실제15BYTE를패딩없이쓴다.
struct ConstructionFailureWorld {
    const SidPool& pool;const SquidHash& hash;std::span<const RiftTypeRecord> types;std::span<const PriestPlainCanonType> frames; // 같은현재월드의물리다.
    std::span<const std::uint32_t,8> patternTypes;const std::uint32_t& noIslandType;const CanonDecoderDiagnostics* diagnostics{}; // Native현재전역/진단의주소를빌린다.
};
void ReceiveConstructionReject(std::span<const std::uint8_t> packet,ConstructionFailureWorld world,const std::function<void(Sid,std::uint32_t)>& destroy); // 칸별모든matching abstract를flags8로지우며첫후보/로컬SID로제한하지않는다.
}
