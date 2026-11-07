// R의 현재 rootservers 문자열 표/체크섬과 실제 파일 재로드를 연결한다.
#pragma once
#include "o/RawFileName.h"
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::root { //004d0e7c의 실제 현재 문자열 표를 C++ 소유권으로 보관한다.
class SharedFileStorage;struct MainLoopHooks; // 현재 사용자 홈/작업 경로 장치와 실제 감시 부모다.
class ServerList final { // 이 owner와 checksum 참조는 Root 응답/메인 루프보다 오래 살아야 한다.
public:
    ServerList(std::uint32_t& checksum,o::RawFileName& names,SharedFileStorage& storage); // Native715a0 flags7/초기20칸을 구성하며 파일/소켓을 열지 않는다.
    ServerList(const ServerList&)=delete;ServerList& operator=(const ServerList&)=delete; // 문자열 주소/현재 표와 callback this를 복제하지 않는다.
    void Reload();void Save(); //471f20/71f50의 실제 목록 읽기/사용자 Saves 쓰기다. 재로드는 기존 표를 비우고 읽는다.
    void Bind(MainLoopHooks& hooks); // 메인 루프의 실제 reloadRootServers를 같은 표/체크섬에 연결한다.
    void Clear();bool Add(std::string_view text,bool recalculate=false);void Remove(std::string_view text,bool recalculate=false);void Replace(std::int32_t index,std::string_view text); //71670/71a00/71ae0/71e40의 현재 순서·중복/재계산 조건이다. Replace는 체크섬을 갱신하지 않는다.
    std::int32_t Find(std::string_view text,bool partial=false) const; //71b80의 전체/부분 조회와 flag2 대소문자 규칙이다. 부분 조회는 저장 문자열만 소문자로 바꾼다.
    const std::string* Entry(std::int32_t index) const; //72160의 실제 현재 행 또는NULL이다. Add 중에는 주소가 유지되고 해당 행 제거/교체·Clear 때 무효화된다.
    std::uint32_t Count() const { return static_cast<std::uint32_t>(entries_.size()); } //71f80의 현재 행 수다.
    std::uint32_t& Flags() { return flags_; } // 현재 stripCRLF1/case2/unique4를 빌린다. 재로드/clear로 초기화하지 않는다.
    std::uint32_t Capacity() const { return capacity_; } // 원문의 논리20→30→45 확장 값이며 호스트 allocator 주소/용량 ABI가 아니다.
    std::uint32_t Recalculate();std::int32_t BufferLength(std::int32_t separatorBytes) const; //71d00의 unsigned 바이트+1 합과71db0의 strlen+separator 합이다.
    static void ParseRow(std::string_view row,std::span<char> name={},std::span<char> description={},std::int32_t* value=nullptr); //71fa0의 고정 세미콜론 패턴/선택 출력·지정 버퍼-1/값79BYTE atoi다.
    void Decode(std::int32_t index,std::span<char> name={},std::span<char> description={},std::int32_t* value=nullptr) const; //720f0처럼 유효한 비NULL 행에서만 ParseRow를 호출한다.
    void Upsert(std::string_view row,bool preserveValue=false);void SetValue(std::int32_t index,std::int32_t value); //72250/721b0의 소문자 이름 부분검색·새행 재계산/기존행 비재계산을 보존한다.
private:
    std::uint32_t& checksum_;o::RawFileName& names_;SharedFileStorage& storage_; // RootDirectory 옵션의 같은004d0e8c와 실제 이름 고리/파일 장치다.
    std::uint32_t flags_{7},capacity_{20};std::vector<std::unique_ptr<std::string>> entries_; //714... 표의 현재 순서/NULL 가능 행과 안정된 각 문자열 주소다.
};
}
