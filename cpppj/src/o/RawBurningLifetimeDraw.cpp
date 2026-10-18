// 좌표의 두 정수화와 원본 색 조회/쓰기·반납 순서를 보존한다.
#include "o/RawBurningLifetimeDraw.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 실제 CRT의 정상 화면 좌표에서0 방향 절삭한다.
std::int32_t Truncate(double value) { if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0) throw std::out_of_range("Burning Draw 원본 정수화 범위 오류");return static_cast<std::int32_t>(value); }
// 원본 signed DWORD 덧셈의 감김이다.
std::int32_t Add(std::int32_t value,std::int32_t amount) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(value)+std::bit_cast<std::uint32_t>(amount)); }
// 실제487e50의 두 float 저장 뒤 넓은 곱이며 life9에서는 인접 DWORD8을 읽을 수 있다.
std::uint8_t Color(float life,const BurningDrawPalette& palette) {
    if (palette.count<0) throw std::invalid_argument("Burning Draw 음수 색수");const auto reciprocal=static_cast<float>(1.0/9.0);
    const auto index=std::max(0,Truncate(static_cast<double>(static_cast<float>(palette.count))*std::min(life,9.0f)*reciprocal));
    if (static_cast<std::size_t>(index)>=palette.readable.size()) throw std::out_of_range("Burning Draw 인접 색 DWORD 미공급");return static_cast<std::uint8_t>(palette.readable[static_cast<std::size_t>(index)]);
}
}
// 보호VT 기록값과 실제 타입을 함께 확인한다.
bool IsBurningArcherCapture(std::span<const std::uint8_t> raw) { const auto table=std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24);return (raw[10]==71 && table==0x50b888U) || (raw[10]==128 && table==0x50b9b8U); }
// +78은TYPE+128이며 캐논의+a4는ret8다. 발사기는 현재 그림 뒤 출력WORD bit0이 꺼졌을 때만 포구 디스크를 더한다.
void DrawBurningArcherCapture(std::span<const std::uint8_t> raw,std::int32_t base,std::int32_t muzzleX,std::int32_t muzzleY,const std::function<void(std::uint32_t,std::uint32_t,std::int32_t,std::int32_t)>& draw,std::int32_t x,std::int32_t y) {
    if (!IsBurningArcherCapture(raw) || !draw) throw std::invalid_argument("연소 캡처의 다른 발사기/그림 경계");if (raw[10]==128) return;
    const auto current=std::uint32_t(raw[36])|(std::uint32_t(raw[37])<<8)|(std::uint32_t(raw[38])<<16)|(std::uint32_t(raw[39])<<24);
    if (raw[40]&9U) { draw(71,current,x,y);return; }if (current==std::bit_cast<std::uint32_t>(base)) return;draw(71,current,x,y);if (!(raw[12]&1U)) draw(83,0,Add(x,muzzleX),Add(y,muzzleY));
}
// 등록된 현재slot과 원본 현재DWORD 프레임만 사용한다.
bool RenderProcessFrameDue(std::int32_t slot,std::uint32_t frame) { return slot<7 || ((std::bit_cast<std::uint32_t>(slot)^frame)&3U)==0; }
// 별도 source 입자는 그리지 않고 양수 수명의 이동 입자만 사용 수까지 순회한다.
void DrawBurningVisual(const BurningVisualFields& fields,const BurningFrameState& state,const BurningDrawPalette& palette,const std::function<void(std::int32_t,std::int32_t,std::uint8_t)>& pixel) {
    if (!fields.capacity) return;if (fields.total>fields.capacity || fields.particles.size()!=fields.capacity || !pixel) throw std::invalid_argument("Burning Draw 실제 입자/출력 연결 오류");
    const auto& clip=state.viewport; // 원본 VFX의 현재 dirty clip이며 전체 viewport로 확대하지 않는다.
    for (std::uint32_t index=0;index<fields.total;++index) { // 원본은 capacity 대신 현재total을 순회한다.
        const auto& particle=fields.particles[index];if (!(particle.life>0) || particle.source) continue;
        const auto fx=static_cast<float>(static_cast<double>(Truncate(particle.x))-state.cameraX),fy=static_cast<float>(static_cast<double>(Truncate(particle.y))-state.cameraY); // 먼저 좌표정수화→카메라감산→float 저장이다.
        const auto color=Color(particle.life,palette);const auto x=Truncate(fx),y=Truncate(fy),right=Add(x,1),top=Add(y,-2); // 색 조회 뒤 두 번째 정수화를 수행한다.
        if (x<clip.left || x>=clip.right || y<clip.top || y>=clip.bottom || right<clip.left || right>=clip.right || top<clip.top || top>=clip.bottom) continue;
        pixel(x,y,color);pixel(right,y,color);pixel(right,Add(y,-1),color);pixel(x,Add(y,-1),color);pixel(x,top,color);
    }
}
// 공유 팔레트는 월드 소유라 이 소멸자가 반납하지 않는다.
void DestroyBurningVisual(BurningVisualFields& fields,RawRenderRegistry& registry,std::uint32_t flags,const std::function<void(BurningRelease)>& release,const std::function<void(std::uint32_t)>& stopSound) {
    if (!release || !stopSound) throw std::invalid_argument("Burning 소멸 실제 반납/소리 경계 누락");
    if (!fields.image.pixels.empty()) { release(BurningRelease::Image);std::vector<std::uint8_t>().swap(fields.image.pixels); }
    if (!fields.rows.empty()) { release(BurningRelease::Rows);std::vector<std::uint8_t>().swap(fields.rows); }
    stopSound(fields.sound);
    if (!fields.particles.empty()) { release(BurningRelease::Particles);std::vector<BurningParticle>().swap(fields.particles); }
    registry.Destroy(fields.registration);if (flags&1U) release(BurningRelease::Object);
}
}
