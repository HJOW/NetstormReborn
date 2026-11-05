// 7차 안티앨리어싱/FSR 1.0: 8비트 팔레트 논리 프레임을 Direct3D 11로 후처리해 창에 표시한다.
// 경로: 팔레트 변환 → (선택) 안티앨리어싱 → (선택) FSR 1.0 EASU 업스케일 → RCAS 선명도 → 레터박스 출력.
// 원본에는 없는 Windows 10/11 호환 확장이며 FSR 알고리즘은 AMD 공식 헤더(third_party/fsr1)를 그대로 HLSL로 컴파일해 쓴다.
#include "client/GpuPresenter.h"
#include "client/FsrConstants.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

namespace netstorm::client { // 빌드 때 third_party/fsr1의 두 헤더를 바이트 배열로 만든 생성 파일(FsrShaderHeaders.cpp)이 정의한다.
extern const unsigned char kEmbeddedFfxA[];
extern const unsigned long long kEmbeddedFfxASize;
extern const unsigned char kEmbeddedFfxFsr1[];
extern const unsigned long long kEmbeddedFfxFsr1Size;
}

namespace netstorm::client { // 출력기 구현이다.
using Microsoft::WRL::ComPtr;
namespace { // 셰이더 원문과 보조 함수다.
// 모든 패스가 공유하는 HLSL이다. EASU/RCAS는 공식 ffx_fsr1.h의 FsrEasuF/FsrRcasF를 그대로 부른다.
constexpr const char* kShaderSource = R"HLSL(
#define A_GPU 1
#define A_HLSL 1
#include "ffx_a.h"
Texture2D<float4> InputTexture : register(t0);
Texture2D<float> IndexTexture : register(t1);
Texture2D<float4> PaletteTexture : register(t2);
SamplerState samLinearClamp : register(s0);
cbuffer Params : register(b0) {
    uint4 const0; uint4 const1; uint4 const2; uint4 const3; // EASU 상수(공식 FsrEasuCon)
    uint4 rcasConst;                                        // RCAS 상수(공식 FsrRcasCon)
    float4 sourceSize;                                      // xy = 입력 텍스처 크기, zw = 크기의 역수
    float4 outputRect;                                      // xy = 출력 사각형 왼쪽 위, zw = 출력 사각형 크기
};
#define FSR_EASU_F 1
AF4 FsrEasuRF(AF2 p) { return InputTexture.GatherRed(samLinearClamp, p, int2(0, 0)); }
AF4 FsrEasuGF(AF2 p) { return InputTexture.GatherGreen(samLinearClamp, p, int2(0, 0)); }
AF4 FsrEasuBF(AF2 p) { return InputTexture.GatherBlue(samLinearClamp, p, int2(0, 0)); }
#define FSR_RCAS_F 1
AF4 FsrRcasLoadF(ASU2 p) { return InputTexture.Load(int3(ASU2(p), 0)); }
void FsrRcasInputF(inout AF1 r, inout AF1 g, inout AF1 b) {}
#include "ffx_fsr1.h"
struct VSOut { float4 pos : SV_Position; float2 uv : TEXCOORD0; };
// 정점 버퍼 없이 화면 전체를 덮는 삼각형 하나다.
VSOut VSMain(uint id : SV_VertexID) {
    VSOut o;
    float2 uv = float2((id << 1) & 2, id & 2);
    o.pos = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
    o.uv = uv;
    return o;
}
// 8비트 팔레트 번호를 256색 표로 RGB로 바꾼다.
float4 PSDecode(VSOut i) : SV_Target {
    int2 p = int2(i.pos.xy);
    uint index = (uint)(IndexTexture.Load(int3(p, 0)) * 255.0 + 0.5);
    return float4(PaletteTexture.Load(int3(index, 0, 0)).rgb, 1.0);
}
float Luma(float3 c) { return dot(c, float3(0.299, 0.587, 0.114)); }
float3 Tap(float2 uv, float2 offset) { return InputTexture.SampleLevel(samLinearClamp, uv + offset * sourceSize.zw, 0).rgb; }
// 윤곽선 안티앨리어싱: 휘도 대비가 큰 가장자리의 수직 방향으로 이웃과 섞는다(FXAA 3.11의 부분 화소 섞기를 단순화한 방식이다).
float4 PSAntiAlias(VSOut i) : SV_Target {
    float2 uv = i.pos.xy * sourceSize.zw;
    float3 m = Tap(uv, float2(0, 0));
    float3 n = Tap(uv, float2(0, -1)), s = Tap(uv, float2(0, 1)), e = Tap(uv, float2(1, 0)), w = Tap(uv, float2(-1, 0));
    float lM = Luma(m), lN = Luma(n), lS = Luma(s), lE = Luma(e), lW = Luma(w);
    float lMax = max(lM, max(max(lN, lS), max(lW, lE))), lMin = min(lM, min(min(lN, lS), min(lW, lE)));
    float range = lMax - lMin;
    if (range < max(0.0312, lMax * 0.125)) return float4(m, 1); // 대비가 작으면 가장자리가 아니다.
    float lNW = Luma(Tap(uv, float2(-1, -1))), lNE = Luma(Tap(uv, float2(1, -1))), lSW = Luma(Tap(uv, float2(-1, 1))), lSE = Luma(Tap(uv, float2(1, 1)));
    float lAvg = (2.0 * (lN + lS + lW + lE) + lNW + lNE + lSW + lSE) / 12.0;
    float blend = saturate(abs(lAvg - lM) / range);
    blend = smoothstep(0.0, 1.0, blend);
    blend = blend * blend * 0.75;
    float edgeH = abs((lNW + lSW) - 2.0 * lW) + 2.0 * abs((lN + lS) - 2.0 * lM) + abs((lNE + lSE) - 2.0 * lE);
    float edgeV = abs((lNW + lNE) - 2.0 * lN) + 2.0 * abs((lW + lE) - 2.0 * lM) + abs((lSW + lSE) - 2.0 * lS);
    bool horizontal = edgeH >= edgeV;
    float g1 = abs((horizontal ? lN : lW) - lM), g2 = abs((horizontal ? lS : lE) - lM);
    float2 step = horizontal ? float2(0, 1) : float2(1, 0);
    if (g1 < g2) step = -step; // 기울기가 큰 쪽으로 반 화소 이동해 가장자리를 이웃 색과 섞는다.
    float3 neighbor = Tap(uv, step * 0.5);
    float3 average = (m + n + s + e + w) * 0.2;
    return float4(lerp(m, lerp(neighbor, average, 0.35), saturate(blend * 1.5 + 0.15)), 1.0);
}
// FSR 1.0 EASU: 출력 픽셀마다 입력 이미지의 가장자리 방향을 따라 12점 필터로 업스케일한다.
float4 PSEasu(VSOut i) : SV_Target {
    AF3 c;
    FsrEasuF(c, AU2(i.pos.xy), const0, const1, const2, const3);
    return float4(c, 1.0);
}
// FSR 1.0 RCAS: 업스케일된 이미지의 선명도를 되살린다. 출력 사각형 왼쪽 위를 원점으로 읽는다.
float4 PSRcas(VSOut i) : SV_Target {
    AF3 c;
    AU2 ip = AU2(i.pos.xy - outputRect.xy);
    FsrRcasF(c.r, c.g, c.b, ip, rcasConst);
    return float4(c, 1.0);
}
// 후처리 없이 출력 사각형에 쌍선형으로 늘려 그린다(안티앨리어싱만 켠 호환 전체화면 등).
float4 PSBlit(VSOut i) : SV_Target {
    float2 uv = (i.pos.xy - outputRect.xy) / outputRect.zw;
    return float4(InputTexture.SampleLevel(samLinearClamp, uv, 0).rgb, 1.0);
}
)HLSL";
// 상수 버퍼의 C++ 모양이다(HLSL cbuffer Params와 같은 순서/크기, 112바이트).
struct ParamsBuffer {
    std::uint32_t const0[4], const1[4], const2[4], const3[4]; // EASU 상수다.
    std::uint32_t rcas[4];                                    // RCAS 상수다.
    float sourceSize[4];                                      // 입력 크기와 역수다.
    float outputRect[4];                                      // 출력 사각형 원점과 크기다.
};
static_assert(sizeof(ParamsBuffer) == 112, "HLSL cbuffer와 크기가 같아야 한다");
// D3DCompile의 #include 요청에 실행 파일에 넣어 둔 AMD 헤더 바이트를 돌려준다.
class EmbeddedInclude final : public ID3DInclude {
public:
    HRESULT STDMETHODCALLTYPE Open(D3D_INCLUDE_TYPE, LPCSTR name, LPCVOID, LPCVOID* data, UINT* bytes) override {
        const std::string_view requested(name);
        if (requested == "ffx_a.h") { *data = kEmbeddedFfxA; *bytes = static_cast<UINT>(kEmbeddedFfxASize); return S_OK; }
        if (requested == "ffx_fsr1.h") { *data = kEmbeddedFfxFsr1; *bytes = static_cast<UINT>(kEmbeddedFfxFsr1Size); return S_OK; }
        return E_FAIL; // 그 밖의 헤더는 없다.
    }
    HRESULT STDMETHODCALLTYPE Close(LPCVOID) override { return S_OK; }
};
// HRESULT 실패를 예외로 바꾼다. what은 어떤 호출이었는지 적는다.
void Check(HRESULT result, const char* what) {
    if (FAILED(result)) throw std::runtime_error(std::string(what) + " 실패(HRESULT 0x" + [&] { char text[16]; std::snprintf(text, sizeof text, "%08lX", static_cast<unsigned long>(result)); return std::string(text); }() + ")");
}
// 장치를 하드웨어 우선, 소프트웨어(WARP) 대체로 만든다. 만든 장치의 어댑터 이름을 name에 적는다.
void CreateDevice(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, std::string& name) {
    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0}; // 셰이더 모델 5.0(Gather4)이 필요하다.
    HRESULT result = E_FAIL;
    for (const auto driver : {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP}) { // 하드웨어가 안 되면 소프트웨어로 대신한다.
        D3D_FEATURE_LEVEL obtained{};
        result = D3D11CreateDevice(nullptr, driver, nullptr, 0, levels, static_cast<UINT>(std::size(levels)), D3D11_SDK_VERSION, &device, &obtained, &context);
        if (result == E_INVALIDARG) result = D3D11CreateDevice(nullptr, driver, nullptr, 0, levels + 1, 1, D3D11_SDK_VERSION, &device, &obtained, &context); // 11_1을 모르는 환경이다.
        if (SUCCEEDED(result)) break;
    }
    Check(result, "D3D11CreateDevice");
    ComPtr<IDXGIDevice> dxgiDevice;
    ComPtr<IDXGIAdapter> adapter;
    if (SUCCEEDED(device.As(&dxgiDevice)) && SUCCEEDED(dxgiDevice->GetAdapter(&adapter))) {
        DXGI_ADAPTER_DESC description{};
        if (SUCCEEDED(adapter->GetDesc(&description))) {
            char text[256]{};
            WideCharToMultiByte(CP_UTF8, 0, description.Description, -1, text, static_cast<int>(sizeof text) - 1, nullptr, nullptr);
            name = text;
        }
    }
}
}

// 출력기 내부 상태다. 장치, 스왑 체인, 패스별 텍스처, 셰이더를 소유한다.
struct GpuPresenter::Impl {
    HWND window{};
    std::string adapterName, lastError;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain1> swapChain;
    int swapWidth{}, swapHeight{};
    ComPtr<ID3D11Texture2D> finalTexture;       // 클라이언트 영역 전체의 최종 그림이다(읽기 확인용으로 보존한다).
    ComPtr<ID3D11RenderTargetView> finalView;
    ComPtr<ID3D11ShaderResourceView> finalSrv;
    ComPtr<ID3D11Texture2D> indexTexture, paletteTexture; // 논리 프레임 번호와 256색 표다(CPU가 매 프레임 갱신한다).
    ComPtr<ID3D11ShaderResourceView> indexSrv, paletteSrv;
    int sourceWidth{}, sourceHeight{};
    ComPtr<ID3D11Texture2D> decodedTexture, antiAliasedTexture; // 논리 크기의 RGB 그림이다.
    ComPtr<ID3D11RenderTargetView> decodedView, antiAliasedView;
    ComPtr<ID3D11ShaderResourceView> decodedSrv, antiAliasedSrv;
    int easuWidth{}, easuHeight{};
    ComPtr<ID3D11Texture2D> easuTexture;        // EASU 출력(출력 사각형 크기)이다.
    ComPtr<ID3D11RenderTargetView> easuView;
    ComPtr<ID3D11ShaderResourceView> easuSrv;
    ComPtr<ID3D11Texture2D> stagingTexture;     // ReadBack용 CPU 읽기 텍스처다.
    ComPtr<ID3D11Buffer> paramsBuffer;
    ComPtr<ID3D11SamplerState> linearSampler;
    ComPtr<ID3D11RasterizerState> rasterizer;
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> decodePs, antiAliasPs, easuPs, rcasPs, blitPs;
    bool presented{};
    // HLSL 한 진입점을 컴파일해 셰이더 객체를 만든다.
    template <class Shader> ComPtr<Shader> Compile(const char* entry, const char* profile);
    void CreateSwapChain();
    void ResizeSwapChain(int width, int height);
    void EnsureSourceTargets(int width, int height);
    void EnsureEasuTarget(int width, int height);
    ComPtr<ID3D11Texture2D> MakeTexture(int width, int height, DXGI_FORMAT format, UINT bind, D3D11_USAGE usage, UINT cpu) const;
    void Draw(ID3D11RenderTargetView* target, const D3D11_VIEWPORT& viewport, ID3D11PixelShader* pixel, ID3D11ShaderResourceView* input, const ParamsBuffer& params, bool decode);
};

// 텍스처 하나를 만든다(밉맵 없음, 샘플 1).
ComPtr<ID3D11Texture2D> GpuPresenter::Impl::MakeTexture(int width, int height, DXGI_FORMAT format, UINT bind, D3D11_USAGE usage, UINT cpu) const {
    D3D11_TEXTURE2D_DESC description{};
    description.Width = static_cast<UINT>(width);
    description.Height = static_cast<UINT>(height);
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = format;
    description.SampleDesc.Count = 1;
    description.Usage = usage;
    description.BindFlags = bind;
    description.CPUAccessFlags = cpu;
    ComPtr<ID3D11Texture2D> texture;
    Check(device->CreateTexture2D(&description, nullptr, &texture), "CreateTexture2D");
    return texture;
}
// 진입점 하나를 컴파일한다. 헤더는 실행 파일에 들어 있는 AMD 원본을 쓴다.
template <class Shader> ComPtr<Shader> GpuPresenter::Impl::Compile(const char* entry, const char* profile) {
    EmbeddedInclude include;
    ComPtr<ID3DBlob> code, errors;
    const HRESULT result = D3DCompile(kShaderSource, std::strlen(kShaderSource), "netstorm_fsr.hlsl", nullptr, &include, entry, profile, D3DCOMPILE_OPTIMIZATION_LEVEL1, 0, &code, &errors);
    if (FAILED(result)) {
        std::string message = std::string("셰이더 컴파일 실패(") + entry + ")";
        if (errors) message += ": " + std::string(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize());
        throw std::runtime_error(message);
    }
    ComPtr<Shader> shader;
    HRESULT created = E_FAIL;
    if constexpr (std::is_same_v<Shader, ID3D11VertexShader>) created = device->CreateVertexShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &shader);
    else created = device->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &shader);
    Check(created, "셰이더 객체 생성");
    return shader;
}
// 창의 현재 클라이언트 크기로 스왑 체인과 최종 텍스처를 만든다.
void GpuPresenter::Impl::CreateSwapChain() {
    RECT client{};
    GetClientRect(window, &client);
    swapWidth = std::max<LONG>(1, client.right);
    swapHeight = std::max<LONG>(1, client.bottom);
    ComPtr<IDXGIDevice> dxgiDevice;
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIFactory2> factory;
    Check(device.As(&dxgiDevice), "IDXGIDevice");
    Check(dxgiDevice->GetAdapter(&adapter), "GetAdapter");
    Check(adapter->GetParent(IID_PPV_ARGS(&factory)), "IDXGIFactory2");
    DXGI_SWAP_CHAIN_DESC1 description{};
    description.Width = static_cast<UINT>(swapWidth);
    description.Height = static_cast<UINT>(swapHeight);
    description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.BufferCount = 2;
    description.Scaling = DXGI_SCALING_NONE; // 창 크기가 바뀌어도 늘리지 않고 새 크기로 다시 만든다.
    description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    Check(factory->CreateSwapChainForHwnd(device.Get(), window, &description, nullptr, nullptr, &swapChain), "CreateSwapChainForHwnd");
    factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES); // Alt+Enter 전체화면 전환은 게임이 직접 관리한다.
    finalTexture = MakeTexture(swapWidth, swapHeight, DXGI_FORMAT_B8G8R8A8_UNORM, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0);
    Check(device->CreateRenderTargetView(finalTexture.Get(), nullptr, &finalView), "CreateRenderTargetView(final)");
    Check(device->CreateShaderResourceView(finalTexture.Get(), nullptr, &finalSrv), "CreateShaderResourceView(final)");
}
// 클라이언트 영역이 바뀌면 스왑 체인과 최종 텍스처를 새 크기로 다시 만든다.
void GpuPresenter::Impl::ResizeSwapChain(int width, int height) {
    finalView.Reset();
    finalSrv.Reset();
    finalTexture.Reset();
    stagingTexture.Reset();
    context->ClearState();
    Check(swapChain->ResizeBuffers(0, static_cast<UINT>(width), static_cast<UINT>(height), DXGI_FORMAT_UNKNOWN, 0), "ResizeBuffers");
    swapWidth = width;
    swapHeight = height;
    finalTexture = MakeTexture(width, height, DXGI_FORMAT_B8G8R8A8_UNORM, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0);
    Check(device->CreateRenderTargetView(finalTexture.Get(), nullptr, &finalView), "CreateRenderTargetView(final)");
    Check(device->CreateShaderResourceView(finalTexture.Get(), nullptr, &finalSrv), "CreateShaderResourceView(final)");
}
// 논리 프레임 크기에 맞는 입력/중간 텍스처를 만든다(크기가 같으면 그대로 둔다).
void GpuPresenter::Impl::EnsureSourceTargets(int width, int height) {
    if (indexTexture && sourceWidth == width && sourceHeight == height) return;
    sourceWidth = width;
    sourceHeight = height;
    indexTexture = MakeTexture(width, height, DXGI_FORMAT_R8_UNORM, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    Check(device->CreateShaderResourceView(indexTexture.Get(), nullptr, &indexSrv), "CreateShaderResourceView(index)");
    if (!paletteTexture) {
        paletteTexture = MakeTexture(256, 1, DXGI_FORMAT_B8G8R8A8_UNORM, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
        Check(device->CreateShaderResourceView(paletteTexture.Get(), nullptr, &paletteSrv), "CreateShaderResourceView(palette)");
    }
    // 같은 모양의 RGB 중간 그림 둘이다(팔레트 변환 결과와 안티앨리어싱 결과). ComPtr의 operator&가 재정의되어 있어 한 줄씩 만든다.
    decodedTexture = MakeTexture(width, height, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0);
    antiAliasedTexture = MakeTexture(width, height, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0);
    Check(device->CreateRenderTargetView(decodedTexture.Get(), nullptr, &decodedView), "CreateRenderTargetView(decoded)");
    Check(device->CreateShaderResourceView(decodedTexture.Get(), nullptr, &decodedSrv), "CreateShaderResourceView(decoded)");
    Check(device->CreateRenderTargetView(antiAliasedTexture.Get(), nullptr, &antiAliasedView), "CreateRenderTargetView(aa)");
    Check(device->CreateShaderResourceView(antiAliasedTexture.Get(), nullptr, &antiAliasedSrv), "CreateShaderResourceView(aa)");
}
// EASU 출력 텍스처를 출력 사각형 크기로 맞춘다.
void GpuPresenter::Impl::EnsureEasuTarget(int width, int height) {
    if (easuTexture && easuWidth == width && easuHeight == height) return;
    easuWidth = width;
    easuHeight = height;
    easuTexture = MakeTexture(width, height, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0);
    Check(device->CreateRenderTargetView(easuTexture.Get(), nullptr, &easuView), "CreateRenderTargetView(easu)");
    Check(device->CreateShaderResourceView(easuTexture.Get(), nullptr, &easuSrv), "CreateShaderResourceView(easu)");
}
// 화면 전체 삼각형으로 한 패스를 그린다. input은 t0, decode 패스는 t1/t2에 번호/팔레트를 묶는다.
void GpuPresenter::Impl::Draw(ID3D11RenderTargetView* target, const D3D11_VIEWPORT& viewport, ID3D11PixelShader* pixel, ID3D11ShaderResourceView* input, const ParamsBuffer& params, bool decode) {
    context->UpdateSubresource(paramsBuffer.Get(), 0, nullptr, &params, 0, 0);
    ID3D11ShaderResourceView* views[3] = {input, decode ? indexSrv.Get() : nullptr, decode ? paletteSrv.Get() : nullptr};
    ID3D11SamplerState* samplers[1] = {linearSampler.Get()};
    ID3D11Buffer* buffers[1] = {paramsBuffer.Get()};
    context->OMSetRenderTargets(1, &target, nullptr);
    context->RSSetViewports(1, &viewport);
    context->RSSetState(rasterizer.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(nullptr);
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixel, nullptr, 0);
    context->PSSetShaderResources(0, 3, views);
    context->PSSetSamplers(0, 1, samplers);
    context->PSSetConstantBuffers(0, 1, buffers);
    context->Draw(3, 0);
    ID3D11ShaderResourceView* none[3] = {nullptr, nullptr, nullptr}; // 다음 패스가 같은 텍스처를 쓰기 대상으로 묶을 수 있게 입력을 푼다.
    context->PSSetShaderResources(0, 3, none);
    ID3D11RenderTargetView* noTarget[1] = {nullptr};
    context->OMSetRenderTargets(1, noTarget, nullptr);
}

GpuPresenter::GpuPresenter() : impl_(std::make_unique<Impl>()) {}
GpuPresenter::~GpuPresenter() = default;
// 장치를 만들 수 있는지만 확인한다.
bool GpuPresenter::Probe(std::string* adapterName) {
    try {
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;
        std::string name;
        CreateDevice(device, context, name);
        if (adapterName) *adapterName = name;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}
// 장치, 스왑 체인, 셰이더를 준비한다.
std::unique_ptr<GpuPresenter> GpuPresenter::Create(void* window, std::string& error) {
    std::unique_ptr<GpuPresenter> presenter(new GpuPresenter());
    auto& s = *presenter->impl_;
    try {
        s.window = static_cast<HWND>(window);
        if (!s.window || !IsWindow(s.window)) throw std::runtime_error("유효한 창 핸들이 필요하다");
        CreateDevice(s.device, s.context, s.adapterName);
        s.CreateSwapChain();
        D3D11_BUFFER_DESC bufferDescription{};
        bufferDescription.ByteWidth = sizeof(ParamsBuffer);
        bufferDescription.Usage = D3D11_USAGE_DEFAULT;
        bufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        Check(s.device->CreateBuffer(&bufferDescription, nullptr, &s.paramsBuffer), "CreateBuffer(Params)");
        D3D11_SAMPLER_DESC samplerDescription{};
        samplerDescription.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        samplerDescription.AddressU = samplerDescription.AddressV = samplerDescription.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDescription.MaxLOD = D3D11_FLOAT32_MAX;
        Check(s.device->CreateSamplerState(&samplerDescription, &s.linearSampler), "CreateSamplerState");
        D3D11_RASTERIZER_DESC rasterizerDescription{};
        rasterizerDescription.FillMode = D3D11_FILL_SOLID;
        rasterizerDescription.CullMode = D3D11_CULL_NONE;
        Check(s.device->CreateRasterizerState(&rasterizerDescription, &s.rasterizer), "CreateRasterizerState");
        s.vertexShader = s.Compile<ID3D11VertexShader>("VSMain", "vs_5_0");
        s.decodePs = s.Compile<ID3D11PixelShader>("PSDecode", "ps_5_0");
        s.antiAliasPs = s.Compile<ID3D11PixelShader>("PSAntiAlias", "ps_5_0");
        s.easuPs = s.Compile<ID3D11PixelShader>("PSEasu", "ps_5_0");
        s.rcasPs = s.Compile<ID3D11PixelShader>("PSRcas", "ps_5_0");
        s.blitPs = s.Compile<ID3D11PixelShader>("PSBlit", "ps_5_0");
    } catch (const std::exception& problem) {
        error = problem.what();
        return nullptr;
    }
    return presenter;
}
const std::string& GpuPresenter::AdapterName() const { return impl_->adapterName; }
const std::string& GpuPresenter::LastError() const { return impl_->lastError; }
// 한 프레임을 후처리해 표시한다.
bool GpuPresenter::Present(const std::uint8_t* pixels, int pitch, int width, int height, const std::uint32_t* palette256, GpuOutputRect output, const GpuPresentOptions& options) {
    auto& s = *impl_;
    try {
        if (!pixels || !palette256 || width <= 0 || height <= 0 || pitch < width) throw std::invalid_argument("논리 프레임 인자가 올바르지 않다");
        RECT client{};
        GetClientRect(s.window, &client);
        if (client.right <= 0 || client.bottom <= 0) return true; // 최소화 등으로 그릴 영역이 없다.
        if (client.right != s.swapWidth || client.bottom != s.swapHeight) s.ResizeSwapChain(client.right, client.bottom);
        const int outputWidth = output.right - output.left, outputHeight = output.bottom - output.top;
        if (outputWidth <= 0 || outputHeight <= 0) throw std::invalid_argument("출력 사각형이 비어 있다");
        s.EnsureSourceTargets(width, height);
        // 1) 번호와 팔레트를 올린다.
        D3D11_MAPPED_SUBRESOURCE mapped{};
        Check(s.context->Map(s.indexTexture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped), "Map(index)");
        for (int row = 0; row < height; ++row) std::memcpy(static_cast<std::uint8_t*>(mapped.pData) + static_cast<std::size_t>(row) * mapped.RowPitch, pixels + static_cast<std::size_t>(row) * pitch, static_cast<std::size_t>(width));
        s.context->Unmap(s.indexTexture.Get(), 0);
        Check(s.context->Map(s.paletteTexture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped), "Map(palette)");
        for (int index = 0; index < 256; ++index) static_cast<std::uint32_t*>(mapped.pData)[index] = palette256[index] | 0xff000000u; // 알파는 불투명으로 고정한다.
        s.context->Unmap(s.paletteTexture.Get(), 0);
        // 2) 상수를 채운다.
        ParamsBuffer params{};
        params.sourceSize[0] = static_cast<float>(width);
        params.sourceSize[1] = static_cast<float>(height);
        params.sourceSize[2] = 1.0f / static_cast<float>(width);
        params.sourceSize[3] = 1.0f / static_cast<float>(height);
        params.outputRect[0] = static_cast<float>(output.left);
        params.outputRect[1] = static_cast<float>(output.top);
        params.outputRect[2] = static_cast<float>(outputWidth);
        params.outputRect[3] = static_cast<float>(outputHeight);
        if (options.fsr) {
            const auto easu = ComputeFsrEasuConstants(static_cast<float>(width), static_cast<float>(height), static_cast<float>(outputWidth), static_cast<float>(outputHeight));
            std::memcpy(params.const0, easu.con0.data(), sizeof params.const0);
            std::memcpy(params.const1, easu.con1.data(), sizeof params.const1);
            std::memcpy(params.const2, easu.con2.data(), sizeof params.const2);
            std::memcpy(params.const3, easu.con3.data(), sizeof params.const3);
            const auto rcas = ComputeFsrRcasConstants(options.rcasSharpnessStops);
            std::memcpy(params.rcas, rcas.data(), sizeof params.rcas);
        }
        // 3) 패스를 순서대로 그린다.
        D3D11_VIEWPORT logical{0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f};
        s.Draw(s.decodedView.Get(), logical, s.decodePs.Get(), nullptr, params, true);
        ID3D11ShaderResourceView* current = s.decodedSrv.Get();
        if (options.antiAliasing) {
            s.Draw(s.antiAliasedView.Get(), logical, s.antiAliasPs.Get(), current, params, false);
            current = s.antiAliasedSrv.Get();
        }
        const float black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        s.context->ClearRenderTargetView(s.finalView.Get(), black); // 레터박스 바깥을 검정으로 만든다.
        const D3D11_VIEWPORT outputView{static_cast<float>(output.left), static_cast<float>(output.top), static_cast<float>(outputWidth), static_cast<float>(outputHeight), 0.0f, 1.0f};
        if (options.fsr) {
            s.EnsureEasuTarget(outputWidth, outputHeight);
            const D3D11_VIEWPORT easuView{0.0f, 0.0f, static_cast<float>(outputWidth), static_cast<float>(outputHeight), 0.0f, 1.0f};
            ParamsBuffer easuParams = params;
            easuParams.outputRect[0] = 0.0f; // EASU는 자기 텍스처 왼쪽 위가 원점이다.
            easuParams.outputRect[1] = 0.0f;
            s.Draw(s.easuView.Get(), easuView, s.easuPs.Get(), current, easuParams, false);
            s.Draw(s.finalView.Get(), outputView, s.rcasPs.Get(), s.easuSrv.Get(), params, false);
        } else {
            s.Draw(s.finalView.Get(), outputView, s.blitPs.Get(), current, params, false);
        }
        // 4) 스왑 체인에 복사해 표시한다.
        ComPtr<ID3D11Texture2D> backBuffer;
        Check(s.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)), "GetBuffer");
        s.context->CopyResource(backBuffer.Get(), s.finalTexture.Get());
        Check(s.swapChain->Present(0, 0), "Present");
        s.presented = true;
        s.lastError.clear();
        return true;
    } catch (const std::exception& problem) {
        s.lastError = problem.what();
        return false;
    }
}
// 마지막으로 그린 클라이언트 영역 전체를 읽는다.
bool GpuPresenter::ReadBack(std::vector<std::uint8_t>& bgra, int& width, int& height) {
    auto& s = *impl_;
    if (!s.presented || !s.finalTexture) return false;
    try {
        if (!s.stagingTexture) s.stagingTexture = s.MakeTexture(s.swapWidth, s.swapHeight, DXGI_FORMAT_B8G8R8A8_UNORM, 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ);
        s.context->CopyResource(s.stagingTexture.Get(), s.finalTexture.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        Check(s.context->Map(s.stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped), "Map(staging)");
        width = s.swapWidth;
        height = s.swapHeight;
        bgra.resize(static_cast<std::size_t>(width) * height * 4);
        for (int row = 0; row < height; ++row) std::memcpy(bgra.data() + static_cast<std::size_t>(row) * width * 4, static_cast<const std::uint8_t*>(mapped.pData) + static_cast<std::size_t>(row) * mapped.RowPitch, static_cast<std::size_t>(width) * 4);
        s.context->Unmap(s.stagingTexture.Get(), 0);
        return true;
    } catch (const std::exception& problem) {
        s.lastError = problem.what();
        return false;
    }
}
}
