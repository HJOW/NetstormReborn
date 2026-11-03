// 원문0041b4c0의 전체 출처 선택 부모를 기존 FILE/부분파일/키 몸체에 연결한다.
#pragma once
#include "o/FileSpecQuery.h"
#include "o/RawBaseFileIO.h"
#include "o/RawFileName.h"
#include <functional>
#include <optional>
#include <span>
#include <string>

namespace netstorm::o { // 원문24BYTE 객체/SEH ABI와 호스트 상태/장치의 수명을 구별한다.
struct BaseFileArchiveOpen { // 41a340의 원래 파일명과 지역변수에 기록하는 세 결과다.
    std::string fileName; // 현재 등록한 실제 TAFF 파일 이름이며 등록 항목보다 오래 필요한 호출은 값으로 소유한다.
    std::int32_t offset{},length{}; // 부분파일 시작/길이의 원문DWORD 비트를 signed32로 해석한다.
    std::uint32_t encrypted{}; //0이 아니면 부분파일 열기 성공 뒤 고정 XOR 키를 설치한다.
};
struct BaseFileOpenPolicyHooks { // 빈 성공이나 추측한 TAFF 경로를 자동 제공하지 않는다.
    std::function<std::string_view()> primaryRoot; // 실제 사용할 때 현재00544d80 문자열을 다시 빌린다.
    std::function<std::string_view()> secondaryRoot; // 보조 디스크에 도달했을 때 현재00544e80을 다시 빌린다.
    std::function<std::optional<BaseFileArchiveOpen>(std::string_view,bool)> archive; // false=기본5450b8/true=보조544f80의 메타데이터 조회이며 내용을 읽지 않는다.
    std::function<void(std::string_view)> unableToOpen; // 원문의 printf/41a0d0/__CxxThrowException 경계다. 반드시 예외를 던지며 정상 복귀하지 않아야 한다.
    FileSpecQueryHooks search; // 원문 FileSpec을 부모 정상/예외 이탈까지 유지할 실제 또는 명시 검색 장치다.
    std::function<std::optional<std::string>(std::string_view,std::string_view)> diskPath; // 명시 ACP 소비자만 root/query를 Unicode OS 경로로 바꾼다. raw고리/TAFF 키는 유지하며 빈 값은 disk 실패다.
};
void OpenBaseFileByPolicy(RawBaseFileState& state,std::span<const std::uint8_t> name,std::uint32_t mode,std::uint32_t create,RawFileName& names,const RawBaseFileIOHooks& io,const BaseFileOpenPolicyHooks& hooks); // NUL 포함 이름 물리를 빌린다. callback 뒤 원래이름/절삭 suffix를 다시 읽고 성공에서만 반환한다. 전체24BYTE/예외256BYTE ABI와 원문 미정 메모리는 합성하지 않는다.
}
