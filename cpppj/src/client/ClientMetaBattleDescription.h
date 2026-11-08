// 실제 CS 설명의 로그·현재 객체 가상88·특정 전투 표시 갱신을 같은 Client에 연결한다.
#pragma once
#include "client/NativeMetaBattleDescription.h"
namespace netstorm::client { //장치 생성만으로 World/CS/화면/통신을 실행하지 않는다.
class Client; //이 실제 소유자는 반환 장치의 동기 사용보다 오래 살아야 한다.
class ScriptStringRing; //HTML 문자열의 기존80칸 수명을 실제 소비 시점에 조회한다.
inline constexpr std::uint32_t kMetaDescriptionAddressTag=0x80000000U; //양수 ScriptStringRing 토큰과 CS 논리 주소를 구별하는 호스트 인자 표식이며 Native 주소값이 아니다.
std::uint32_t EncodeClientMetaDescriptionAddress(const ClientChallengeGlobals& challenge,std::uint32_t offset); //현재 CS+offset을 같은32비트 큐 인자로 표식화하며 문자열 고리를 소비하지 않는다.
std::optional<std::string_view> ResolveClientMetaDescriptionText(const ClientChallengeGlobals& challenge,const ScriptStringRing& strings,std::uint32_t parameter); //NULL·실제 고리 문자열·표식된 현재 CS를 구별하고 소비 때 범위/NUL/수명을 검사한다.
NativeMetaBattleDescriptionHooks MakeClientMetaBattleDescriptionHooks(Client& client); //선택된 원문 효과 위치에서만 현재 물리와 Renderer를 조회한다.
}
