// 돌 버튼이 이미 쓰는 단일 공통 Gump 트리의 실제 처리기 묶음이다(cpppj client/RawMenuControls.h 의 RawMenuTree, 기준선 1f00a25a). 원본 상태/등록표 사본을 만들지 않고 같은 객체를 빌려 주는 접합 구조체다.
// 필수 멤버가 없으면 생성 때 거부하고, 선택 멤버(background·baseRedraw·invalidate)가 없으면 같은 Client 의 변경 통지(`changed`)로 대신한다. 모든 멤버는 호출보다 오래 살아야 한다.
// 사용: const tree = createRawMenuTree({ create, life, factory, collection, tree, lifetime, events, input, stones, token, node, attach, changed }); tree.requestInvalidate(rect, flags). (2026-11-14 WB04-E462)

// 필수 멤버 이름이다(C++ 참조 멤버와 필수 std::function). 하나라도 없으면 TypeError 이다.
const REQUIRED_OBJECTS = ['create', 'life', 'factory', 'collection', 'tree', 'lifetime', 'events', 'input', 'stones'];
const REQUIRED_FUNCTIONS = ['token', 'node', 'attach', 'changed'];

// 같은 공통 처리기들을 한 묶음으로 만든다. 원본 멤버: create(공통 등록표와 루트)·life(overlay/idle/삭제 목록)·factory(기본 생성자)·collection(위치/경계/수집)·tree(부착/분리)·lifetime(자식 우선 수명)
// ·events(재귀 통지)·input(focus/overlay 입력)·stones(Slider 화살표의 돌 버튼 생성자), background(token)→공통 돌 버튼 소유 배경, token()→단조 증가 호스트 식별값, node(token)→모든 파생의 같은 기본 노드,
// attach(token,x,y,parent,mode)→파생 가상 부착, changed()→표시 변경 요청, baseRedraw(token,erase)→기본 Gump 재그리기 요청, invalidate(rect,flags)→현재 Renderer 에 사각형/flags 전달.
export function createRawMenuTree(parts) {
    if (!parts || typeof parts !== 'object') throw new TypeError('RawMenuTree 공통 처리기 묶음 필요');
    for (const name of REQUIRED_OBJECTS) if (!parts[name] || typeof parts[name] !== 'object') throw new TypeError(`RawMenuTree 필수 멤버 없음: ${name}`);
    for (const name of REQUIRED_FUNCTIONS) if (typeof parts[name] !== 'function') throw new TypeError(`RawMenuTree 필수 멤버 없음: ${name}`);
    for (const name of ['background', 'baseRedraw', 'invalidate']) if (parts[name] !== undefined && parts[name] !== null && typeof parts[name] !== 'function') throw new TypeError(`RawMenuTree 선택 멤버는 함수여야 함: ${name}`);
    const tree = { ...parts };
    // 원문 파생이 이미 계산한 사각형/flags 를 현재 Renderer 에 전달한다. 장치 없는 독립 표시에서는 기존 변경 요청을 유지한다(C++ Invalidate).
    tree.requestInvalidate = (rect, flags) => { if (parts.invalidate) parts.invalidate(rect, flags); else parts.changed(); };
    // 같은 기본 Gump 의 실제 token/인수를 전달한다. 파생 Redraw 의 가상 분배는 호출자가 유지한다. 장치 없는 독립 표시에서는 기존 변경 통지를 유지한다(C++ RedrawBase).
    tree.requestRedrawBase = (token, erase) => { if (parts.baseRedraw) parts.baseRedraw(token, erase); else parts.changed(); };
    return tree;
}
