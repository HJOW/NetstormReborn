// 마나 볼트의 실제 주기 프레임과 건설 대상 옆 화면 위치를 복원한다.
#pragma once
#include "o/RawManaBoltArrival.h"
#include "o/SquidDisplay.h"
#include "o/SquidProcess.h"
namespace netstorm::o {
// 주기 함수가 읽는 실제 간격 전역이다. 카메라는 별도 현재 투영 경계에서 읽는다.
struct ManaBoltRegularState {
    float interval{0.05f}; // 00532750의 다음 실행 간격이다.
    std::int32_t spacing{16}; // 005325e0의 아이콘 가로/세로 간격이다.
};
// 계산 밖의 실제 Frame/가상 공간/음향/현재 카메라 경계다.
struct ManaBoltRegularHooks {
    std::function<void(Sid,std::int32_t,std::uint32_t)> setFrame; // 4acee0(frame,flags0)를 같은 raw 객체에 적용한다.
    std::function<void(float,float,std::string_view)> sound; // event0/count0의 현재 위치 소리다.
    std::function<std::array<std::int32_t,2>(float,float)> project; // 497220의 현재 정수 화면점이다.
    std::function<std::array<float,2>(std::int32_t,std::int32_t)> inverse; // 4971e0의 현재 float 카메라 역투영이다.
    std::function<void(Sid,std::uint32_t)> unpop,destroy; // 실제 가상+48/+10이며 flags0이다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 실제 가상+90이며 flags0이다.
};
// 45a8e0을 같은 TYPE/프레임/SHP/대상 검색에 적용한다. 미복원 다른 파생은 처리하지 않는다.
class RawManaBoltRegular {
public:
    RawManaBoltRegular(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
        std::span<const SquidDisplayShape> shapes,const RawManaBoltArrival& arrival,ManaBoltRegularState& state,ManaBoltRegularHooks hooks); // 모든 참조는 이 모듈보다 오래 살아야 한다.
    bool Handles(Sid sid) const; // 실제105/509810 쌍만 현재 Regular 분배에서 받는다.
    float Run(Sid sid,std::uint32_t event,std::uint32_t count) const; // 현재 프레임→대상/위치→0.05 또는 삭제 후-1이다.
    const SidPool& Pool() const; // 다른 월드의 분배기에 연결하지 않게 한다.
private:
    const SidPool& pool_; // 프레임/좌표/WORD/현재 대상 raw의 같은 풀이다.
    std::span<const RiftTypeRecord> types_; // 현재 TYPE의 해제 흔적과 플래그를 읽는다.
    std::span<const RiftTypeFrames> frames_; // 현재 물리 frame의 signed 글자와 구간 표다.
    std::span<const SquidDisplayShape> shapes_; // 자기 TYPE frame0의 원본 pixel 폭/핫스폿이다.
    const RawManaBoltArrival& arrival_; // 같은 finder의 실제 현재 Target을 재사용한다.
    ManaBoltRegularState& state_; // 외부 효과 뒤에도 현재 간격/spacing을 읽는다.
    ManaBoltRegularHooks hooks_; // Frame/장치/화면/가상 공간의 실제 효과다.
};
}
