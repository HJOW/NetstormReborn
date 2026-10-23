// 원문 직접 쓰기·현재값 재조회·호출 중단의 앞선 효과를 보존한다. 전체 Chat 전송/창 생성과 구별한다.
#include "o/RawChatManagerPrefix.h"
#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 도달한 실제 장치만 요구하며 다른 원문 분기 장치를 선행 검사하지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("Chatmanager 실제 장치 누락");return function; }
// 원문 한 store의 전체 폭을 확인한 뒤 little endian 필드만 수정한다.
void Write(std::span<std::uint8_t> memory,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    if (offset>memory.size() || width>memory.size()-offset) throw std::out_of_range("Chatmanager store 물리 범위");
    for (std::size_t byte=0;byte<width;++byte) memory[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); //인접 미작성 BYTE는 보존한다.
}
// 각 사용 위치에서 토큰의 같은 실제 할당을 빌리며 순수 memory 조회는 주소/전역을 바꾸지 않아야 한다.
std::span<std::uint8_t> Memory(const ChatManagerPrefixHooks& hooks,std::uint32_t token) { return Need(hooks.memory)(token); }
// 원문 지역 C문자열만 읽으며 source의 첫 NUL 뒤 문자는 서식/복사에 사용하지 않는다.
std::string_view CString(std::string_view source) { return source.substr(0,source.find('\0')); }
// 같은 현재 subscriber의 signed%d와 고정 suffix를 지역 문자열로 작성한다.
std::string Key(std::uint32_t subscriber,std::string_view suffix) { return std::to_string(std::bit_cast<std::int32_t>(subscriber))+std::string(suffix); }
}

// 실제 CRT의 고정 폭 복사와 NUL padding을 같은 대상 물리에 적용한다.
void CopyChatManagerFormat(std::span<std::uint8_t> destination,std::string_view source,std::uint32_t count) {
    if (count>destination.size()) throw std::out_of_range("Chatmanager strncpy 물리 범위");source=CString(source);
    for (std::size_t index=0;index<count;++index) destination[index]=index<source.size() ? static_cast<std::uint8_t>(source[index]) : std::uint8_t{}; //원문 strncpy의 NUL padding까지 실제 작성한다.
}
// 실제 메모리를 빌리는 누락 CRT 경계만 채우고 사용자 callback을 그대로 보존한다.
ChatManagerPrefixHooks BindChatManagerMemoryCopy(ChatManagerPrefixHooks hooks) {
    if (!hooks.copyFormat) hooks.copyFormat=[memory=hooks.memory](auto token,auto offset,auto source,auto count) {
        auto bytes=Need(memory)(token);if (offset>bytes.size()) throw std::out_of_range("Chatmanager CRT target 물리 범위");CopyChatManagerFormat(bytes.subspan(offset),source,count);
    }; //호출자는 memory를 소유한 실제 할당 장치보다 오래 이 hook를 보관하지 않는다.
    return hooks;
}
// 원문 고정 그림 서식을 현재 C문자열 끝에 NUL 포함 append한다.
void AppendChatRankIcon(std::span<std::uint8_t> destination,char kind,std::int32_t value) {
    const auto end=std::find(destination.begin(),destination.end(),std::uint8_t{});if (end==destination.end()) throw std::out_of_range("Chat 랭크 append의 NUL 없음");
    const auto text=std::string("~[Iicon.")+kind+std::to_string(value)+']';const auto offset=static_cast<std::size_t>(end-destination.begin()); //고정 서식의 signed 십진수다.
    if (text.size()+1>destination.size()-offset) throw std::out_of_range("Chat 랭크 append 물리 범위");
    for (std::size_t index=0;index<text.size();++index) destination[offset+index]=static_cast<std::uint8_t>(text[index]); //기존 문자열 뒤에만 작성한다.
    destination[offset+text.size()]=0;
}
// 생성자0042e000의 sparse 쓰기만 수행하고 별도 내부 할당은 초기화하지 않는다.
std::uint32_t ConstructChatManager(std::uint32_t object,const ChatManagerPrefixHooks& hooks) {
    const auto internal=Need(hooks.allocate)(kChatManagerBufferBytes);Write(Memory(hooks,object),8,internal);
    if (!internal) Need(hooks.assertion)(0xdc);
    Write(Memory(hooks,object),0,0);Write(Memory(hooks,object),0xc,0);
    Write(Memory(hooks,object),0x14,0,1);Write(Memory(hooks,object),0x50,0,1);Write(Memory(hooks,object),kChatManagerRankPrefixOffset,0,1);Write(Memory(hooks,object),4,0x20);
    return object;
}
// 초기화0042e050은 현재 manager/Subscriber를 하위 복귀 뒤 원문 위치에서 재조회한다.
void InitializeChatManager(ChatManagerPrefixGlobals globals,const ChatManagerPrefixHooks& hooks) {
    const auto object=Need(hooks.allocate)(kChatManagerBytes);globals.current=object ? ConstructChatManager(object,hooks) : 0U;
    Need(hooks.registerCommand)("format",0x42ddf0,1);Need(hooks.registerCommand)("back",0x42ddf0,1);Need(hooks.registerCommand)("front",0x42def0,1);
    const auto front=Need(hooks.readConfig)(Key(globals.subscriber,"chatFormat")); //첫 지역 복사는 두번째 Config callback 뒤에도 같은 내용을 유지한다.
    const auto back=Need(hooks.readConfig)(Key(globals.subscriber,"chatFormat2"));
    if (!globals.current) Need(hooks.assertion)(0x382);
    const auto first=globals.current;
    Need(hooks.copyFormat)(first,0x14,front ? CString(*front) : std::string_view{},60);Write(Memory(hooks,first),0x4f,0,1);
    const auto second=globals.current;
    Need(hooks.copyFormat)(second,0x50,back ? CString(*back) : std::string_view{},500);Write(Memory(hooks,second),0x243,0,1);
}
// setter0042d620의 순방향 겹침 복사를 물리 범위 내에서 그대로 수행한다.
void SetChatManagerRankPrefix(std::span<std::uint8_t> manager,std::span<const std::uint8_t> source) {
    for (std::size_t index=0;;++index) { //원문 source BYTE를 읽고 같은 순서로 써서 겹치는 버퍼의 효과도 보존한다.
        if (index>=source.size()) throw std::out_of_range("Chat 랭크 prefix source 물리 범위");const auto value=source[index];
        Write(manager,kChatManagerRankPrefixOffset+index,value,1);if (!value) return;
    }
}
// formatter0042f480의 signed clamp·캡처 divisor·전체256BYTE 출력 순서를 수행한다.
std::span<const std::uint8_t> ChatPrefixIcons(std::span<std::uint8_t> output,std::span<std::uint8_t> temporary,std::int32_t rank,const ChatManagerPrefixHooks& hooks) {
    if (temporary.size()<kChatRankOutputBytes) throw std::out_of_range("Chat 랭크 temporary256 물리 범위");temporary[0]=0;
    if (rank>=0) {
        const auto first=Need(hooks.maximumRank)();const auto compared=first<rank ? Need(hooks.maximumRank)() : rank;
        if (compared<0) rank=0;
        else if (Need(hooks.maximumRank)()<rank) rank=Need(hooks.maximumRank)();
        const auto minor=Need(hooks.minorCount)();if (minor<=0) Need(hooks.assertion)(0x9aa);
        if (minor==0 || (rank==std::numeric_limits<std::int32_t>::min() && minor==-1)) throw std::domain_error("Chat 랭크 원문 idiv fault");
        const auto quotient=rank/minor,remainder=rank%minor; //진단이 현재 getter를 바꿔도 ESI에 캡처한 divisor를 다시 읽지 않는다.
        if (quotient>0) Need(hooks.appendIcon)(temporary.first(kChatRankOutputBytes),'C',quotient-1);
        Need(hooks.appendIcon)(temporary.first(kChatRankOutputBytes),'B',remainder);
    }
    for (std::size_t offset=0;offset<kChatRankOutputBytes;offset+=4) { //rep movsd의 순방향 DWORD 단위 읽기/쓰기를 보존한다.
        if (offset+4>output.size()) throw std::out_of_range("Chat 랭크 output256 물리 범위");const std::array<std::uint8_t,4> word{temporary[offset],temporary[offset+1],temporary[offset+2],temporary[offset+3]};
        std::copy(word.begin(),word.end(),output.begin()+static_cast<std::ptrdiff_t>(offset));
    }
    return output.first(kChatRankOutputBytes);
}
// wrapper0042f540은 실제 자연 관문 뒤 현재 CoreData와 마지막 현재 manager를 빌린다.
void RefreshChatManagerRankPrefix(ChatManagerPrefixGlobals globals,const ChatManagerPrefixHooks& hooks,std::span<std::uint8_t> output,std::span<std::uint8_t> temporary) {
    if (!globals.current || !globals.coreDataActive) return;
    const auto core=Need(hooks.core)();if (core.size()<5) throw std::out_of_range("Chat 랭크 현재 CoreData+4 물리 범위");
    const auto text=ChatPrefixIcons(output,temporary,core[4],hooks);SetChatManagerRankPrefix(Memory(hooks,globals.current),text);
}
}
