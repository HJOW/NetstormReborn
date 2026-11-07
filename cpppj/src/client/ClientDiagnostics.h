// 선택 trace와 개발자 F7 덤프가 공유하는 Client 수명의 원문 진단 전역이다.
#pragma once
#include "client/RawTraceLog.h"
#include "client/InputEvent.h"
#include <cstdio> // 기록기의 실제 FILE 수명을 Client 종료까지 보유한다.
#include <filesystem>
#include <memory>

namespace netstorm::o { class Kernel; } // 실제 Client Kernel을 빌리며 새 실행기를 만들지 않는다.

namespace netstorm::client { // 장기 로그 상태와 실제 입력 관문만 선언하며 월드를 별도 소유하지 않는다.
class Client; // 실제 현재 기록/시간/경로 장치만 빌리고 새 게임 세션을 만들지 않는다.
class RawSurfaceStorage; // 현재 Pool/TYPE/진단 장치의 실제 소유자다.
class ClientGameplayGlobals; // 상세 문자열의 장기 소유권을 Kernel 장치 수명까지 유지한다.
// 시작/창 없는 집중 검사 모두 같은 요약 본문·사용자 경로·현재 단언을 연결한다. Kernel/Surface는 약한 참조로 순환 소유를 피한다.
void BindClientKernelDiagnostics(const std::shared_ptr<o::Kernel>& kernel,const std::shared_ptr<RawSurfaceStorage>& surface,const std::shared_ptr<ClientGameplayGlobals>& gameplay,const std::filesystem::path& directory,bool userPolicy);
// queryDirectDraw의 원문 로그 순서와 시스템 ddraw.dll 버전 리소스만 조회한다. DLL/DirectDraw 장치를 초기화하지 않는다.
void QueryDeveloperDirectDraw(const std::function<void(std::string_view)>& log);
// 원문 개발자 Ctrl+F6의 현재 special/정지 관문 뒤 기록 시작·종료를 수행한다.
bool DispatchBuildRecordKey(Client& client,InputEvent event);
// 같은 실제 기록 파일에 RL 라벨 또는 배치 정보 한 행을 쓰며 현재 cursor.recording 관문은 호출자가 유지한다.
void RecordDeveloperBuildLabel(Client& client,std::string_view label);
void RecordDeveloperBuildPlacement(Client& client,std::uint32_t token,std::string_view type,float x,float y,std::uint32_t argument,std::uint32_t rotation);
struct ClientBuildReplayGlobals; // 실제 재생 전역은 Client 초기화 때 한 번만 만들고 Session에 같은 소유자를 빌린다.
struct ClientTraceGlobals {
    std::uint32_t heapTrace{},heapAllocated{}; //0054dab0/00557dfc의 BSS0이며 Session 교체와 무관한 원문 프레임 진단이다. 빈437f0에 호스트 힙 집계를 추가하지 않는다.
    std::shared_ptr<FILE> buildRecord; //495860의 실제 기록 stream이며 Client 반납 때 fclose하고 Session 교체에서 유지한다.
    float lastBuildRecordTime{}; // 실제 기록기+18의 단정도 시각이며 시작/배치 기록만 갱신하고 라벨은 바꾸지 않는다.
    std::shared_ptr<ClientBuildReplayGlobals> buildReplay; //59a7e0/e4의 현재 재생 객체와 playing을 같은 Client 수명으로 보유한다.
    std::shared_ptr<std::uint8_t> buildReplayLifetime{std::make_shared<std::uint8_t>(std::uint8_t{0})}; // 외부 Kernel의 Client 반납 후 호출을 거부하는 C++ 수명 표식이며 원문 게임 값이 아니다.
    TraceLogState log; //00542658/005ca8d0 이후의 같은 FILE/버퍼/들여쓰기/필터다.
    std::uint32_t depth{},suppress{},traceToChat{}; //0055a4a0/005c8454/00531854의 최초BSS0이며 미션 전환 때 보존한다.
};
// Guide/Gump 뒤·console/정지/일반F7 전에 원래 세 호출 경계를 실행한다. 단일RET은 별도 효과가 없다.
bool DispatchDiagnosticKey(InputEvent event,const std::function<void()>& dump);
}
