// 원본 NSInfo(준비 창)와 항상 생성되는 PlayerInfo/BattleInfo/특수 버튼의 실제 부모 생성자를 옮긴다(원문 0047ca50/0047c4b0/00479d40/0047afa0/004256d0/00479fb0/0047b480, cpppj client/HudNsInfoCreate.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 생성은 항상 전체 자식을 만들며(일반/메타 모드 관문은 PlayerInfo/BattleInfo 의 가상 부착에만 있다) 부모 생성자의 현재 포인터 재조회·형제 생성 순서·초기 파싱/크기 갱신을 그대로 보존한다.
// 하위 생성자/번역/문자열/TYPE 은 실제 기존 처리기를 부르는 호스트 훅이고, 선택하지 않은 효과는 요구하지 않는다. 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: const create = new HudNsInfoCreate({ value: 0 }, hooks); const token = create.createHidden(); create.attachPlayer(token, x, y, parent, mode).
// (2026-11-14 WB04-E444: RawMenuControls 선행 NSInfo 생성 이식)
import { GumpCreateNode } from './gump-create.js';
import { ButtonGumpNode } from './button-gump.js';

// 보호 PE 의 실제 종류 표 주소(실행 포인터가 아닌 근거 식별자)다: NSInfo/PlayerInfo/BattleInfo/특수 버튼.
const NS_VTABLE = 0x0050d074; const PLAYER_VTABLE = 0x0050cd64; const BATTLE_VTABLE = 0x0050ce00; const BUTTON_VTABLE = 0x005038e8;
// 번역 입력의 원문 문자열 토큰이다(마스터 표시 / Start 돌 버튼 글자).
const MASTER_TEXT = 0x0050cd3c; const START_TEXT = 0x0050cd2c;

// PlayerInfo 가 후속 메타맵 전체 배치를 구분하는 자식 종류다.
export const HudNsInfoChild = Object.freeze({ Player: 0, Battle: 1 });

// PlayerInfo 원문 0xbc 의 미작성 필드까지 같은 저장소에 보존하는 노드다.
export class HudPlayerInfoNode extends GumpCreateNode {
    // +a0/+a4/+a8/+ac: 스크롤 글·마스터 표시·특수 버튼·Start 돌 버튼 토큰이다.
    text = 0; master = 0; button = 0; start = 0;
    // +b0/+b4/+b8: 현재 메타 플레이어/버튼 TYPE/상태다.
    player = 0; buttonType = 0; reserved = 0;
}

// BattleInfo 원문 0x16c 의 선택 값과 22개의 현재 설정 쌍이다.
export class HudBattleInfoNode extends GumpCreateNode {
    // +a0/+a4/+a8/+ac/+b0: 제목·요약·본문 글 토큰과 전투/변경 값이다.
    title = 0; summary = 0; body = 0; battle = 0; changed = 0;
    // +b4..+163: 22개의 [값, 활성] 쌍이다.
    settings = Array.from({ length: 22 }, () => [0, 0]);
    // +164..167: 생성자는 첫 BYTE 만 0 으로 쓴다. +168 의 모드 초기값은 2 다(생성자가 쓴다).
    bytes = new Uint8Array(4); mode = 0;
}

// 004256d0 은 기본 버튼을 생성한 뒤 가상 표와 +c4 만 바꾸는 실제 파생 버튼이다. +c0 은 보존하고 +c4 만 0 으로 쓴다.
export class HudPlayerButtonNode extends ButtonGumpNode {
    // +c0 미작성 필드와 +c4 항목이다.
    untouchedC0 = 0; entry = 0;
}

// 선택한 원문 효과를 빈 성공으로 대체하지 않는다(C++ logic_error → Error).
function need(hook) { if (typeof hook !== 'function') throw new Error('NSInfo의 실제 생성/자식 효과 미연결'); return hook; }

// 같은 실제 파생 저장소를 원문 접근 위치에서 요구한다(없으면 Error, C++ runtime_error).
function nodeOf(hook, token) {
    const node = token ? need(hook)(token) : null;
    if (!node) throw new Error('NSInfo의 실제 파생 저장소 없음');
    return node;
}

// 종류 전역의 signed BYTE 를 부호 확장한 DWORD 로 쓴다.
function kind(value) { return ((value << 24) >> 24) >>> 0; }

// 플래그 DWORD 의 최하위 비트(visible)를 끈다.
const hidden = (flags) => (flags & ~1) >>> 0;

// 한 Client 의 NSInfo 생성기다. current 는 00593150 의 장기 소유 DWORD 상자({ value })이고, 훅/파생 저장소도 콜백보다 오래 살아야 한다.
// hooks = { node, player, battle, button, base(token), textFlags(token)/buttonFlags(token) → { value } 상자,
//   read(주소) → 현재 DWORD, allocate(요청 크기) → 토큰, baseConstruct(token, cmd, w, h, flags), background(token, w, h, flags, 1, type, frame) → 토큰, scroll/text(token,0,0,0,0) → 토큰,
//   buttonBase(token, w, h, 0), stone(token,1,1,label,color,0) → 토큰, scrollable/slider/wrap/parseScroll/parseText(token, 값), fitText(token,0,0), fitStone(token),
//   attach/baseAttach(token,x,y,parent,mode), translate(토큰) → 토큰, type(토큰) → 값, serverName() → 토큰, formatZone(format, server, zone) → 토큰, assertion(줄), metamapLayout(kind, token) } 이다.
export class HudNsInfoCreate {
    // 현재 NSInfo 전역 상자와 훅 묶음이다(선택하지 않은 하위 효과는 요구하지 않는다).
    #current; #hooks;
    constructor(current, hooks) { this.#current = current; this.#hooks = hooks; }

    // 같은 등록표의 현재 기본 노드를 요구한다(null/다른 저장소는 실제 접근 위치에서 거부).
    #base(token) { return nodeOf(this.#hooks.base, token); }

    // 생성 중 콜백이 배경 전역을 바꿔도 각 원문 new 뒤에 다시 읽는다: 폭 0xb8 을 할당하고 현재 frame(005ca924)/TYPE(005ca920)로 실제 배경을 만든다. 할당 실패(0)면 하위 생성자를 실행하지 않는다.
    #newBackground(flags) {
        const token = need(this.#hooks.allocate)(0xb8); if (!token) return 0;
        const frame = need(this.#hooks.read)(0x005ca924); const type = this.#hooks.read(0x005ca920);
        return need(this.#hooks.background)(token, 1, 1, flags, 1, type, frame);
    }

    // 네 생성 인자는 항상 0 이다. 스크롤 글은 0xd8, 일반 글은 0xa4 를 할당하고 할당 실패 시 하위 생성자는 실행하지 않는다.
    #newText(scrolling) {
        const token = need(this.#hooks.allocate)(scrolling ? 0xd8 : 0xa4); if (!token) return 0;
        return scrolling ? need(this.#hooks.scroll)(token, 0, 0, 0, 0) : need(this.#hooks.text)(token, 0, 0, 0, 0);
    }

    // 가상 분배기의 같은 자식 토큰과 현재 부모를 그대로 전달한다(자식 위치 0,0 / 모드 0).
    #attach(token, parent) { this.#base(token); need(this.#hooks.attach)(token, 0, 0, parent, 0); }

    // 0047ca50: 0xd8 을 할당해 전체 생성하고 current 에 저장한 뒤 반환 노드의 visible 비트를 끈다. 숨김 앞에서도 전체 자식/번역/초기 갱신을 생략하지 않는다.
    createHidden() {
        const token = need(this.#hooks.allocate)(0xd8); const result = token ? this.construct(token) : 0; this.#current.value = result;
        if (result) { const base = this.#base(result); base.flags = hidden(base.flags); }
        return result;
    }

    // 0047c4b0: NSInfo 의 배경 직계 자식 1개와 그 배경 아래 필수 자식(명단·제목·위/아래 구분선·전투 정보·플레이어 8개)을 항상 실제로 생성한다.
    construct(token) {
        const hooks = this.#hooks; const node = nodeOf(hooks.node, token); need(hooks.baseConstruct)(token, 0, 0, 0, 1); node.vtable = NS_VTABLE; node.command = kind(need(hooks.read)(0x00540421));
        node.background = this.#newBackground(0x02000000); this.#attach(node.background, token);
        node.roster = this.#newText(true); { const base = this.#base(node.roster); base.flags = hidden(base.flags); }
        { const box = need(hooks.textFlags)(node.roster); box.value = (box.value | 8) >>> 0; }
        need(hooks.scrollable)(node.roster, 1); need(hooks.slider)(node.roster, 1); this.#attach(node.roster, node.background);
        node.title = this.#newText(true); const format = need(hooks.translate)(need(hooks.read)(0x00540bac));
        const zone = hooks.read(0x005467b8) | 0; const server = need(hooks.serverName)();
        const text = need(hooks.formatZone)(format, server, zone); need(hooks.parseScroll)(node.title, text);
        { const base = this.#base(node.title); base.flags = hidden(base.flags); } this.#attach(node.title, node.background);
        node.upperDivider = this.#newBackground(0x02a00000); { const base = this.#base(node.upperDivider); base.flags = hidden(base.flags); } this.#attach(node.upperDivider, node.background);
        node.lowerDivider = this.#newBackground(0x02a00000); { const base = this.#base(node.lowerDivider); base.flags = hidden(base.flags); } this.#attach(node.lowerDivider, node.background);
        const battle = need(hooks.allocate)(0x16c); node.battle = battle ? this.constructBattle(battle) : 0;
        { const base = this.#base(node.battle); base.flags = hidden(base.flags); } this.#attach(node.battle, node.background);
        // 원문 배열의 8개 주소는 고정이지만 각 부착의 배경은 현재 값을 다시 읽는다.
        for (let index = 0; index < node.players.length; index += 1) {
            const next = need(hooks.allocate)(0xbc); node.players[index] = next ? this.constructPlayer(next) : 0; this.#attach(node.players[index], node.background);
        }
        node.flags = (node.flags | 0x800) >>> 0; return token;
    }

    // 00479d40: PlayerInfo 의 스크롤 글·실제 파생 버튼(16×15, +ac 플래그 6)·마스터 표시·Start 돌 버튼을 항상 생성하고 같은 부모에 부착한다. 버튼 TYPE 이 0 이면 진단(줄 1912)을 알리고 계속한다.
    constructPlayer(token) {
        const hooks = this.#hooks; const node = nodeOf(hooks.player, token); need(hooks.baseConstruct)(token, 0, 0, 0, 1); node.vtable = PLAYER_VTABLE; node.reserved = 0;
        node.command = kind(need(hooks.read)(0x00540440)); node.player = 0;
        node.text = this.#newText(true); need(hooks.scrollable)(node.text, 0);
        node.buttonType = need(hooks.type)(hooks.read(0x005411c8)); if (!node.buttonType) need(hooks.assertion)(1912);
        const button = need(hooks.allocate)(0xc8); node.button = button ? this.constructButton(button, 16, 15) : 0; { const box = need(hooks.buttonFlags)(node.button); box.value = (box.value | 6) >>> 0; }
        node.master = this.#newText(false); const master = need(hooks.translate)(MASTER_TEXT); need(hooks.parseText)(node.master, master); need(hooks.fitText)(node.master, 0, 0);
        const stone = need(hooks.allocate)(0x170);
        if (stone) { const label = hooks.translate(START_TEXT); const color = hooks.read(0x005bec3c); node.start = need(hooks.stone)(stone, 1, 1, label, color, 0); } else node.start = 0;
        need(hooks.fitStone)(node.start); this.#attach(node.text, token); this.#attach(node.button, token); this.#attach(node.start, token); this.#attach(node.master, token); return token;
    }

    // 0047afa0: BattleInfo 의 22개 설정 쌍(뒤 값 1 → 앞 값 0 순서)·모드 2·종류·첫 BYTE 0·스크롤 제목/일반 요약/스크롤 본문 자식을 초기화한다.
    constructBattle(token) {
        const hooks = this.#hooks; const node = nodeOf(hooks.battle, token); need(hooks.baseConstruct)(token, 0, 0, 0, 1); node.vtable = BATTLE_VTABLE;
        // 22개 설정 쌍을 원문 순서(뒤 값 1 → 앞 값 0)로 쓴다.
        for (const setting of node.settings) { setting[1] = 1; setting[0] = 0; }
        node.mode = 2; node.command = kind(need(hooks.read)(0x00540457)); node.bytes[0] = 0;
        node.title = this.#newText(true); need(hooks.scrollable)(node.title, 0); need(hooks.wrap)(node.title, 0); this.#attach(node.title, token);
        node.summary = this.#newText(false); need(hooks.fitText)(node.summary, 0, 0); this.#attach(node.summary, token);
        node.body = this.#newText(true); need(hooks.scrollable)(node.body, 0); need(hooks.wrap)(node.body, 0); this.#attach(node.body, token);
        node.flags = (node.flags | 0x800) >>> 0; node.battle = 0; node.changed = 0; return token;
    }

    // 004256d0: 원문 기본 Button 생성자를 그대로 쓰고 +c0 등 미작성 필드는 보존한 채 가상 표와 +c4 만 쓴다.
    constructButton(token, width, height) {
        const node = nodeOf(this.#hooks.button, token); need(this.#hooks.buttonBase)(token, width, height, 0); node.vtable = BUTTON_VTABLE; node.entry = 0; return token;
    }

    // 00479fb0/0047b480 의 일반 부착: 공통 부착 뒤에만 현재 메타맵/플레이어 인식을 읽고, 둘 다 켜진 경우에만 전체 메타맵 배치(종류, 토큰)를 부른다.
    #attachChild(childKind, token, x, y, parent, mode) {
        need(this.#hooks.baseAttach)(token, x, y, parent, mode);
        if (!need(this.#hooks.read)(0x00594fc8) || !this.#hooks.read(0x00595344)) return;
        need(this.#hooks.metamapLayout)(childKind, token);
    }

    // PlayerInfo 의 메타맵 전체 후속은 원문 일반 관문과 별도 경계로 유지한다.
    attachPlayer(token, x, y, parent, mode) { this.#attachChild(HudNsInfoChild.Player, token, x, y, parent, mode); }
    // BattleInfo 도 현재 메타 모드/현재 playerKnown 두 값을 분리해 조회한다.
    attachBattle(token, x, y, parent, mode) { this.#attachChild(HudNsInfoChild.Battle, token, x, y, parent, mode); }
}
