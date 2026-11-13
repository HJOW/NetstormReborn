// 원문 CD00484140의 앞단만 연결하며 미복원된 일반 전투 종료를 성공/실패 상수로 대신하지 않는다.
#include "o/RawCdBattleCompletion.h"
#include "o/RawPlayerInitialization.h"
#include <array>
#include <stdexcept>

namespace netstorm::o { // 실제 Client 기록과 진단 장치를 빌리는 CD 전용 경계다.
namespace { // 네 SID 쌍의 원문 읽기 순서와 진단 문자열을 보관한다.
constexpr std::array<std::array<std::uint32_t,2>,4> kCdSidPairs{{{0x78,0x74},{0x7c,0x80},{0x88,0x84},{0x90,0x8c}}}; // CD0048415f..41cb의 MOV 뒤 CMP 메모리 읽기 순서다.
constexpr std::array<std::string_view,4> kCdExpressions{
    "p->firstTempleSid == p->debugFirstTempleSid","p->inProcessTempleSid == p->debugInProcessTempleSid",
    "p->firstAltarSid == p->debugFirstAltarSid","p->inProcessAltarSid == p->debugInProcessAltarSid"}; // CD0052e71c..0052e7cc의 실제 네 식이다.
std::uint32_t CdPlayerWord(std::span<const std::uint8_t> row,std::uint32_t offset) { // 원문의 EDI가 캡처한 같은 행에서 네 BYTE만 읽는다.
    if (row.size()!=kCdInitializedPlayerStride || offset>kCdInitializedPlayerStride-4) throw std::out_of_range("CD 후반 프레임의 캡처한 Player172BYTE 물리 범위");
    std::uint32_t value{};
    for (std::uint32_t byte=0;byte<4;++byte) value|=std::uint32_t(row[offset+byte])<<(byte*8); // 원문 little endian DWORD 읽기다.
    return value;
}
}
void RunCdBattleCompletionPrefix(const BattleCompletionGlobals& globals,const BattleCompletionFrameGlobals& frame,
    const BattleCompletionHooks& playerHooks,const BattleCompletionFrameHooks& frameHooks) {
    for (std::uint32_t owner=1;owner<kInitializedPlayerCount;++owner) { // CD 원문은 메뉴에서도1..8 기록을 순서대로 진단한다.
        const auto rows=playerHooks.currentPlayers ? std::span<const std::uint8_t>(playerHooks.currentPlayers()):globals.players; // CD00484157은 현재 base를 행마다 한 번 읽는다.
        if (rows.size()!=kCdInitializedPlayerBytes) throw std::out_of_range("CD 후반 프레임의 현재 Player1548BYTE 물리 범위");
        const auto row=rows.subspan(owner*kCdInitializedPlayerStride,kCdInitializedPlayerStride); // 이 행의 EDI는 진단 복귀 뒤에도 유지한다.
        for (std::size_t pair=0;pair<kCdSidPairs.size();++pair) { // 같은 행의 현재 BYTE는 다시 읽고 다음 행에서 새 base를 선택한다.
            const auto left=CdPlayerWord(row,kCdSidPairs[pair][0]);
            const auto right=CdPlayerWord(row,kCdSidPairs[pair][1]);
            if (left!=right) {
                if (!frameHooks.assertion) throw std::logic_error("CD 후반 프레임의 현재 SID 진단 장치 미연결");
                frameHooks.assertion(kCdExpressions[pair],"dais.cpp",0x5efU+static_cast<std::uint32_t>(pair));
            }
        }
    }
    if (!globals.battle || globals.missionPost) return;
    if (!playerHooks.serverConnection) throw std::logic_error("CD 후반 프레임의 현재 서버 객체 조회 미연결");
    if (!playerHooks.serverConnection() || frame.phase!=14) return;
    throw std::logic_error("CD 일반 전투의 활성 종료 판정 본문 미복원"); // 실제 종료·승패·MP 관문을 PATCH 본문이나 정상 반환으로 우회하지 않는다.
}
}
