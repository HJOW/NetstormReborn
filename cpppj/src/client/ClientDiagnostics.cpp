// 원문 진단 부품을 현재 Client의 Pool/TYPE/Kernel과 실제 CRT 파일에 연결한다.
#include "client/ClientDiagnostics.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientChatWindowGlobals.h" // 초기/종료 경계도 같은 현재 Chat 포인터를 읽는다.
#include "client/UberGump.h" // 진단 문자열을 실제 현재 공통 UI에 전달한다.
#include "client/TraceChat.h"
#include "client/RawSurfaceWorld.h"
#include "client/ObjectDumpDisk.h"
#include "o/RawTerrainBitmap.h"
#include "o/RawProcessSummary.h"
#include "o/SquidProcess.h"
#include <array>
#include <cstdio>
#include <stdexcept>

namespace netstorm::client { // 출력 모드/재귀 깊이는 같은 Client 객체에서만 변경한다.
namespace { // 실제 CRT와 Kernel 포인터만 플랫폼 경계로 변환한다.
FILE* DiagnosticFile(o::ProcessSummaryHandle handle) { if (!handle) throw std::logic_error("프로세스 요약 NULL FILE");return reinterpret_cast<FILE*>(handle); } // 원문 NULL CRT 오류를 빈 성공으로 바꾸지 않는다.
const o::SquidProcess& DiagnosticProcess(o::ProcessSummaryHandle handle) { // 실제 제품의 모든 프로세스가 같은 원문 Form/부모를 가진다.
    const auto* process=dynamic_cast<const o::SquidProcess*>(reinterpret_cast<const o::BaseProcess*>(handle));if (!process) throw std::logic_error("현재 Kernel 프로세스의 실제 Form/부모 없음");return *process;
}
// 소진과 개발자 F7은 같은 프로세스 요약 본문을 사용한다. 이 함수는 객체 덤프/trace를 호출하지 않는다.
void WriteClientProcessSummary(const RawSurfaceStorage& surface,const o::Kernel& kernel,const std::filesystem::path& diagnosticsDirectory,bool userPolicy) {
    if (userPolicy) platform::UserDataRoot::EnsureDirectory(diagnosticsDirectory);
    o::ProcessSummaryHooks summary;summary.slot=[&kernel](auto slot) { return reinterpret_cast<o::ProcessSummaryHandle>(kernel.Get(slot)); };
    summary.form=[](auto handle) { return std::uint32_t(DiagnosticProcess(handle).Form().value); };summary.parent=[](auto handle) { return std::int32_t(DiagnosticProcess(handle).Parent().value); };
    summary.open=[&diagnosticsDirectory,userPolicy](auto name,auto mode) { // 원문 상대 FILE과5차 사용자 Diagnostics 정책을 기존 본문 그대로 공유한다.
        const auto path=userPolicy ? diagnosticsDirectory/std::filesystem::path(std::string(name)) : std::filesystem::path(std::string(name));const std::string text(mode);const std::wstring wide(text.begin(),text.end());
        return reinterpret_cast<o::ProcessSummaryHandle>(userPolicy ? ::_wfopen(path.c_str(),wide.c_str()) : std::fopen(std::string(name).c_str(),text.c_str())); };
    summary.write=[](auto handle,std::string_view row) { static_cast<void>(std::fwrite(row.data(),1,row.size(),DiagnosticFile(handle))); };summary.close=[](auto handle) { std::fclose(DiagnosticFile(handle)); };
    o::WriteRawProcessSummary(surface.pool,surface.types,summary);
}
}
// 같은 장기 상세 소유자만 pin하고 실제 물리/Kernel은 선택 위치에서 잠가 Client 종료 뒤의 해제 주소를 읽지 않는다.
void BindClientKernelDiagnostics(const std::shared_ptr<o::Kernel>& kernel,const std::shared_ptr<RawSurfaceStorage>& surface,const std::shared_ptr<ClientGameplayGlobals>& gameplay,const std::filesystem::path& directory,bool userPolicy) {
    if (!kernel || !surface || !gameplay) throw std::invalid_argument("Client Kernel 진단의 실제 소유자 필요");
    o::KernelDiagnostics diagnostics;diagnostics.edition=surface->pool.Edition();
    diagnostics.summary=[weakKernel=std::weak_ptr<o::Kernel>(kernel),weakSurface=std::weak_ptr<RawSurfaceStorage>(surface),directory,userPolicy] { // 복귀/예외까지 두 실제 소유자를 유지한다.
        const auto actualKernel=weakKernel.lock();const auto actualSurface=weakSurface.lock();if (!actualKernel || !actualSurface) throw std::logic_error("Client Kernel 요약의 실제 물리 수명 종료");WriteClientProcessSummary(*actualSurface,*actualKernel,directory,userPolicy);
    };
    diagnostics.detail=[gameplay](std::string_view text) { gameplay->AiDebug().AssertionDetail()=std::string(text); }; //00436d20의 문자열 복사이며 경고창을 새로 만들지 않는다.
    diagnostics.assertion=[weakSurface=std::weak_ptr<RawSurfaceStorage>(surface)](auto expression,auto file,auto line) { const auto actual=weakSurface.lock();if (!actual) throw std::logic_error("Client Kernel 단언의 실제 물리 수명 종료");actual->AssertDisplay(expression,file,line); }; // 소비 위치의 현재 진단을 읽는다.
    kernel->BindDiagnostics(std::move(diagnostics));
}
bool DispatchDiagnosticKey(InputEvent event,const std::function<void()>& dump) { // 원문 누름 관문과 두 수정키를 함께 요구하며 Alt는 별도 금지하지 않는다.
    if ((event.code&0x03ffffffU)!=0x760000U || (event.code&InputCode::kRelease) || (event.code&(InputCode::kShift|InputCode::kControl))!=(InputCode::kShift|InputCode::kControl)) return false;
    if (!dump) throw std::logic_error("개발자 F7 실제 덤프 미연결");dump();return true;
}
ClientTraceGlobals& Client::DiagnosticGlobals() { return *trace_; } // 같은 장기 owner를 빌리며 조회로 상태를 초기화하지 않는다.
TraceLogHooks Client::DiagnosticLogHooks() { //434f40의 현재 관문/색/포인터와 같은 실제 Chat 부모에 연결한다.
    return MakeObjectDumpDiskLogHooks([this](std::string_view text) {
        if (menu_) { menu_->ForwardDiagnosticChat(trace_->traceToChat,text);return; }
        TraceChatHooks hooks;hooks.color=[this] { return screen_ ? screen_->Colors().named.at(9).index : 0U; }; // 원문 화면 초기화 전의 색 전역은 BSS0이다.
        hooks.currentChat=[this] { return chatWindowGlobals_->window.current; };
        hooks.append=[](auto,auto,auto) { throw std::logic_error("trace Chat 현재 포인터의 실제 UI 소유자 없음"); }; // 실제 포인터가 남아 있는데 UI가 없으면 가짜 성공으로 넘기지 않는다.
        ForwardTraceToChat(trace_->traceToChat,text,hooks);
    });
}
void Client::WriteDiagnosticLog(std::int32_t level,std::string_view text,std::size_t prefixBytes) { WriteRawTraceLog(trace_->log,level,text,prefixBytes,DiagnosticLogHooks()); } // 스크린샷과 선택 trace도 같은 현재 필터/FILE을 쓴다.
void Client::TraceObject(std::uint32_t sid) { // 현재 물리 소유자를 고정하지만 count/검사/메타맵/권한은 같은 실제 주소를 빌린다.
    if (!sid) return; // 원문SID0은 Pool 초기화 여부나 로그 장치조차 조회하지 않는다.
    const auto surface=SurfaceStorage();const auto gameplay=GameplayGlobals();
    const o::ObjectTraceGlobals globals{trace_->depth,surface->pool.CapacityValue(),gameplay->finder.checkingDead,surface->ownerMode.challenge,gameplay->Boss()};
    TraceRawObjectToLog(surface->pool,surface->types,trace_->suppress,globals,trace_->log,DiagnosticLogHooks(),sid,[] { throw std::logic_error("원문 객체 trace dead 부모 진단"); });
}
void Client::DumpDiagnostics() { //004437f0은 실제 단일RET이며 이어서004d6020→00471890을 호출한다.
    const auto surface=SurfaceStorage();const auto gameplay=GameplayGlobals();
    const ObjectDumpGlobals globals{trace_->suppress,surface->pool.CapacityValue(),gameplay->GameAux()->TerrainType(),gameplay->TerrainBitmaps().fringeType,gameplay->GameAux()->EdgeFarmType()};
    const auto diagnosticsDirectory=UserData() ? UserData()->Diagnostics() : GameDirectory(); // 5차: squids.txt/procSumm.txt는 설치 폴더·CWD가 아닌 사용자 Diagnostics에 쓴다.
    if (UserData()) platform::UserDataRoot::EnsureDirectory(diagnosticsDirectory);
    const auto hooks=MakeObjectDumpDiskHooks(diagnosticsDirectory,[this](auto sid) { TraceObject(sid); },[] { throw std::logic_error("원문 객체 덤프 trace FILE 열기 진단"); },&Files()); // 원문의 빈 prefix/재읽기/재쓰기는 실제 Client의 같은10칸을 사용하고 OS 상대 열기만 가상 게임 CWD로 대체한다.
    std::array<std::uint8_t,24> file{};DumpRawObjects(surface->pool,surface->types,trace_->log,globals,DiagnosticLogHooks(),hooks,file);
    WriteClientProcessSummary(*surface,GetKernel(),diagnosticsDirectory,UserData().has_value());
}
}
