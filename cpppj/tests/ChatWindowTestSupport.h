#pragma once
// ChatWindow 독립 검사에서 같은 실제 TextEntry/버퍼와 명시 장치의 장기 저장소를 구성한다.
#include "TestSupport.h"
#include "client/TextEntryGump.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace netstorm::client;
namespace {
constexpr std::uint32_t kEntryNode=0x01009000,kEntryBuffer=0x01001000,kEntryVertical=0x0100a000,kEntryHorizontal=0x0100b000,kEntryVtable=0x0100c000; // 원문 격리 주소와 같은 토큰이다.
constexpr std::uint32_t kEntryBlocks=0x01010000,kEntryStride=0x10000; // 동일 allocator의 실제 가상주소 순서다.
constexpr std::size_t kEntryRows=2292,kEntryColumns=23; //1146입력의 두 x87 독립 관측 수다.
// 마지막 빈 필드까지 원문 입력/관측 경계를 유지한다.
std::vector<std::string> EntrySplit(std::string_view value,char separator) {
    std::vector<std::string> result;std::size_t begin{}; // 현재 구간과 순서가 있는 필드다.
    for (;;) { const auto end=value.find(separator,begin);result.emplace_back(value.substr(begin,end==value.npos ? value.npos : end-begin));if (end==value.npos) return result;begin=end+1; } // 구분자만 제거한다.
}
// signed 입력도 하위 DWORD 비트를 보존한다.
std::uint32_t EntryNumber(std::string_view value) { return static_cast<std::uint32_t>(std::stoll(std::string(value))); }
// 실제 signed32 필드 해석이다.
std::int32_t EntrySigned(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 원문 메모리 입력을 바이트로 읽는다.
std::vector<std::uint8_t> EntryBytes(std::string_view value) {
    std::vector<std::uint8_t> result;if (value.size()%2) throw std::runtime_error("TextEntry 16진 폭");
    for (std::size_t i=0;i<value.size();i+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(value.substr(i,2)),nullptr,16))); // 두 문자씩 한 바이트다.
    return result;
}
// raw BYTE를 작은문자 16진으로 관측한다.
std::string EntryHex(std::span<const std::uint8_t> value) {
    constexpr char digits[]="0123456789abcdef";std::string result; // fixture의 동일 문자표다.
    for (auto byte:value) { result+=digits[byte>>4];result+=digits[byte&15]; }return result; // 실제 저장 순서다.
}
// NUL까지의 현재 바이트만 반환한다.
std::span<const char> EntryCString(std::span<const char> value) { const auto end=std::find(value.begin(),value.end(),0);if (end==value.end()) throw std::runtime_error("TextEntry 장치 NUL 없음");return value.first(static_cast<std::size_t>(end-value.begin())); }
// char 문자열도 UTF-8 변환 없이 raw BYTE로 관측한다.
std::string EntryHex(std::span<const char> value) { return EntryHex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); }
// 현재 DWORD만 구분자로 연결한다.
template<std::size_t Size> std::string EntryPacked(const std::array<std::uint32_t,Size>& value) {
    std::string result;for (auto word:value) { if (!result.empty()) result+=':';result+=std::to_string(word); }return result; // 원문 필드 순서다.
}
// 실제 원문 필드와 C++의 같은 공유 노드를 직접 매핑한다.
std::array<std::uint32_t,48> EntryFields(const ConsoleGumpNode& node) {
    const auto time=std::bit_cast<std::uint64_t>(node.blinkAt); // double의 두 원문 DWORD다.
    return {node.vtable,node.flags,node.command,node.serial,std::bit_cast<std::uint32_t>(node.scale),std::bit_cast<std::uint32_t>(node.baseWidth),std::bit_cast<std::uint32_t>(node.baseHeight),std::bit_cast<std::uint32_t>(node.width),std::bit_cast<std::uint32_t>(node.height),std::bit_cast<std::uint32_t>(node.x),std::bit_cast<std::uint32_t>(node.y),node.textContext[0],node.textContext[1],node.textContext[2],node.textContext[3],node.textContext[4],node.textContext[5],node.textContext[6],node.textContext[7],node.backgroundFlags,node.verticalSlider,node.horizontalSlider,node.ownsBuffer,node.reservedC4,std::uint32_t(time),std::uint32_t(time>>32),node.blinkPhase,node.readOnly,std::uint32_t(node.shiftAnchor),std::uint32_t(node.shiftEnd),std::uint32_t(node.scrollLine),std::uint32_t(node.scrollColumn),node.showVertical,node.showHorizontal,node.forwardInput,node.multiline,node.changeNotice,node.recipientHighlight,std::uint32_t(node.maxLength),node.excludedChars,node.buffer,node.reserved10C,node.autocomplete,node.submit,node.resetWord,node.wordStart,node.wordLength,node.reserved124};
}
// fixture의 raw 필드를 같은 실제 노드에 설치한다.
void EntrySetFields(ConsoleGumpNode& node,const std::vector<std::string>& fields) {
    if (fields.size()!=48) throw std::runtime_error("TextEntry 노드 필드 폭");std::array<std::uint32_t,48> value{}; // 원시 입력 복사본이다.
    for (std::size_t i=0;i<value.size();++i) value[i]=EntryNumber(fields[i]); // 기대관측을 읽지 않는다.
    node.vtable=value[0];node.flags=value[1];node.command=value[2];node.serial=value[3];node.scale=std::bit_cast<float>(value[4]);node.baseWidth=std::bit_cast<float>(value[5]);node.baseHeight=std::bit_cast<float>(value[6]);node.width=std::bit_cast<float>(value[7]);node.height=std::bit_cast<float>(value[8]);node.x=std::bit_cast<float>(value[9]);node.y=std::bit_cast<float>(value[10]);
    for (std::size_t i=0;i<8;++i) node.textContext[i]=value[11+i]; // 같은 기본 문맥이다.
    node.backgroundFlags=value[19];node.verticalSlider=value[20];node.horizontalSlider=value[21];node.ownsBuffer=value[22];node.reservedC4=value[23];node.blinkAt=std::bit_cast<double>(std::uint64_t(value[24])|(std::uint64_t(value[25])<<32));node.blinkPhase=value[26];node.readOnly=value[27];node.shiftAnchor=EntrySigned(value[28]);node.shiftEnd=EntrySigned(value[29]);node.scrollLine=EntrySigned(value[30]);node.scrollColumn=EntrySigned(value[31]);node.showVertical=value[32];node.showHorizontal=value[33];node.forwardInput=value[34];node.multiline=value[35];node.changeNotice=value[36];node.recipientHighlight=value[37];node.maxLength=EntrySigned(value[38]);node.excludedChars=value[39];node.buffer=value[40];node.reserved10C=value[41];node.autocomplete=value[42];node.submit=value[43];node.resetWord=value[44];node.wordStart=value[45];node.wordLength=value[46];node.reserved124=value[47];
}
// Slider의 실제 기본 크기/위치와 position만 관측한다.
std::array<std::uint32_t,11> EntryChildFields(const ScrollGumpNode& node) { return {node.vtable,node.flags,node.serial,std::bit_cast<std::uint32_t>(node.scale),std::bit_cast<std::uint32_t>(node.baseWidth),std::bit_cast<std::uint32_t>(node.baseHeight),std::bit_cast<std::uint32_t>(node.width),std::bit_cast<std::uint32_t>(node.height),std::bit_cast<std::uint32_t>(node.x),std::bit_cast<std::uint32_t>(node.y),std::uint32_t(node.position)}; }
// 가드를 포함한 실제 할당 메모리다.
struct EntryBlock { std::uint32_t nominal{},live{};std::vector<std::uint8_t> bytes; }; // 요청폭·반납 여부·물리 저장소다.
// 현재 노드/버퍼와 명시 장치를 소유한다. 뒤쪽 기대관측은 읽지 않는다.
struct EntryScene {
    ConsoleGumpNode node;ScrollGumpNode vertical,horizontal; // 실제 파생/기본 저장소가 같은 세 노드다.
    TextEntryBufferState state;TextEntryGumpState globals; // 같은 버퍼의 원문 필드와 현재 색/종류/사건이다.
    std::map<std::uint32_t,EntryBlock> blocks;std::uint32_t nextBlock{},sliderAllocated{},fill{}; // 명시 allocator 저장소와 순번이다.
    std::vector<char> text,excluded,clipboardInput,clipboardOutput; // 장치 문자열은 재할당 전까지 같은 수명을 가진다.
    std::vector<double> times;std::size_t timeIndex{};std::optional<std::string> completion;bool recipient{}; // 시계/자동완성/recipient의 독립 입력이다.
    std::string trace;std::unique_ptr<TextEntryBuffer> buffer;std::unique_ptr<TextEntryGump> gump; // 현재 처리기와 실제 외부 호출 관측이다.

    // 같은 현재 10DWORD만 원문 순서대로 읽는다.
    std::array<std::uint32_t,10> BufferFields() const { return {state.vtable,state.offsets,std::uint32_t(state.offsetCapacity),std::uint32_t(state.lineCount),state.dirty,std::uint32_t(state.selectionBegin),std::uint32_t(state.selectionEnd),std::uint32_t(state.length),std::uint32_t(state.capacity),state.data}; }
    // 현재 물리 메모리의 남은 폭만 빌린다.
    std::span<std::uint8_t> Memory(std::uint32_t address) {
        if (address==0x01006000) return {reinterpret_cast<std::uint8_t*>(excluded.data()),excluded.size()};
        auto found=blocks.upper_bound(address);if (found==blocks.begin()) throw std::runtime_error("TextEntry 메모리 주소");--found;const auto offset=std::size_t(address-found->first);if (offset>=found->second.bytes.size()) throw std::runtime_error("TextEntry 물리 범위");return std::span<std::uint8_t>(found->second.bytes).subspan(offset);
    }
    // 같은 실제 기본/파생 주소를 공급한다.
    GumpCreateNode* Base(std::uint32_t token) { if (token==kEntryNode) return &node;if (token==kEntryVertical) return &vertical;if (token==kEntryHorizontal) return &horizontal;return nullptr; }
    // 실제 현재 Slider 장치를 조회한다.
    ScrollGumpNode& Slider(std::uint32_t token) { if (token==kEntryVertical) return vertical;if (token==kEntryHorizontal) return horizontal;throw std::runtime_error("TextEntry Slider 주소"); }
    // 명시 장치의 실제 호출 인자/순서만 기록한다.
    void Event(std::string_view name,std::vector<std::string> values={}) { if (!trace.empty()) trace+=';';trace+=name;for (const auto& value:values) trace+=':'+value; } // 관측용 문자열에만 추가한다.
    // raw DWORD 인자를 같은 표기로 기록한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values) { std::vector<std::string> words;for (auto value:values) words.push_back(std::to_string(value));Event(name,std::move(words)); } // 실제 장치 호출의 정수폭이다.
    // 원문 nominal과 +16BYTE 물리 가드를 별도로 유지한다.
    std::uint32_t Allocate(std::uint32_t size) { const auto address=kEntryBlocks+nextBlock++*kEntryStride;blocks.emplace(address,EntryBlock{size,1,std::vector<std::uint8_t>(std::size_t(size)+16,static_cast<std::uint8_t>(fill))});return address; }
    // 실제 버퍼 본체용 소유 메모리·clipboard 장치를 연결한다.
    TextEntryBufferHooks BufferHooks() {
        TextEntryBufferHooks hooks;hooks.bytes=[this](auto address) { return Memory(address); };
        hooks.allocate=[this](auto size) { const auto address=Allocate(size);Event("allocate",{size,address});return address; };
        hooks.release=[this](auto address) { if (address) blocks.at(address).live=0;Event("release",{address}); };
        hooks.resize=[this](auto previous,auto size) { const auto address=Allocate(size);if (previous) { auto& old=blocks.at(previous);std::copy_n(old.bytes.begin(),std::min(old.nominal,size),blocks.at(address).bytes.begin());old.live=0; }Event("resize",{previous,size,address});return address; };
        hooks.clipboardRead=[this](std::span<char> target) { std::fill(target.begin(),target.end(),char{});std::copy_n(clipboardInput.begin(),std::min(target.size(),clipboardInput.size()),target.begin());Event("clipboardRead",{static_cast<std::uint32_t>(target.size())}); };
        hooks.clipboardWrite=[this](std::span<const char> value) { clipboardOutput.assign(value.begin(),value.end());Event("clipboardWrite",std::vector<std::string>{value.empty() ? "-" : EntryHex(value)}); };return hooks;
    }
    // 일반 트리/글꼴/Slider는 독립 관측과 동일한 명시 장치 계약이다.
    TextEntryGumpHooks Hooks() {
        TextEntryGumpHooks hooks;hooks.node=[this](auto token)->TextEntryGumpNode* { return token==kEntryNode ? &node : nullptr; };hooks.console=[this](auto token)->ConsoleGumpNode* { return token==kEntryNode ? &node : nullptr; };hooks.base=[this](auto token) { return Base(token); };hooks.buffer=[this](auto token) { return token==kEntryBuffer ? buffer.get() : nullptr; };hooks.slider=[this](auto token) { return &Slider(token); };
        hooks.allocate=[this](auto size) { const auto token=size==40 ? kEntryBuffer : (sliderAllocated++ ? kEntryHorizontal : kEntryVertical);Event("new",{static_cast<std::uint32_t>(size),token});return token; };
        hooks.releaseBuffer=[this](auto token) { Event("delete",{token}); };
        hooks.constructBackground=[this](auto token,auto flags) { node.flags=1;node.scale=1;node.baseWidth=0;node.baseHeight=0;node.backgroundFlags=flags;Event("constructBackground",{token,flags}); };
        hooks.constructSlider=[this](auto token,auto width,auto height,auto brown,auto black) { auto& child=Slider(token);child.vtable=kEntryVtable;child.flags=1;child.scale=1;child.baseWidth=child.width=static_cast<float>(width);child.baseHeight=child.height=static_cast<float>(height);child.position=0;Event("constructSlider",{token,std::uint32_t(width),std::uint32_t(height),brown,black}); };
        const auto attach=[this](std::string_view name,auto token,float x,float y,auto parent,auto mode) { auto* target=Base(token);target->x=x;target->y=y;Event(name,{token,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),parent,mode}); }; // 실제 공통 장치처럼 로컬 좌표만 쓴다.
        hooks.baseAttach=[attach](auto token,float x,float y,auto parent,auto mode) { attach("baseAttach",token,x,y,parent,mode); };hooks.attach=[attach](auto token,float x,float y,auto parent,auto mode) { attach("attach",token,x,y,parent,mode); };
        hooks.push=[this](auto token) { Event("push",{token}); };hooks.redraw=[this](auto token,bool value) { Event("redraw",{token,std::uint32_t(value)}); };hooks.idle=[this](auto token,bool value) { Event("idle",{token,std::uint32_t(value)}); };hooks.bubble=[this](auto token,auto event,auto first,auto second) { Event("bubble",{token,event,first,second}); };
        hooks.sliderRange=[this](auto token,auto first,auto last) { Event("range",{token,std::uint32_t(first),std::uint32_t(last)}); };hooks.sliderPage=[this](auto token,auto value) { Event("page",{token,std::uint32_t(value)}); };hooks.sliderPosition=[this](auto token,auto value) { Event("position",{token,std::uint32_t(value)});Slider(token).position=value; };
        hooks.now=[this] { const auto value=times[std::min(timeIndex++,times.size()-1)];Event("now",std::vector<std::string>{std::to_string(std::bit_cast<std::uint64_t>(value))});return value; };
        hooks.measure=[this](auto,std::span<const char> value,std::int32_t length) { const auto visible=length<0 ? EntryCString(value) : value.first(static_cast<std::size_t>(length));std::int32_t width{};for (auto byte:visible) width+=4+static_cast<unsigned char>(byte)%5;Event("measure",std::vector<std::string>{visible.empty() ? "-" : EntryHex(visible),std::to_string(std::uint32_t(length)),std::to_string(width),"14"});return std::array<std::int32_t,2>{width,14}; }; // 실제 글꼴 대신 명시 BYTE 측정 장치다.
        hooks.position=[this](auto token) { Event("screen",{token});return ScreenPoint{17,23}; };hooks.drawBackground=[this](auto token) { Event("background",{token}); };
        hooks.drawText=[this](auto,std::span<const char> value,ScreenPoint point,const std::array<std::uint32_t,8>* context) { const auto visible=EntryCString(value);Event("text",std::vector<std::string>{visible.empty() ? "-" : EntryHex(visible),std::to_string(std::uint32_t(point.x)),std::to_string(std::uint32_t(point.y)),context ? "1" : "0",std::to_string(context ? (*context)[2] : node.textContext[2])}); };
        hooks.fill=[this](ScreenRect rect,auto color) { Event("fill",{std::uint32_t(rect.left),std::uint32_t(rect.top),std::uint32_t(rect.right),std::uint32_t(rect.bottom),color}); };hooks.recipient=[this](std::string_view value) { Event("recipient",std::vector<std::string>{value.empty() ? "-" : EntryHex(std::span<const char>(value.data(),value.size()))});return recipient; };
        hooks.allocateBytes=[this](auto size) { const auto token=Allocate(static_cast<std::uint32_t>(size));Event("new",{static_cast<std::uint32_t>(size),token});return token; };hooks.releaseBytes=[this](auto token) { if (blocks.contains(token)) blocks.at(token).live=0;Event("delete",{token}); };hooks.bytes=[this](auto token) { return Memory(token); };
        hooks.complete=[this](auto,std::optional<std::string_view> word,std::int32_t length,bool shift) { Event("complete",std::vector<std::string>{word ? EntryHex(std::span<const char>(word->data(),word->size())) : "~",std::to_string(std::uint32_t(length)),shift ? "1" : "0"});return completion; };hooks.submit=[this](auto,std::string_view value) { Event("submit",std::vector<std::string>{value.empty() ? "-" : EntryHex(std::span<const char>(value.data(),value.size()))}); };hooks.assertion=[this](auto line) { Event("assert",{line}); };hooks.destructBase=[this](auto token) { Event("destructBase",{token}); };return hooks;
    }
    // 앞13열의 raw 입력만 실제 소유 상태에 설치한다.
    explicit EntryScene(const std::vector<std::string>& row) {
        EntrySetFields(node,EntrySplit(row[2],':'));const auto values=EntrySplit(row[3],':');if (values.size()!=10) throw std::runtime_error("TextEntry 버퍼 필드 폭");std::array<std::uint32_t,10> words{};
        for (std::size_t i=0;i<words.size();++i) words[i]=EntryNumber(values[i]); // 원문10DWORD 설치다.
        state={words[0],words[1],EntrySigned(words[2]),EntrySigned(words[3]),words[4],EntrySigned(words[5]),EntrySigned(words[6]),EntrySigned(words[7]),EntrySigned(words[8]),words[9]};
        if (row[4]!="-") for (const auto& item:EntrySplit(row[4],';')) { const auto parts=EntrySplit(item,':');blocks.emplace(EntryNumber(parts[0]),EntryBlock{EntryNumber(parts[1]),EntryNumber(parts[2]),EntryBytes(parts[3])}); } // 실제 기존 할당의 모든 바이트다.
        nextBlock=static_cast<std::uint32_t>(blocks.size());fill=EntryNumber(row[12]);const auto setText=[](std::string_view hex,bool terminate) { const auto bytes=EntryBytes(hex);std::vector<char> output(bytes.begin(),bytes.end());if (terminate) output.push_back(0);return output; }; // 원문 문자 인코딩을 해석하지 않는다.
        text=setText(row[6],true);excluded=setText(row[7],true);clipboardInput=setText(row[8],false);for (const auto& time:EntrySplit(row[9],':')) times.push_back(std::stod(time)); // 모든 장치값은 앞부분 입력이다.
        if (row[10]!="~") { const auto bytes=setText(row[10],false);completion=std::string(bytes.begin(),bytes.end()); }recipient=row[11]=="1";
        for (const auto token:{kEntryVertical,kEntryHorizontal}) { auto& child=Slider(token);child.vtable=kEntryVtable;child.flags=1;child.serial=token==kEntryVertical ? 31 : 32;child.scale=1;child.baseWidth=child.width=token==kEntryVertical ? 10.0F : 100.0F;child.baseHeight=child.height=token==kEntryVertical ? 100.0F : 10.0F;child.position=3; } // 독립 장치 초기 상태다.
        globals.brown=9;globals.black=0;globals.yellow=5;buffer=std::make_unique<TextEntryBuffer>(state,BufferHooks());gump=std::make_unique<TextEntryGump>(globals,Hooks());
    }
    // 반납 블록/물리 가드를 포함한 현재 전체 저장소를 관측한다.
    std::string BlocksOutput() const { std::string output;for (const auto& [address,block]:blocks) { if (!output.empty()) output+=';';output+=std::to_string(address)+':'+std::to_string(block.nominal)+':'+std::to_string(block.live)+':'+EntryHex(block.bytes); }return output.empty() ? "-" : output; } // 주소 순서대로만 직렬화한다.
};
}

// 실제 전체 진입/RET 관측과 제품의 raw 필드·버퍼 전체·장치 순서를 대조한다.
