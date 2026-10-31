// 실제 Smacker 스트림과 원본 MovieGump를 현재 공통 UI 트리/수명에 조립한다.
#pragma once
#include "client/MovieLifecycle.h"
#include "client/GumpCreate.h"
#include <filesystem>
#include <string>

namespace netstorm::client { // 기존32비트 SDK를 별도64비트 디코더/PCM 출력으로 연결한다.
struct RawMenuTree;class GumpPixelTarget; // 현재 Client의 같은 공통 처리기를 빌린다.
struct RawMovieContext { // 실제 Client가 소유하는 화면/시계/음향/정지 경계다.
    MovieLifecycleHooks devices;std::function<double()> now; // 현재 Screen/음향 경계와 게임 정지에 영향받지 않는 벽시계다.
    std::function<std::uint32_t()> milliseconds; // 명시 검사 시계다. 실제 장치는 원문과 같은 timeGetTime을 사용한다.
    std::function<void()> clearAndPresent; // 원문 공장의 전체 검정 Fill/즉시 출력이다.
    std::function<void(ScreenRect,std::uint32_t)> invalidate; // 원문 flags4의 현재 Renderer 경계다.
    bool audio{},mute{};std::int8_t type{42}; // 실제 PCM 허용/검사 음소거와 보호PE00540445의 초기2a다.
};
class RawMovieControls { // singleton과 삭제대기/살아 있는 고정 노드들을 같은 소유 수명에 유지한다.
public:
    RawMovieControls(RawMenuTree tree,RawMovieContext context); // 등록표/장치 수명은 이 소유자보다 길어야 한다.
    ~RawMovieControls(); // 공통 소멸 후 남은 스트림/PCM만 반납한다.
    bool Open(const std::filesystem::path& path); //00484010: 미종료 영상은 거부하고 파일 실패는false다. 성공은 dialog/단일 override를 같은 원문 순서로 설치하며 Kill이 override를 반납한다.
    bool Active() const; //00483db0: Kill 예약과 구별되는 실제 singleton의 생존이다.
    GumpCreateNode* Node(std::uint32_t token) const; // 공통 가상 분배가 같은 기본 주소를 사용한다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event) const; // 같은592 원문 입력 판단/종료·Kill이다.
    void Idle(std::uint32_t token) const; // 같은592 판단에 실제 디코더/현재 벽시계를 공급한다.
    void Draw(std::uint32_t token,const GumpPixelTarget& pixels,ScreenRect clip) const; // 같은593 표시로 원래 해상도를 출력한다.
    void DeleteScalar(std::uint32_t token,std::uint32_t flags); // close→singleton 정리→기본 소멸→실제 메모리 반납이다.
    void Close(); // 종료 중에도 각 영상의 실제 end와 공통 Kill/Cleanup을 사용한다.
    std::string Report() const; // 현재 노드/디코더 프레임/PCM 수락/생존만 읽는 검사 보고다.
private:
    struct Impl;std::unique_ptr<Impl> impl_; // 고정 Gump 노드·압축 스트림/PCM/시각의 단일 소유자다.
};
}
