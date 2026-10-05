// 원본 SquidHash.cpp의 4단계 배열 초기화·버킷 주소·Squid 해시 단계 선택을 복원한다.
// 버킷 값은 객체 번호 체인의 머리다. 전체 발자국 점유·체인 삽입/제거·일반 탐색은 별도다.
#pragma once
#include "o/RiftType.h"
#include "o/TerrainBuilder.h"

namespace netstorm::o {
class SquidHash {
public:
    // 패치 005424f8 / CD 0052d598: 각 해시 버킷이 담당하는 월드 칸 한 변이다.
    static constexpr std::array<int,4> kScales{1,2,4,16};
    // 004b28f0 ↔ CD 00479ad0의 기존 배열 초기화 경로. 새 코드는 메모리를 직접 소유한다.
    explicit SquidHash(int selected=0);
    // 원본 초기화처럼 모든 단계의 머리를 0으로 지우고 현재 선택 단계는 보존한다.
    void Reset();
    // 원본 초기화/탐색기의 단계 선택 대입을 이름 있는 함수로 옮긴다.
    void Select(int level);
    // 현재 선택 단계와 배열 한 변을 읽는다.
    int Selected() const;
    static int Side(int level);
    // 004b29a0 ↔ CD 00479be0: 현재 단계의 버킷 칸 좌표를 조회한다. 월드 좌표가 아니다.
    std::uint16_t& At(int x,int y);
    const std::uint16_t& At(int x,int y) const;
    // 004b2b00 ↔ CD 00479d90: 지정한 단계의 버킷 칸으로 머리를 조회한다.
    std::uint16_t& Cell(int level,int x,int y);
    const std::uint16_t& Cell(int level,int x,int y) const;
    // 004b2a90 ↔ CD 00479d20: 월드 float를 0 방향으로 절삭한 후 단계 크기로 나눈다.
    std::uint16_t& Bucket(int level,float x,float y);
    const std::uint16_t& Bucket(int level,float x,float y) const;
    // 주소를 새 배열의 인덱스로 바꾼 계산. 표면 조회의 +0.9999를 이 함수에 넣지 않는다.
    static std::size_t BucketIndex(int level,float x,float y);
    // SurfaceFinder 등은 0단계를 읽는다. 객체 번호 체인의 삽입/제거는 이 배열을 사용하는 호출자 범위다.
    std::span<const std::uint16_t> Entries(int level) const;
    // 004ace40 ↔ CD 004acb00: 섬/다리는 0, 다른 객체는 현재 SHP 크기의 최대 변으로 1~3을 선택한다.
    static int ObjectLevel(std::uint32_t flags2,float frameWidth,float frameHeight);
private:
    // 배열 밖/잘못된 단계를 원본 포인터 산술로 진행하지 않고 오류로 보고한다.
    static std::size_t Index(int level,int x,int y);
    std::array<std::vector<std::uint16_t>,4> heads_;
    int selected_{};
};
}
