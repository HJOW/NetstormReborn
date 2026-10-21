// Factory의 가상입력004547d0을 기존 연구/공통 객체 입력과 같은 월드에서 합성한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string_view>

namespace netstorm::o {
// 현재 모드/로컬소유자는 참조하며 원본 명령 전역의 기본값을 유지한다.
struct FactoryInputState {
    const std::uint32_t& editor; //005c85a4:SP 조회 뒤 자금 부족 분기만 생략한다.
    const std::uint32_t& localOwner; //00540c70:현재 로컬 SP를 조회하는 전체 DWORD다.
    std::uint32_t cycleCommand{0x03000063U}; //00542884:레벨을 세 상태로 순환시킨다.
    std::uint32_t upgradeCommand{0x03000032U}; //005427c0:자금/진행 관문 뒤 실제 업그레이드를 요청한다.
    std::uint32_t researchCommand{0x03000080U}; //005428f8:기존 연구 입력에 같은 원문을 전달한다.
};
// 선택한 원본 가상·분산SP·안내·입력 하위만 요구한다.
struct FactoryInputHooks {
    std::function<std::uint32_t(Sid)> level; //가상80의 현재 저장 레벨이다.
    std::function<void(Sid,std::uint32_t)> setLevel; //가상84의 저장/표시 갱신이다.
    std::function<void(Sid)> adjustFrame; //00453780의 실제 현재 상태 프레임 보정이다.
    std::function<std::int32_t(std::uint32_t)> cost; //59ab50/type*23의 signed 복호화 비용 단위다.
    std::function<float(std::uint32_t)> money; //0040ec50의 현재 로컬 분산SP를 float 비트 그대로 읽는다.
    std::function<bool(Sid)> upgrading; //00453c90의 그림→event4→event3 단락 조회다.
    std::function<void(Sid)> request; //00453bf0의 같은 실제 요청/예약/58h 송신이다.
    std::function<void(std::string_view)> tell; //004cf960의 upgradeNoBucks/alreadyUpgrading 섹션이다.
    std::function<std::uint32_t(Sid,std::uint32_t,std::uint32_t)> research; //기존 연구의 같은 code/packedArgument를 전달한다.
    std::function<std::uint32_t(Sid,std::uint32_t,std::uint32_t)> parent; //004ae010 공통 부모에 수정키 포함 원문을 위임한다.
};
// 메뉴와 직접 명령이 동일한 자금/예약 판단을 사용하며 임의 소유자/상한 관문을 추가하지 않는다.
class RawFactoryInput {
public:
    RawFactoryInput(const SidPool& pool,const FactoryInputState& state,FactoryInputHooks hooks); //같은 PATCH 풀/현재 상태의 수명을 빌린다.
    std::uint32_t Input(Sid parent,std::uint32_t code,std::uint32_t packedArgument) const; //원본 순서cycle→upgrade→research→부모를 실행한다.
    const SidPool& Pool() const; //같은 실제 월드 연결을 확인한다.
private:
    const SidPool& pool_; //현재 Factory TYPE를 읽는 실제 풀이다.
    const FactoryInputState& state_; //하위 효과 뒤 현재 모드/소유자/명령을 다시 읽는다.
    FactoryInputHooks hooks_; //원본 실제 하위와 UI 경계다.
};
}
