// 옵션 저장의 CRT 종료와 Windows 교체 실패 복구를 같은 제품 함수로 검증하는 내부 경계다.
#pragma once
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <span>

namespace netstorm::client::options_detail { // 외부 게임 기능이 아닌 ClientOptionsDisk의 실제 파일 소유 경계다.
// 전달받은 FILE을 소유하며 쓰기/flush/선택적 commit 결과와 무관하게 한 번 닫는다. 실패는 runtime_error로 전달한다.
void FinishOptionsStream(FILE* file,std::span<const std::uint8_t> bytes,bool durable);
// ReplaceFile 실패 뒤 기존 이름이 없을 때만 백업 이름 복원을 시도한다. 복구 파일은 지우지 않고 원래 Windows 오류로 예외를 던진다.
[[noreturn]] void ThrowOptionsReplacementFailure(const std::filesystem::path& path,const std::filesystem::path& previous,
    const std::filesystem::path& directory,unsigned long error);
}
