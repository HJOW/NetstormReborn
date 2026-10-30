// 원래 기술 도움말의 설정/경로 가용성과 별도 비대기 실행 부모를 공유한다.
#pragma once
#include "o/RiftType.h"
#include <functional>
#include <optional>
#include <string>
#include <string_view>
namespace netstorm::client { // 같은 실제 메뉴와 명시 원문 관측의 공통 정책이다.
class Client; //현재 Config/등록 TAFF/설치 경로를 빌린다.
struct TechnicalHelpHooks { //도달한 조회/실행만 필수이며 출력의 앞선 변경을 되돌리지 않는다.
    std::function<std::uint32_t(std::string_view,std::string&)> readString; //441300의 두 실제 반환을 bitwise AND한다.
    std::function<std::optional<std::string>(std::string_view)> resolve; //41ad10의 null 실패와 비null 빈 경로를 구별한다.
    std::function<void(std::string_view,std::string_view)> launch; //4dd6b0(program,file,0)의 같은 비대기 실행이다.
};
bool ResolveTechnicalHelp(o::OriginalEdition edition,std::string& exe,std::string& file,const TechnicalHelpHooks& hooks); //435e40의 출력 초기화/두 조회/경로 순서다.
bool TechnicalHelpAvailable(o::OriginalEdition edition,const TechnicalHelpHooks& hooks); //435ef0은 로컬 두 버퍼만 준비하며 실행하지 않는다.
bool LaunchTechnicalHelp(o::OriginalEdition edition,const TechnicalHelpHooks& hooks); //435f10은 경로 성공일 때만 실행하고 OS 반환과 무관하게 가용성을 돌려준다.
struct ExternalFileHooks { //41ad10의 현재 파일/두 TAFF 집합을 명시 경계로 연결한다.
    std::function<bool(std::string_view)> device; //원래 FileSpec 생성/Device의 장치 접두 존재다.
    std::function<std::string(bool,std::string_view)> path; //false=기본544d80, true=보조544e80와 -2/-3 조합이다.
    std::function<bool(std::string_view)> readable; //fopen rb/닫기의 실제 파일 가용성이다.
    std::function<bool(bool,std::string_view)> archive; //false=5450b8, true=544f80의 메타데이터 이름 조회다.
};
std::optional<std::string> ResolveExternalFilePath(std::string_view name,std::uint32_t* source,const ExternalFileHooks& hooks); //장치5/기본디스크1/기본TAFF2/보조디스크3/보조TAFF4 순서다.
std::string JoinExternalFileRoot(std::string_view root,std::string_view name); //실제 -2/root/-3/name/0 호출에서만 사용하는 Filespec 조합이다.
bool ClientTechnicalHelpAvailable(Client& client); //현재 메뉴의 실제 경로 존재까지 조회한다.
bool LaunchClientTechnicalHelp(Client& client); //같은 현재 설정을 다시 읽어 선택된 실행만 요청한다.
bool DispatchTechnicalHelpInput(std::uint32_t event,const std::function<void()>& launch); //일반 명령02000049의 누름만 실행하고 release도 소비한다.
}
