// 원본 입력 사건 큐(FUN_00452f00·00452f90·00452fe0). 창 프로시저가 키·마우스를 여기에 넣고 입력 처리(UserInput)가 꺼낸다.
// 원본 소스 파일 이름은 확인되지 않았다: assert 문자열이 없고, 주소가 Editgump.cpp(004528e0)와 Evilray.cpp(00453540) 사이다.
#pragma once
#include <array>
#include <cstdint>

namespace netstorm::client {
// 사건 코드의 비트(창 프로시저 FUN_00436550·FUN_00436020이 만든다).
namespace InputCode {
inline constexpr std::uint32_t kRelease = 0x80000000;      // 뗌(키 올림, 버튼 올림)
inline constexpr std::uint32_t kPressedSince = 0x40000000; // 폴링(00452e00): 지난 확인 뒤에 눌린 적 있음
inline constexpr std::uint32_t kShift = 0x20000000;
inline constexpr std::uint32_t kControl = 0x10000000;
inline constexpr std::uint32_t kAlt = 0x08000000;
inline constexpr std::uint32_t kOutside = 0x04000000;      // 폴링: 커서가 창 밖이거나 창이 비활성
inline constexpr std::uint32_t kCommand = 0x02000000;      // 프로그램이 넣은 명령 사건(00452fe0)
inline constexpr std::uint32_t kLeftButton = 0x00010000;
inline constexpr std::uint32_t kRightButton = 0x00020000;
inline constexpr std::uint32_t kMiddleButton = 0x00040000;
// 글자가 아닌 키는 가상 키 코드를 16비트 왼쪽으로 밀어 넣고, 글자는 아래 16비트에 넣는다.
inline constexpr unsigned kVirtualKeyShift = 16;
}

// 사건 하나: 코드와 화면 좌표(창 테두리를 뺀 커서 위치).
struct InputEvent {
    std::uint32_t code{};
    std::int32_t x{}, y{};
};

// 원본 전역 큐(DAT_0055a510의 40칸, 읽기 위치 DAT_0055a508, 쓰기 위치 DAT_0055a50c).
class InputQueue {
public:
    // 큐의 칸 수(원본 0x28). 한 칸은 가득 참을 구분하는 데 쓰여 39개까지 담긴다.
    static constexpr std::size_t kCapacity = 40;
    // 원본 FUN_00452f90: 끝에 넣는다. 가득 차면 방금 넣은 사건을 버린다.
    void Push(std::uint32_t code, std::int32_t x, std::int32_t y);
    // 원본 FUN_00452fe0: 명령 사건을 넣는다(코드는 0x0100ffff만 남기고 명령 표시를 붙인다).
    void PushCommand(std::uint32_t code, std::int32_t x, std::int32_t y);
    // 원본 FUN_00452f00: 하나 꺼낸다. 비었으면 0으로 채운 사건이다.
    // skipReleases가 참이면 뗌 사건은 꺼내서 버리고 0으로 채운 사건을 돌려준다.
    InputEvent Pop(bool skipReleases);
    // 꺼낼 사건이 없는가.
    bool Empty() const;
private:
    std::array<InputEvent, kCapacity> events_{};
    std::size_t read_{};
    std::size_t write_{};
};
}
