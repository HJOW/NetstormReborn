// 7차 안티앨리어싱/FSR 1.0: 8비트 팔레트 논리 프레임을 Direct3D 11로 후처리해 창에 표시하는 선택적 출력기다.
// 원본(DirectDraw/GDI)에는 없는 Windows 10/11 호환 확장이며 옵션이 꺼져 있으면 만들지 않는다. 장치를 만들 수 없으면 기존 GDI 경로가 그대로 쓰인다.
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace netstorm::client { // 그래픽 장치 의존 출력기다. 게임 규칙/입력 좌표 계산은 Screen이 맡는다.
// 한 프레임을 표시할 때의 후처리 선택이다.
struct GpuPresentOptions {
    bool fsr{};               // FSR 1.0(EASU 업스케일 + RCAS 선명도)을 적용한다.
    bool antiAliasing{};      // 논리 해상도에서 윤곽선 안티앨리어싱(FXAA 3.11 방식을 단순화한 후처리)을 적용한다.
    float rcasSharpnessStops{0.2f}; // RCAS 선명도 스톱 값이다(0 최대, 클수록 약함, 공식 권장 0.2).
};
// 클라이언트 영역 안에서 논리 프레임이 놓일 사각형이다(레터박스 바깥은 검정).
struct GpuOutputRect { int left{}, top{}, right{}, bottom{}; };
class GpuPresenter {
public:
    // window(HWND)에 스왑 체인을 만든다. 하드웨어 장치를 먼저, 안 되면 소프트웨어(WARP)를 쓴다.
    // 장치/셰이더 준비에 실패하면 nullptr을 돌려주고 error에 이유를 적는다.
    static std::unique_ptr<GpuPresenter> Create(void* window, std::string& error);
    // 장치만 만들어 보고 가능 여부를 돌려준다(스왑 체인/셰이더 없음). FSR 해상도를 선택 목록에 보일지 정할 때 쓴다.
    static bool Probe(std::string* adapterName = nullptr);
    ~GpuPresenter();
    GpuPresenter(const GpuPresenter&) = delete;
    GpuPresenter& operator=(const GpuPresenter&) = delete;
    // 논리 프레임(pixels: width×height 8비트 팔레트 번호, pitch는 줄 간격)과 256색 팔레트(0x00RRGGBB가 아닌 B,G,R,X 바이트 순 RGBQUAD)를
    // output 사각형에 표시한다. 클라이언트 영역이 바뀌었으면 스왑 체인 크기도 맞춘다. 성공하면 참이다.
    bool Present(const std::uint8_t* pixels, int pitch, int width, int height, const std::uint32_t* palette256, GpuOutputRect output, const GpuPresentOptions& options);
    // 마지막으로 표시한 클라이언트 영역 전체를 BGRA(위→아래)로 읽는다. 아직 표시한 적이 없으면 거짓이다.
    bool ReadBack(std::vector<std::uint8_t>& bgra, int& width, int& height);
    // 장치(어댑터) 이름이다. 소프트웨어 장치면 "Microsoft Basic Render Driver" 같은 이름이 된다.
    const std::string& AdapterName() const;
    // 마지막 실패 이유(성공하면 빈 문자열)다.
    const std::string& LastError() const;
private:
    GpuPresenter();
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
