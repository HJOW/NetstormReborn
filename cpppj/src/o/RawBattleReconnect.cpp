// 원문 후보 순환/부분 효과를 보존하며 월드 교체는 다음 State7 부모가 담당한다.
#include "o/RawBattleReconnect.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace netstorm::o { // 원본32비트 상태와 호스트 문자열 범위를 명시적으로 연결한다.
namespace { // 비정렬 DWORD와 원문의 NUL 복사만 처리한다.
template<class F> const F& Need(const F& effect) { if (!effect) throw std::logic_error("재접속의 실제 장치 미연결");return effect; } // 선택된 효과만 요구한다.
std::span<const std::uint8_t> Row(const BattleReconnectGlobals& globals,std::uint32_t owner) { const auto rows=Need(globals.players)();if (rows.size()!=9*180 || owner>8) throw std::out_of_range("재접속 Player9 물리 범위");return rows.subspan(owner*180,180); } // 현재 base를 매조회 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> row,std::size_t offset) { std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=std::uint32_t(row[offset+index])<<(8*index);return value; } // 실제 네BYTE를 낮은 순서로 읽는다.
std::string_view Text(std::span<const std::uint8_t> bytes) { const auto end=std::find(bytes.begin(),bytes.end(),0);if (end==bytes.end()) throw std::out_of_range("재접속 이름/주소 NUL 누락");return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())}; } // 다음 필드의 바이트를 문자열로 읽지 않는다.
void Copy(std::span<std::uint8_t> target,std::span<const std::uint8_t> source) { for (std::size_t index=0;;++index) { if (index>=source.size() || index>=target.size()) throw std::out_of_range("재접속 이름 복사 물리 범위");target[index]=source[index];if (!source[index]) return; } } // NUL 포함/뒤 바이트 보존/실패 전 부분 쓰기다.
std::uint32_t Truncate(float value) { const auto wide=static_cast<double>(value);return !std::isfinite(wide) || wide>=9223372036854775808.0 || wide< -9223372036854775808.0 ? 0U : static_cast<std::uint32_t>(static_cast<std::int64_t>(wide)); } // x87_ftol signed64 하위DWORD와 indefinite 하위0이다.
}
bool ReconnectBattle(bool reloadFile,BattleReconnectGlobals g,const BattleReconnectHooks& h) {
    if (g.abortPending || Need(h.battleFinished)()) return false;g.scriptBreak=1;Need(h.closeAll)();Need(h.readSaved)();
    std::uint32_t serverCount{},serverOwner{}; // flags8 집계는 subscriber 유무와 독립이다.
    for (std::uint32_t owner=1;owner<9;++owner) { if (Read(Row(g,owner),0x50)&8U) { ++serverCount;serverOwner=owner; } } // 원문의 마지막 발견 owner를 보관한다.
    if (serverCount!=1 || !serverOwner) { g.sourceType=0;g.state.next[0]=3;return false; }
    std::uint32_t activeCount{},activeOwner{}; // flags1 집계 역시 미가입 행을 미리 제외하지 않는다.
    for (std::uint32_t owner=1;owner<9;++owner) { if (Read(Row(g,owner),0x50)&1U) { ++activeCount;activeOwner=owner; } } // 원문의 마지막 flags1 행이다.
    const auto takeover=[&] { g.state.current[0]=15;Need(h.takeServer)(); }; // next/immediate를 쓰지 않고 같은 현재15를 동기 실행한다.
    if (activeCount==1 && !reloadFile && Read(Row(g,activeOwner),0x2c)==g.mySubscriber) { takeover();return false; }
    std::uint32_t attempt=reloadFile ? 0U:1U,index=serverOwner+attempt-1U; // 자동7후보/복구8후보의 서로 다른 최초 위치다.
    for (;attempt<8;++attempt,++index) { // 원문 owner1..8을 한 바퀴 이내로 순환하며 실패 때만 다음 후보를 읽는다.
        const auto owner=index%8+1;const auto row=Row(g,owner);const auto flags=Read(row,0x50);if (!(flags&1U) || (flags&0x200U) || (reloadFile && owner==g.restoreOwner)) continue;
        if (Read(row,0x2c)==g.mySubscriber) { if (Need(h.checking)() && reloadFile) Need(h.assertion)("!reloadFile",0xf24);takeover();return false; }
        g.mySubscriber=g.restoreSubscriber;Copy(g.handle,g.previousHandle);Need(h.setHandle)(Text(g.handle));
        auto bytes=std::span(reinterpret_cast<std::uint8_t*>(g.state.next.data()),sizeof(g.state.next));const auto address=row.subspan(0x30,30);Copy(bytes.subspan(0x54,40),address);g.state.next[31]=Need(h.gamePort)();g.state.next[0]=7;g.state.next[33]=1;g.state.next[34]=Truncate(Need(h.localSp)());
        Need(h.log)("Server down, reconnecting to new server "+std::string(Text(address))+"\n");return true;
    }
    if (!reloadFile) { if (Need(h.serialAlive)(g.noRestartSerial)) return false;g.noRestartSerial=Need(h.tellNotRestart)(); }Need(h.deleteSaved)();return false; // 자동은 안내 신규 생성 때만, 수동은 후보 소진 때마다 삭제한다.
}
}
