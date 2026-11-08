//원문69h의 현재 타입 사용 목록 누적·기술 contained 보충과 송신 관찰기만 제공한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o { //Client 전용 실제 등록 caller가 장기 목록/현재 기술표시 장치를 빌린다.
class RawPacketDispatch; //동일 Client 공유표에만 연결하며 Root 기본표를 변경하지 않는다.
using TechnologyTypesIndicate=std::function<void(std::uint32_t)>; //현재 목록의 unsigned BYTE를 DWORD로 올려 항목마다 실제 부모에 전달한다.
using TechnologyTypesAssertion=std::function<void(std::string_view,std::string_view,std::uint32_t)>; //진단 정상 복귀 뒤 같은 현재 패킷을 다시 읽는다.
void AddTechnologyTypes(std::span<const std::uint8_t> input,std::span<std::uint8_t,256> typeUse); //0041c340의 첫NUL/중복/첫빈칸 순서이며 앞선 부분 쓰기를 되돌리지 않는다.
void ReceiveTechnologyTypesPacket(std::span<const std::uint8_t> packet,std::span<std::uint8_t,256> typeUse,const TechnologyTypesIndicate& indicate); //004b49e0:payload+5 합집합 뒤 기존/신규 전체 현재 목록을 다시 순회한다.
void ObserveTechnologyTypesPacket(std::span<std::uint8_t> packet,std::uint32_t sending,const TechnologyTypesAssertion& assertion); //0040af10의 송신방향 BYTE+4 진단/재조회/선언WORD+5만 수행한다.
void RegisterTechnologyTypesPacket(RawPacketDispatch& dispatch,std::span<std::uint8_t,256> typeUse,TechnologyTypesIndicate indicate,TechnologyTypesAssertion assertion); //69h/version1/policy0/가변ffff·mutable관찰기·nullable직접수신을 같은 슬롯에 설치한다.
}
