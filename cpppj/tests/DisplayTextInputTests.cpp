// 실제 TextEntryBuffer/Gump와 일반 입력 큐를 사용하는 창 없는 한국어 입력 검사다.
#include "TestSupport.h"
#include "client/DisplayTextInput.h"
#include "client/TextClipboard.h"
#include "client/TextEntryGump.h"
#include "client/TextEntryStorage.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::client;
namespace { // 실제 버퍼/노드만 만들며 창/OS 입력/전송을 사용하지 않는 장치다.
// UTF-8 기대 본문을 기존 표시 경계로만 준비한다. 입력 API의 결과를 기대값으로 재사용하지 않는다.
std::string Display(std::string_view value) { const auto bytes=netstorm::o::EncodeDisplayText(value);return {bytes.begin(),bytes.end()}; }
// 일반 Insert가 요구하는 마지막 NUL을 별도 소유 문자열에 붙인다.
std::string Terminated(std::string value) { value.push_back(0);return value; }
// 실제 공통 노드/메모리/선택 상태와 별도 장치 관측을 소유한다.
struct KoreanEntryScene {
    ConsoleGumpNode node{}; // 같은 파생/기본 주소를 갖는 실제 편집 노드다.
    ScrollGumpNode vertical{},horizontal{}; // 현재 가로/세로 position을 보유하는 실제 두 Slider다.
    TextEntryGumpState state{}; // 현재 색/통지 전역이다.
    std::string clipboard; // 명시 장치가 반환하는 표시 BYTE 본문이다.
    std::vector<std::string> drawn; // 실제 DrawText에 전달한 토큰 경계를 관측한다.
    std::vector<ScreenRect> fills; // 실제 선택/커서 사각형을 관측한다.
    std::vector<std::uint32_t> notices; // 일반 통지/원시 사건 순서를 관측한다.
    std::string completed; // Console이 전달한 실제 자동완성 단어다.
    TextEntryStorage storage; // 실제 TextEntryBuffer 할당/선택/Insert 처리를 소유한다.
    std::unique_ptr<TextEntryGump> entry; // 같은 노드/버퍼를 사용하는 실제 입력 처리기다.
    // 생성자는 원문 편집 버퍼를 만들고 필요한 조회/Draw/Slider 장치만 연결한다.
    KoreanEntryScene():storage([this](auto bytes) { std::fill(bytes.begin(),bytes.end(),0);std::copy_n(clipboard.begin(),std::min(clipboard.size(),bytes.size()-1),bytes.begin()); },[this](auto bytes) { clipboard.assign(bytes.begin(),bytes.end()); }) {
        auto& buffer=storage.Create(4);buffer.Construct();buffer.Reset();node.buffer=4;node.verticalSlider=2;node.horizontalSlider=3;node.baseWidth=100;node.baseHeight=16;node.serial=17;
        TextEntryGumpHooks hooks;
        hooks.node=[this](auto token) { return token==1 ? static_cast<TextEntryGumpNode*>(&node) : nullptr; }; // 실제 기본/파생 주소다.
        hooks.console=[this](auto token) { return token==1 ? &node : nullptr; }; // 같은 Console 주소다.
        hooks.base=[this](auto token)->GumpCreateNode* { if (token==1) return &node;if (token==2) return &vertical;if (token==3) return &horizontal;return nullptr; }; // 같은 세 실제 노드다.
        hooks.buffer=[this](auto token) { return storage.Find(token); }; // 같은 메모리 소유자를 조회한다.
        hooks.slider=[this](auto token) { return token==2 ? &vertical : &horizontal; }; // 두 실제 Slider를 조회한다.
        hooks.redraw=[](auto,bool) {};hooks.bubble=[this](auto,auto event,auto,auto) { notices.push_back(event); }; // 픽셀/통지만 관측한다.
        hooks.sliderRange=[](auto,auto,auto) {};hooks.sliderPage=[](auto,auto) {};hooks.sliderPosition=[this](auto token,auto at) { (token==2 ? vertical : horizontal).position=std::max(0,at); }; // SetScroll의 실제 읽기/쓰기다.
        hooks.bytes=[this](auto address) { return storage.Bytes(address); }; // 제외목록도 같은 실제 저장소다.
        hooks.drawBackground=[](auto) {};hooks.position=[](auto) { return ScreenPoint{10,20}; }; // 배경 외부 효과를 만들지 않는다.
        hooks.drawText=[this](auto,auto bytes,auto,const auto*) { const auto end=std::find(bytes.begin(),bytes.end(),0);drawn.emplace_back(bytes.begin(),end); }; // 실제 전달한 각 조각을 기록한다.
        hooks.fill=[this](auto rect,auto) { fills.push_back(rect); }; // 커서와 선택 폭을 관측한다.
        hooks.measure=[](auto,std::span<const char> bytes,std::int32_t count) { // 글자당8픽셀의 독립 장치이며 유효한 토큰만 한 글자로 센다.
            const auto nul=std::find(bytes.begin(),bytes.end(),0);auto length=static_cast<std::size_t>(nul-bytes.begin());if (count>=0) length=std::min(length,static_cast<std::size_t>(count));
            const std::span<const std::uint8_t> raw{reinterpret_cast<const std::uint8_t*>(bytes.data()),length};std::int32_t width{};
            for (std::size_t at=0;at<length;) { at+=netstorm::o::DecodeExtendedSequence(raw,at) ? 4U : 1U;width+=8; } // 기대 의미는 글자 수이며 입력 경계 헬퍼를 재사용하지 않는다.
            return std::array<std::int32_t,2>{width,16};
        };
        hooks.complete=[this](auto,auto word,auto,bool)->std::optional<std::string> { if (word) completed=*word;return std::nullopt; }; // Console이 자른 단어를 그대로 기록한다.
        entry=std::make_unique<TextEntryGump>(state,std::move(hooks));
    }
    // 일반 입력 경로가 사용하는 같은 실제 버퍼를 조회한다.
    TextEntryBuffer& Buffer() { return *storage.Find(4); }
    // 입력 본문을 일반 Reset에 전달한다. 테스트 상태에 기대 결과를 쓰지 않는다.
    void Set(std::string_view value) { const auto text=Terminated(Display(value));Buffer().Reset(text); }
    // UTF-8 입력을 실제 일반 사건 생성→같은 Gump.Handle로 전달한다.
    void Type(std::string_view value) { for (const auto& event:MakeCommittedTextEvents(value)) entry->Handle(1,event.code,0,0); } // 네 BYTE 토큰을 단일 사건으로 처리한다.
    // 현재 편집 본문을 실제 표시 경계에서 UTF-8로 관측한다.
    std::string Text() { const auto text=Buffer().Text();return netstorm::o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
};
}

// 미지원/비정상 입력이 사건을 일부 생성하지 않고 한글마다 단일 태그를 만드는지 확인한다.
TEST_CASE(KoreanInput_Utf8Utf16AndAtomicQueue) {
    const auto events=MakeCommittedTextEvents("A한글");CHECK(events.size()==3);CHECK(events[0].code=='A');CHECK(IsCommittedTextInput(events[1].code));CHECK(CommittedTextBytes(events[1].code)==Display("한"));
    CHECK(CommittedUtf16ToUtf8(u"한글")=="한글");CHECK(DisplayTextToUtf16(std::span<const char>(Display("한글")))==u"한글");
    for (const auto& invalid:std::vector<std::string>{std::string("A\0B",3),"A\nB",std::string("\xe0\x80\x80",3),"A😀"}) { bool rejected{};try { static_cast<void>(MakeCommittedTextEvents(invalid)); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected); } // 각 입력은 전체 검증에서 거부한다.
    bool rejected{};try { static_cast<void>(CommittedUtf16ToUtf8(std::u16string(1,static_cast<char16_t>(0xd800)))); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
    InputQueue queue;for (unsigned i=0;i<38;++i) queue.Push('x',0,0); // 실제 링에 한 칸만 남긴다.
    CHECK(!queue.TryPushBatch(MakeCommittedTextEvents("한글")));CHECK(queue.FreeSlots()==1);
    for (unsigned i=0;i<38;++i) CHECK(queue.Pop(false).code=='x'); // 실패한 본문의 일부가 들어가지 않았는지 확인한다.
    CHECK(queue.Empty());CHECK(queue.TryPushBatch(events));CHECK(queue.Pop(false).code=='A');CHECK(IsCommittedTextInput(queue.Pop(false).code));CHECK(IsCommittedTextInput(queue.Pop(false).code));CHECK(queue.Empty());
}

// 한글 중간의 모든 BYTE 위치와 제외목록/길이 상한을 확인한다.
TEST_CASE(KoreanInput_DisplayBoundariesAndFiltering) {
    const auto text=Display("A한글B");CHECK(text.size()==10);
    for (std::size_t at=2;at<5;++at) { CHECK(DisplayTextFloor(text,at)==1);CHECK(DisplayTextCeil(text,at)==5); } // 첫 한글 내부를 양쪽 경계로 보정한다.
    CHECK(PreviousDisplayText(text,5)==1);CHECK(NextDisplayText(text,1)==5);CHECK(FilterDisplayText(text,"A",7)==Display("한"));
    CHECK(FilterDisplayText(text,std::string(1,static_cast<char>(0x1f)),100)=="AB");CHECK(FilterDisplayText(Display("갈"),std::string(1,static_cast<char>(11)),100)==Display("갈"));
    CHECK(DisplayTextLine(Display("a\n한글\nb"),1)==Display("한글"));
}

// 실제 Gump에서 왼쪽/오른쪽/선택/삭제가 네 BYTE 중간에 멈추지 않는지 확인한다.
TEST_CASE(KoreanInput_EditCursorSelectionAndDeletion) {
    KoreanEntryScene scene;scene.Type("A한글B");CHECK(scene.Text()=="A한글B");CHECK(scene.Buffer().State().selectionBegin==10);
    scene.entry->Handle(1,0x250000,0,0);CHECK(scene.Buffer().State().selectionBegin==9);scene.entry->Handle(1,0x250000,0,0);CHECK(scene.Buffer().State().selectionBegin==5);
    scene.entry->Handle(1,8,0,0);CHECK(scene.Text()=="A글B");CHECK(scene.Buffer().State().selectionBegin==1);
    scene.entry->Handle(1,0x2e0000,0,0);CHECK(scene.Text()=="AB");scene.Buffer().Select(1,1);scene.Type("한글");
    scene.entry->Handle(1,InputCode::kShift|0x250000,0,0);CHECK(scene.Buffer().State().selectionBegin==5);CHECK(scene.Buffer().State().selectionEnd==9);scene.Type("가");CHECK(scene.Text()=="A한가B");
    scene.Buffer().Select(2,3);scene.entry->Handle(1,8,0,0);CHECK(scene.Text()=="A가B"); // 손상된 외부 선택도 한 문자 전체로 확장한다.
}

// 기존 물리 BYTE 상한을 유지하면서 선택 교체/숫자 전용 제외목록을 확인한다.
TEST_CASE(KoreanInput_ByteLimitAndNumericExclusion) {
    KoreanEntryScene scene;scene.node.maxLength=9;scene.Type("한글");CHECK(scene.Text()=="한글");scene.Type("가");CHECK(scene.Text()=="한글");
    scene.Buffer().Select(0,4);scene.Type("나");CHECK(scene.Text()=="나글");scene.Buffer().Select(0,4);scene.Type("A");CHECK(scene.Text()=="A글");
    const auto excluded=scene.storage.Allocate(2);auto bytes=scene.storage.Bytes(excluded);bytes[0]=0x1f;bytes[1]=0;scene.node.excludedChars=excluded;scene.Type("한");CHECK(scene.Text()=="A글");
}

// 붙여넣기의 제외문자/상한이 한글 내부 제어 숫자를 지우지 않는지 확인한다.
TEST_CASE(KoreanInput_PasteWholeCharacters) {
    KoreanEntryScene scene;scene.node.maxLength=10;scene.clipboard=Display("갈한글");scene.entry->Handle(1,InputCode::kShift|0x2d0000,0,0);CHECK(scene.Text()=="갈한");
    scene.Buffer().Select(0,4);scene.clipboard=Display("가나");scene.entry->Handle(1,InputCode::kShift|0x2d0000,0,0);CHECK(scene.Text()=="가한");
}

// 가로 스크롤/선택/커서가 같은 BYTE 경계와 실제 글자 폭을 사용하는지 확인한다.
TEST_CASE(KoreanInput_DrawScrollAndCaretBoundaries) {
    KoreanEntryScene scene;scene.Set("한글B");scene.Buffer().Select(8,8);scene.node.scrollColumn=2;scene.node.blinkPhase=1;scene.entry->Draw(1);
    CHECK(!scene.drawn.empty());CHECK(scene.drawn.front()==Display("한글B"));CHECK(!scene.fills.empty());CHECK(scene.fills.back().left==26);
    scene.drawn.clear();scene.fills.clear();scene.node.scrollColumn=5;scene.entry->Draw(1);CHECK(scene.drawn.front()==Display("글B"));CHECK(scene.fills.back().left==18);
    scene.entry->SetScroll(1,0,6);CHECK(scene.node.scrollColumn==4);CHECK(scene.horizontal.position==4);
}

// Console 자동완성이 한글 토큰 내부의 공백과 같은 숫자 BYTE를 단어 경계로 오해하지 않는지 확인한다.
TEST_CASE(KoreanInput_ConsoleCompletionWordBoundary) {
    KoreanEntryScene scene;scene.Set("prefix 갈");scene.Buffer().Select(scene.Buffer().State().length,scene.Buffer().State().length);scene.node.autocomplete=1;scene.node.resetWord=1;
    scene.entry->HandleConsole(1,9,0,0);CHECK(scene.completed==Display("갈"));CHECK(scene.node.wordStart==7);CHECK(scene.node.wordLength==4);
}

// 실제 OS 대신 명시 Unicode 장치로 완전한 토큰 복사/붙여넣기와 미지원 거부를 확인한다.
TEST_CASE(KoreanInput_UnicodeClipboardBoundary) {
    TextClipboardDevices devices;std::u16string written;devices.readUnicode=[] { return std::optional<std::u16string>(u"한글"); };devices.writeUnicode=[&](auto text) { written=text; };TextClipboard clipboard(std::move(devices));
    std::array<char,8> shortOutput{};clipboard.Read(shortOutput);CHECK(std::string(shortOutput.data())==Display("한"));const auto text=Display("한글");clipboard.Write(text);CHECK(written==u"한글");
    TextClipboardDevices unsupported;unsupported.readUnicode=[] { return std::optional<std::u16string>(u"A😀"); };TextClipboard reject(std::move(unsupported));std::array<char,20> output{};output[0]='x';reject.Read(output);CHECK(output[0]==0);
}

TEST_CASE(KoreanInput_AsciiPasteIntoExtendedBodyHonorsFinalByteLimit) { // 기존본문이한글일때긴ASCII도물리상한을넘기지않는다.
    KoreanEntryScene scene;scene.node.maxLength=10;scene.Set("한");scene.Buffer().Select(4,4);scene.clipboard=std::string(255,'A');scene.entry->Handle(1,InputCode::kShift|0x2d0000,0,0);
    CHECK(scene.Text()=="한AAAAA" && scene.Buffer().State().length==9);scene.entry->Handle(1,InputCode::kShift|0x2d0000,0,0);CHECK(scene.Text()=="한AAAAA"); // 남은상한0이면본문/선택을자르지않는다.
    scene.Set("한글");scene.Buffer().Select(0,4);scene.entry->Handle(1,InputCode::kShift|0x2d0000,0,0);CHECK(scene.Text()=="AAAAA글" && scene.Buffer().State().length==9); // 지워질선택을차감한최종길이다.
    scene.node.maxLength=5;scene.Set("한");scene.Buffer().Select(0,0);scene.entry->Handle(1,InputCode::kShift|0x2d0000,0,0);CHECK(scene.Text()=="한");
}

TEST_CASE(KoreanInput_StaleShiftAnchorsAlwaysEndOnWholeCharacters) { // 외부선택뒤Shift가부분한글을다시선택하지않는다.
    for (const std::uint32_t key:{0x250000U,0x270000U,0x230000U,0x240000U}) { // Left/Right/End/Home은같은원자문자경계를사용한다.
        KoreanEntryScene scene;scene.Set("한A");scene.Buffer().Select(0,4);scene.node.shiftAnchor=2;scene.node.shiftEnd=2;scene.entry->Handle(1,InputCode::kShift|key,0,0);
        const auto boundary=[](std::int32_t at) { return at==0 || at==4 || at==5; }; // 한글1자와ASCII1자의독립적인의미경계다.
        CHECK(boundary(scene.Buffer().State().selectionBegin) && boundary(scene.Buffer().State().selectionEnd));CHECK(boundary(scene.node.shiftAnchor) && boundary(scene.node.shiftEnd));
    }
}

TEST_CASE(KoreanInput_UnicodeMissingUsesOriginalAsciiClipboardPath) { // Unicode부재/ASCII복사에서는CF_TEXT와원문할당순서를유지한다.
    TextClipboardDevices devices;std::string calls;std::array<char,16> memory{};const std::string source="ASCII";std::copy(source.begin(),source.end(),memory.begin());std::uint32_t flags{},allocated{},unicodeWrites{}; // 시스템대신명시된기존장치를관측한다.
    devices.readUnicode=[&]()->std::optional<std::u16string> { calls+='U';return {}; };devices.writeUnicode=[&](auto) { ++unicodeWrites; };
    devices.formatAvailable=[&](auto format) { CHECK(format==1);calls+='F';return 1U; };devices.open=[&](auto window) { CHECK(window==0);calls+='O';return 1U; };devices.get=[&](auto format) { CHECK(format==1);calls+='G';return std::uintptr_t{1}; };
    devices.lock=[&](auto handle) { CHECK(handle==1);calls+='L';return memory.data(); };devices.unlock=[&](auto) { calls+='N';return 1U; };devices.close=[&] { calls+='C';return 1U; };
    devices.empty=[&] { calls+='E';return 1U; };devices.allocate=[&](auto value,auto size) { flags=value;allocated=size;calls+='A';return std::uintptr_t{1}; };devices.set=[&](auto format,auto handle) { CHECK(format==1 && handle==1);calls+='S';return handle; };
    TextClipboard clipboard(std::move(devices));std::array<char,16> output{};clipboard.Read(output);CHECK(std::string(output.data())==source && calls=="UFOGLNC");
    calls.clear();const std::string copied="COPY";clipboard.Write(copied);CHECK(calls=="OEALNSC" && unicodeWrites==0 && flags==0x2000 && allocated==5);CHECK(std::string(memory.data())==copied);
}
