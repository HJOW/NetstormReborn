// 원본 다중 줄 스타일 입력을 같은 TextGump 문맥/태그·입력 버퍼·그림·가상 표시 요청에 연결한다.
#pragma once
#include "client/TextGump.h"
#include "client/StyleTextMarkup.h"

namespace netstorm::client {
// 조회는 생성/표시/입력 처리기와 같은 안정된 객체를 반환해야 한다. 복사본이나 파서 전용 노드를 만들지 않는다.
struct ScrollingTextGumpMarkupHooks {
    std::function<ScrollingTextGumpNode*(std::uint32_t)> node;
    std::function<GumpCreateNode*(std::uint32_t)> base;
};
// 004ba8f0/CD 004b6c80의 다중 줄 파서다. 짧은 명령과 정규식은 기존 StyleText 처리기를 공유한다.
class ScrollingTextGumpMarkup {
public:
    // 같은 판본·조각/그림/팔레트/옵션을 사용하는 처리기와 노드 저장소는 이 파서보다 오래 살아야 한다.
    ScrollingTextGumpMarkup(const ScrollingTextGump& text,const StyleText& style,const Markup& pictures,
        const StyleTextMarkup& markup,ScrollingTextGumpMarkupHooks hooks);
    // 현재 기본 문맥을 복사하되 ~./~n에서는 현재 기본 문맥을 재조회한다. T 태그는 +c8에 호출 간 유지된다.
    // 구간/리터럴/그림마다 Append→dirty→Redraw를 호출하며 마지막 빈 구간도 버퍼 문자열 통지를 수행한다.
    void Parse(std::uint32_t token,std::string_view text) const;
private:
    // 기본/파생 조회가 같은 주소인지 실제 도달 시 검사하고 소멸·미등록 토큰을 거부한다.
    ScrollingTextGumpNode& Node(std::uint32_t token) const;
    const ScrollingTextGump& text_;
    const StyleText& style_;
    const Markup& pictures_;
    const StyleTextMarkup& markup_;
    ScrollingTextGumpMarkupHooks hooks_;
};
}
