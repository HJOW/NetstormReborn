// 원문 005e4794의 전체 현재 DWORD를 소비 위치마다 읽는 비소유 진단 뷰다.
#pragma once
#include <bit>
#include <cstdint>

namespace netstorm::o { // Client owner와 독립 raw 진단 입력을 같은 비영 관문으로 읽는다.
class RawCheckingFlag {
public:
    RawCheckingFlag()=default; // 독립 raw 입력의 최초 checking0을 만든다.
    RawCheckingFlag(const bool& value):boolean_(&value) {} // 살아 있는 기존 bool fixture 입력을 그대로 빌린다.
    RawCheckingFlag(bool&& value):local_(value ? 1U : 0U) {} // 리터럴/임시 bool의 주소를 보관하지 않는다.
    RawCheckingFlag(const bool&& value):local_(value ? 1U : 0U) {} // const 임시 bool도 값을 소유한다.
    RawCheckingFlag(const std::uint32_t& value):word_(&value) {} // 원문 DWORD 입력의 모든 현재 비트를 빌린다.
    RawCheckingFlag(std::uint32_t&& value):local_(value) {} // 임시 DWORD는 자체 값으로 보관한다.
    RawCheckingFlag(const std::uint32_t&& value):local_(value) {} // const 임시 DWORD도 주소를 빌리지 않는다.
    RawCheckingFlag(const std::int32_t& value):signed_(&value) {} // ReadInt가 쓰는 같은 signed DWORD를 빌린다.
    RawCheckingFlag(std::int32_t&& value):local_(std::bit_cast<std::uint32_t>(value)) {} // 임시 signed 입력도 수명 없는 주소를 남기지 않는다.
    RawCheckingFlag(const std::int32_t&& value):local_(std::bit_cast<std::uint32_t>(value)) {} // const signed 임시 입력도 자체 비트를 소유한다.
    RawCheckingFlag(const RawCheckingFlag&)=default; // 원래 owner 주소 또는 독립 자체값을 복사하며 뷰 객체 주소를 빌리지 않는다.
    RawCheckingFlag& operator=(const RawCheckingFlag&)=default; // 파생 모듈에 넘긴 사본도 같은 장기 owner를 가리킨다.
    RawCheckingFlag& operator=(bool value) { Clear();local_=value ? 1U : 0U;return *this; } // 독립 입력의 기존 bool 대입 의미를 유지하며 외부 owner에는 쓰지 않는다.
    RawCheckingFlag& operator=(std::uint32_t value) { Clear();local_=value;return *this; } // 기존 DWORD 값형 입력도 비트를 좁히지 않는다.
    RawCheckingFlag& operator=(std::int32_t value) { return *this=std::bit_cast<std::uint32_t>(value); } // signed 독립 입력의 상위 비트도 유지한다.
    void Bind(const std::int32_t& value) { Clear();signed_=&value; } // 같은 ReadInt/DebugSwitch owner에 명시 연결한다.
    void Bind(const std::uint32_t& value) { Clear();word_=&value; } // 원문 DWORD 독립 관측도 동일 주소에서 읽는다.
    void Bind(const bool& value) { Clear();boolean_=&value; } // 독립 bool 소유자의 기존 참조 계약을 보존한다.
    void Bind(std::int32_t&&)=delete;void Bind(std::uint32_t&&)=delete;void Bind(bool&&)=delete; // 임시값을 명시 비소유 주소로 빌릴 수 없게 한다.
    void Bind(const std::int32_t&&)=delete;void Bind(const std::uint32_t&&)=delete;void Bind(const bool&&)=delete; // const 임시값의 명시 Bind도 같은 이유로 금지한다.
    std::uint32_t Read() const { return signed_ ? std::bit_cast<std::uint32_t>(*signed_) : word_ ? *word_ : boolean_ ? (*boolean_ ? 1U : 0U) : local_; } // 외부 효과 뒤 현재 비트를 다시 읽는다.
    operator bool() const { return Read()!=0; } // 관문만 비영으로 판정하며 원래 저장소를 0/1로 고치지 않는다.
private:
    void Clear() { signed_=nullptr;word_=nullptr;boolean_=nullptr; } // 명시 재연결 때 기존 비소유 주소를 모두 버린다.
    const std::int32_t* signed_{}; // 실제 ReadInt의 같은 장기 signed owner다.
    const std::uint32_t* word_{}; // 원문 독립 DWORD의 비소유 주소다.
    const bool* boolean_{}; // 기존 bool 참조 입력의 비소유 주소다.
    std::uint32_t local_{}; // 기본/리터럴로 만든 독립 raw 입력에만 쓰는 자체값이다.
};
}
