// 시간 동기화 Reset의 최초 원문 전체 BYTE와 실제 참조·인접 카운터 보존을 검사한다.
#include "TestSupport.h"
#include "o/RawClockSyncReset.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {
using namespace netstorm::o;
// 빈 칸을 포함한 최초 TSV의 칼럼을 그대로 분리한다.
std::vector<std::string> SyncFields(std::string_view line) {
    std::vector<std::string> fields; // 칼럼 순서를 보존하는 지역 파서 결과다.
    std::size_t first{}; // 아직 소비하지 않은 칼럼의 시작이다.
    for (;;) { // 마지막 칼럼까지 한 번씩만 읽고 반환한다.
        const auto next=line.find('\t',first); // 다음 구분자이며 기대값 계산에는 사용하지 않는다.
        fields.emplace_back(line.substr(first,next==line.npos?line.npos:next-first));
        if (next==line.npos) return fields;
        first=next+1;
    }
}
// 독립 원문 hex 96BYTE를 실제 24DWORD 저장소에 little endian으로 구성한다.
std::array<std::uint32_t,24> SyncWords(std::string_view hex) {
    if (hex.size()!=192) throw std::logic_error("ClockSyncReset 원시96BYTE 길이");
    std::array<std::uint32_t,24> result{}; // 호스트 객체 배치나 비정렬 포인터 캐스트를 사용하지 않는다.
    for (std::size_t byte=0;byte<96;++byte) { // 입력의 모든 BYTE를 한 번씩 원문 순서로 읽는다.
        const auto value=static_cast<std::uint32_t>(std::stoul(std::string(hex.substr(byte*2,2)),nullptr,16)); // 한 BYTE의 고정 원시 값이다.
        result[byte/4]|=value<<((byte%4)*8);
    }
    return result;
}
// 실제 DWORD 배열 전체를 원문과 같은 little endian hex로 비교한다.
std::string SyncHex(const std::array<std::uint32_t,24>& words) {
    constexpr std::string_view digits="0123456789abcdef"; // 원시 hex 표기이며 십진 반올림을 사용하지 않는다.
    std::string result; // 인접 미작성 BYTE도 포함하는 96BYTE 출력이다.
    for (const auto word:words) { // 원시 DWORD 순서를 보존한다.
        for (unsigned byte=0;byte<4;++byte) { // DWORD의 하위 BYTE부터 출력한다.
            const auto value=(word>>(byte*8))&255U; // 현재 출력 BYTE다.
            result+=digits[value>>4];result+=digits[value&15U];
        }
    }
    return result;
}
}

// 제품 전 고정한 두 정밀도·반복 전체64관측과 같은 실제 저장소를 대조한다.
TEST_CASE(ClockSyncReset_ReplaysIndependentPe64) {
    static_assert(std::is_same_v<decltype(&ResetClockSynchronization),void(*)(const ClockSyncResetGlobals&)>); // 현재 시각·난수·복제된 DWORD 인자를 추가하지 않는다.
#ifdef NETSTORM_CLOCK_SYNC_RESET_FIXTURE
    const std::filesystem::path path=NETSTORM_CLOCK_SYNC_RESET_FIXTURE; // 공용 등록은 최초 TSV의 절대 경로를 제공한다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/clock-sync-reset-x86.tsv"; // 독립 빌드도 같은 최초 자료를 읽는다.
#endif
    std::ifstream file(path); // 최초 Native 기대값을 읽는 실제 파일이다.
    if (!file) throw std::runtime_error("ClockSyncReset 최초 fixture 없음");
    std::string line,key; // 현재 원시 행과 같은 반복열의 식별자다.
    std::array<std::uint32_t,24> sync{},clock{}; // 원문의 서로 다른 실제96BYTE 두 영역이다.
    std::size_t rows{},first{},repeated{}; // 최초/반복 관측의 누락을 구별한다.
    while (std::getline(file,line)) { // 최초64행 전체를 순서대로 대조한다.
        if (line.empty() || line[0]=='#') continue;
        if (line.back()=='\r') line.pop_back();
        const auto fields=SyncFields(line); // 원시 입력과 기대를 같은 parser가 재계산하지 않는다.
        if (fields.size()!=13) throw std::logic_error("ClockSyncReset TSV 칼럼 수");
        const auto current=fields[0]+':'+fields[1]; // 제어어·입력별 반복 두 번의 동일 저장소를 선택한다.
        if (current!=key) { key=current;sync=SyncWords(fields[6]);clock=SyncWords(fields[7]);CHECK(fields[2]=="0"); }
        if (fields[2]=="0") ++first;else { CHECK(fields[2]=="1");++repeated; }
        ResetClockSynchronization({sync[5],sync[8],clock[3]});
        CHECK(SyncHex(sync)==fields[8] && SyncHex(clock)==fields[9]);
        CHECK(fields[10]=="efcdab8900ceab8911ceab8922ceab89" && fields[11]=="005c8bf4:4:00000000;005c8c00:4:00000000;0055b4ac:4:00000000" && fields[12]=="5");
        ++rows;
    }
    CHECK(rows==64 && first==32 && repeated==32);
}

// 실제 세 참조를 재사용하고 인접 카운터·별도 시간·기록을 보존하며 바인딩 자체는 쓰지 않는다.
TEST_CASE(ClockSyncReset_BorrowsCurrentFieldsAndPreservesNeighbours) {
    std::array<std::uint32_t,12> words{11,12,13,14,15,16,17,18,19,20,21,22}; // 테스트 owner의 실제 고정 저장소다.
    const auto before=words; // 바인딩 전의 비영 값과 인접 필드를 보존한다.
    ClockSyncResetGlobals first{words[1],words[5],words[9]}; // 실제 세 DWORD만 빌리고 초기화는 수행하지 않는다.
    ClockSyncResetGlobals next=first; // 후속 소비자도 같은 참조를 빌리며 값을 복제하지 않는다.
    CHECK(words==before && &next.connection==&words[1] && &next.replyMode==&words[5] && &next.gameTicks==&words[9]);
    ResetClockSynchronization(first);
    for (std::size_t word=0;word<words.size();++word) CHECK(words[word]==(word==1 || word==5 || word==9?0:before[word])); // 원문 밖 카운터/시간/기록에 대한 추가 0쓰기를 검출한다.
    words[1]=0xffffffffU;words[5]=0x80000000U;words[9]=0x12345678U;words[2]=99; // 같은 owner의 현재 값이 바뀌어도 이전 참조를 재사용한다.
    ResetClockSynchronization(next);
    CHECK(words[1]==0 && words[5]==0 && words[9]==0 && words[2]==99 && words[3]==before[3] && words[10]==before[10]);
}
