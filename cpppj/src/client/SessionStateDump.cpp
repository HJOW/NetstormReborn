// 실제 Session/World 상태 덤프(검사 전용 관측). 형식은 docs/exe/cpp-session-state-dump-20261109.md를 따른다.
// 2026-11-09: webpj/dotnetpj의 L2·L3 대조용 cpppj Session 덤프 요청으로 추가했다. 읽기만 하며 게임 상태를 바꾸지 않는다.
#include "client/SessionStateDump.h"
#include "client/CampaignTestPlay.h" // 자동 플레이에서만 선택 진단 ai 행을 추가하고 일반 L2/L3 덤프 바이트를 보존한다.
#include "o/RawAiBrain.h" // 실제 AI 본문의 원본 크기와 관측 계약이다.
#include "client/ClientMain.h"
#include "client/ClientRandomGlobals.h"
#include "client/RawCampaignSession.h"
#include "client/RawSurfaceWorld.h"
#include "o/Kernel.h"
#include "o/RawGraph.h"
#include "o/SidPool.h"
#include "o/SpStore.h"
#include "o/SquidPostPop.h"
#include "o/SquidProcess.h"
#include <array> // 선택 AI 진단의 DWORD/QWORD 원문 위치 표다.
#include <bit>
#include <sstream>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 Player 한 기록의 크기다(9기록 1620BYTE). (2026-11-09 추가)
constexpr std::size_t kPlayerBytes=180;
// 바이트 배열을 소문자 16진 문자열로 쓴다. 빈 배열은 "-"다. (2026-11-09 추가)
void WriteHex(std::ostringstream& out,std::span<const std::uint8_t> bytes) {
    static constexpr char kDigits[]="0123456789abcdef"; // 한 nibble의 글자표다.
    if (bytes.empty()) { out<<'-';return; }
    // 각 바이트를 상위/하위 nibble 순으로 쓴다.
    for (const auto byte:bytes) out<<kDigits[byte>>4]<<kDigits[byte&15];
}
// 리틀 엔디언 원문에서 width 바이트(1/2/4)를 부호 없이 읽는다. (2026-11-09 추가)
std::uint32_t ReadLittle(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) {
    if (offset+width>raw.size()) throw std::out_of_range("Session 덤프 슬롯 필드 범위");
    std::uint32_t value{};
    // 낮은 바이트부터 차례로 합친다.
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(raw[offset+index])<<(8*index);
    return value;
}
// 판본 이름은 fixture 첫 열과 같은 originals/originalCD로 쓴다. (2026-11-09 추가)
const char* EditionName(o::OriginalEdition edition) { return edition==o::OriginalEdition::Patch1078 ? "originals" : "originalCD"; }
}

std::uint32_t DecodeScrambledSp(std::span<const std::uint32_t> pools,std::span<const std::int16_t> permutation,std::uint32_t offset,std::uint32_t owner) {
    constexpr auto bits=o::ScrambledSpStore::kBits,owners=o::ScrambledSpStore::kOwners; // 32개 배열과 9소유자다.
    if (owner>=owners || pools.size()!=bits*owners || permutation.size()!=bits) throw std::invalid_argument("Session 덤프 분산 SP 입력 크기");
    std::uint32_t value{};
    // k번째 비트는 배열 permutation[k]의 (offset+k)&31번째 비트에 있다(0040ec50).
    for (std::uint32_t k=0;k<bits;++k) {
        const auto index=permutation[k];if (index<0 || static_cast<std::size_t>(index)>=bits) throw std::out_of_range("Session 덤프 분산 SP 순열 범위");
        if (pools[static_cast<std::size_t>(index)*owners+owner]&(std::uint32_t{1}<<((offset+k)&31U))) value|=std::uint32_t{1}<<k;
    }
    return value;
}

std::uint64_t SessionDumpHash(std::span<const std::uint8_t> bytes) {
    std::uint64_t value=0xcbf29ce484222325ULL; // FNV-1a 64 시작값이다.
    // 바이트마다 XOR 뒤 FNV 소수를 곱한다.
    for (const auto byte:bytes) value=(value^byte)*0x100000001b3ULL;
    return value;
}

std::string FormatSessionStateDump(const SessionStateDumpInput& input) {
    const bool patch=input.edition==o::OriginalEdition::Patch1078; // 슬롯 필드 폭을 고른다.
    std::ostringstream out;
    out<<"netstorm_session_dump\t"<<kSessionStateDumpVersion<<'\n';
    out<<"edition\t"<<EditionName(input.edition)<<'\n';
    out<<"frame\t"<<input.frame<<'\t'<<std::bit_cast<std::uint64_t>(input.gameTime)<<'\n';
    out<<"random\t"<<input.gameRandom<<'\t'<<input.crtRandom<<'\n';
    out<<"world\t"<<(input.world ? 1 : 0)<<'\n';
    out<<"kernel\t"<<input.kernelSize<<'\t'<<input.kernelCapacity<<'\n';
    // Kernel 프로세스는 pid 순서 그대로 쓴다. 정기 프로세스는 사건 칸을 덧붙인다.
    for (const auto& process:input.processes) {
        out<<"process\t"<<process.pid<<'\t'<<process.form<<'\t'<<process.parent<<'\n';
        if (process.regular) out<<"regular\t"<<process.pid<<'\t'<<process.event<<'\t'<<process.count<<'\t'<<process.payloadBits<<'\t'<<process.timeBits<<'\n';
    }
    if (!input.world) { out<<"end\n";return out.str(); }
    if (!input.stride || input.poolBytes.size()!=static_cast<std::size_t>(input.poolCapacity)*input.stride) throw std::invalid_argument("Session 덤프 풀 원문 크기");
    out<<"pool\t"<<input.poolCapacity<<'\t'<<input.stride<<'\t'<<input.freeCount<<'\t'<<input.firstFreeServer<<'\t'<<input.firstFreeClient<<'\t'<<input.predictableCursor<<'\t';
    out<<std::hex<<SessionDumpHash(input.poolBytes)<<std::dec<<'\n';
    // 예약 0~4번과 free가 아닌 슬롯은 원문 전체를, 그중 자산 TYPE은 해석한 필드를 함께 쓴다. free 슬롯 잔여는 pool 해시로만 비교한다.
    for (std::uint32_t id=0;id<input.poolCapacity;++id) {
        const auto raw=input.poolBytes.subspan(static_cast<std::size_t>(id)*input.stride,input.stride);
        if (id>=5 && (raw[11]&1U)) continue;
        out<<"slot\t"<<id<<'\t';WriteHex(out,raw);out<<'\n';
        if (id<5 || raw[10]<o::kFirstAssetTypeNumber) continue;
        out<<"object\t"<<id<<'\t'<<unsigned(raw[10])<<'\t'<<unsigned(raw[11])<<'\t'<<unsigned(raw[patch ? 34 : 32])<<'\t'<<ReadLittle(raw,patch ? 36 : 34,patch ? 4 : 1)<<'\t'
            <<ReadLittle(raw,14,4)<<'\t'<<ReadLittle(raw,18,4)<<'\t'<<ReadLittle(raw,26,patch ? 4 : 2)<<'\n';
    }
    if (input.players.size()%kPlayerBytes) throw std::invalid_argument("Session 덤프 Player 원문 크기");
    // Player 9기록을 번호 순서로 쓴다.
    for (std::size_t index=0;index<input.players.size()/kPlayerBytes;++index) { out<<"player\t"<<index<<'\t';WriteHex(out,input.players.subspan(index*kPlayerBytes,kPlayerBytes));out<<'\n'; }
    out<<"hidden_players\t";WriteHex(out,input.hiddenPlayers);out<<'\n';
    out<<"alliances";
    // 방향별 9×9 DWORD를 행 우선 순서로 쓴다.
    for (const auto value:input.alliances) out<<'\t'<<value;
    out<<'\n';
    out<<"sp\t"<<(input.spInitialized ? 1 : 0)<<'\t'<<input.spOffset;
    // 소유자 0~8의 현재 SP float 비트다. 미초기화 저장소는 값을 읽지 않는다.
    for (std::uint32_t owner=0;owner<o::ScrambledSpStore::kOwners;++owner) {
        if (input.spInitialized) out<<'\t'<<DecodeScrambledSp(input.spPools,input.spPermutation,input.spOffset,owner);else out<<"\t-";
    }
    out<<'\n';
    // Graph 표의 모든 칸(예약 0번 포함)을 그대로 쓴다.
    for (std::size_t index=0;index<input.graph.size();++index) out<<"graph\t"<<index<<'\t'<<input.graph[index].surfaces<<'\t'<<input.graph[index].inUse<<'\t'<<input.graph[index].reserved<<'\n';
    out<<"totals\t"<<input.totalCost<<'\t'<<input.postPopDepth<<'\t'<<input.productionDirty<<'\n';
    if (input.totalLocal.size()!=input.totalBuilt.size() || input.totalLocal.size()!=input.totalGlobal.size()) throw std::invalid_argument("Session 덤프 TotalMade 표 크기");
    // 세 표 가운데 하나라도 0이 아닌 TYPE만 쓴다.
    for (std::size_t type=0;type<input.totalLocal.size();++type) {
        if (input.totalLocal[type] || input.totalBuilt[type] || input.totalGlobal[type]) out<<"totalmade\t"<<type<<'\t'<<input.totalLocal[type]<<'\t'<<input.totalBuilt[type]<<'\t'<<input.totalGlobal[type]<<'\n';
    }
    // 테스트 자동 플레이의 선택 관측만 덧붙인다. 기존 행과 일반 덤프의 판번호/바이트는 바꾸지 않는다.
    for (const auto& ai:input.ais) {
        if (!patch || ai.owner<1 || ai.owner>=9 || !ai.token || ai.body.size()!=o::RawAiBrainConstructor::kBytes ||
            input.players.size()!=9U*kPlayerBytes || ReadLittle(input.players,ai.owner*kPlayerBytes+0xb0,4)!=ai.token || ReadLittle(ai.body,0x260,4)!=ai.owner)
            throw std::invalid_argument("Session AI 관측의 실제 본문/Player 계약 오류");
        out<<"ai\t"<<ai.owner<<'\t'<<ai.token;
        // 원본 initialized/scan/stop/enemy/flags/wallet/goal/예산/수량/그래프/사제/신전/검색 상태를 지정 순서로 읽는다.
        constexpr std::array<std::size_t,24> fields{0x258,0x25c,0x254,0x264,0x248,0x2d8,0x2d4,0x290,0x234,0x2e4,0x298,0x2ac,0x29c,0x2a0,0x294,0x2a8,0x2cc,0x2d0,0x394,0x5a0,0x5a4,0x3cc,0x3d8,0x5f0};
        // DWORD는 원본 비트를 십진수로 내보낸다. signed 지갑과 목표 미정 float도 소비자가 같은 비트로 해석한다.
        for (const auto offset:fields) out<<'\t'<<ReadLittle(ai.body,offset,4);
        // 다음 행동의 실제 QWORD 기한을 double 비트 그대로 보존한다(전체 행동/생산/사제/검색/운반/건설/다리 재고).
        constexpr std::array<std::size_t,10> deadlines{0x5a8,0x5b0,0x5b8,0x5c0,0x5c8,0x5d0,0x5d8,0x5e0,0x5e8,0x630};
        // 두 DWORD를 낮은 쪽부터 합쳐 64비트 기한 한 칸으로 쓴다.
        for (const auto offset:deadlines) out<<'\t'<<(std::uint64_t(ReadLittle(ai.body,offset+4,4))<<32|ReadLittle(ai.body,offset,4));
        out<<'\n';
    }
    out<<"end\n";
    return out.str();
}

std::string DumpSessionState(Client& client,RawCampaignSession* session) {
    SessionStateDumpInput input;
    input.edition=client.Edition();input.frame=client.Time().number;input.gameTime=client.Time().game;
    const auto& random=client.RandomGlobals();if (!random) throw std::logic_error("Session 덤프의 실제 난수 소유자 없음");
    input.gameRandom=random->Game().State();input.crtRandom=random->CrtState();
    const auto& kernel=client.GetKernel();input.kernelSize=kernel.Size();input.kernelCapacity=kernel.Capacity();
    // 현재 Kernel 슬롯을 앞에서부터 읽기만 한다(RunFrame/Remove 없음).
    for (o::ProcessId pid=1;pid<=kernel.Capacity();++pid) {
        const auto* process=kernel.Get(pid);if (!process) continue;
        SessionDumpProcess row;row.pid=pid;
        if (const auto* squid=dynamic_cast<const o::SquidProcess*>(process)) { row.form=squid->Form().value;row.parent=squid->Parent().value; }
        if (const auto* regular=dynamic_cast<const o::RegularProcess*>(process)) {
            row.regular=true;row.event=regular->Event();row.count=regular->Count();row.payloadBits=std::bit_cast<std::uint32_t>(regular->Payload());row.timeBits=std::bit_cast<std::uint64_t>(regular->Time());
        }
        input.processes.push_back(row);
    }
    if (!session) return FormatSessionStateDump(input);
    input.world=true;
    const auto core=session->Core();const auto& pool=core.pool;
    input.edition=pool.Edition();input.poolBytes=pool.Bytes();input.poolCapacity=pool.Capacity();input.stride=pool.Layout().stride;
    input.freeCount=pool.FreeCount();input.firstFreeServer=pool.FirstFree(false).value;input.firstFreeClient=pool.FirstFree(true).value;input.predictableCursor=pool.PredictableCursor();
    input.players=session->PlayerRecords();input.hiddenPlayers=session->HiddenPlayers();input.alliances=session->Alliances();
    // 명령줄 테스트 AI가 활성일 때만 살아 있는 현재 본문을 읽고, 일반 상태 덤프의 L2/L3 기준은 유지한다.
    if (ConfiguredCampaignTestPlay().Active()) {
        // 실제 Player1..8의 현재 핸들 순서대로 관측한다. 본문이 없는 Player에 가짜 행을 만들지 않는다.
        for (std::uint32_t owner=1;owner<9;++owner) {
            const auto body=session->AiBrainBytes(owner); // 이 호출은 수명/크기를 확인할 뿐 AI를 실행하지 않는다.
            if (!body.empty()) input.ais.push_back({owner,ReadLittle(input.players,owner*kPlayerBytes+0xb0,4),body});
        }
    }

    const auto& sp=session->SpStore();input.spInitialized=sp.Initialized();input.spOffset=sp.Offset();input.spPools=sp.Pools();input.spPermutation=sp.Permutation();
    input.graph=core.graph.Records();
    auto& bookkeeping=core.bookkeeping; // TotalCost는 같은 장기 비용 DWORD의 쓰기 참조만 제공하므로 값만 복사해 읽는다.
    input.totalCost=bookkeeping.TotalCost();input.postPopDepth=bookkeeping.depth;input.productionDirty=bookkeeping.productionDirty;
    input.totalLocal=bookkeeping.LocalCounts();input.totalBuilt=bookkeeping.LocalSecondaryCounts();input.totalGlobal=bookkeeping.GlobalCounts();
    return FormatSessionStateDump(input);
}
}
