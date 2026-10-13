// recovery-hash-evidence.json의 실제 두 판본 반환 주소·초기화·해시 단계로 검증한다.
#include "o/SquidHash.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
// 원본 네 배열과 같은 개수의 short를 할당한다. 초기화와 메모리 소유권은 새 C++의 방식이다.
SquidHash::SquidHash(int selected) {
    Select(selected);
    // 원본의 0→3 단계 순서로 모든 버킷을 0으로 만든다.
    for (int level=0;level<static_cast<int>(kScales.size());++level)
        heads_[static_cast<std::size_t>(level)].resize(static_cast<std::size_t>(Side(level)*Side(level)));
}
// 원본의 이미 할당된 배열 초기화와 같은 결과이며 선택 단계를 바꾸지 않는다.
void SquidHash::Reset() {
    // 큰 표면 배열부터 작은 일반 공간 배열까지 모두 지운다.
    for (auto& heads:heads_) std::fill(heads.begin(),heads.end(),std::uint16_t{});
}
// 원본은 일부 호출에서 잘못된 단계를 무시한다. 새 인터페이스는 유효한 0~3만 받는다.
void SquidHash::Select(int level) { Side(level); selected_=level; }
// 선택 상태를 복사 없이 읽는다.
int SquidHash::Selected() const { return selected_; }
// 보드 256을 실제 단계별 크기 1/2/4/16으로 나눈다.
int SquidHash::Side(int level) {
    if (level<0 || level>=static_cast<int>(kScales.size())) throw std::out_of_range("Squid hash level");
    return kWorldCells/kScales[static_cast<std::size_t>(level)];
}
// y 바깥·x 안쪽 주소 계산이며 여기의 x/y는 이미 단계 크기로 나눈 좌표다.
std::size_t SquidHash::Index(int level,int x,int y) {
    const int side=Side(level);
    if (x<0 || y<0 || x>=side || y>=side) throw std::out_of_range("Squid hash bucket cell");
    return static_cast<std::size_t>(y*side+x);
}
// 원본 CRT _ftol의 정상 비음수 월드 입력은 정수 절삭이며 거의 올림을 적용하지 않는다.
std::size_t SquidHash::BucketIndex(int level,float x,float y) {
    Side(level);
    if (!std::isfinite(x) || !std::isfinite(y) || x<0 || y<0 || x>=kWorldCells || y>=kWorldCells)
        throw std::out_of_range("Squid hash world coordinate");
    const int scale=kScales[static_cast<std::size_t>(level)];
    return Index(level,static_cast<int>(x)/scale,static_cast<int>(y)/scale);
}
// 현재 선택한 단계의 쓰기 가능한 머리다. 값은 발자국 점유가 아니라 공간 체인의 첫 객체 번호다.
std::uint16_t& SquidHash::At(int x,int y) { return Cell(selected_,x,y); }
// 같은 버킷 계산의 읽기 전용 경로다.
const std::uint16_t& SquidHash::At(int x,int y) const { return Cell(selected_,x,y); }
// 원본 integer 주소 함수와 같은 단계별 y·x 인덱스를 사용한다.
std::uint16_t& SquidHash::Cell(int level,int x,int y) { return heads_.at(static_cast<std::size_t>(level)).at(Index(level,x,y)); }
// 읽기 전용 조회도 단계를 먼저 검증한다.
const std::uint16_t& SquidHash::Cell(int level,int x,int y) const { return heads_.at(static_cast<std::size_t>(level)).at(Index(level,x,y)); }
// 지정한 단계에서 월드 좌표의 버킷 머리를 쓸 수 있게 한다.
std::uint16_t& SquidHash::Bucket(int level,float x,float y) { return heads_.at(static_cast<std::size_t>(level)).at(BucketIndex(level,x,y)); }
// 실제 표면/일반 조회에서 사용할 동일 주소 계산이다.
const std::uint16_t& SquidHash::Bucket(int level,float x,float y) const { return heads_.at(static_cast<std::size_t>(level)).at(BucketIndex(level,x,y)); }
// 0단계 배열이 원본의 표면 지도 전역에 연결되는 관계를 보존한다.
std::span<const std::uint16_t> SquidHash::Entries(int level) const { Side(level); return heads_[static_cast<std::size_t>(level)]; }
// 발자국 크기 대신 현재 SHP 헤더의 실제 가로/세로 크기를 입력으로 받는다.
int SquidHash::ObjectLevel(std::uint32_t flags2,float frameWidth,float frameHeight) {
    if ((flags2 & (TypeFlag2::kIsland|TypeFlag2::kBridge))!=0) return 0;
    if (!std::isfinite(frameWidth) || !std::isfinite(frameHeight) || frameWidth<0 || frameHeight<0)
        throw std::out_of_range("Squid hash frame size");
    const float size=std::max(frameWidth,frameHeight);
    return size<=2 ? 1 : size<=4 ? 2 : 3;
}
}
