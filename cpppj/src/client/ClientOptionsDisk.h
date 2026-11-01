// 설정 저장의 원문 바이트/일반 열기 모드를 유지하며 Windows10/11의 매핑 중인 파일 저장만 보완한다.
#pragma once
#include <cstdint>
#include <filesystem>
#include <span>

namespace netstorm::client { // 호환 저장은 Client 설정에만 적용하며 원본 BaseFile의 일반 쓰기를 변경하지 않는다.
// 기존 w+b 열기가 ERROR_USER_MAPPED_FILE로 실패한 경우에만 같은 폴더의 완성된 파일로 교체한다.
// 상위 폴더는 생성하지 않으며 실패는 예외로 전달한다. 원본과 다른 Windows10/11 호환 처리다.
void WriteClientOptionsDisk(const std::filesystem::path& path,std::span<const std::uint8_t> bytes);
}
