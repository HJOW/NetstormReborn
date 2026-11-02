// 원문 ChangePal/ChangePriority/SetFortType의 현재 설정·공유 DWORD·State/Notify 순서를 보존한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::client { // Guide/Gump/뗌 뒤이며 일반 Console/정지 검사보다 앞인 세 일반 명령이다.
inline constexpr std::uint32_t kChangePalCommand=0x02000029U,kChangePriorityCommand=0x0200008aU,kSetFortTypeCommand=0x020000c7U; //54279c/54291c/542a10의 원문 등록값이다.
std::uint32_t ConfigurationCommandScriptCode(std::string_view name); //실제 세 이름만 ASCII 대소문자를 무시하고 조회한다.
bool IsConfigurationCommand(std::uint32_t code); //원문 전체 명령번호를 비교하며 키의 낮은 BYTE로 대신하지 않는다.
struct NativeThreadPriorityRequest { //WinMain의 raw wanted와 현재 OS 등급 비교 결과이며 OS 장치 호출은 소유하지 않는다.
    std::int32_t wanted{},currentLevel{},priority{}; //원문 ESI 요청/비교용 EAX등급/실제 SetThreadPriority 인자다.
    bool change{}; //raw wanted와 현재 정규화 등급이 다를 때만 실제 OS 설정을 요청한다.
};
NativeThreadPriorityRequest PlanNativeThreadPriority(std::int32_t wanted,std::int32_t currentOs); //00439983 국소 부모의 비교/요청값을 보존하며 범위 밖 wanted를 clamp하지 않는다.
struct ConfigurationCommandReads { //외부 효과 뒤 현재 값을 다시 읽을 원문 전역들이다.
    std::function<std::uint32_t()> battle,challenge,editor,owner; //594fbc/594fc8/5c85a4/540cac를 선택된 원문 위치에서만 조회한다.
    std::int32_t& highPriority; //54db2c의 같은 현재 signed DWORD이며 설정 reader와 메인 루프도 이 주소를 사용한다.
};
struct ConfigurationCommandHooks { //해당 명령이 도달한 실제 부모만 필수로 요구한다.
    std::function<std::string_view(std::uint32_t)> text; //현재 ScriptStringRing의 두 번째 인자 문자열을 소비 직전에 조회한다.
    std::function<void(std::string_view,std::string_view)> set; //4414d0의 실제 Config Set/동기 알림이다. 반환 bool은 원문처럼 사용하지 않는다.
    std::function<std::string(std::string_view)> get; //4410f0(key,0,0,0)의 현재 치환 결과를 소유 문자열로 얻는다.
    std::function<void(std::string_view)> palette; //현재 설정 이름을 기존004b6100의 실제 reader/구름/팔레트/색 표 부모에 전달한다.
    std::function<void(std::int32_t)> thread; //435c80이 선택한 Win32 priority 0/1/15를 현재 스레드에 요청한다. OS false 반환은 무시한다.
    std::function<void(std::string_view)> config; //441580의 실제 설정문 파서다. OS 호출 뒤 공유 DWORD를 재조회한 signed%d를 전달한다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> state; //4bd550의 현재 State28BYTE이며 getter가 반환한 새 할당을 쓴다.
    std::function<void(std::uint32_t)> notify; //현재 owner를 다시 읽어4bd540→4bd360의 실제 저장/자연 반환 부모를 호출한다.
    std::function<void(std::uint32_t)> assertion; //SetFortType의 UserInput.cpp line2012 진단이며 반환하면 쓰기를 계속한다.
};
bool DispatchConfigurationCommandInput(InputEvent event,ConfigurationCommandReads reads,const ConfigurationCommandHooks& hooks); //대상 뗌은 효과 없이 true이며 다른 명령은 false다.
}
