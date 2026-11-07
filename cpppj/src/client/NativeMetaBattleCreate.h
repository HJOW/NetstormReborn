// Ch.cpp의전투기록/그림객체생성과23h추가수신의실제부모를복원한다.
#pragma once
#include "client/ClientChallengeGlobals.h"
#include <array>
#include <functional>
#include <string_view>

namespace netstorm::client { //원문식별자의6유효BYTE와호출스택8BYTE구조체를구별한다.
struct NativeMetaBattleCreateData {
    std::uint32_t id{}; //생성인자id를현재기록+4에저장한다.
    std::array<std::uint8_t,6> identity; //호출스택두DWORD에들어온식별자의유효6BYTE이며기록+8/+c에4/2로쓴다.
    std::array<std::uint32_t,2> position; //원문x/y float32의비트이며단계별기록재조회전까지형변환하지않는다.
    std::uint32_t existing{}; //마지막스택DWORD다.0일때만새전투Rig애니메이션을요청한다.
};
struct NativeMetaBattleCreateHooks {
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //Ch.cpp1d1의실제진단이며복귀시기록쓰기를계속한다.
    std::function<void(std::string_view)> log; //수준9의이미존재전투진단이다.
    std::function<std::uint32_t()> battleType; //같은현재54120c를객체생성직전에읽는다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> create; //현재TYPE/flags2의실제4af530공장이같은Pool의SID를반환한다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> raw; //생성한같은50BYTE객체의현재물리다.
    std::function<void(std::uint32_t,std::uint32_t)> setKind,owner; //현재QA WORD의4ac220과실제asset owner0의4adca0이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t)> pop; //생성객체의가상+90(현재x/y/0)이다.
    std::function<void(std::uint32_t,std::uint32_t)> animate; //4194c0의같은객체/지연10 Rig생성이다.기존전투는요구하지않는다.
};
void ConstructNativeMetaBattle(std::span<std::uint8_t> record,NativeMetaBattleCreateData input,const NativeMetaBattleCreateHooks& hooks); //0042b3a0 전체기록쓰기/실제공장·QA/owner/Pop·조건부Rig부모다.논리인자5개중식별자가스택2DWORD를쓴다.
void ReceiveNativeMetaBattleAdd(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> packet,const NativeMetaBattleCreateHooks& hooks); //0042c670 전체:이미active면진단반환·23h필드/식별자6BYTE·packet5&2→같은생성자다.
}
