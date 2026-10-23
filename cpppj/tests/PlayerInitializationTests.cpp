// 실제 Player 초기화236BYTE의 전체 물리/순서와 앞선 Core 선택 해제 수명을 검사한다.
#include "TestSupport.h"
#include "o/RawPlayerInitialization.h"
#include "o/RawPlayerParticipation.h"
#include "client/RawChatChannels.h"
#include "client/ClientCoreDataGlobals.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace netstorm::o;
using namespace netstorm::client;
constexpr std::array<std::uint32_t,3> kBases{0x12200000,0x12201000,0x12202000}; // Native와 같은 opaque32 세 Player 물리 식별자다.
constexpr std::uint32_t kNewFactoryBase=0x12310000; // 명시 할당 장치가 발급하는 공장 블록 토큰 기준이다.
// 원문 TSV/명시 입력의 마지막 빈 칸도 유지한다.
std::vector<std::string> InitFields(std::string_view text,char delimiter) {
    std::vector<std::string> result;std::size_t first{}; // 다음 칸의 시작 위치다.
    for (;;) { // 마지막 칸까지 원문 순서로 읽는다.
        const auto end=text.find(delimiter,first);result.emplace_back(text.substr(first,end==text.npos?text.npos:end-first));
        if (end==text.npos) return result;
        first=end+1;
    }
}
// Native 입력의 unsigned DWORD를 읽는다.
std::uint32_t InitNumber(std::string_view text) { return static_cast<std::uint32_t>(std::stoul(std::string(text))); }
// 입력과 출력 표시용으로만 DWORD를 little endian으로 쓴다.
void InitPut(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 지정한4BYTE만 쓴다.
}
// 같은 실제 BYTE를 원문 hex 형식으로 출력한다. 기대 상태를 계산하지 않는다.
std::string InitHex(std::span<const std::uint8_t> bytes) {
    constexpr char digits[]="0123456789abcdef"; // 최초 Native와 같은 소문자 표기다.
    std::string result;result.reserve(bytes.size()*2); // 전체 물리 출력 폭이다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; } // 한 BYTE를 두 글자로 기록한다.
    return result;
}
// 현재 입력 장치의 선택 중단과 실제 NULL 물리 오류를 구별한다.
struct InitStop {};
struct InitNull : std::out_of_range { InitNull():std::out_of_range("초기화 입력의 NULL 물리") {} }; // Native fault-null과 대응하는 명시 호스트 물리 실패다.
// 원문과 다른 부분만 짧게 표시해 대규모 물리 덤프를 불필요하게 반복하지 않는다.
void InitCompare(std::size_t row,const char* field,const std::string& actual,const std::string& expected) {
    if (actual!=expected) {
        const auto pair=std::mismatch(actual.begin(),actual.end(),expected.begin(),expected.end());const auto at=static_cast<std::size_t>(pair.first-actual.begin()); // 최초 차이의 문자 위치다.
        std::fprintf(stderr,"PlayerInit row%zu %s at%zu actual=%s expected=%s\n",row,field,at,actual.substr(at,128).c_str(),expected.substr(at,128).c_str());
    }
    CHECK(actual==expected);
}
// 할당 주소/외부 입력/실제 동맹·Chat 하위만 제공한다. Player 부모 계산은 제품에 맡긴다.
struct InitScene {
    std::array<std::vector<std::uint8_t>,3> blocks; // free 뒤 Native 관측과 같은 사후 비교를 위해 물리를 보존한다.
    std::array<std::uint32_t,81> allies{}; // 실제 방향별9×9 DWORD 표다.
    std::array<std::uint8_t,9> hidden{}; // 실제 숨김9BYTE 입력이다.
    std::array<std::uint8_t,4800> chat{}; // 실제 RawChatChannels가 직접 읽고 쓰는 한 저장소다.
    PlayerWorldState player; // 기존 동맹/실제 ClientCoreData가 같은 known/owner를 빌린다.
    std::uint32_t base{},fortCount{0x12345678U},lastPlayer{0x89abcdefU}; // Native의 실제 세 초기화 전역이다.
    std::uint32_t allocated{},nullNew{},my{},current{kBases[0]+180+0x6c},active{0xa5a5a5a5U}; // 명시 할당/두 현재Core 입력이며 부모가 Core를 임의 변경하지 않는다.
    std::array<std::uint32_t,5> calls{}; // freeFactory/freePlayers/malloc/new/alliance의 전체 호출 순서다.
    std::string stop,mutation; // 명시 callee 진입 중단과 현재 입력 변경이다.
    std::vector<std::string> trace,chatEvents; // 부모 주소 재조회와 실제 자식 진입을 구별한다.
    std::function<void(std::uint32_t)> onFreePlayers,onAssignedBase; // Core 수명 검사만 실제 반납/슬롯 쓰기 위치를 관찰한다.

    // 기대 결과 없이 Native와 같은 세 초기 물리/이름/포인터/표 패턴을 구성한다.
    explicit InitScene(std::string_view input) {
        const auto fields=InitFields(input,',');if (fields.size()!=10) throw std::runtime_error("Player 초기화 입력 칸 수");
        const auto seed=InitNumber(fields[0]),old=InitNumber(fields[1]),mask=InitNumber(fields[4]),hiddenMask=InitNumber(fields[8]); // 같은 명시 입력 값이다.
        allocated=InitNumber(fields[2]);player.playerKnown=InitNumber(fields[3]);nullNew=InitNumber(fields[5]);stop=fields[6];mutation=fields[7];my=InitNumber(fields[9]);
        if (old>3 || allocated>3) throw std::out_of_range("Player 초기화 입력 base 번호");base=old==3 ? 0 : kBases[old];
        for (std::size_t block=0;block<blocks.size();++block) { // 이전/새주소의 stale BYTE를 각각 독립 준비한다.
            auto& bytes=blocks[block];bytes.resize(1620);
            for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(seed+block*53+index*17); // Native 도구와 같은 독립 패턴이다.
            for (std::size_t owner=0;owner<9;++owner) { // 명시 공장 포인터/이름/subscriber만 입력으로 쓴다.
                const auto offset=owner*180;
                InitPut(bytes,offset,(mask&(1U<<owner)) ? static_cast<std::uint32_t>(0x12408000U+block*0x2000+owner*0x100) : 0);
                std::fill_n(bytes.begin()+offset+12,32,static_cast<std::uint8_t>(65+owner));
                constexpr std::string_view name{"OldName",8}; // 마지막 NUL을 포함한 원문 명시 이름이다.
                std::copy(name.begin(),name.end(),bytes.begin()+offset+12);InitPut(bytes,offset+44,static_cast<std::uint32_t>(700+owner+block*100));
            }
        }
        for (std::size_t index=0;index<allies.size();++index) { // 기존81DWORD의 BYTE 패턴은 동맹 기대 결과가 아니다.
            for (std::size_t byte=0;byte<4;++byte) allies[index]|=std::uint32_t(static_cast<std::uint8_t>(seed+(index*4+byte)*7))<<(byte*8); // Native의 같은 초기 BYTE를 조립한다.
        }
        for (std::size_t index=0;index<hidden.size();++index) hidden[index]=static_cast<std::uint8_t>((hiddenMask>>index)&1U); // 명시 숨김 입력이다.
        for (std::size_t index=0;index<chat.size();++index) chat[index]=static_cast<std::uint8_t>(index*37); // 기존 Chat 입력 장치의 stale BYTE다.
        for (std::size_t index=0;index<100;++index) { // 실제 빈 Subscriber와 NUL 뒤 stale 이름/flags를 준비한다.
            const auto offset=index*48;std::fill_n(chat.begin()+offset,40,static_cast<std::uint8_t>(65+index%26));
            constexpr std::string_view name{"OldName",8}; // 같은 독립 이름 입력이며 Chat 계산 결과가 아니다.
            std::copy(name.begin(),name.end(),chat.begin()+offset);chat[offset+40]=static_cast<std::uint8_t>(index*13+0x95);InitPut(chat,offset+44,0);
        }
    }
    // 현재 같은 물리 주소를 해석하며 NULL과 다른 물리 범위 위반을 구별한다.
    std::span<std::uint8_t> Memory(std::uint32_t address,std::uint32_t size) {
        if (address<1620) throw InitNull{};
        for (std::size_t index=0;index<blocks.size();++index) { // 현재 실제 세 할당에서 요청한 영역만 빌린다.
            if (address>=kBases[index] && std::uint64_t(address)+size<=std::uint64_t(kBases[index])+blocks[index].size())
                return std::span(blocks[index]).subspan(address-kBases[index],size);
        }
        throw std::out_of_range("Player 초기화의 미등록 실제 물리");
    }
    // 같은 현재 슬롯의 원문 load/store만 trace에 표시한다.
    PlayerInitializationGlobals Globals() {
        return {fortCount,lastPlayer,[this] { trace.push_back("R:"+std::to_string(base));return base; },
            [this](auto address) { trace.push_back("W:"+std::to_string(address));base=address;if (onAssignedBase) onAssignedBase(address); }};
    }
    // callee 진입 관측이며 원문에 없는 기본 성공이나 부모 결과를 계산하지 않는다.
    std::uint32_t Boundary(std::size_t kind,std::string_view name,std::uint32_t argument,std::uint32_t mask=0) {
        const auto count=++calls[kind];
        trace.push_back(kind==4 ? "A:"+std::to_string(argument)+':'+std::to_string(mask) : std::string(name)+':'+std::to_string(argument));
        if (stop==std::string(name)+std::to_string(count)) throw InitStop{};
        return count;
    }
    // 명시 외부 장치의 현재 base/필드 변경과 실제 부모 재진입이다.
    void Mutate(std::string_view boundary) {
        const auto dash=mutation.find('-');if (dash==mutation.npos || std::string_view(mutation).substr(0,dash)!=boundary) return;
        const auto action=mutation.substr(dash+1);trace.push_back("M:"+action);
        if (action=="baseB") base=kBases[1];
        else if (action=="baseNull") base=0;
        else if (action=="row64") InitPut(Memory(base+0x64,4),0,0x12345678U);
        else if (action=="known1") player.playerKnown=1;
        else if (action=="reenter") { trace.push_back("RE:begin");InitializePlayerRecords(Globals(),Hooks());trace.push_back("RE:end"); }
        else throw std::runtime_error("Player 초기화의 미해석 명시 변경");
    }
    // 실제 기존 방향별 동맹/Chat 몸체를 같은 현재 Player/표/기록에 연결한다.
    void Alliance(std::uint32_t owner,std::uint32_t mask) {
        const auto count=Boundary(4,"alliance",owner,mask);Mutate("alliance"+std::to_string(count));
        RawChatChannels channels(OriginalEdition::Patch1078,chat,[this] { return my; },[this](auto call,auto subscriber) {
            chatEvents.push_back(call==ChatChannelCall::NameChanged ? "N,"+std::to_string(subscriber) : "R");
        });
        PlayerParticipationState state;state.localOwner=player.battleOwner;state.playerKnown=player.playerKnown;PlayerParticipationHooks hooks; // 하위 계산은 기존 실제 모듈이 수행한다.
        hooks.clearChat=[&](auto channel) { channels.Clear(channel); };hooks.addChat=[&](auto subscriber,auto name,auto channel) { static_cast<void>(channels.AddToChannel(subscriber,name,channel)); };
        hooks.assertion=[](auto) { throw std::logic_error("Player 초기화 입력의 예상 밖 동맹 진단"); };
        RawPlayerParticipation body(OriginalEdition::Patch1078,Memory(base,1620),allies,hidden,state,std::move(hooks));body.SetAlliance(owner,mask);
    }
    // 실제 명시 allocator/free와 이미 검증한 동맹 자식을 반환한다.
    PlayerInitializationHooks Hooks() {
        PlayerInitializationHooks hooks;hooks.memory=[this](auto address,auto size) { return Memory(address,size); };
        hooks.freeFactory=[this](auto token) { const auto count=Boundary(0,"freeFactory",token);Mutate("freeFactory"+std::to_string(count)); };
        hooks.freePlayers=[this](auto token) { const auto count=Boundary(1,"freePlayers",token);if (onFreePlayers) onFreePlayers(token);Mutate("freePlayers"+std::to_string(count)); };
        hooks.mallocPlayers=[this](auto size) { CHECK(size==1620);const auto count=Boundary(2,"malloc",size);Mutate("malloc"+std::to_string(count));return allocated==3 ? 0U : kBases.at(allocated); };
        hooks.newFactory=[this](auto size) { CHECK(size==120);const auto count=Boundary(3,"new",size);Mutate("new"+std::to_string(count));return count==nullNew ? 0U : kNewFactoryBase+(count-1)*0x100; };
        hooks.alliance=[this](auto owner,auto mask) { Alliance(owner,mask); };return hooks;
    }
    // 정상/외부 중단/NULL 물리 실패만 원문과 같은 이름으로 분류한다.
    std::string Run() {
        try { InitializePlayerRecords(Globals(),Hooks());return "normal"; }catch (const InitStop&) { return "stop"; }catch (const InitNull&) { return "fault-null"; }
    }
    // 원문 출력과 같은 unsigned DWORD 순서다.
    std::string GlobalsText() const {
        return std::to_string(base)+','+std::to_string(fortCount)+','+std::to_string(lastPlayer)+','+std::to_string(player.playerKnown)+','+std::to_string(player.battleOwner)+','+std::to_string(my)+','+std::to_string(current)+','+std::to_string(active);
    }
    // 같은 세 Player 물리 전체를 출력한다.
    std::string PlayersHex() const { return InitHex(blocks[0])+InitHex(blocks[1])+InitHex(blocks[2]); }
    // 실제81DWORD를 원문 little endian BYTE로 표시한다.
    std::string AlliesHex() const {
        std::array<std::uint8_t,324> bytes{};
        for (std::size_t index=0;index<allies.size();++index) InitPut(bytes,index*4,allies[index]); // 표시만 수행하며 동맹 기대를 계산하지 않는다.
        return InitHex(bytes);
    }
    // 같은 이벤트/효과 trace를 원문 구분자로 표시한다.
    static std::string Join(const std::vector<std::string>& values) {
        std::string result;
        for (const auto& value:values) { if (!result.empty()) result+=';';result+=value; } // 순서를 바꾸거나 중복 호출을 합치지 않는다.
        return result.empty() ? "-" : result;
    }
};
}

// 제품 전에 고정한 전체 정상/중단/NULL/reentry와 실제 동맹·Chat 결과를 각각 대조한다.
TEST_CASE(PlayerInitialization_Replays202IndependentNativeObservations) {
    std::ifstream input(NETSTORM_PLAYER_INITIALIZATION_FIXTURE);if (!input) throw std::runtime_error("Player 초기화 최초 fixture 없음");
    std::string line;std::size_t rows{},normal{},fault{},stopped{};std::set<std::string> inputs; // 독립 입력과 관측 결과를 별도로 센다.
    while (std::getline(input,line)) { // 헤더를 제외한 모든 최초 행을 비교한다.
        if (line.starts_with("control\t")) continue;if (!line.empty() && line.back()=='\r') line.pop_back();
        const auto fields=InitFields(line,'\t');if (fields.size()!=10) throw std::runtime_error("Player 초기화 원문 칼럼 수");InitScene scene(fields[1]);
        InitCompare(rows,"status",scene.Run(),fields[2]);InitCompare(rows,"globals",scene.GlobalsText(),fields[3]);InitCompare(rows,"players",scene.PlayersHex(),fields[4]);
        InitCompare(rows,"allies",scene.AlliesHex(),fields[5]);InitCompare(rows,"chat",InitHex(scene.chat),fields[6]);InitCompare(rows,"chatEvents",InitScene::Join(scene.chatEvents),fields[7]);InitCompare(rows,"trace",InitScene::Join(scene.trace),fields[8]);
        CHECK((fields[0]=="027f" || fields[0]=="037f") && InitNumber(fields[9])>0);
        if (fields[2]=="normal") ++normal;else if (fields[2]=="fault-null") ++fault;else { CHECK(fields[2]=="stop");++stopped; }
        inputs.insert(fields[1]);++rows;
    }
    CHECK(rows==202 && inputs.size()==101 && normal==174 && fault==8 && stopped==20);
}

// 선택하지 않은 free는 요구하지 않으며 선택 누락/짧은 DWORD는 앞선 실제 효과를 남긴다.
TEST_CASE(PlayerInitialization_RequiredDevicesKeepEarlierPartialEffects) {
    InitScene missing("165,3,2,0,511,0,none,none,0,4294967295");auto hooks=missing.Hooks();hooks.freeFactory={};hooks.freePlayers={};hooks.mallocPlayers={};bool rejected{};
    try { InitializePlayerRecords(missing.Globals(),hooks); }catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && missing.fortCount==0 && missing.lastPlayer==0 && missing.base==0 && missing.calls[0]==0 && missing.calls[1]==0 && missing.calls[3]==0);
    InitScene shortStore("165,3,2,0,511,0,none,none,0,4294967295");const auto before=shortStore.blocks[2];auto shortHooks=shortStore.Hooks();
    shortHooks.memory=[&](auto address,auto size) { return shortStore.Memory(address,size).first(3); };rejected=false;
    try { InitializePlayerRecords(shortStore.Globals(),shortHooks); }catch (const std::out_of_range&) { rejected=true; }
    CHECK(rejected && shortStore.base==kBases[2] && shortStore.fortCount==0 && shortStore.lastPlayer==0 && shortStore.calls[2]==1 && shortStore.calls[3]==0 && shortStore.blocks[2]==before);
}

// 실제 ClientCoreData의4·4·2 cache/NULL을 이전Player 해제보다 먼저 끝내고 새base에서만 재선택한다.
TEST_CASE(PlayerInitialization_CoreDetachCacheBeforeReleaseAndNewAliasAfterAllocation) {
    InitScene scene("165,0,2,0,511,0,none,none,0,4294967295");ClientCoreDataGlobals core;std::uint32_t metamap{}; // Core 장기 소유자와 실제 Player 물리는 각각 한 개다.
    core.Players()=scene.Memory(scene.base,1620);core.Select(scene.player,metamap,1,1);core.Active()=0x12345678U;
    const std::vector<std::uint8_t> previous(core.Current().begin(),core.Current().end());scene.player.playerKnown=0;core.Select(scene.player,metamap,0,1); // 실제 정리 부모의 앞선 known0→Select0 순서다.
    std::size_t released{};
    scene.onFreePlayers=[&](auto token) { ++released;CHECK(token==kBases[0] && core.Current().data()==nullptr && core.Active()==0 && std::equal(previous.begin(),previous.end(),core.Cached().begin())); };
    scene.onAssignedBase=[&](auto address) { core.Players()=address ? scene.Memory(address,1620) : std::span<std::uint8_t>{}; }; // 같은 실제 base 쓰기를 Core의 현재 물리뷰에도 직접 제공한다.
    CHECK(scene.Run()=="normal" && released==1 && scene.base==kBases[2]);
    CHECK(core.Current().data()==nullptr && core.Active()==0 && scene.player.playerKnown==0 && scene.player.battleOwner==0);
    CHECK(std::equal(previous.begin(),previous.end(),core.Cached().begin()));
    core.Select(scene.player,metamap,1,1);CHECK(core.Current().data()==scene.blocks[2].data()+180+0x6c && scene.player.playerKnown==1);
    CHECK(std::all_of(core.Current().begin(),core.Current().end(),[](auto byte) { return byte==0; }) && std::equal(previous.begin(),previous.end(),core.Cached().begin()));
}
