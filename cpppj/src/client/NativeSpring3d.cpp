// 원본 spring의 완료 비트·저장 폭·시간 반복 순서를 보존한다.
#include "client/NativeSpring3d.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client { // 실제 Rig의 생성/프레임 부모가 같은 필드 뷰를 넘긴다.
namespace { // 보호 PE의 상수와 순수 거리 계산만 공유한다.
constexpr double kStep=0x1.1111111111111p-5; //00501a10의 double 1/30초이며 float 근사로 바꾸지 않는다.
constexpr float kNear=0.1818181872367859F; //00501a0c의 도착 거리 임계값이다.
constexpr std::uint32_t kHorizontalDone=0x40000000U,kHeightDone=0x20000000U; // 두 축 묶음의 별도 완료 비트다.
double Distance(float x,float y,float tx=0.0F,float ty=0.0F) { const double dx=static_cast<double>(x)-tx,dy=static_cast<double>(y)-ty;return std::sqrt(dx*dx+dy*dy); } //0041cfc0의 차이·제곱합에 중간 F32 저장을 넣지 않는다.
}
// 네이티브 포인터 밖 접근만 거부하며 입력 수치/플래그의 정규화는 하지 않는다.
NativeSpring3d::NativeSpring3d(std::span<std::uint8_t> fields,NativeSpring3dGlobals globals):fields_(fields),globals_(globals) { if (fields.size()<0x5c) throw std::invalid_argument("Animator3d spring 실제 필드 범위 부족"); }
// 비정렬 원문 DWORD의 하위 바이트부터 조합한다.
std::uint32_t NativeSpring3d::Read(std::size_t at) const { std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(fields_[at+byte])<<(8*byte);return value; }
// 쓰는 필드의 인접 패딩과 다른 축 상태는 보존한다.
void NativeSpring3d::Put(std::size_t at,std::uint32_t value) const { for (std::size_t byte=0;byte<4;++byte) fields_[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); }
float NativeSpring3d::Float(std::size_t at) const { return std::bit_cast<float>(Read(at)); } // 동일 F32 비트를 읽는다.
void NativeSpring3d::Float(std::size_t at,float value) const { Put(at,std::bit_cast<std::uint32_t>(value)); } // 동일 F32 비트를 쓴다.
double NativeSpring3d::Time() const { return std::bit_cast<double>(std::uint64_t(Read(0x50))|(std::uint64_t(Read(0x54))<<32)); } // 하위/상위 DWORD 순서다.
void NativeSpring3d::Time(double value) const { const auto bits=std::bit_cast<std::uint64_t>(value);Put(0x50,static_cast<std::uint32_t>(bits));Put(0x54,static_cast<std::uint32_t>(bits>>32)); } // 실제 QWORD 필드만 변경한다.
// 원문 생성자는 전체 구조를 지우지 않으며 +4c/끝 패딩은 쓰지 않는다.
void NativeSpring3d::Initialize() const {
    Time(0.0);Float(0x2c,8.0F);Float(0x20,8.0F);Put(0xc,0);Put(0,0);Float(0x48,0.5F);Float(0x1c,16.0F);Float(0x24,4.0F);
    Put(0x10,0);Put(4,0);Put(0x14,0);Put(8,0);Time(globals_.now);Put(0x18,kHorizontalDone|kHeightDone);
    Put(0x44,0);Put(0x40,0);Put(0x3c,0);Put(0x38,0);Put(0x34,0);Put(0x30,0);Put(0x28,0);Put(0x58,0);
}
// 원문 마스크는 두 완료 비트만 이전 필드에서 가져오며 나머지는 입력 플래그다.
void NativeSpring3d::Target(float x,float y,float z,std::uint32_t flags) const {
    flags=((Read(0x18)^flags)&(kHorizontalDone|kHeightDone))^flags;Put(0x18,flags);
    if (flags&4U) { Float(0,x);Float(4,y);Put(0x18,flags&~kHorizontalDone); }
    if (Read(0x18)&8U) { Float(8,z);Float(0x48,globals_.fast ? 12.0F:0.5F);if (z<Float(0x14)) Float(0x48,-Float(0x48));Put(0x18,Read(0x18)&~kHeightDone); }
    Time(globals_.now);
}
// 완료 상태여도 호출 횟수와 마지막 좌표 저장은 원문 위치에서 수행한다.
bool NativeSpring3d::Advance() const {
    float y=Float(0x10),z=Float(0x14);Put(0x58,Read(0x58)+1U);double remaining=globals_.now-Time();float x=Float(0xc);bool advanced{};
    // Native는 경과 시간이 정확히 한 간격일 때 아직 진행하지 않으며 반복 상한/시간 clamp도 없다.
    while (kStep<remaining) {
        advanced=true;const auto flags=Read(0x18);
        if (!(flags&kHeightDone)) Put(0x38,Read(0x48));else { Put(0x38,0);Put(0x44,0); }
        if (flags&kHorizontalDone) { Put(0x40,0);Put(0x3c,0);Put(0x34,0);Put(0x30,0); }
        x=static_cast<float>(static_cast<double>(Float(0x3c))*kStep+x);y=static_cast<float>(static_cast<double>(Float(0x40))*kStep+y);z=static_cast<float>(static_cast<double>(Float(0x44))*kStep+z);
        if (flags&0x10U) z=globals_.cameraHeight;
        const auto distance=static_cast<float>(Distance(x,y,Float(0),Float(4)));
        if (!(Read(0x18)&kHorizontalDone) && distance<kNear) { x=Float(0);y=Float(4);Put(0x18,Read(0x18)|kHorizontalDone); }
        if (!(Read(0x18)&kHeightDone) && ((Float(0x38)<0.0F && z<=Float(8)) || (Float(0x38)>0.0F && z>=Float(8)) || Float(0x38)==0.0F)) { z=Float(8);Put(0x18,Read(0x18)|kHeightDone); }
        if (!(Read(0x18)&kHorizontalDone)) {
            const double vx=static_cast<double>(Float(0x30))*kStep+Float(0x3c),vy=static_cast<double>(Float(0x34))*kStep+Float(0x40);Float(0x3c,static_cast<float>(vx));Float(0x40,static_cast<float>(vy));
            if (Float(0x20)<std::hypot(vx,vy)) { float nx=Float(0x3c),ny=Float(0x40);const double length=Distance(nx,ny);if (length!=0.0) { const double inverse=1.0/length;nx=static_cast<float>(nx*inverse);ny=static_cast<float>(ny*inverse); }Float(0x3c,static_cast<float>(static_cast<double>(Float(0x20))*nx));Float(0x40,static_cast<float>(static_cast<double>(Float(0x20))*ny)); }
            const double inverse=1.0/static_cast<double>(distance);const float dx=static_cast<float>((static_cast<double>(Float(0))-x)*inverse),dy=static_cast<float>((static_cast<double>(Float(4))-y)*inverse);
            float nx=Float(0x3c),ny=Float(0x40);Float(0x28,Float(0x1c));const double pull=static_cast<double>(Float(0x1c))/Float(0x24);const float ax=static_cast<float>(dx*pull),ay=static_cast<float>(dy*pull);
            const double length=Distance(nx,ny);double normalizedY=ny;if (length!=0.0) { const double scale=1.0/length;nx=static_cast<float>(nx*scale);normalizedY=ny*scale; }
            const double damping=static_cast<double>(Float(0x2c))/Float(0x24);nx=static_cast<float>(-(nx*damping));ny=-static_cast<float>(normalizedY*damping);Float(0x30,static_cast<float>(static_cast<double>(nx)+ax));Float(0x34,static_cast<float>(static_cast<double>(ny)+ay));
        }
        if (!(Read(0x18)&kHeightDone)) Float(0x44,static_cast<float>(static_cast<double>(Float(0x38))*kStep+Float(0x44)));
        remaining-=kStep;Time(Time()+kStep);
    }
    Float(0xc,x);Float(0x10,y);Float(0x14,z);return advanced;
}
}
