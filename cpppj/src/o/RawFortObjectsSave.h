// 10.78 전체 Objects 부모004be430의 Temp·현재 manager·버전 쓰기 순서를 보존한다.
#pragma once
#include "o/RawMemStream.h"
#include <string_view>

namespace netstorm::o {
// 원본 Template.cpp957/958 및 MemStream36행의 진단 후 계속 지점을 구별한다.
enum class FortObjectsSaveAssertion { MissingTemplate,NotMine,WriteCapacity }; // 진단 뒤 현재 저장소의 변경을 허용한다.
// 실제 클라이언트·Session의 장기 전역을 빌리며 callback 뒤 값도 같은 주소에서 읽는다.
struct FortObjectsSaveGlobals {
    const std::uint32_t& templateOwner; // 00540cac: 고정 this 슬롯과 비교하는 현재 선택이다.
    std::uint32_t& version; // 005c89c0: 시작에2를 쓰고 각 새 stream에 현재 하위 BYTE를 쓴다.
    std::function<std::uint32_t()> fort; // 00594fb8:20개 영역이 끝난 뒤 한 번 읽는다.bool현재소유자도임시DWORD참조없이연결한다.
};
// 실제 같은 manager/DataManager와 기존 영역·Chaff writer를 연결하는 명시 하위 경계다.
struct FortObjectsSaveHooks {
    std::function<std::uint32_t(std::int32_t)> manager; // 고정 this 슬롯의 현재 첫 DWORD를 매 원본 위치에서 읽는다.
    std::function<void(FortObjectsSaveAssertion)> assertion; // 도달한 진단만 요구하며 복귀 뒤 현재 값으로 계속한다.
    std::function<std::uint32_t(std::uint32_t,std::string_view)> find; // 0044ce60:현재manager에서Terr 이름을 검색한다.
    std::function<std::uint32_t(std::uint32_t,std::string_view)> create; // 0044d1b0:없는Terr/새Temp/Chaff의 실제 생성/반납 수명이다.
    std::function<std::uint32_t(std::uint32_t,std::string_view,std::int32_t)> select; // 0044ceb0:현재manager의Temp를writable1로선택하고pos0을쓴다.
    std::function<void(std::uint32_t,std::string_view,std::string_view)> rename; // 0044d160(manager,destination,source)의정확한순서다.
    std::function<void(std::uint32_t,std::uint32_t,std::int32_t)> territory; // 004be230(stream,clampedOwner,region)의전체영역순회이며한청크로대체하지않는다.
    std::function<void(std::uint32_t)> chaff; // 004be350(stream)의전체지도Chaff순회다.
    std::function<void(std::uint32_t,const std::function<void(std::span<std::uint8_t>,const MemStreamHooks&)>&)> access; //같은stream16BYTE/물리버퍼의동기빌림이다.검색/선택효과를추가하지않는다.
};
// 고정this슬롯의20영역→조건부Chaff를원문순서로저장한다.전체snapshot/Notify/파일쓰기/UI는호출자의경계다.
void SaveFortObjects(std::int32_t slot,FortObjectsSaveGlobals globals,const FortObjectsSaveHooks& hooks);
}
