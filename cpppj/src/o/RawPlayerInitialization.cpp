// 전역 base 재조회와 new 직전에 캡처한 행 주소를 구별하며 앞선 부분효과를 되돌리지 않는다.
#include "o/RawPlayerInitialization.h"
#include <stdexcept>

namespace netstorm::o {
namespace {
// 도달한 실제 장치만 필요로 하며 선택하지 않은 free/동맹 경계를 미리 실행하지 않는다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("Player 전체 초기화의 선택된 실제 장치 미연결");return function;
}
// 요청한4BYTE 전체를 확인한 뒤 원문 DWORD를 읽는다. 부분 load를 새 값으로 합치지 않는다.
std::uint32_t Read(const PlayerInitializationHooks& hooks,std::uint32_t address) {
    const auto bytes=Need(hooks.memory)(address,4);if (bytes.size()!=4) throw std::out_of_range("Player 초기화의 실제 DWORD load 물리 부족");
    std::uint32_t value{}; // 원문 레지스터에 캡처할 정확한 DWORD다.
    for (std::uint32_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[byte])<<(byte*8); // 원문 little endian 네 BYTE만 읽는다.
    return value;
}
// 단일DWORD store의 호스트 물리를 먼저 확인하며 이전 store의 결과는 그대로 보존한다.
void Write(const PlayerInitializationHooks& hooks,std::uint32_t address,std::uint32_t value) {
    const auto bytes=Need(hooks.memory)(address,4);if (bytes.size()!=4) throw std::out_of_range("Player 초기화의 실제 DWORD store 물리 부족");
    for (std::uint32_t byte=0;byte<4;++byte) bytes[byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 현재 물리 네 BYTE만 쓴다.
}
}
// 이전 공장9개→이전Player→NULL→malloc/405DWORD0→각행/new/동맹 순서를 그대로 유지한다.
void InitializePlayerRecords(const PlayerInitializationGlobals& globals,const PlayerInitializationHooks& hooks) {
    const auto previous=Need(globals.playerBase)(); //4910a0은 두 카운터 쓰기보다 먼저 첫 base/NULL 관문을 캡처한다.
    globals.fortCount=0;globals.lastLoginPlayer=0;
    if (previous!=0) {
        for (std::uint32_t owner=0;owner<kInitializedPlayerCount;++owner) { //free 복귀 뒤 다음 행은 현재 전역base를 다시 읽는다.
            const auto token=Read(hooks,Need(globals.playerBase)()+owner*kInitializedPlayerStride);
            Need(hooks.freeFactory)(token); //원문은 NULL factory도 CRT free 호출을 생략하지 않는다.
        }
        Need(hooks.freePlayers)(Need(globals.playerBase)());
        Need(globals.writePlayerBase)(0); //free callback의 base 변경이 있어도 원문이 이 위치에서NULL을 다시 쓴다.
    }
    const auto allocated=Need(hooks.mallocPlayers)(kInitializedPlayerBytes); //같은 malloc 반환을 REP의 EDI와 전역에 각각 사용한다.
    Need(globals.writePlayerBase)(allocated);
    for (std::uint32_t word=0;word<kInitializedPlayerBytes/4;++word) Write(hooks,allocated+word*4,0); //REP의405DWORD는 캡처한 반환 주소를 순서대로 쓴다.
    for (std::uint32_t owner=0;owner<kInitializedPlayerCount;++owner) { // 각 행의 원문 전역 load 위치와 순서를 보존한다.
        const auto offset=owner*kInitializedPlayerStride; // ESI의 현재 행 상대 위치다.
        Write(hooks,Need(globals.playerBase)()+offset+0x64,0);
        Write(hooks,Need(globals.playerBase)()+offset+0x58,0);
        const auto record=Need(globals.playerBase)()+offset; // new 호출 전에 EDI에 캡처한 실제 행 주소다.
        const auto factory=Need(hooks.newFactory)(kInitializedFactoryBytes); //NULL 반환도 부모 자체는 진단/추가 초기화하지 않는다.
        Write(hooks,record,factory);Write(hooks,record+4,30);Write(hooks,record+8,0);
        Write(hooks,record+0x2c,0);Write(hooks,record+0x78,127);
        const auto mask=1U<<owner; // 원문 owner0..8의 자기 동맹 bit다.
        Write(hooks,Need(globals.playerBase)()+offset+0x54,mask);
        const auto currentMask=Read(hooks,Need(globals.playerBase)()+offset+0x54); // 동맹 인자도 new 전 캡처한 record가 아니라 현재base에서 읽는다.
        Need(hooks.alliance)(owner,currentMask);
    }
}
}
