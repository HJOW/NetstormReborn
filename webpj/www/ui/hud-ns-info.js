// 원본 NSInfo(준비 창, 0xd8 바이트 파생)의 일반 미션 부착을 실제 배경 자식과 같은 Gump 트리에 연결한다(원문 0047e9d0, cpppj client/HudNsInfo.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 기본 부착 → 현재 배경 Push → 배경 폭/높이·배율 쓰기 → 배경 부착 → 현재 메타맵/플레이어 인식 관문 순서이며, 전체 자식 생성(HudNsInfoCreate)과 메타맵 전체 배치(0047ea71 이후)는 별도 단위/훅이다.
// 메타맵·플레이어 인식 전역은 복사본 없이 소비 시점에 직접 읽는다(현재 값 객체 또는 조회 함수 모두 허용). 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: const info = new HudNsInfo({ metamap: () => meta, playerKnown: () => known }, hooks); info.attach(token, x, y, parent, mode);
// (2026-11-14 WB04-E444: RawMenuControls 선행 NSInfo 부착 이식)
import { GumpCreateNode } from './gump-create.js';

// 원본 0xd8 바이트 NSInfo 의 전체 자식 토큰이다. 생성/메타맵 본문의 완료를 뜻하지 않는다.
export class HudNsInfoNode extends GumpCreateNode {
    // +a0: 모든 하위 패널의 실제 배경 부모 토큰이다.
    background = 0;
    // +a4~c0: 현재 PlayerInfo 8개의 토큰이다.
    players = new Uint32Array(8);
    // +c4/+c8/+cc/+d0/+d4: 명단·제목·위/아래 구분선·전투 정보 자식 토큰이다.
    roster = 0; title = 0; upperDivider = 0; lowerDivider = 0; battle = 0;
}

// 도달한 실제 공통 효과만 필수로 검사한다(C++ std::function 미연결은 생성자 invalid_argument/본체 runtime_error).
function missing(message) { return new Error(message); }

// 이 부착이 읽는 메타맵(594fc8)·플레이어 인식(595344) 조회 두 개다. 함수가 아니면 같은 객체의 현재 속성을 매번 읽는다.
function modeView(globals) {
    if (typeof globals?.metamap === 'function' && typeof globals.playerKnown === 'function') return globals;
    if (globals && 'metamap' in globals && 'playerKnown' in globals) return { metamap: () => globals.metamap, playerKnown: () => globals.playerKnown };
    return null;
}

// 일반 미션의 NSInfo 부착 본체다. hooks = { node(token), base(token), baseAttach(token,x,y,parent,mode), push(token), attach(token,x,y,parent,mode), metamapLayout(token) } 이다.
// metamapLayout 은 메타맵 관문을 통과한 부착에서만 필요하다(없으면 Error).
export class HudNsInfo {
    // 현재 모드 조회와 훅 묶음이다(복제하지 않는다).
    #globals; #hooks;
    // 같은 노드/트리 연결이 없으면 TypeError 다(C++ invalid_argument).
    constructor(globals, hooks) {
        const view = modeView(globals);
        if (!view || !hooks || typeof hooks.node !== 'function' || typeof hooks.base !== 'function' || typeof hooks.baseAttach !== 'function' || typeof hooks.push !== 'function' || typeof hooks.attach !== 'function') throw new TypeError('NSInfo의 같은 노드/트리 연결 필요');
        this.#globals = view; this.#hooks = hooks;
    }

    // 호출 중 같은 주소를 유지해야 하는 파생 노드를 조회한다(없으면 Error).
    #node(token) {
        const node = token ? this.#hooks.node(token) : null;
        if (!node) throw missing('NSInfo 실제 저장소 없음');
        return node;
    }

    // 배경은 같은 공통 Gump 저장소에서 조회하며 임시 복사본을 받지 않는다(없으면 Error).
    #base(token) {
        const node = token ? this.#hooks.base(token) : null;
        if (!node) throw missing('NSInfo 현재 배경 없음');
        return node;
    }

    // 0047e9d0: 기본 부착 → 현재 배경 Push → 폭(float32 기본 폭, 배경 표시 폭 = 기본 폭×배경 배율의 float32) → 높이(별도 조회로 같은 계산) → 배경 부착(0,0,token,0) → 메타맵과 플레이어 인식이 모두 켜진 경우에만 전체 메타맵 배치를 호출한다.
    // 현재 배경 토큰은 Push/부착 콜백에서 바뀔 수 있으므로 축마다 다시 읽는다.
    attach(token, x, y, parent, mode) {
        const node = this.#node(token); this.#hooks.baseAttach(token, x, y, parent, mode); this.#hooks.push(node.background);
        const width = node.baseWidth; const horizontal = this.#base(node.background); horizontal.baseWidth = width; horizontal.width = Math.fround(width * horizontal.scale);
        const vertical = this.#base(node.background); const height = node.baseHeight; vertical.baseHeight = height; vertical.height = Math.fround(height * vertical.scale);
        this.#hooks.attach(node.background, 0, 0, token, 0);
        if (!this.#globals.metamap() || !this.#globals.playerKnown()) return;
        if (typeof this.#hooks.metamapLayout !== 'function') throw missing('NSInfo 전체 메타맵 배치 미연결');
        this.#hooks.metamapLayout(token);
    }
}
