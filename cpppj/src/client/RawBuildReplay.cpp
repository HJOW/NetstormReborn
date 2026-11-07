// 정밀 gaps.tsv의4957a0/180BYTE를 PE 기계어와 대조해 원문 소비 순서와 float/double 폭을 보존한다.
#include "client/RawBuildReplay.h"
#include <algorithm>
#include <bit>
#include <cctype>
#include <stdexcept>

namespace netstorm::client { // 기존 파일/프로세스/건설 경계를 새 게임 규칙으로 대체하지 않는다.
namespace { // 고정 이벤트의 안전한 물리 접근과 원문 부분 scanf 쓰기만 소유한다.
// 도달한 하위의 누락은 해당 원문 부분 효과 뒤에 명시 오류로 구별한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("건설 기록 재생의 실제 하위 미연결");return function; }
// EOF/불완전 행도 이미 쓰인 이름/숫자 필드를 되돌리지 않는다. 원문 이름 영역을 넘기는 scanf는 호스트에서 거부한다.
int ScanEvent(FILE* file,BuildReplayEvent& event) {
    const auto first=std::fscanf(file,"%19s",event.name.data());if (first!=1) return first;
    const auto next=std::fgetc(file);if (next!=EOF) { std::ungetc(next,file);if (!std::isspace(static_cast<unsigned char>(next))) throw std::out_of_range("건설 재생 이름의 원문20BYTE 범위 초과"); }
    const auto rest=std::fscanf(file," (%f, %f) canon=%d rotation=%d delta=%f\n",&event.x,&event.y,&event.argument,&event.rotation,&event.delay);return rest>0 ? rest+1 : 1;
}
// 원문 C 문자열을 지정된 현재 BYTE부터 읽는다. 라벨은 prefix 검사 없이 두 BYTE를 생략한다.
std::string_view Name(const BuildReplayEvent& event,std::size_t offset=0) {
    const auto start=event.name.begin()+static_cast<std::ptrdiff_t>(offset);const auto end=std::find(start,event.name.end(),'\0');if (end==event.name.end()) throw std::out_of_range("건설 재생 이벤트 이름 NUL 누락");return {&*start,static_cast<std::size_t>(end-start)};
}
// count 뒤의 tail도 원문처럼 실제200행 물리 안에서 조회하며 호스트 밖의 인덱스만 거부한다.
const BuildReplayEvent& Current(const BuildReplayState& state) { if (state.current>=state.events.size()) throw std::out_of_range("건설 재생 현재 이벤트 물리 범위");return state.events[state.current]; }
// 비정렬 DWORD/WORD를 lowBYTE부터 기록하고 인접 품질/시각 자리는 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) {
    for (std::size_t index=0;index<width;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8)); // 선택된 원문 필드의 BYTE만 순서대로 쓴다.
}
}
void ReadBuildReplay(FILE* file,BuildReplayState& state,const BuildReplayHooks& hooks) {
    if (!file) throw std::invalid_argument("건설 기록 판독의 NULL FILE");std::uint32_t count{};
    for (;;) { // fscanf가6필드를 모두 읽은 행만 누적하며 다음 판독 전 원문200행 진단을 수행한다.
        if (count>=state.events.size()) {
            Need(hooks.assertion)("i < NUM_EVENTS",".\\Recorder.cpp",0x69);BuildReplayEvent overflow;
            if (ScanEvent(file,overflow)!=EOF) throw std::out_of_range("건설 기록의 원문200행 밖 부분 scanf 쓰기");break; // 진단 복귀 뒤 실제 EOF는200개를 유지하지만 배열 밖 쓰기는 합성하지 않는다.
        }
        if (ScanEvent(file,state.events[count])!=6) break;++count;
    }
    state.current=0;state.count=count;state.previousTime=Need(hooks.now)(); // Native1f70→1f74→1f68 쓰기 순서다.
}
void RunBuildReplayFrame(BuildReplayState& state,std::uint32_t& playing,std::span<std::uint8_t> packet,const BuildReplayHooks& hooks) {
    const auto index=state.current;
    if (std::bit_cast<std::int32_t>(index)>=std::bit_cast<std::int32_t>(state.count)) { playing=0;Need(hooks.kill)();return; } // 종료 플래그0은 실제 Kill보다 먼저다.
    const auto& event=Current(state);if (!(Need(hooks.now)()-state.previousTime>static_cast<double>(event.delay))) return; // equality/NaN은 원문 fcompp의C0/C3 관문에서 소비하지 않는다.
    state.previousTime=Need(hooks.now)();state.current=index+1U;
    if (!Need(hooks.validPoint)(event.x,event.y)) return; // 라벨도 시각/위치를 먼저 진행하지만 패킷 생성/TYPE 조회는 하지 않는다.
    if (packet.size()<25) throw std::out_of_range("건설 재생 송신의 현재25BYTE 부족");Need(hooks.constructPacket)(packet);
    const auto type=Need(hooks.findType)(Name(event));Put(packet,9,std::bit_cast<std::uint32_t>(event.y));packet[4]=static_cast<std::uint8_t>(type);Put(packet,5,std::bit_cast<std::uint32_t>(event.x));Put(packet,13,std::bit_cast<std::uint32_t>(event.argument),2);packet[15]=static_cast<std::uint8_t>(event.rotation); // PE의Y→타입→X→WORD canon→BYTE rotation 순서이며 quality16은 쓰지 않는다.
    Need(hooks.send)(9,packet.first(25)); // 한 프레임에서 추가 이벤트를 따라잡거나 로컬 예측 배치를 실행하지 않는다.
}
bool IsBuildReplayLabel(const BuildReplayState& state,const BuildReplayHooks& hooks) { const auto& event=Current(state);return !Need(hooks.validPoint)(event.x,event.y); }
void NotifyBuildReplayLabel(const BuildReplayState& state,std::uint32_t sid,const BuildReplayHooks& hooks) {
    if (!hooks.label) return;const auto& event=Current(state);const auto label=Need(hooks.validPoint)(event.x,event.y) ? std::string_view{} : Name(event,2);hooks.label(label,sid); // NULL callback은 현재 이벤트/좌표를 조회하지 않는다.
}
}
