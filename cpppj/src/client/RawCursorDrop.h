// 원본 Cursor.cpp 004473e0 / CD 0043d660: 커서 조각을 월드 또는 GUI 부모에 내려놓는다.
#pragma once
#include "client/Sound.h"
#include "o/RawConstructionLocal.h"

namespace netstorm::client {
// 원본 커서 전역의 논리 상태다. GUI/기록 핸들은 호스트 포인터가 아닌 호출자가 관리하는 식별값이다.
struct CursorDropState {
    std::uint32_t lockDepth{},legal{},type{},argument{},direction{},inputFlags{},costFlags{};
    o::Sid preview{}; // 조회 뒤에 캡처한 미리보기 번호는 prepare 효과가 전역 번호를 바꿔도 유지한다.
    float x{},y{};
    std::uint32_t legalGump{},cursorGump{},recording{},recorder{},battle{},blocked{},localPlayer{};
    std::int32_t screenX{},screenY{},cursorWidth{}; // SP 표시 위치: screenX - footX*16/2 - cursorWidth/2.
};
// 미복원 커서 판정·Form/Gump·공간 이동·기록·화면 효과만 명시 경계로 둔다. 모든 경계가 필요하다.
struct CursorDropHooks {
    std::function<void()> queryCursor,prepareCursor; // legal/위치 전역 갱신과 미리보기 위치 준비.
    std::function<void(std::uint32_t)> highlight; // 내려놓기 소리 전에 강조를 0으로 지운다.
    std::function<void()> playDrop; // droppiece.wav를 반복/음량/좌우/우선/한도 모두 0으로 재생.
    std::function<std::uint32_t(o::Sid)> previewSource; // Form 가상 +0. GUI 경로에서는 두 번 조회한다.
    std::function<void(o::Sid,std::uint32_t)> destroy; // 가상 +0x10, 삭제 플래그는 0.
    std::function<void(o::Sid,float,float,std::uint32_t)> pop; // 가상 +0x90: 기존 연관 조각을 좌표에 놓는다.
    std::function<void(std::uint32_t,std::string_view,float,float,std::uint32_t,std::uint32_t)> record;
    std::function<void(const o::ConstructionLocalRequest&)> placeLocal;
    std::function<void(float)> addLocalSp;
    std::function<void(std::int32_t,std::int32_t,std::int32_t,std::uint32_t)> showSp; // 절삭 비용·화면 x/y·0.
    std::function<void()> battleRefresh,invalidGui;
    std::function<std::uint32_t(std::uint32_t)> guiFlags; // Gump +0x28: 전체 DWORD 반환으로 겹침을 검사한다.
    std::function<std::uint32_t(std::uint32_t)> guiParent; // Gump +36 부모 SID. 0이면 원본 assert 계약 위반이다.
    std::function<void(o::Sid,std::uint32_t)> detach; // Form +0x48(0).
    std::function<void(o::Sid,std::uint32_t,std::uint32_t)> attach; // Form +0x50(부모,0x10).
    std::function<void(std::uint32_t,o::Sid)> guiReceive; // Gump +0x28: 내려놓은 미리보기 SID를 통지한다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> returnPiece; // 기존 WORD 플래그·두 번째 연관 번호·품질.
    std::function<void(std::uint32_t)> activatePreview,selectCursor; // 00445aa0 / CD 0043cf40 및 하드웨어 커서 선택.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> refresh;
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> closeGump; // Gump +0x1c(3,0,0).
};
// raw Form 필드와 현재 커서 전역을 사용한다. 실제 입력/GUI 루프 연결은 이 몸체와 별도다.
class RawCursorDrop {
public:
    // 자료와 상태는 객체보다 오래 살아야 한다. raw Form 품질은 패치 +30 / CD +26에서 읽는다.
    RawCursorDrop(o::SidPool& pool,std::span<const o::RiftTypeRecord> types,CursorDropState& state,CursorDropHooks hooks);
    // 잠금/불법 위치이면 조기 반환한다. 성공 시 잠금 증가·갱신·커서 GUI 닫기·미리보기 0·활성화 순서다.
    void Drop();
    // 실제 로컬 배치/보상 어댑터가 같은 풀인지 확인할 때 쓴다.
    const o::SidPool& Pool() const;
private:
    // 원본 floor(float좌표 + 0.5)의 double 중간값을 단정도로 저장한다. 비유한 좌표는 거부한다.
    static float Round(float value);
    // 타입 전역을 현재 시점에서 조회하며 자료 범위를 검증한다.
    const o::RiftTypeRecord& Type() const;
    // DWORD 연관 번호를 호스트 SID로 바꾼다. 0·풀 범위 밖 번호는 거부한다.
    o::Sid SourceSid(std::uint32_t value) const;
    // 잠금이 처음이면 미리보기/하드웨어 커서를 바꾸고, 현재 깊이를 DWORD 덧셈으로 증가시킨다.
    void Lock();
    // 월드 분기에서만 비용을 차감하고 절삭 비용을 화면 위치에 표시한다.
    void Charge();
    o::SidPool& pool_;
    std::span<const o::RiftTypeRecord> types_;
    CursorDropState& state_;
    CursorDropHooks hooks_;
};
// 같은 풀의 실제 로컬 배치·SP 보상·소리 재생을 연결한다. 나머지 Form/Gump 경계는 유지한다.
CursorDropHooks MakeCursorDropHooks(const o::SidPool& pool,o::RawConstructionLocal& local,o::SquidReward& reward,
    SoundPlayer& sound,const CursorDropState& state,CursorDropHooks hooks);
}
