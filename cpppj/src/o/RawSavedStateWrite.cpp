// 원본의 진단→열기→개별 쓰기/현재 전역 갱신→닫기와 부분 실패 효과를 유지한다.
#include "o/RawSavedStateWrite.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace { // 비정렬 BYTE 물리와 호스트 계약만 처리한다.
template<class F> const F& Need(const F& effect) { if (!effect) throw std::logic_error("저장상태 writer 실제 장치 미연결");return effect; } // 선택된 외부 효과만 요구한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) { // 현재 Player DWORD를 little endian으로 읽는다.
    if (offset+4>bytes.size()) throw std::out_of_range("저장상태 writer DWORD 물리 범위");std::uint32_t value{};
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[offset+index])<<(index*8);return value; // 원문의 정확 네BYTE만 읽는다.
}
void Copy(std::span<std::uint8_t> target,std::span<const std::uint8_t> source) { // 실제 한BYTE 복사/NUL 순서와 실패 전 부분 출력을 보존한다.
    for (std::size_t index=0;;++index) { // NUL을 포함해 복사하며 물리끝을 넘어 미정 메모리를 읽지 않는다.
        if (index>=source.size() || index>=target.size()) throw std::out_of_range("저장상태 writer 이름 NUL/물리 범위");const auto value=source[index];target[index]=value;if (!value) return;
    }
}
std::uint32_t Truncate(double value) { // 원본_ftol의 signed64 정수부/하위DWORD와 범위 밖 indefinite 하위0이다.
    return !std::isfinite(value) || value>=9223372036854775808.0 || value< -9223372036854775808.0 ? 0U : static_cast<std::uint32_t>(static_cast<std::int64_t>(value));
}
}
// 게임 정지와 별개인 실제 wall을 두 번 읽으며 저장 예외가 나면 새 예약을 작성하지 않는다.
void PollSavedStateWrite(const bool& battle,double& deadline,const std::function<double()>& wallSeconds,const std::function<void()>& save) {
    if (!battle) return;const auto now=Need(wallSeconds)();if (deadline<=now) { Need(save)();deadline=Need(wallSeconds)()+10.0; } // 보호 PE00501450의 double10.0이며 missed 주기를 여러 번 재생하지 않는다.
}
// 파일 생성 이전 관문을 유지하며 실제 열기가 정상 복귀한 뒤에만 해제 책임을 가진다.
void WriteSavedState(const SavedStateWriteGlobals& globals,const SavedStateWriteHooks& hooks) {
    if (Need(globals.missionPost)() || !Need(globals.playerKnown)()) return;
    Need(hooks.diagnose)();Need(hooks.open)(2,1);bool active=true;
    const auto close=[&] { active=false;Need(hooks.close)(); }; // 원문 SEH의 -1 설정처럼 예외 재진입의 중복 닫기를 막는다.
    const auto write=[&](std::span<const std::uint8_t> bytes) { Need(hooks.write)(bytes,-1); }; // 파일 쓰기 반환으로 새 성공/실패 분기를 만들지 않는다.
    const auto dword=[&](std::uint32_t value) { std::array<std::uint8_t,4> bytes{}; // 원문의 개별 DWORD 지역 값을 동기 기록한다.
        for (std::size_t index=0;index<4;++index) bytes[index]=static_cast<std::uint8_t>(value>>(index*8));write(bytes); // 네BYTE 뒤의 다음 필드/현재 전역은 아직 조회하지 않는다.
    };
    try {
        if (Need(globals.checking)()) {
            const auto name=Need(hooks.currentFort)(); // checking은 write할 이름의 나중 복사와 별도 현재 조회다.
            for (const auto byte:name) { if (!byte) break;if (byte=='.') { Need(hooks.assertion)("!(strchr(curFort,'.'))",0xf79);break; } } // 점의 첫 발견에서 진단하고 계속한다.
        }
        dword(0xa004e);std::array<std::uint8_t,256> name{};Copy(name,Need(hooks.currentFort)());write(name);
        const auto owner=Need(globals.localOwner)();globals.restoreOwner=owner;dword(owner);globals.restoreSubscriber=Need(globals.mySubscriber)();Copy(globals.previousHandle,Need(globals.handle)());
        dword(Truncate(Need(hooks.wallSeconds)()));dword(Truncate(Need(hooks.localSp)()));std::uint32_t serverCount{};
        // 현재 Player base를 매반복 읽어 그 기록의 실제 일곱 필드와 뒤 flags8을 순서대로 사용한다.
        for (std::uint32_t player=1;player<9;++player) {
            const auto records=Need(globals.players)();if (records.size()!=9*180) throw std::out_of_range("저장상태 writer 실제9 Player 누락");const auto row=records.subspan(player*180,180);
            dword(player);dword(Read(row,0x2c));write(row.subspan(12,30));write(row.subspan(0x30,30));dword(Read(row,0x50));dword(Read(row,0x58));write(row.subspan(0x6c,10));if (row[0x50]&8U) ++serverCount;
        }
        if (Need(globals.checking)() && serverCount>1) Need(hooks.assertion)("serverCount <= 1",0xfa3);
        for (const auto& field:globals.tail) dword(Need(field)()); // 같은 세 전역을 각 write 위치에서 현재 조회한다.
        dword(globals.loginEnd!=0.0 ? 1U:0U);close();
    } catch (...) { if (active) close();throw; } // 선택 예외에서 파일만 반납하며 Player/이름/진단의 부분 효과는 되돌리지 않는다.
}
}
