// 원문 제출/수신자 추가의 독립 전체 물리와 실제 소유자의 확장·반납을 대조한다.
#include "TestSupport.h"
#include "client/RawChatSubmit.h"
#include "client/ClientChatManagerGlobals.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 관측 입력 구성/원시 직렬화만 수행하며 기대 결과를 계산하지 않는다.
using namespace netstorm::client;
constexpr std::uint32_t kManager=0x12010000,kBuffer=0x12001000,kNew=0x12070000; // 격리 원문 관측의 실제 식별자다.
// 입력/장치 응답의 DWORD를 같은 실제 네 바이트에 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8)); // 하위BYTE부터 실제 DWORD를 구성한다.
}
// 이미 정의한 실제 DWORD만 읽는다.
std::uint32_t Word(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[at+index])<<(index*8);return value; // 실제 네 바이트 외에는 접근하지 않는다.
}
// 빈 열도 보존하는 원시 TSV/할당 목록 분리다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1; } // 원문 순서의 각 조각만 읽는다.
}
// 관측 hex를 유니코드 변환 없이 원시 바이트로 되돌린다.
std::vector<std::uint8_t> Bytes(std::string_view text) {
    std::vector<std::uint8_t> result;if (text=="-") return result;
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16))); // 두 hex문자를 한 BYTE로 읽는다.
    return result;
}
// 미작성 꼬리도 포함해 전체 물리를 같은 hex로 쓴다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; // 실제 BYTE 직렬화용 문자표다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; } // 어떤 바이트도 생략하지 않는다.
    return result.empty() ? "-" : result;
}
// 장치에 전달된 C문자열만 NUL 앞까지 빌린다.
std::string_view Text(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),0);if (end==bytes.end()) throw std::runtime_error("검사 장치의 NUL 없음");return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
}
// 현재 문자열을 원문 trace의 빈 문자열 허용 hex로 쓴다.
std::string TextHex(std::string_view text) { const auto result=Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()});return result=="-" ? "" : result; }
// 기록은 최초 원문 입력과 같은 세 장면만 구성한다.
std::array<std::uint8_t,4800> Records(unsigned scene) {
    std::array<std::uint8_t,4800> rows{};constexpr std::array<std::uint32_t,6> subscribers{11,22,11,0,33,44},flags{0x20,0x40,0x28,0xff,0x80,0xa0}; // 중복/빈 subscriber/부호 비트의 입력이다.
    for (std::size_t index=0;index<100;++index) { // scene0은빈행,1은6행,2는확장을요구하는100행이다.
        if (!scene || (scene==1 && index>=6)) continue;rows[index*48+40]=static_cast<std::uint8_t>(scene==2 ? 0x20U : flags[index]);Put(rows,index*48+44,scene==2 ? static_cast<std::uint32_t>(index+11) : subscribers[index]);
    }
    return rows;
}
// 원문 장치에서 선택된 중단을 정상 복귀와 구별한다.
struct Stop { std::string where; }; // 동일 장치 중단 이름이다.
}

// 실제 PE의 전체 manager/이전·새 배열/지역/호출 순서를 모든 고정 행과 대조한다.
TEST_CASE(ChatSubmit_IndependentNativeFullState) {
    std::ifstream file(NETSTORM_CHAT_SUBMIT_FIXTURE);if (!file) throw std::runtime_error("Chat 제출의 원문 관측 없음");std::string line;std::getline(file,line);std::size_t count{},stops{};
    while (std::getline(file,line)) { // 입력 열과 명시 초기 지역만 먼저 읽어 독립 실행을 구성한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto row=Split(line,'\t');if (row.size()!=16) throw std::runtime_error("Chat 제출 관측 열수 오류");
        const auto scene=static_cast<unsigned>(std::stoul(row[2])),mode=static_cast<unsigned>(std::stoul(row[3]));const auto initialCount=static_cast<std::uint32_t>(std::stoul(row[4])),capacity=static_cast<std::uint32_t>(std::stoul(row[5])),subscriber=static_cast<std::uint32_t>(std::stoul(row[6]));
        std::map<std::uint32_t,std::vector<std::uint8_t>> memory;auto& manager=memory[kManager];manager.resize(2712);
        for (std::size_t index=0;index<manager.size();++index) manager[index]=static_cast<std::uint8_t>(165+index*17); // 부모가 안 쓰는 꼬리까지 원문 명시 입력으로 정의한다.
        Put(manager,0,initialCount);Put(manager,4,capacity);Put(manager,8,kBuffer);Put(manager,12,7);Put(manager,16,0xaabbccdd);auto& old=memory[kBuffer];old.resize(capacity*4);
        for (std::uint32_t index=0;index<capacity;++index) Put(old,index*4,100+index); // 새 관측의 이전 capacity 전체 입력이다.
        auto records=Records(scene);auto input=Bytes(row[1]);std::string source(input.begin(),input.end());netstorm::o::PlayerIdentityRecord identity;identity.fill(0xa5);auto work=Bytes(row[13]);if (row[11]!="-") { const auto bytes=Bytes(row[11]);std::copy(bytes.begin(),bytes.end(),identity.begin()); }
        ChatSubmitHooks hooks;std::vector<std::string> trace;std::vector<std::uint32_t> allocations;std::uint32_t next=kNew;std::size_t resolves{};
        hooks.memory=[&](auto token)->std::span<std::uint8_t> { return memory.at(token); };
        hooks.allocate=[&](auto size) { trace.push_back("N:"+std::to_string(size));if (mode==6) return 0U;const auto token=next;next+=(size+15U)&~15U;memory[token].assign(size,0xd3);allocations.push_back(token);return token; };
        hooks.release=[&](auto token) { trace.push_back("F:"+std::to_string(token));if (mode==7) throw Stop{"free"}; }; // 관측용 옛 물리는 남기며 제품 소유자의 실제 반납은 별도 통합 검사한다.
        hooks.assertion=[&](auto number) { trace.push_back("A:"+std::to_string(number));throw Stop{"assertion"}; };
        hooks.resolve=[&](auto& record) { const auto name=std::string(Text(std::span(record).subspan(8)));trace.push_back("R:"+TextHex(name));++resolves;if (mode==5 && resolves==2) throw Stop{"resolve"};if (mode==3 && name=="Unknown") { constexpr std::string_view replacement="ALLIES";std::copy(replacement.begin(),replacement.end(),record.begin()+8);record[8+replacement.size()]=0; }const bool accepted=mode==2 || (mode!=1 && (name=="Alice" || name=="Bob"));if (accepted) Put(record,4,name=="Bob" ? 22U : 11U);return accepted; };
        hooks.translate=[&](auto key) { trace.push_back("T:"+TextHex(key));return std::string(mode==4 && key=="EVERYONE" ? "Tous" : key); };
        hooks.records=[&]()->std::span<const std::uint8_t> { return records; };
        hooks.inputVisible=[&](auto visible) { trace.push_back("V:"+std::to_string(visible)); };
        hooks.dispatch=[&](auto text) { trace.push_back("D:"+TextHex(Text(text))); };
        std::string status="normal";
        try { if (row[0]=="submit") SubmitChatInput(kManager,source,identity,work,hooks);else AddChatRecipient(kManager,subscriber,hooks); } catch (const Stop& stop) { status=stop.where;++stops; }
        CHECK(status==row[7]);CHECK(Hex(manager)==row[8]);CHECK(Hex(old)==row[9]);std::string buffers;
        for (const auto token:allocations) { if (!buffers.empty()) buffers+='|';buffers+=std::to_string(token)+":"+Hex(memory.at(token)); } // 실제 요청 순서/전체 새 할당을 보존한다.
        CHECK((buffers.empty() ? "-" : buffers)==row[10]);if (row[12]!="-") CHECK(Hex(identity)==row[12]);if (row[14]!="-") CHECK(Hex(work)==row[14]);std::string joined;
        for (const auto& event:trace) { if (!joined.empty()) joined+=';';joined+=event; } // 외부 장치의 선택 순서를 같은 한 열로 쓴다.
        CHECK((joined.empty() ? "-" : joined)==row[15]);++count;
    }
    CHECK(count==383 && stops>0);
}

// 실제 Client 소유자의100수신자 확장/이전 배열 반납/중복 재제출/전역 교체를 조립한다.
TEST_CASE(ChatSubmit_ActualOwnerGrowthAndRetainedManager) {
    auto owner=std::make_shared<ClientChatManagerGlobals>();auto prefix=MakeClientChatManagerHooks(owner);const auto manager=owner->Allocate(netstorm::o::kChatManagerBytes);CHECK(netstorm::o::ConstructChatManager(manager,prefix)==manager);owner->Current()=manager;
    const auto oldBuffer=Word(owner->Bytes(manager),8);auto hooks=MakeClientChatSubmitHooks(owner);auto records=Records(2);std::string message;std::size_t sends{};
    hooks.resolve=[](auto&) { return false; };hooks.translate=[](auto key) { return std::string(key); };hooks.records=[&]()->std::span<const std::uint8_t> { return records; };hooks.dispatch=[&](auto text) { message=Text(text);++sends; };
    netstorm::o::PlayerIdentityRecord identity;identity.fill(0xa5);std::array<std::uint8_t,kChatSubmitWorkBytes> work;work.fill(0xa5);
    SubmitChatInput(manager,"EVERYONE:hello",identity,work,hooks);CHECK(message=="hello" && sends==1);CHECK(Word(owner->Bytes(manager),0)==100 && Word(owner->Bytes(manager),4)==128);
    const auto expanded=Word(owner->Bytes(manager),8);CHECK(owner->Bytes(expanded).size()==512);bool released{};
    try { static_cast<void>(owner->Bytes(oldBuffer)); } catch (const std::out_of_range&) { released=true; }CHECK(released);
    for (std::uint32_t index=0;index<100;++index) CHECK(Word(owner->Bytes(expanded),index*4)==index+11); // 실제 기록의 같은 순서와 중복 제거 결과를 확인한다.
    const auto other=owner->Allocate(netstorm::o::kChatManagerBytes);owner->Current()=other;SubmitChatInput(manager,"EVERYONE:again",identity,work,hooks);CHECK(message=="again" && sends==2 && owner->Current()==other);CHECK(Word(owner->Bytes(manager),8)==expanded && Word(owner->Bytes(manager),0)==100);
    bool rejected{};try { owner->ReleaseRecipientBuffer(manager); } catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);
    std::weak_ptr<ClientChatManagerGlobals> retained=owner;prefix={};owner.reset();CHECK(!retained.expired() && hooks.memory(manager).data()!=nullptr);
}

// 도달한 분배/입력줄만 요구하고 비어 있는 본문도 먼저 현재 수신자 표를 갱신한다.
TEST_CASE(ChatSubmit_EmptyBodyAndLazyDevices) {
    auto owner=std::make_shared<ClientChatManagerGlobals>();auto prefix=MakeClientChatManagerHooks(owner);const auto manager=owner->Allocate(netstorm::o::kChatManagerBytes);netstorm::o::ConstructChatManager(manager,prefix);auto hooks=MakeClientChatSubmitHooks(owner);
    netstorm::o::PlayerIdentityRecord identity;identity.fill(0xa5);std::array<std::uint8_t,kChatSubmitWorkBytes> work;work.fill(0xa5);std::size_t hides{};
    hooks.inputVisible=[&](auto visible) { CHECK(visible==0);++hides; };SubmitChatInput(manager,":",identity,work,hooks);CHECK(hides==1 && Word(owner->Bytes(manager),12)==1 && Word(owner->Bytes(manager),16)==0);
    bool missing{};hooks.records=[]()->std::span<const std::uint8_t> { static std::array<std::uint8_t,4800> empty{};return empty; };
    try { SubmitChatInput(manager,"hello",identity,work,hooks); } catch (const std::logic_error&) { missing=true; }CHECK(missing && hides==1 && Word(owner->Bytes(manager),16)==0x20);
    owner->ReleaseRecipientBuffer(0);bool rejected{};try { static_cast<void>(owner->AllocateRecipientBuffer(3)); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
