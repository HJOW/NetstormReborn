// 중앙섬 군도의 실제 지면 연결을 섬 목록의 독립 지역으로 나눈다.
#pragma once
#include "o/RawIslandRuntime.h"

namespace netstorm::o { // 원문0046f7d0/0046ed50/0046ec00의 현재 표면/프레임/지역을 사용한다.
class RawIslandPartition {
public:
    RawIslandPartition(const SidPool& pool,std::span<const std::uint16_t> surfaces,std::span<const RiftTypeFrames> frames,const std::uint32_t& terrainType,RawIslandRuntime& islands,std::uint32_t& worldRedraw,std::function<std::uint32_t(Sid)> getRegion,std::function<void(Sid,std::uint32_t)> setRegion); // 모든 현재 자료/장치는 Split이 끝날 때까지 유효해야 한다.
    std::uint32_t Split() const; // y행/x열 순서로 지역0의 지면을 찾고 실제 raw 좌표에서 flood하여 새 섬/경계를 기록한다.
private:
    Sid Surface(std::int32_t x,std::int32_t y) const; // 월드 밖/빈 칸은0이며 free/dead/owner 필터를 추가하지 않는다.
    FortRandomRectangle Flood(std::int32_t x,std::int32_t y,std::uint32_t region,std::span<const FrameCode> codes) const; // 북/동/남/서 순서로 현재 지역을 지정하고 좌표 경계를 누적한다.
    const SidPool& pool_; // 같은 실제 표면 SID를 읽는 현재 Pool이다.
    std::span<const std::uint16_t> surfaces_; //0046f7d0이 직접 읽는 같은65536WORD hash0 머리다.
    std::span<const RiftTypeFrames> frames_; // component 생성 때 현재 지면 TYPE가 선택하는 실제 코드 표다.
    const std::uint32_t& terrainType_; //005411dc의 현재 전체 DWORD다.
    RawIslandRuntime& islands_; // 실제100기록의 count/Add/Bounds를 사용하는 장기 목록이다.
    std::uint32_t& worldRedraw_; // flood wrapper가 매번 증가시키는 같은0059a8b0이다.
    std::function<std::uint32_t(Sid)> getRegion_; //004acda0의 현재 TYPE 진단/다리/noRegion 관문을 사용한다.
    std::function<void(Sid,std::uint32_t)> setRegion_; //004acd20의 현재 TYPE 진단/같은 raw WORD 쓰기다.
};
}
