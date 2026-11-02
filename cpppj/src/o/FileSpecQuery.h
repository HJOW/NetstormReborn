// 원문 FileSpec의 생성/검색 핸들 수명을 경로 조회 동안 소유한다. 전체264BYTE ABI나 FindNext는 별도다.
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace netstorm::o { // ANSI 원문 경로 규칙과 Windows10/11의 Unicode 파일 장치를 구별한다.
struct FileSpecFindResult { std::uintptr_t handle{};std::string name; }; // 실제 FindFirst 핸들과 첫 파일명이며 실패 핸들도 보존한다.
struct FileSpecQueryHooks { // 두 경계가 모두 비어 있으면 실제 Windows 장치를 사용한다.
    std::function<FileSpecFindResult(std::string_view)> find; // 원래 입력 패턴을 조회하고 단일 첫 이름을 반환한다.
    std::function<void(std::uintptr_t)> close; // 비NULL/실패 핸들도 원문처럼 한 번 반납하며 예외를 던지지 않는다.
};
class FileSpecQuery { // 일반 입력은 핸들 없이 정규화하고 패턴은 원래 입력으로 검색한다.
public:
    explicit FileSpecQuery(std::string_view name,const FileSpecQueryHooks& hooks={}); // 원문459610/45a220의 첫NUL·접두·slash·FindFirst 순서다.
    FileSpecQuery(const FileSpecQuery&)=delete;FileSpecQuery& operator=(const FileSpecQuery&)=delete; // 검색 핸들을 복사하거나 이중 반납하지 않는다.
    std::string_view Text() const; // 현재 FileSpec 문자열을 같은 수명 안에서 빌린다.
private:
    struct Search { // 생성 도중 문자열 할당이 실패해도 이미 얻은 핸들을 반납한다.
        std::uintptr_t handle{}; // 일반 입력0·검색 성공 핸들·INVALID_HANDLE_VALUE를 구별한다.
        std::function<void(std::uintptr_t)> close; // 현재 검색을 연 같은 장치의 반납 함수다.
        ~Search(); // 원문4595f0처럼 비NULL에만 FindClose를 호출한다.
    } search_; // 파일 내용을 읽는 콜백보다 오래 살며 정상/예외 모두 정리된다.
    std::string value_; // 원문 기본 접두/구분자 또는 검색 결과로 선택한 현재 문자열이다.
};
}
