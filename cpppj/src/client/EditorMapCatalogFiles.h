// 한 번 이관한 장부와 한 writable 디렉터리의 실제 OS 목록/현재 읽기를 연결한다.
#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::client { //원본 불변을 유지하는 호스트 I/O 경계이며 게임 규칙을 소유하지 않는다.
class EditorFiles; //기존 canonical/원본 보호/권한/쓰기/삭제 장치다.
struct EditorCatalogSourceDevices { //loose root와 archive-only reader를 분리하여 삭제된 원본 재등장을 막는다.
    std::filesystem::path root; //source 절대 경로의 전체 상대 매핑과 최초 물리 복사에 사용하는 명시 root다.
    std::function<std::optional<std::vector<std::uint8_t>>(std::string_view)> archiveOnly; //등록 순서의 TAFF만 읽으며 disk/secondary fallback을 포함하지 않는다.
};
class EditorCatalogWorkspace { //같은 작업 namespace와 별도 형제-state 장부를 장기 소유한다.
public:
    EditorCatalogWorkspace(std::shared_ptr<EditorFiles> files,EditorCatalogSourceDevices source); //생성은 경로/장치만 보관하며 첫 실제 사용 전에는 파일을 쓰지 않는다.
    ~EditorCatalogWorkspace(); //동기 reader가 모두 끝난 뒤 metadata 잠금을 반납한다.
    EditorFiles& Files() const; //기존 Native writer/readonly/삭제가 빌리는 같은 파일 장치다.
    std::filesystem::path Resolve(std::string_view configuredPath) const; //relative/insideSource absolute/insideWork absolute를 검사된 같은 work 경로로 매핑한다.
    void ImportDisk(std::string_view configuredPattern); //현재 source의 미이관 physical 파일/빈 directory만 한 번 복사하며 명시 부재는 기록하지 않는다.
    std::filesystem::path Prepare(std::string_view configuredPath); //직접 Editor 진입도 같은 장부를 지키며 archive 실제 설치는 별도 명시 이관한다.
    std::vector<std::string> Enumerate(std::string_view configuredPattern); //이관 뒤 실제 work Win32 결과만 전달하며 논리 중복/디렉터리/명시 부재를 제거하지 않는다.
    std::optional<std::vector<std::uint8_t>> Read(std::string_view configuredPath); //분류/메타마다 현재 작업 파일을 읽고 완료 missing에서는 모든 source fallback을 막는다.
private:
    struct Impl; //source identity/검증된 metadata/잠금/버전 장부의 고정 owner다.
    std::unique_ptr<Impl> impl_; //World나 listener보다 오래 유지하는 실제 I/O 정책 수명이다.
};
}
