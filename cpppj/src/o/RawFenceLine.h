// PATCH 펜스 사이의 기존 연결선 삭제00455b20(mode0)을 실제 finder/가상 삭제에 연결한다.
#pragma once
#include "o/RawSquidFinder.h"
#include <array>

namespace netstorm::o {
// 원본 연결선의 정수 검색점과 현재 fencemark 프레임만 처리한다.
class RawFenceLine {
public:
    RawFenceLine(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<const RiftTypeFrames> frames,const std::uint32_t& markType,std::function<void(Sid,std::uint32_t)> destroy); // 실제 동적 저장소와 삭제 소비자를 빌린다.
    static std::vector<std::array<std::int32_t,2>> SearchPoints(std::array<float,4> endpoints); // 원문100회 상한/포함 끝점/CRT 절삭을 보존한다.
    void Clear(std::array<float,4> endpoints) const; // 방향 코드J/K가 맞는 현재 연결선만 가상+10(flags40)으로 삭제한다.
private:
    SidPool& pool_; // 현재 mark 기록과 삭제 후 체인을 읽는 실제 풀이다.
    const SquidHash& hash_; // mode0의 flags0 일반 finder가 사용하는 실제 해시다.
    std::span<const RiftTypeRecord> types_; // 현재 finder 발자국 표다.
    std::span<const RiftTypeFrames> frames_; // 004ad680의 실제 첫 프레임 코드BYTE 표다.
    const std::uint32_t& markType_; // DAT00541300의 현재 연결선 타입 번호다.
    std::function<void(Sid,std::uint32_t)> destroy_; // 원본 가상 삭제 전체 몸체이며 선택한 경우 필수다.
};
}
