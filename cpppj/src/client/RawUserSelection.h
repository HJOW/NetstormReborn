// 원본 Userinput.cpp의 세 인자 선택과 다중 선택 정리를 같은 현재 저장소에서 실행한다.
#pragma once
#include "client/InputEvent.h"
#include "o/RiftType.h"
#include "o/SidPool.h"
#include <array>
#include <functional>

namespace netstorm::client {
// 선택·마우스 TYPE·기억 좌표와 다중 선택은 서로 다른 전역이다. 참조 대상은 처리기보다 오래 살아야 한다.
struct UserSelectionState {
    std::uint32_t& selected; // 005caea0의 현재 SID이며 가상 통지 뒤 원본 위치에서만 변경한다.
    std::uint32_t& mouseType; // 005caea4의 기억한 이동 타입이며 강조 모드와 합치지 않는다.
    std::array<std::uint32_t,2>& rememberedPoint; // 005caf38/3c의 두 float 원문 비트다.
    std::span<const std::uint32_t> multiple; // 005caf2c의 다중 선택 배열이며 정리 중에도 현재 항목을 읽는다.
    std::int32_t& multipleCount; // 005caf34의 signed 개수이며 통지 뒤 다음 반복 조건에서 다시 읽는다.
};
// 현재 게임 전역과 실제 파생 가상함수·마나/사정거리·이동·장치만 공급한다. 도달한 경계의 누락은 오류다.
struct UserSelectionHooks {
    std::function<std::uint32_t()> paused,enabled,blocked; // 460de0/5434e4/463b50의 별도 첫 관문이다.
    std::function<std::uint32_t()> localOwner,editor; // 540c70/5c85a4를 실제 이동 강조 분기에서 읽는다.
    std::function<std::uint32_t()> dais,manaProcess,rangeProcess; // 현재5412ec/541138/54113c의 타입 DWORD다.
    std::function<void(o::Sid,std::uint32_t)> selectNotice; // 객체 가상+7c의0/1 통지이며 현재 selected를 이미 쓴 뒤 호출할 수 있다.
    std::function<void(o::Sid)> refresh; // 객체 가상+88이며 앞선 통지가 바꾼 현재 VT로 호출한다.
    std::function<void(o::Sid,std::uint32_t,std::uint32_t,std::uint32_t)> processNotice; // ProcessFinder 대상의 실제 프로세스 가상+1c다.
    std::function<void(o::Sid)> dump; // 44dab0의 재귀 진단이며 원본 호출 순서를 생략하지 않는다.
    std::function<void(o::Sid,std::uint32_t)> showMana,showRange; // 4741d0/474900의 실제 마나/사정거리 생성이며 둘째0은Form미지정이다.
    std::function<std::uint32_t(std::uint32_t)> rangeType; // 474240의 사정거리 종류이며 이동 가능 판정과 다르다.
    std::function<o::Sid(o::Sid)> content,altar; // 4ac9f0(1,0,0)/447930의 현재 내용물과 같은 위치 제단이다.
    std::function<std::uint32_t(o::Sid)> moving; // 425ed0의 현재 운반/이동 상태다.
    std::function<void(o::Sid)> movementPreview; // 426080의 실제491 Regular 생성 경계다.
    std::function<void(std::uint32_t)> highlight; // 같은 현재 전역을 사용하는 RawUserHighlight::Highlight다.
    std::function<void(std::uint32_t,std::uint32_t,std::string_view,std::uint32_t)> sound; // 4a9d70의 두 float 비트/이름/0이다.
    std::function<void()> assertion; // selectedSid 진단 뒤 현재 선택을 다시 읽는다. 임의 성공을 만들지 않는다.
    std::function<void()> previewAssertion; // 426080의 !s.isForm() 진단 뒤 현재 TYPE를 다시 읽는다.
};
// PATCH10.78의 전체 선택 부모와 짧은 관련 몸체다. Select의 둘째·셋째 인자는 독립 DWORD다.
class RawUserSelection {
public:
    // 같은 물리 풀/188 TYPE/현재 UI 저장소를 빌린다. 생성 시 외부 효과를 실행하지 않는다.
    RawUserSelection(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,UserSelectionState state,UserSelectionHooks hooks);
    std::uint32_t Select(std::uint32_t sid,std::uint32_t forceSelect,std::uint32_t activateMove) const; // 4d5c90: 거부0 또는 최종 대상 SID다.
    void ClearMultiple() const; // 4d5490: 범위와 state&3만 검사하고+7c(0)→+88 뒤 최종 개수를0으로 쓴다.
    void Remember(std::uint32_t x,std::uint32_t y,std::uint32_t type) const; // 4d5440: flags2&30000일 때만 좌표와 TYPE를 기억한다.
    std::uint32_t CarrierMenu(o::Sid sid,const InputEvent& event,const std::function<std::uint32_t(o::Sid,const InputEvent&)>& common) const; // 4268c0/492080: Select(sid,0,0) 뒤 실제 공통+ac다.
    std::uint32_t ArcherMenu(o::Sid sid,const InputEvent& event,const std::function<std::uint32_t(o::Sid,const InputEvent&)>& common) const; // 41e660: 셋째 인자만1이며 둘째는0이다.
private:
    std::span<const std::uint8_t> Raw(std::uint32_t sid) const; // 원본의 물리 범위만 확인하며 free/dead를 추가 제외하지 않는다.
    const o::RiftTypeRecord& Type(std::uint32_t type) const; // 현재 TYPE 번호를 검증한다.
    static std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4); // 정렬되지 않은 원문 DWORD/WORD다.
    void ClearPrevious(o::Sid sid) const; // ProcessFinder의 다음 링크를 효과 전에 저장하여50/51만 통지한다.
    const o::SidPool& pool_; // 같은 실제 객체와 종속 Form 저장소다.
    std::span<const o::RiftTypeRecord> types_; // 콜백 뒤 다시 읽는 실제 TYPE 속성이다.
    UserSelectionState state_; // 단일 선택/다중 선택/기억 좌표의 현재 참조다.
    UserSelectionHooks hooks_; // 큰 실제 하위와 장치의 명시 소비자다.
};
}
