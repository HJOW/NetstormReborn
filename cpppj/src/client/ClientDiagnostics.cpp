// 원문 진단 부품을 현재 Client의 Pool/TYPE/Kernel과 실제 CRT 파일에 연결한다.
#include "client/ClientDiagnostics.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
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
}
bool DispatchDiagnosticKey(InputEvent event,const std::function<void()>& dump) { // 원문 누름 관문과 두 수정키를 함께 요구하며 Alt는 별도 금지하지 않는다.
    if ((event.code&0x03ffffffU)!=0x760000U || (event.code&InputCode::kRelease) || (event.code&(InputCode::kShift|InputCode::kControl))!=(InputCode::kShift|InputCode::kControl)) return false;
    if (!dump) throw std::logic_error("개발자 F7 실제 덤프 미연결");dump();return true;
}
ClientTraceGlobals& Client::DiagnosticGlobals() { return *trace_; } // 같은 장기 owner를 빌리며 조회로 상태를 초기화하지 않는다.
TraceLogHooks Client::DiagnosticLogHooks() { // 434f40은 시작에서0을 쓰는 같은 관문이며 현재 정상 제품에서는 전달을 생략한다.
    return MakeObjectDumpDiskLogHooks([this](std::string_view) { if (trace_->traceToChat) throw std::logic_error("진단 traceToChat의 42f190 스타일 전달 미복원"); });
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
    const auto hooks=MakeObjectDumpDiskHooks(GameDirectory(),[this](auto sid) { TraceObject(sid); },[] { throw std::logic_error("원문 객체 덤프 trace FILE 열기 진단"); }); // 원문의 빈 설치 prefix는 같은 게임 폴더다. CRT 프로세스 요약의 상대경로와 구별한다.
    std::array<std::uint8_t,24> file{};DumpRawObjects(surface->pool,surface->types,trace_->log,globals,DiagnosticLogHooks(),hooks,file);
    o::ProcessSummaryHooks summary;summary.slot=[this](auto slot) { return reinterpret_cast<o::ProcessSummaryHandle>(GetKernel().Get(slot)); };
    summary.form=[](auto handle) { return std::uint32_t(DiagnosticProcess(handle).Form().value); };summary.parent=[](auto handle) { return std::int32_t(DiagnosticProcess(handle).Parent().value); };
    summary.open=[](auto name,auto mode) { return reinterpret_cast<o::ProcessSummaryHandle>(std::fopen(std::string(name).c_str(),std::string(mode).c_str())); }; // 원문은 고정ASCII procSumm.txt/wb를 현재 작업 폴더에서 연다.
    summary.write=[](auto handle,std::string_view row) { static_cast<void>(std::fwrite(row.data(),1,row.size(),DiagnosticFile(handle))); };summary.close=[](auto handle) { std::fclose(DiagnosticFile(handle)); };
    o::WriteRawProcessSummary(surface->pool,surface->types,summary);
}
}
