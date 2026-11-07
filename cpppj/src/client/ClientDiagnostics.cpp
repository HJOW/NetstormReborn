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
#include <windows.h> // Windows10/11의 유니코드 시스템 경로와 버전 리소스 조회 경계다.
#include <winver.h>
#include <bit>
#include <vector>
#include <array>
#include <cstdio>
#include <cstring> // 버전 리소스의 고정 크기를 정렬과 무관하게 복사한다.
#include <stdexcept>

namespace netstorm::client { // 출력 모드/재귀 깊이는 같은 Client 객체에서만 변경한다.
// 4429e0은 실제 xor eax,eax/ret이므로 원문 실패 두 줄 뒤에만 DLL의 버전 리소스를 읽는다.
void QueryDeveloperDirectDraw(const std::function<void(std::string_view)>& log) {
    if (!log) throw std::invalid_argument("개발자 DirectDraw 조회 로그 미연결");log("You don't have DirectDraw installed, or some error occured during\n");log("the query.\n");
    VS_FIXEDFILEINFO version{}; // 원문 세 getter의 자료 부재는 각각 두DWORD0이며 실제 OS 파일 생성 시각과 구별한다.
    std::array<wchar_t,32768> directory{};const auto length=GetSystemDirectoryW(directory.data(),static_cast<UINT>(directory.size())); // ANSI 대신 W 경로를 써 Windows10/11의 시스템 경로를 보존한다.
    if (length && length<directory.size()) {
        const auto path=std::filesystem::path(directory.data())/L"ddraw.dll";DWORD unused{};const auto size=GetFileVersionInfoSizeW(path.c_str(),&unused);
        if (size) {
            std::vector<std::uint8_t> bytes(size); // 생성자/소멸자의 임시 버전 리소스를 호출 수명 안에서만 소유한다.
            if (GetFileVersionInfoW(path.c_str(),0,size,bytes.data())) { void* data{};UINT count{};if (VerQueryValueW(bytes.data(),L"\\",&data,&count) && data && count>=sizeof(version)) std::memcpy(&version,data,sizeof(version)); } // 손상 리소스의 범위 밖 포인터 접근 자체는 호스트에서 합성하지 않는다.
        }
    }
    log("Information for DDRAW.DLL:\n");
    const auto word=[](DWORD value) { return static_cast<int>(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(value))); }; // 원문 %d의 signed WORD 승격이다.
    const auto printVersion=[&](std::string_view label,DWORD high,DWORD low) { std::array<char,128> text{};const auto count=std::snprintf(text.data(),text.size(),"%s Version: %d.%d.%d.%d\n",std::string(label).c_str(),word(high>>16),word(high),word(low>>16),word(low));if (count<0 || static_cast<std::size_t>(count)>=text.size()) throw std::length_error("개발자 DLL 버전 로그 범위");log({text.data(),static_cast<std::size_t>(count)}); }; // 고정 원문 두 라벨의 버전을 동일 서식으로 전달한다.
    printVersion("Product",version.dwProductVersionMS,version.dwProductVersionLS);printVersion("File",version.dwFileVersionMS,version.dwFileVersionLS);
    const auto date=(std::uint64_t(version.dwFileDateMS)<<32)|version.dwFileDateLS;std::array<char,64> text{};const auto count=std::snprintf(text.data(),text.size(),"Creation Date: %llx\n",static_cast<unsigned long long>(date));if (count<0 || static_cast<std::size_t>(count)>=text.size()) throw std::length_error("개발자 DLL 날짜 로그 범위");log({text.data(),static_cast<std::size_t>(count)}); //4ddfe0의 VS_FIXEDFILEINFO 날짜 두DWORD를64비트16진수로 출력한다.
}

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
// 495330/495460/4954f0의 공통 기록 서식이며 원문 stack 이름 영역을 넘기는 복사는 호스트에서 거부한다.
void WriteDeveloperBuildRecord(Client& client,std::string_view name,float x,float y,std::uint32_t argument,std::uint32_t rotation,float delta) {
    if (name.size()>=20 || name.find('\0')!=std::string_view::npos) throw std::length_error("건설 기록의 원문20BYTE 이름 범위");auto& state=client.DiagnosticGlobals();if (!state.buildRecord) throw std::logic_error("활성 건설 기록의 실제 FILE 없음");
    static_cast<void>(std::fprintf(state.buildRecord.get(),"%s (%f, %f) canon=%d rotation=%d delta=%f\n",std::string(name).c_str(),static_cast<double>(x),static_cast<double>(y),std::bit_cast<std::int32_t>(argument),std::bit_cast<std::int32_t>(rotation),static_cast<double>(delta))); // 원문 fprintf 실패 반환은 소비하지 않고 다음 시각 갱신을 계속한다.
}
void RecordDeveloperBuildPlacement(Client& client,std::uint32_t token,std::string_view type,float x,float y,std::uint32_t argument,std::uint32_t rotation) {
    if (!token || token!=client.GameplayGlobals()->cursor.recorder) throw std::logic_error("건설 기록의 현재 recorder 식별자 불일치");auto& state=client.DiagnosticGlobals();const auto now=static_cast<float>(client.Time().game);const auto delta=static_cast<float>(now-state.lastBuildRecordTime);WriteDeveloperBuildRecord(client,type,x,y,argument,rotation,delta);state.lastBuildRecordTime=static_cast<float>(client.Time().game); // 출력 뒤 현재 시각을 다시 읽어 단정도로 저장한다.
}
void RecordDeveloperBuildLabel(Client& client,std::string_view label) {
    const auto point=client.SurfaceStorage()->boardTargetState.invalid;WriteDeveloperBuildRecord(client,"RL"+std::string(label),point.x,point.y,0xffffffffU,0xffffffffU,0.0F); // 실제 RL 접두와 현재 실패점/두-1/기한0이며 마지막 배치 시각은 바꾸지 않는다.
}
bool DispatchBuildRecordKey(Client& client,InputEvent event) {
    if ((event.code&0x03ffffffU)!=0x750000U || (event.code&InputCode::kRelease) || client.Paused() || !client.GameplayGlobals()->player.special || !(event.code&InputCode::kControl)) return false;auto& cursor=client.GameplayGlobals()->cursor;auto& state=client.DiagnosticGlobals();
    if (cursor.recording) { client.WriteDiagnosticLog(29,"closing record file\n");state.buildRecord.reset();cursor.recording=0;return true; } // 원문 로그→닫기→플래그0 순서다.
    if (!cursor.recorder) throw std::logic_error("건설 기록의 실제 초기화495860 이전 입력");state.lastBuildRecordTime=static_cast<float>(client.Time().game);
    const auto root=client.UserData() ? *client.UserData() : platform::UserDataRoot::Resolve();const auto directory=root.Captures()/L"BuildRecords"/L"d";platform::UserDataRoot::EnsureDirectory(directory);std::filesystem::path path; // 5차 정책: 자동 생성 rec도 설치 폴더가 아닌 사용자 홈에 쓰며 원문의 d/번호.rec 규칙을 유지한다.
    for (std::uint32_t number=0;;++number) { path=directory/(std::to_string(std::bit_cast<std::int32_t>(number))+".rec");if (!std::filesystem::exists(path)) break; } // 원문0부터의 첫 빈 파일을 고르고 기존 기록은 덮어쓰지 않는다.
    FILE* file{};if (_wfopen_s(&file,path.c_str(),L"w") || !file) throw std::runtime_error("건설 기록 파일 열기 실패");state.buildRecord=std::shared_ptr<FILE>(file,[](FILE* current) { std::fclose(current); });const auto bytes=path.u8string();client.WriteDiagnosticLog(29,"recording to "+std::string(reinterpret_cast<const char*>(bytes.data()),bytes.size())+"\n");cursor.recording=1;return true; // 열기→현재 로그→활성1이며 텍스트 모드의 원문 CRLF를 유지한다.
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
void Client::DumpObjectDiagnostics() { //004d6020/CD0041f3e0의 객체 덤프 본문만 공유한다.
    const auto surface=SurfaceStorage();const auto gameplay=GameplayGlobals();
    const ObjectDumpGlobals globals{trace_->suppress,surface->pool.CapacityValue(),gameplay->GameAux()->TerrainType(),gameplay->TerrainBitmaps().fringeType,gameplay->GameAux()->EdgeFarmType()};
    const auto diagnosticsDirectory=UserData() ? UserData()->Diagnostics() : GameDirectory(); // 5차: squids.txt/procSumm.txt는 설치 폴더·CWD가 아닌 사용자 Diagnostics에 쓴다.
    if (UserData()) platform::UserDataRoot::EnsureDirectory(diagnosticsDirectory);
    const auto hooks=MakeObjectDumpDiskHooks(diagnosticsDirectory,[this](auto sid) { TraceObject(sid); },[] { throw std::logic_error("원문 객체 덤프 trace FILE 열기 진단"); },&Files()); // 원문의 빈 prefix/재읽기/재쓰기는 실제 Client의 같은10칸을 사용하고 OS 상대 열기만 가상 게임 CWD로 대체한다.
    std::array<std::uint8_t,24> file{};DumpRawObjects(surface->pool,surface->types,trace_->log,globals,DiagnosticLogHooks(),hooks,file);
}
void Client::DumpDiagnostics() { //004437f0은 실제 단일RET이며 이어서004d6020→00471890을 호출한다.
    DumpObjectDiagnostics();const auto surface=SurfaceStorage();const auto directory=UserData() ? UserData()->Diagnostics() : GameDirectory();WriteClientProcessSummary(*surface,GetKernel(),directory,UserData().has_value());
}
}
