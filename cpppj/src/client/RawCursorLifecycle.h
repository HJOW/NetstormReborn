// 원본10.78 Cursor.cpp의 취득00447000·취소00446e70·미리보기 정리00445aa0을 같은 커서 상태로 실행한다.
#pragma once
#include "client/RawCursorDrop.h"
#include "client/GumpText.h"
#include "client/RawPriestMenu.h"

namespace netstorm::client {
class GameAssets; // 실제 자산 저장소의 선언이며 헤더에서 게임 자산을 중복 소유하지 않는다.
// 내려놓기와 겹치는 전역은 CursorDropState에서 빌린다. 이 구조체에는 나머지 원본 전역만 둔다.
struct CursorLifecycleState {
    std::uint32_t abstractType{}; // 0055a280의 TYPE 포인터를 현재 표의 번호로 보관한다.
    std::uint32_t directionArgument{},currentGump{}; // 0055a2a0·0055a278이며 direction/커서 프로세스와 구별한다.
    std::array<o::Sid,30> previews{}; // 0055a1d8의 실제 임시 월드 객체 목록이다.
    std::int32_t previewCount{}; // 0055a1b8이며 정리 도중에도 현재 값을 다시 읽는다.
    std::uint32_t warningFont{},costFont{},costType{92},teleportBlocked{}; // 원본 글꼴 두 전역·비용 그림 타입·금액 진단이다.
    std::array<std::uint32_t,8> warningContext{2,0,0,0,0,0,1,1},costContext{2,3,0,0,0,0,1,1}; // 측정 문맥의 +8만 현재 글꼴로 바뀐다.
    std::array<std::int32_t,5> warningWidths{},warningHeights{}; // 원본 다섯 배치 거부 문구의 개별 치수다.
    std::int32_t warningWidth{},warningHeight{},costTextWidth{},costHeight{}; // 각각 최대 경고 폭/높이와 금액의 측정 폭/최종 높이다.
    std::int32_t costHotspotX{},costHotspotY{},imageHeight{}; // 비용 그림 프레임3의 signed WORD와 별도 높이다.
    std::uint32_t costImage{}; // 실제 SHP 블록의 호스트 식별값이며 원본 포인터로 역참조하지 않는다.
    std::string costText; // 0055a1bc의 절삭된 비용 문자열이다.
};
// 프레임3의 원본 추가 헤더다. 그림 식별값과 signed WORD 치수를 실제 자산 연결기가 공급한다.
struct CursorCostImage {
    std::uint32_t token{}; // 같은 SHP 저장소의 블록 식별값이다.
    std::int16_t width{},height{},hotspotX{},hotspotY{}; // 원본 헤더+24/26/28/30이다.
};
// 아직 복원하지 않은 위치 판정·미리보기 생성·GUI/가상 효과만 명시 경계다. 도달한 경계만 요구한다.
struct CursorLifecycleHooks {
    std::function<void(std::uint32_t)> highlight,selectCursor; // 기존 강조004d5bb0와 실제 하드웨어 커서004a4410이다.
    std::function<void()> prepareCursor,closePopups,createPreviews,queryCursor; // 기존 준비·팝업 정리·445b30·445440이다.
    std::function<o::Sid(o::Sid)> source; // Form 가상+0의 실제 연관 객체 번호다.
    std::function<void(o::Sid,o::Sid,std::uint32_t)> sourceNotice; // 연관 객체 가상+24(preview,4)다.
    std::function<void(o::Sid,std::uint32_t)> detach,unpop,destroy; // 가상+48/+4c/+10이며 각각 인자0이다.
    std::function<void(std::uint32_t,std::uint32_t)> removeProcess; // 현재 마나 프로세스의 실제0041bf10(process,0)이다.
    std::function<std::uint32_t(o::Sid,std::uint32_t)> createMana,createRange; // 004741d0/00474900(firstPreview,0)이다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> processEvent; // 프로세스 가상+1c(1,1,0)이다.
    std::function<std::uint32_t(std::uint32_t)> defaultArgument,typeNumber; // 현재 TYPE+118과0049adc0(TYPE)의 번호 반환이다.
    std::function<bool(std::uint32_t)> hasRange; // 실제00474240(TYPE 번호)의 현재 group/flags2 판정이다.
    std::function<std::string(std::string_view)> translate; // 원본 다섯 문구의 현재 번역이다.
    std::function<std::array<std::int32_t,2>(std::string_view,const std::array<std::uint32_t,8>&)> measure; // 실제004a3df0(text,w,h,context,-1)다.
    std::function<CursorCostImage(std::uint32_t,std::uint32_t)> costImage; // 원본 frameCheck 뒤 SHP 물리프레임3의 추가 헤더다.
    std::function<void(std::int32_t,std::int32_t)> updatePosition; // 현재 mouseX/Y의00446b50이며 Take의 꼬리 호출이다.
    std::function<void(std::string_view)> sound; // teleporterActive.wav의 원본 여섯 인자 재생이다.
    std::function<void(float)> addLocalSp; // 실제0040eec0의 float 비용 차감이다.
    std::function<ScreenPoint()> mouse; // 현재005cae94/98을 꼬리 호출 직전에 읽는다.
};
// 사제 건설 메뉴와 생산 칸이 같은 Form·임시 객체·취소/내려놓기 상태를 공유하게 하는 원본 수명 몸체다.
class RawCursorLifecycle {
public:
    // 참조한 풀/타입/상태/효과는 이 객체보다 오래 살아야 한다. 현재 복원은10.78만 허용한다.
    RawCursorLifecycle(o::SidPool& pool,std::span<const o::RiftTypeRecord> types,CursorDropState& drop,
        CursorLifecycleState& state,CursorLifecycleHooks hooks);
    // 00447000의 두 cdecl 인자다. 잘못 추론된 decompile unaff_retaddr는 실제 두 번째 flags다.
    void Take(o::Sid preview,std::uint32_t flags) const;
    // 00446e70: 잠금/현재 Form·저장 WORD를 다시 읽으며 반환·임시 삭제·프로세스 정리를 수행한다.
    void Cancel() const;
    // 00445aa0: 준비 뒤 flags0x200 또는 force!=0일 때 현재 목록을 가상 삭제하고30칸을 지운다.
    void Activate(std::uint32_t force) const;
    // 실제 메뉴/내려놓기 어댑터가 같은 풀을 공유하는지 확인한다.
    const o::SidPool& Pool() const;
private:
    // 도달한 raw 번호와 TYPE 논리 포인터의 범위를 확인한다.
    std::span<const std::uint8_t> Raw(o::Sid sid) const;
    const o::RiftTypeRecord& Type() const;
    o::SidPool& pool_; // 취득/취소가 실제로 바꾸는 단일 SID 풀이다.
    std::span<const o::RiftTypeRecord> types_; // 메뉴·건설·내려놓기와 같은 현재188TYPE다.
    CursorDropState& drop_; // 잠금·Form·저장 WORD·flags·커서 프로세스와 위치의 단일 상태다.
    CursorLifecycleState& state_; // 나머지 목록·측정·TYPE 논리 포인터 전역이다.
    CursorLifecycleHooks hooks_; // 복원 계산을 대신하지 않는 외부 효과와 현재 자료 조회다.
};
// 사제 선택의 실제 Form 취득과 내려놓기의 잠금/정리를 같은 수명 몸체에 연결한다.
void BindCursorLifecycle(PriestMenuHooks& priest,CursorDropHooks& drop,RawCursorLifecycle& lifecycle);
// 00474240은 TYPE 번호로 현재 group/flags2를 읽는다. SHP 블록 토큰을 받지 않는다.
bool HasCursorRange(std::span<const o::RiftTypeRecord> types,std::uint32_t typeNumber);
// 장치 없이 같은 SHP/defaultFrame/TYPE 조회를 연결하며 기존 측정기를 유지한다.
void BindCursorLifecycleAssets(CursorLifecycleHooks& hooks,const GameAssets& assets);
// 실제 글자 측정·SHP/defaultFrame·TYPE 번호와 사정거리 판정을 기존 자산 저장소에 연결한다.
void BindCursorLifecycleAssets(CursorLifecycleHooks& hooks,const GameAssets& assets,const GumpText& text);
}
