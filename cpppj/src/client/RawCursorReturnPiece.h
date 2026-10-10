// Cursor.cpp 004468b0 / CD 0043e930: 사용한 생산 칸 조각을 다시 만든다.
#pragma once
#include "client/RawCursorDrop.h"
#include "o/RawContainedCreate.h"
#include "o/SpStore.h"
#include "o/SquidProcess.h"

namespace netstorm::client {
// cursorType 전역은 원본의 선택 타입 번호다. 유지 옵션 0이면 일반 조각에 자동 소멸을 붙인다.
struct CursorReturnPieceState {
    std::uint32_t type{},bridgeType{82},keepPieces{},fort{};
    std::uint8_t lifetimeIndex{}; // 패치 0052f7e0+2 / CD 00565b00+1. 일반 모드에서 0..2만 유효하다.
};
// 원본 operator_new(0x28)의 성공/실패와 Regular 생성 경계다. 실제 프로세스는 어댑터로 연결한다.
struct CursorReturnPieceHooks {
    std::function<bool(std::uint32_t)> allocateTimer;
    std::function<void(o::Sid,std::uint32_t,float)> createTimer;
};
// 원본 조각 반환 몸체와 실제 게임 난수/다리 패턴 선택/contained 생성을 결합한다.
class RawCursorReturnPiece {
public:
    // 상태·난수·contained 생성기는 반환 객체보다 오래 살아야 한다.
    RawCursorReturnPiece(o::RawContainedCreate& contained,const CursorReturnPieceState& state,
        o::GameRandom& random,CursorReturnPieceHooks hooks);
    // 플래그 bit 0x100을 재설정하고 일반 품질을 signed DWORD 감소·0 제한 후 BYTE로 저장한다.
    // 다리는 난수를 한 번 소비하고 패턴을 뽑으며 품질/타이머 옵션을 사용하지 않는다.
    void Return(std::uint32_t flags,std::uint32_t source,std::uint32_t quality) const;
    // 로컬 배치/GUI/타이머 어댑터의 동일 풀 계약을 검증한다.
    const o::SidPool& Pool() const;
private:
    o::RawContainedCreate& contained_;
    const CursorReturnPieceState& state_;
    o::GameRandom& random_;
    CursorReturnPieceHooks hooks_;
};
// 같은 SID 풀의 기존 RegularProcess 생성/Kernel 부착을 실제로 연결한다. 할당 실패 경계는 보존한다.
CursorReturnPieceHooks MakeCursorReturnPieceHooks(const o::SidPool& pool,o::SquidProcessHost& host,CursorReturnPieceHooks hooks);
// 로컬 건설 배치에서 생산 칸 반환을 실제 몸체로 연결한다. 나머지 효과는 유지한다.
o::ConstructionLocalHooks MakeReturnPieceHooks(const o::SidPool& pool,RawCursorReturnPiece& body,o::ConstructionLocalHooks hooks);
// 커서 GUI 수신 뒤 생산 칸 반환을 같은 몸체로 연결한다.
CursorDropHooks MakeReturnPieceHooks(const o::SidPool& pool,RawCursorReturnPiece& body,CursorDropHooks hooks);
}
