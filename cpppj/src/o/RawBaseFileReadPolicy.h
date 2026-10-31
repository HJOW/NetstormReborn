// PATCH0041b4c0의 모드0 읽기 조회 순서와459c60의 root 조합을 공통 파일 계층에 제공한다.
#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// UI/FILE 객체를 새로 소유하지 않고 선택된 실제 자료 조회만 빌리는 공통 정책이다.
namespace netstorm::o {
// 원문이 실제로 방문하는 다섯 읽기 위치다. 장치 경로 실패 뒤에는 기본 디스크를 다시 방문하지 않는다.
enum class BaseFileReadSite : std::uint32_t {
    DirectDisk=0, //원래 장치 경로를 직접 연다.
    PrimaryDisk=1, //기본 root와 상대 이름을 조합한다.
    PrimaryArchive=2, //기본 TAFF 목록에서 같은 이름을 찾는다.
    SecondaryDisk=3, //보조 root와 현재 조회 이름을 조합한다.
    SecondaryArchive=4 //보조 TAFF 목록에서 현재 조회 이름을 찾는다.
};
// 빈 optional만 미발견이다. 빈 vector는 정상 파일이며 아카이브 열기 실패 예외는 하위에서 그대로 전파한다.
using BaseFileReadLookup=std::function<std::optional<std::vector<std::uint8_t>>(BaseFileReadSite,std::string_view)>;
//459c60(-2,root,-3,name,0)의 NUL/구분자/끝 구분자 조합이다. 실제 Help도 같은 몸체를 쓴다.
std::string JoinBaseFileRoot(std::string_view root,std::string_view name);
//0041b4c0의 읽기0 조회 부분이다. 미발견은 기존 TryRead 계약의nullopt로 반환하며 FILE/예외 객체 ABI를 합성하지 않는다.
std::optional<std::vector<std::uint8_t>> ReadBaseFileByPolicy(std::string_view name,std::string_view primaryRoot,const BaseFileReadLookup& lookup);
}
