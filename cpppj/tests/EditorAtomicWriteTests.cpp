// 실제 Windows 공유 거부의 회복과 영구 실패에서 장부 원문/부분 쓰기 보존을 확인한다.
#include "client/EditorFiles.h"
#include "o/BaseFile.h"
#include "TestSupport.h"
#include <chrono>
#include <filesystem>
#include <thread>
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX //Windows 매크로가 검사에서 사용하는 표준 함수 이름을 바꾸지 않게 한다.
#endif
#include <windows.h>

namespace { //이번 검사만의 파일·핸들 소유자를 다른 번역 단위와 분리한다.
using namespace netstorm::client; //실제 보호 경로와 장부 쓰기 장치를 직접 검사한다.
// 실제 게임·원본과 겹치지 않는 새 NTFS 임시 디렉터리를 선택한다.
std::filesystem::path NewAtomicRoot() {
    return std::filesystem::temp_directory_path()/("netstorm-atomic-write-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
}
// 읽기/쓰기는 공유하되 삭제/교체를 공유하지 않는 외부 reader의 실제 Win32 HANDLE이다.
HANDLE BlockReplacement(const std::filesystem::path& path) { return CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr); }
}

// 외부 reader가 실제 HANDLE을 반납하면 같은 atomic 교체가 성공하고 최종 파일을 완전히 읽을 수 있어야 한다.
TEST_CASE(EditorAtomicWrite_TransientExternalReaderRecoversWithoutDiscardingLedger) {
    const auto root=NewAtomicRoot();EditorFiles files(root/"state",{root/"protected"});const std::vector<std::uint8_t> old{1,2,3},next{4,5,6,7}; //서로 다른 길이/내용의 독립 입력이다.
    files.WriteAtomic("catalog-v1.dat",old);const auto target=files.ResolvePath("catalog-v1.dat");const auto blocked=BlockReplacement(target);CHECK(blocked!=INVALID_HANDLE_VALUE);if (blocked==INVALID_HANDLE_VALUE) return;
    std::jthread release([blocked] { Sleep(120);CloseHandle(blocked); }); //다른 실제 reader가 짧게 사용한 뒤 반납하는 OS 수명을 재현한다.
    files.WriteAtomic("catalog-v1.dat",next);release.join();CHECK(netstorm::o::ReadFileBytes(target)==next && !std::filesystem::exists(files.ResolvePath("catalog-v1.dat.pending")));
}

// 계속 잠긴 대상은 제한 시간 뒤 실패하며 old 장부/새 임시 바이트가 모두 보존되어야 한다.
TEST_CASE(EditorAtomicWrite_PersistentExternalReaderPreservesOldAndPendingBytes) {
    const auto root=NewAtomicRoot();EditorFiles files(root/"state",{root/"protected"});const std::vector<std::uint8_t> old{11,12,13},next{14,15,16,17}; //기존 장부와 다음 durable 갱신의 독립 원문이다.
    files.WriteAtomic("catalog-v1.dat",old);const auto target=files.ResolvePath("catalog-v1.dat");const auto blocked=BlockReplacement(target);CHECK(blocked!=INVALID_HANDLE_VALUE);if (blocked==INVALID_HANDLE_VALUE) return;
    bool rejected{};try { files.WriteAtomic("catalog-v1.dat",next); } catch (const std::runtime_error&) { rejected=true; } //예외만 정상 실패 관측으로 인정하며 저장 성공값을 합성하지 않는다.
    CHECK(rejected && netstorm::o::ReadFileBytes(target)==old && netstorm::o::ReadFileBytes(files.ResolvePath("catalog-v1.dat.pending"))==next);CHECK(CloseHandle(blocked)!=0);
    files.WriteAtomic("catalog-v1.dat",next);CHECK(netstorm::o::ReadFileBytes(target)==next && !std::filesystem::exists(files.ResolvePath("catalog-v1.dat.pending"))); //실제 외부 잠금 해제 뒤에는 새 직접 장치 요청이 정상 교체한다.
}
