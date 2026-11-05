// 4차 로컬 MCP의 화면 캡처 전송용 PNG 인코더다. 원본에는 없는 새 장치이며 외부 압축 라이브러리 없이 zlib(고정 Huffman+LZ77)을 직접 만든다.
#pragma once
#include <cstdint>
#include <span>
#include <vector>

namespace netstorm::platform { // 이미지 파일 형식의 OS 독립 부품이다.
// zlib 스트림(헤더+deflate+Adler-32)을 만든다. LZ77(32KB 창)과 고정 Huffman 코드를 쓰며 어떤 입력이든 한 블록으로 압축한다.
std::vector<std::uint8_t> ZlibCompress(std::span<const std::uint8_t> data);
// CRC-32(PNG/zlib 다항식)다. seed에 이전 결과를 주면 이어서 계산한다.
std::uint32_t Crc32(std::span<const std::uint8_t> data, std::uint32_t seed = 0);
// Adler-32다(zlib 꼬리).
std::uint32_t Adler32(std::span<const std::uint8_t> data);
// 8비트 팔레트 화면을 인덱스 색 PNG로 만든다. pixels는 pitch 간격의 행 목록이고 palette는 256개 RGB(768바이트)다.
// 게임의 논리 화면이 8비트 팔레트라 가장 작고 원본 색 번호를 그대로 보존한다.
std::vector<std::uint8_t> EncodePngIndexed(std::uint32_t width, std::uint32_t height, std::uint32_t pitch,
                                           std::span<const std::uint8_t> pixels, std::span<const std::uint8_t, 768> palette);
// RGBA(행 간격 없음) 이미지를 24비트+알파 PNG로 만든다. 실제 창 출력 캡처 등 팔레트가 없는 이미지용이다.
std::vector<std::uint8_t> EncodePngRgba(std::uint32_t width, std::uint32_t height, std::span<const std::uint8_t> rgba);
}
