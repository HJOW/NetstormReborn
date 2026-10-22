// 사제491db0의 임시TYPE 속도/신전 복귀와 같은 영역 임의 이동을 복원한다.
#pragma once
#include "o/RawDudeNotice.h"

namespace netstorm::o {
// 실제 현재 신전/HP/지면·영역/난수와 경로 시작만 큰 하위로 연결한다.
struct PriestPanicHooks {
    std::function<Sid(float,float)> temple; //46f950의 plain 좌표→현재 섬 신전이다.
    std::function<std::int32_t(Sid)> maximum; //4adcb0의 현재 최대HP다.
    std::function<bool(float,float)> land; //46f6e0의 plain 좌표→현재 isle TYPE 일치다.
    std::function<std::uint32_t(float,float)> region; //46f530의 거의올림 후 현재 지역이다.
    std::function<double(float,float)> random; //455930의 두float 끝점과53비트 중간 결과다.
    std::function<std::uintptr_t(Sid,Sid)> target; //4894e0(actor,신전,0,0)의 실제 경로 주소다.
    std::function<std::uintptr_t(Sid,float,float)> point; //489080(actor,x,y,0,0,0,0,0)의 실제 경로 주소다.
};
// 같은 mutable TYPE 속도를 경로 생성자가 관측하고 같은 원래 레코드에 절삭값을 복원한다.
class RawPriestPanic {
public:
    RawPriestPanic(const SidPool& pool,std::span<DudeNoticeTypeData> speeds,PriestPanicHooks hooks); // PATCH 풀과188개 현재 속도 표를 빌린다.
    void Run(Sid priest) const; //491db0 전체 순서를 실행하며 첫 실제 임의 경로 시도 뒤에는 실패해도 반환한다.
private:
    std::uint32_t Read(Sid sid,std::size_t at,std::size_t width=4) const; // 현재 비정렬 raw 필드를 실제 폭으로 조회한다.
    const SidPool& pool_; // 효과 뒤 위치/owner/HP/TYPE를 다시 읽는 원본 풀이다.
    std::span<DudeNoticeTypeData> speeds_; // 기존 Dude/Path와 공유하는 현재TYPE+e0다.
    PriestPanicHooks hooks_; // 실제 도달한 큰 하위의 현재 효과다.
};
}
