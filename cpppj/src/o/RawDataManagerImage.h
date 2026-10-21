// 10.78 파일 이미지의 같은 manager/MarkStream20/이름 표를 실제 섹션 목록에 읽는다.
#pragma once
#include "o/RawDataManagerResize.h"
namespace netstorm::o {
// 진단 뒤 계속하는 원본 위치를 구별하며 물리 범위 초과는 별도로 거부한다.
enum class DataManagerImageAssertion { SourcePosition,MissingMark,MarkLength,MarkPosition,NameSeparator };
// 가상 이름 표와 같은 노드/버퍼의 실제 경계를 제공한다.
struct DataManagerImageHooks {
    DataManagerResizeHooks streams; // 생성/검색/제거/압축은 같은 실제 노드와 버퍼를 빌린다.
    std::function<std::span<std::uint8_t>()> names; // manager 가상+4가 반환한 같은 변경 가능한 SNART 이름 표다.
    std::function<void(DataManagerImageAssertion)> assertion; // 해당 원본 진단이 선택된 위치에서만 호출한다.
};
// 0044cdb0: 이전 mark 길이 확인→현재 pos를 mark로 기록→signed pos/end 진단→pos+2다.
void AdvanceDataManagerMark(std::span<std::uint8_t> source,const DataManagerImageHooks& hooks);
// 0044d380: 이름 표 순회/생성/복사/빈 섹션 유지/기존 제거/추가/mark 진행을 수행한다.
void ReadDataManagerImage(std::span<std::uint8_t> manager,std::span<std::uint8_t> source,const DataManagerImageHooks& hooks);

// Template manager +c의 실제 서명 판정에 필요한 현재 메모리/전역/파일 경계다.
struct TemplateFileSignatureHooks {
    std::function<std::span<std::uint8_t>(std::uint32_t)> buffer; // 같은 입력 포인터의 물리 버퍼를 빌린다. 읽기 조회 자체는 상태를 변경하지 않는다.
    std::function<std::int32_t()> typeCount; // 현재 signed numTypes이며 원본의 numTypes<256 비교에 사용한다.
    std::function<void()> tooManyTypes; // 원본 진단 뒤 복귀할 수 있으며 이후 현재 source를 다시 읽는다.
    std::function<std::string_view()> fileName; // 서명 거부 때 가상+0에서 다시 읽는 현재 파일 이름이다.
    std::function<void(std::string_view)> rejectFile; // 원본41aef0의 파일 제거 요청 경계다. 보호 자료의 실제 삭제 허가는 이 함수가 부여하지 않는다.
};
// 004bce10: 비영 data/end→현재 두 BYTE 읽기/pos 증가→manager+10c/+110 쓰기→F 판정/거부 파일 요청이다.
bool ReadTemplateFileSignature(std::span<std::uint8_t> manager,std::span<std::uint8_t> source,const TemplateFileSignatureHooks& hooks);

// 원본 파일 할당과 가상 판정을 같은 MarkStream20/섹션 읽기에 연결한다.
struct DataManagerFileHooks {
    DataManagerImageHooks image; // 서명 성공 뒤 현재 같은 source로 실행할 섹션 본문이다.
    std::function<std::string_view()> fileName; // 읽기 요청 직전 가상+0의 현재 이름이다.
    std::function<std::uint32_t(std::string_view,std::int32_t,std::span<std::uint8_t>,std::int32_t)> readFile; // 원본41baa0의 이름/1/길이DWORD 주소/0과 반환 data 토큰이다.
    std::function<bool(std::span<std::uint8_t>)> signature; // 현재 같은 MarkStream20을 받는 manager 가상+c다.
    std::function<void(std::uint32_t)> releaseFile; // 읽기 성공/서명 실패/예외 뒤 현재 source.data 할당을 한 번만 반납한다.
};
// 0044d980: MarkStream20 초기화→파일 읽기→end=alloc→가상 서명→섹션 본문→현재 파일 버퍼 반납이다.
bool ReadDataManagerFile(std::span<std::uint8_t> manager,const DataManagerFileHooks& hooks);
}
