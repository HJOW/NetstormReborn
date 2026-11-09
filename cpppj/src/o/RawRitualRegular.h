// Rune·다이스·제단 정기의 분기 본문을 같은 실제 부모/현재 장치로 공유한다.
#pragma once
#include "o/SquidProcess.h"
#include "o/RiftType.h"
#include <functional>

namespace netstorm::o { // 장기 Native와 기존 독립 typed가 같은 원문 분기 순서를 실행한다.
struct RuneRegularHooks { // 304에서 선택한 현재 검색/진행/가상 삭제만 요구한다.
    std::function<Sid(Sid)> findDais; // 같은 현재 역할과 owner로 반경1을 검색한다.
    std::function<void(Sid)> progress; // 발견한 실제 다이스의 전체 진행 본문이다.
    std::function<void(Sid,std::uint32_t)> destroy; // Rune의 현재 가상10에 flags48을 전달한다.
};
float RunRuneRegular(const SidPool& pool,Sid sid,std::uint32_t event,float payload,const RuneRegularHooks& hooks); // 44a3a0의304 또는 실제base payload 반환이다.
struct DaisHandleHooks { // 305/306/307과 공통 Damageable을 원래 선택 위치에서 호출한다.
    std::function<std::uint32_t(Sid,std::int32_t)> stage; // 저장한 프레임에 현재 가상 Count/단계 표를 적용한다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> frame; // 같은 실제 Frame.Set이다.
    std::function<float(Sid)> sacrificeAdvance; // 실제44a200 전체 후단이다.
    std::function<void(Sid)> ritual; // 현재 강한 사제 검색 뒤 실제447e40을 호출한다.
    RegularHandler damageable; // 나머지 사건의 같은44b900 본문이다.
};
float RunDaisHandle(const SidPool& pool,Sid sid,std::uint32_t event,std::uint32_t count,float payload,const DaisHandleHooks& hooks); // 44b150의 현재 프레임과 분기 순서를 보존한다.
struct AltarSacrificeState { // 원본 제단의 현재 저항 지연/확률이며 Session에서 재설정하지 않는다.
    float struggleSeconds{3.0F}; //00532698의 다음 주기 덧셈 값이다.
    std::int32_t escapePercent{40}; //0053269c의 원문 난수/1000 비교 값이다.
};
struct AltarRegularHooks { // 조회·프레임·난수·예약과 선택된 큰 후단을 구별한다.
    std::function<Sid(Sid)> dais,priest; // 기준점의 typed 다이스와 그 안의 현재 사제 내용물을 검색한다.
    std::function<bool()> boss; // 각 원문 관문에서 현재 권한을 읽는다.
    std::function<std::int32_t(std::uint32_t,std::uint32_t)> alliance; // 저장된 두 owner로 현재 방향별 동맹DWORD를 읽는다.
    std::function<void(Sid)> release,removeAnimation; // 실제 해방/애니메이션 제거 전체 본문이다.
    std::function<void(Sid,std::uint32_t)> escapeFrame,destroy; // 다이스 이탈 프레임 또는 실제 가상 삭제이다.
    std::function<bool(Sid,std::int32_t,std::uint32_t)> advance; // 실제 FrameAdvance의 구간 끝 여부를 반환한다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> frame; // 같은 Frame.Set이다.
    std::function<bool()> reserveShared; // 원문 new28의 실제 확보 성공이다.
    std::function<void(Sid,std::uint32_t,float)> addShared; // 같은 확보 주소에 실제 SharedRegular를 구성/부착한다.
    std::function<std::int32_t(std::int32_t,std::int32_t)> randomInt; // 단일 현재 게임 난수를 선택된 범위에서 소비한다.
    std::function<float(float,float)> randomFloat; // 단일 현재 게임 난수의 원문 실수 구간이다.
    std::function<void(float,float,std::string_view,std::uint32_t)> sound; // 원문 위치/이름/flags 그대로 실제 소리 장치에 전달한다.
    RegularHandler damageable; // 나머지 사건의 기존44b900 본문이다.
};
float RunAltarRegular(const SidPool& pool,std::span<const RiftTypeFrames> frames,Sid sid,std::uint32_t event,std::uint32_t count,float payload,const AltarSacrificeState& state,const AltarRegularHooks& hooks); //44a5e0 전체 분기를 기존 typed/Native에서 함께 사용한다.
}
