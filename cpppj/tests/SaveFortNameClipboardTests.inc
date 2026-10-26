// 최초 TextEntry 문맥의 실제 clipboard 부모가 Save as Route/마우스 확정/모달 반납까지 유지되는지 확인한다.
#include "client/TextClipboard.h"
#include <memory>

// 실제 OS 대신 명시 HANDLE/메모리/반환만 제공하며 제품의 공통 Route와 저장 부모를 그대로 실행한다.
TEST_CASE(SaveFortNameDialog_FirstTextContextClipboardRoutesToMouseSaveAndRetirement) {
    constexpr std::uintptr_t handle=0x12345678U; //명시 장치가 빌려 주는 HANDLE이며 실제 OS 클립보드 핸들이 아니다.
    std::vector<char> memory(512,static_cast<char>(0xa5));std::string trace,submitted;std::uint32_t firstNow{},replacedNow{},firstSubmit{},replacedSubmit{}; //OS 더블의 물리/실제 요청 순서와 첫 문맥 생존 관측이다.
    const auto event=[&](std::string value) { if (!trace.empty()) trace+=';';trace+=value; }; //제품의 선택된 OS 요청만 기록하고 결과 문자열을 계산하지 않는다.
    TextClipboardDevices devices; //어느 callback도 Win32TextClipboardDevices를 호출하지 않는다.
    devices.formatAvailable=[&](auto format) { event("available:"+std::to_string(format));return 1U; };
    devices.open=[&](auto window) { event("open:"+std::to_string(window));return 1U; };
    devices.get=[&](auto format) { event("get:"+std::to_string(format));return handle; };
    devices.lock=[&](auto value)->char* { event("lock:"+std::to_string(value));CHECK(value==handle);return memory.data(); };
    devices.unlock=[&](auto value) { event("unlock:"+std::to_string(value));return 0U; }; //성공0을 원문 부모가 무시하는 장치도 그대로 제공한다.
    devices.empty=[&] { event("empty");return 0U; };
    devices.allocate=[&](auto flags,auto count) { event("allocate:"+std::to_string(flags)+','+std::to_string(count));CHECK(count<=memory.size());return handle; };
    devices.set=[&](auto format,auto value) { event("set:"+std::to_string(format)+','+std::to_string(value));return std::uintptr_t{}; }; //반환0도 원문 부모가 Close까지 진행해야 한다.
    devices.close=[&] { event("close");return 0U; };
    SaveNameScene scene;RawTextEntryContext first;first.now=[&] { ++firstNow;return 12.5; }; //Save as보다 먼저 설치된 실제 공통 처리기의 벽시계다.
    first.submit=[&](auto callback,auto text) { CHECK(callback==0x42f440U);++firstSubmit;submitted=text; }; //Save as 뒤 만든 Console도 처음 설치한 제출 장치를 빌려야 한다.
    auto clipboard=std::make_shared<TextClipboard>(std::move(devices));BindTextClipboard(first,clipboard);clipboard.reset(); //두 실제 callback이 같은 부모 수명을 소유한다.
    auto& entries=scene.menus->EnableTextEntry(std::move(first));CHECK(scene.menus->TextEntryControls()==&entries);
    auto context=scene.Context();context.editor.now=[&] { ++replacedNow;return 999.0; };context.editor.submit=[&](auto,auto) { ++replacedSubmit; }; //후속 Save as 문맥을 공통 처리기 전체 교체로 오해하면 관측이 달라진다.
    const auto path=scene.work.ResolvePath("d/Clipboard New.fort");scene.work.Delete(path.string());const auto token=scene.menus->ShowSaveFortNameDialog(std::move(context),1,1,"CopySeed");
    const auto* root=scene.menus->SaveFortNameNode(token);CHECK(root && scene.buttons.CurrentDialog()==token && scene.menus->TextEntryControls()==&entries);if (!root) return;
    const auto entry=root->entry;auto* node=entries.Node(entry);CHECK(node);if (!node) return;auto* buffer=entries.Storage().Find(node->buffer);CHECK(buffer);if (!buffer) return;
    CHECK(buffer->Text()=="CopySeed" && buffer->State().selectionBegin==0 && buffer->State().selectionEnd==8 && node->blinkAt==13.0 && firstNow>0 && !replacedNow);
    firstNow=0;node->blinkAt=0;entries.Idle(entry);CHECK(firstNow==2 && node->blinkAt==13.0 && !replacedNow); //실제 Idle의 두 조회가 최초 문맥을 사용하는지 확인한다.
    const auto copy=scene.buttons.RouteInput({0x002d0000U|InputCode::kControl,0,0});CHECK(copy.consumed!=0);
    CHECK(std::string(memory.data())=="CopySeed" && memory[8]==0 && static_cast<unsigned char>(memory[9])==0xa5U);
    CHECK(trace=="open:0;empty;allocate:8192,9;lock:305419896;unlock:305419896;set:1,305419896;close");
    trace.clear();const std::string pasted="Clipboard New";std::copy(pasted.begin(),pasted.end(),memory.begin());memory[pasted.size()]=0; //OS가 제공한 독립 CF_TEXT 원문만 바꾸고 TextEntry 버퍼는 직접 설정하지 않는다.
    const auto paste=scene.buttons.RouteInput({0x002d0000U|InputCode::kShift,0,0});CHECK(paste.consumed!=0 && buffer->Text()==pasted);
    CHECK(trace=="available:1;open:0;get:1;lock:305419896;unlock:305419896;close" && scene.saved.empty() && !scene.pathCalls && !replacedNow && !replacedSubmit);
    const auto point=scene.menus->Point("OK");CHECK(point);if (!point) return;
    scene.buttons.RouteInput({InputCode::kLeftButton,point->x,point->y});scene.buttons.RouteInput({InputCode::kLeftButton|InputCode::kRelease,point->x,point->y}); //실제 hit/press/release와 버튼 serial로 부모의 저장 확정을 실행한다.
    CHECK(scene.saved==pasted && scene.pathCalls==1 && Get(scene.header,8)==0 && scene.work.Exists(path.string())!=0 && (root->flags&0x10000U));
    CHECK(netstorm::o::ReadFileBytes(path)==std::vector<std::uint8_t>({1,2,3,4}));scene.buttons.CleanupGumps();CHECK(!scene.menus->SaveFortNameNode(token) && !entries.Node(entry) && !scene.buttons.CurrentDialog() && entries.Storage().BufferCount()==0);
    const auto retained=entries.Storage().AllocationCount(); //원문 제외문자 소멸 경계의 기존 보존 블록과 Console 수명을 구별한다.
    const auto console=entries.ConstructConsole(220,48,"Console Kept",0,scene.palette.named[30].index,scene.palette.named[31].index);auto* consoleNode=entries.Console(console);CHECK(consoleNode);if (!consoleNode) return;
    consoleNode->submit=0x42f440U;consoleNode->multiline=0;scene.menus->Attach(console,20,30,scene.buttons.RootToken(),0);scene.buttons.HandleGump(console,13,0,0);
    CHECK(firstSubmit==1 && submitted=="Console Kept" && !replacedSubmit && !replacedNow);scene.menus->CloseTextEntries();CHECK(!entries.Node(console) && entries.Storage().BufferCount()==0 && entries.Storage().AllocationCount()==retained);
    scene.work.Delete(path.string()); //이번 검사에서 생성한 실제 작업 파일만 반납한다.
}
