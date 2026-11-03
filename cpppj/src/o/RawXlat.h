// 10.78 Xlat의 일곱 파일 버퍼와 같은 원문 키의 언어별 값을 Client 수명에 보관한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::o { // 원문 바이트 파서와 번역 장부이며 UTF-8 변환은 호출자가 수행한다.
class RawXlat {
public:
    void Load(std::uint32_t which,std::span<const std::uint8_t> bytes); //004de710의 파일 읽기 뒤 마지막BYTE/CR/레코드 병합이다.
    bool HasBuffer(std::uint32_t which) const; // 같은 textBuffer 슬롯의 설치 여부다. 길이0의 설치와 미설치를 구별한다.
    std::span<std::uint8_t> AllocateFileBuffer(std::uint32_t which,std::uint32_t length); // 실제 File 부모 전용 exactLength 설치다. 중복 버퍼의 미정 포인터 재사용은 거부한다.
    void NormalizeFileBuffer(std::uint32_t which); // 같은 설치 버퍼의 끝BYTE/NUL·CR 이동만 수행하며 File 닫기 전에 호출한다.
    void ParseFileBuffer(std::uint32_t which); // 정규화한 같은 버퍼를 File 닫기 뒤 기존 레코드 장부에 병합한다.
    std::string_view Translate(std::string_view original,std::int32_t language) const; //004de9a0의 영어 우회/정확 키/NULL 칸 폴백이다.
    std::span<const std::uint8_t> Buffer(std::uint32_t which) const; //같은 실제 버퍼의 원문 부분 쓰기를 관측한다.
    std::size_t Size() const; //언어 수와 별개인 고유 원문 키 수다.
private:
    std::array<std::optional<std::vector<std::uint8_t>>,7> buffers_; //원문 textBuffer[0..6]의 파일별 소유와 물리 바이트다.
    std::map<std::string,std::array<std::optional<std::string>,7>,std::less<>> records_; //NULL 언어 칸과 빈 번역을 구별하고 다른 언어의 값을 보존한다.
};
struct RawXlatStartupHooks { //원문004dea00의 세 파일 적재에 선택된 실제 파일/진단 경계다.
    std::function<bool(std::string_view)> exists; //0041ab10이며 내용을 읽지 않는 출처 관문이다.
    std::function<std::vector<std::uint8_t>(std::string_view)> read; //같은 d/ 경로의 실제 원문 바이트를 읽으며 실패를 빈 표로 바꾸지 않는다.
    std::function<void(std::string_view)> missing; //없을 때9번 원문 진단을 출력하고 다음 파일로 간다.
};
void InitializeRawXlat(RawXlat& xlat,const RawXlatStartupHooks& hooks); //German3→English1→Portuguese6이며 프랑스어/스페인어를 임의 추가하지 않는다.
}
