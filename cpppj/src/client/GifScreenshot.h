// 원본4d8c10/4db140의 스크린샷 입력·파일 번호·8비트 GIF 출력 경계다.
#pragma once
#include "client/InputEvent.h"
#include <array>
#include <functional>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::client { // 저장 규칙은 현재 Client의 설정/번호/논리 화면을 빌린다.
struct ScreenshotSaveHooks { // 원문이 호출하는 외부 장치를 실행 순서대로 제공한다.
    std::function<void(std::string_view,std::string_view)> set; //4414d0의 GifOutName 쓰기다.
    std::function<bool(std::string_view,std::string&)> read; //441300의 GifOutSpec1 치환 조회다.
    std::function<bool(std::string_view)> exists; //41ab10의 실제 경로 존재 조회다.
    std::function<void()> lock; //4a14e0의 현재 논리 화면 잠금이다.
    std::function<void(std::string_view)> gif; //4daff0의 현재 폭/높이/행 간격·고정 primary 색 저장이다.
    std::function<void()> unlock; //4a1550의 같은 화면 잠금 반납이다.
    std::function<void(std::string_view)> log; //4c2630의29번 저장 완료 기록이다.
};
// Shift만 켠 F7 누름의 저장→Tell을 처리한다. Guide/Gump 뒤·console/정지 조회 전에 호출한다.
bool DispatchScreenshotKey(InputEvent event,const std::function<void()>& save,const std::function<void(std::string_view)>& tell);
// 5e1830의 같은 번호를 증가시키며 충돌 경로를 건너뛴다. 실패 시 이미 증가한 번호는 유지한다.
std::string SaveNumberedScreenshot(std::uint32_t& counter,const ScreenshotSaveHooks& hooks);
//41b4c0은 드라이브 없는 이름을 설치 기본 경로에 붙인다. 선행 구분자도 기존 BaseFile과 같은 규칙을 쓴다.
std::filesystem::path ResolveScreenshotDiskPath(const std::filesystem::path& installRoot,std::string_view spec);
//555d88의256×56BYTE에서+18/20/28의 현재 double RGB를 원문 기본53비트×255/절삭으로 읽는다.
std::array<std::uint8_t,768> ScreenshotPrimaryPalette(std::span<const std::uint8_t> primary);
// 원본처럼 팔레트 번호와256색을 유지해 첫 GIF 이미지를 쓴다. pitch 패딩을 픽셀에 포함하지 않는다.
std::vector<std::uint8_t> EncodeScreenshotGif(std::uint32_t width,std::uint32_t height,std::uint32_t pitch,
    std::span<const std::uint8_t> pixels,std::span<const std::uint8_t,768> rgb);
}
