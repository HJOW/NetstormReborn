// 10.78 사제 건설 메뉴(00494870)·건설 행(00461cf0)·건설 선택 관문(00461bd0)·테마 조회(00493ac0)·프레임 상태(004ad710)를 현재 raw 월드와 controlled 메뉴에 연결하는 모듈이다(cpppj `client/RawPriestMenu.cpp/.h`, 10.78 전용).
// 사용:
//   const menu = new RawPriestMenu(pool, types, frames, players, excluded, contained, state, hooks);
//     pool = SidPool(10.78), types = 타입 레코드 188개(`flags2`·`theme`), frames = 타입별 프레임 코드 표 188개(`codes[프레임].side`), players = 9×180BYTE Player 기록(Uint8Array),
//     excluded = SquidPostPopList(0052f7e0 건설 금지 타입 목록, items()), contained = ContainedFinderState, state = createPriestMenuState(), hooks = 아래.
//   menu.open(priest) → 내 사제·493 부재에서만 하위 메뉴를 구성하고 어떤 정상 분기에서도 기본 메뉴(baseMenu)를 마지막에 호출한다.
//   menu.addBuild(type, disabled, priest, manaIcon) → 금액은 문구 색/아이콘만 바꾸고 행 활성화는 명시 금지와 제외 목록으로만 결정한다.
//   menu.selectBuild(type, priest) → 워크샾 테마 → 첫 float 금액 → 둘째 정수 금액 검사 뒤 미리보기 Form 을 커서로 넘기고 true.
//   menu.hasTheme(theme) / menu.frameState(priest) → 현재 Technology 의 flags4·동일 테마·flags2 0x800 아닌 내용물 유무 / 현재 물리 프레임 첫 signed 문자 − 'A'.
//   hooks = { hasEvent(sid, event 0x493|0x25d), immobile(sid), currentTechnology(), containedType(parent, mask, kind), allowed(type), cost(type) → 정수, money(owner) → float, description(type),
//             format(키, 인자[]) → 번역/printf 서식(인자는 문자열 또는 int32), beginFancy(), beginVertical(), separator(), title(문자열), takeMenu() → 토큰, submenu(문자열, 토큰, enabled),
//             item(문자열, 명령, 인자, enabled), baseMenu(sid), tell(키), createPreview(parent, type) → sid, takePreview(sid, 0x160) }.
//   bindPriestMenuControls(hooks, widgets, controls, menus) / bindPriestMenuWorld(hooks, pool, priest, processes, players, carrier, reward, create) — 이미 옮긴 메뉴/월드 처리기에 위임한다.
// 도달한 미연결 효과를 성공이나 빈 목록으로 대체하지 않으며(Error '미연결') 범위 밖 SID/타입/Player/프레임은 RangeError 이다. 모드·소유자·금액은 사용 시점마다 현재 값을 다시 읽는다.
// 원본 관측: `priestmenu-x86.tsv` 905행(Open 329·Select 64·Theme 256·Frame 256)을 tests/raw-priest-menu.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E540: 사제 건설 메뉴 RawPriestMenu 이식)
import { RawContainedFinder } from './raw-contained-finder.js';

// 원본 Player 기록 크기·개수와 메뉴/금액 포맷 버퍼 한도이다. (2026-11-15 WB04-E540)
const PLAYER_BYTES = 0xb4;
const PLAYERS = 9;
const FORMAT_BYTES = 256;
// 실제 Regular 사건(0x493 막힘·0x25d 건설 중)과 contained 기술/기도 플래그이다. (2026-11-15 WB04-E540)
const BLOCKED = 0x493;
const BUILDING = 0x25d;
const TECHNOLOGY = 4;
const PRAYER = 1;

// 원본의 mutable 타입·명령·모드 전역이며 기본값은 보호 PE 의 초기 DWORD 이다. 읽기 함수가 연결되면 사용 시점마다 현재 전역을 조회한다. (2026-11-15 WB04-E540)
export function createPriestMenuState(overrides = {}) {
    return {
        localOwner: 1, // 00540c70 의 현재 소유자 번호이다.
        temples: [123, 124, 125], // 00541244/48/4c 의 원본 신전 표시 순서이다.
        workshops: [85, 86, 87, 88], // 005411ac/b0/b4/b8 의 워크샾 표시 순서이다.
        outpost: 126, dais: 165, altar: 164, devastation: 168, // 00541250/ec/e8/f8 의 현재 타입이다.
        buildCommand: 0x02000060, prayCommand: 0x030000de, knowledgeCommand: 0x020000a0, // 00542878/2a6c/2974 의 입력 토큰이다.
        workshopMode: false, special: false, editor: false, chargeBlocked: false, // 00594fbc/0055a480/005c85a4/00594fa4 이다.
        localOwnerRead: null, workshopModeRead: null, editorRead: null, chargeBlockedRead: null, // 실제 현재 전역 조회기(없으면 위 값형 독립 입력)이다.
        ...overrides,
    };
}

// 도달한 미연결 효과를 성공이나 빈 목록으로 대체하지 않는다. (2026-11-15 WB04-E540)
function need(effect) {
    if (typeof effect !== 'function') throw new Error('사제 메뉴의 실제 효과/조회 미연결');
    return effect;
}

// 정렬되지 않은 원본 WORD/DWORD 필드를 낮은 바이트부터 읽는다. (2026-11-15 WB04-E540)
function read(raw, offset, width = 4) {
    if (offset + width > raw.length) throw new RangeError('사제 메뉴 raw 필드 범위 오류');
    let result = 0;
    // 원본 little endian 바이트의 순서를 유지한다.
    for (let index = 0; index < width; index += 1) result += raw[offset + index] * 2 ** (8 * index);
    return result >>> 0;
}

// 10.78 사제 건설 메뉴의 조건/순서와 두 번의 금액 조회를 원본 기계어대로 유지하는 처리기이다. 모든 참조와 훅의 실제 대상은 메뉴보다 오래 살아야 한다. (2026-11-15 WB04-E540)
export class RawPriestMenu {
    // PATCH 의 실제 저장소 크기를 확인하며 게임 효과는 생성 중 실행하지 않는다.
    constructor(pool, types, frames, players, excluded, contained, state, hooks) {
        if (pool.edition !== 'patch1078' || types.length !== 188 || frames.length !== types.length || players.length !== PLAYER_BYTES * PLAYERS) {
            throw new TypeError('사제 메뉴 PATCH 공유 저장소 크기 오류');
        }
        this.pool = pool;
        this.types = types;
        this.frames = frames;
        this.players = players;
        this.excluded = excluded;
        this.contained = contained;
        this.state = state;
        this.hooks = hooks;
    }

    // 실제 세션이 연결한 원본 전역 조회기를 각 사용 시점에 호출한다. 기존 독립 입력은 그대로 지원한다. (2026-11-15 WB04-E540)
    localOwner() { return (this.state.localOwnerRead ? this.state.localOwnerRead() : this.state.localOwner) >>> 0; }
    // 전투 모드와 다른 특수 모드를 합치지 않는다.
    workshopMode() { return Boolean(this.state.workshopModeRead ? this.state.workshopModeRead() : this.state.workshopMode); }
    // 원본 편집 조건을 현재 로더에서 읽는다.
    editor() { return Boolean(this.state.editorRead ? this.state.editorRead() : this.state.editor); }
    // 원본 미션 조건을 현재 저장소에서 읽는다.
    chargeBlocked() { return Boolean(this.state.chargeBlockedRead ? this.state.chargeBlockedRead() : this.state.chargeBlocked); }

    // 원본 메뉴는 상태 비트로 별도 free/dead 필터를 하지 않는다. 잘못된 물리 SID 만 거부한다. 원본의 살아 있는 span 처럼 접근 시점마다 현재 슬롯을 다시 읽는다.
    raw(sid) {
        if (!sid || sid >= this.pool.capacity) throw new RangeError('사제 메뉴 SID 범위 오류');
        return this.pool.slot(sid);
    }

    // 메뉴에서 실제 도달한 타입 속성만 확인한다.
    type(type) {
        if (type >= this.types.length) throw new RangeError('사제 메뉴 TYPE 범위 오류');
        return this.types[type];
    }

    // 콜백 뒤에도 현재 동일 Player 기록을 다시 읽는다.
    player(owner, offset) {
        if (owner >= PLAYERS || offset + 4 > PLAYER_BYTES) throw new RangeError('사제 메뉴 Player 범위 오류');
        return read(this.players, owner * PLAYER_BYTES + offset);
    }

    // 번역기는 원본 %s/%d 인자 순서를 보존한다. 반환 NUL 뒤 바이트는 원본 문자열이 아니므로 제거하고 256바이트 스택 버퍼 한도를 지킨다.
    format(key, args = []) {
        let result = need(this.hooks.format)(key, args);
        const nul = result.indexOf('\0');
        if (nul >= 0) result = result.slice(0, nul);
        if (result.length >= FORMAT_BYTES) throw new RangeError('사제 메뉴 원본 포맷 버퍼 한도 초과');
        return result;
    }

    // 실제 FrameCode 의 첫 바이트는 MSVC signed char 이다. 큰 frame 이나 음수 frame 의 원본 무효 메모리는 호스트 예외이다. (004ad710)
    frameState(priest) {
        const raw = this.raw(priest);
        const type = raw[10];
        this.type(type);
        const codes = this.frames[type].codes;
        const frame = read(raw, 36);
        if (frame >= codes.length) throw new RangeError('사제 메뉴 현재 물리 프레임 범위 오류');
        return (((codes[frame].side & 0xff) << 24) >> 24) - 65;
    }

    // 동일 내용을 여러 번 묻더라도 현재 Technology 와 체인을 매 호출 새로 조회한다. finder 는 다음 번호를 반환 전에 저장하는 기존 실제 몸체를 공유한다. (00493ac0)
    hasTheme(theme) {
        const technology = need(this.hooks.currentTechnology)();
        if (!technology || technology >= this.pool.capacity) throw new RangeError('사제 메뉴 현재 Technology SID 오류');
        const finder = new RawContainedFinder(this.pool, this.contained);
        // 기술 Form 체인을 따라 현재 Technology 의 내용물을 모두 본다.
        for (let sid = finder.begin(technology); sid; sid = finder.next()) {
            const raw = this.raw(sid);
            if (!(read(raw, 20, 2) & TECHNOLOGY)) continue;
            const record = this.type(read(raw, 18, 2));
            if ((record.theme | 0) === (theme | 0) && !(record.flags2 & 0x800)) return true;
        }
        return false;
    }

    // 금액 부족은 접두 ~r 와 mana 아이콘군만 바꾸며 enabled 에는 포함하지 않는다. (00461cf0)
    addBuild(type, disabled, priest, manaIcon) {
        const h = this.hooks;
        if (!need(h.allowed)(type)) return;
        const amount = need(h.cost)(type) | 0;
        const money = need(h.money)(this.localOwner());
        // x87 은 fild 의 정수를 단정도로 저장하지 않고 SP float 와 비교한다. NaN 은 지불 불가이다.
        const affordable = this.editor() || amount <= money;
        const price = amount ? this.format(' for %s%d~[IfortGump.3]', [affordable ? '' : '~r', amount]) : '';
        disabled = disabled || this.excluded.items().includes(type);
        let label;
        if (type === this.state.dais || type === this.state.altar) {
            const level = (this.player(this.localOwner(), 0x70) >>> 8) & 255;
            label = this.format('Build Level %d %s%s', [level + 1, need(h.description)(type), price]);
        } else if (manaIcon) {
            const theme = (((this.type(type).theme >>> 0) + (affordable ? 4 : 0)) | 0);
            label = this.format('~[Imana.%d] Build %s%s', [theme, need(h.description)(type), price]);
        } else label = this.format('Build %s%s', [need(h.description)(type), price]);
        need(h.item)(label, this.state.buildCommand, ((type << 16) | priest) >>> 0, !disabled);
    }

    // 두 번째 SP 조회 전에 비용도 다시 조회하는 원본 순서를 유지한다. (00461bd0)
    selectBuild(type, priest) {
        const h = this.hooks;
        const record = this.type(type);
        if (!this.editor() && (record.flags2 & 0x4000) && !this.hasTheme(record.theme)) {
            need(h.tell)('NoTechTheme');
            return false;
        }
        const first = Math.fround(need(h.cost)(type) | 0);
        const firstMoney = need(h.money)(this.localOwner());
        if (!this.editor() && !(first <= firstMoney)) {
            need(h.tell)('BuildNoBucks');
            return false;
        }
        const second = need(h.cost)(type) | 0;
        const secondMoney = need(h.money)(this.localOwner());
        // 둘째 fild 비교는 int 를 float 로 축소하지 않는다. 시점 사이 잔액이 줄면 안내 없이 반환한다.
        if (!this.editor() && !(second <= secondMoney)) return false;
        const preview = need(h.createPreview)(priest, type);
        need(h.takePreview)(preview, 0x160);
        return true;
    }

    // 사제의 비가상 기본 객체 메뉴 호출을 포함한 전체 원본 분기 순서이다. (00494870)
    open(priest) {
        const h = this.hooks;
        const state = this.state;
        if (this.raw(priest)[34] !== this.localOwner() || need(h.hasEvent)(priest, BLOCKED)) {
            need(h.baseMenu)(priest);
            return;
        }
        need(h.beginFancy)();
        need(h.title)(this.format('Construct Building\n'));
        const owner = this.raw(priest)[34];
        const templePresent = this.player(owner, 0x7c) !== 0 || this.player(owner, 0x84) !== 0;
        need(h.beginVertical)();
        // 세 신전 타입은 원본 전역 순서대로 현재 값을 읽는다.
        for (let index = 0; index < state.temples.length; index += 1) this.addBuild(state.temples[index], templePresent, priest, true);
        const templeLabel = this.format('%sTemple', [templePresent ? '~x' : '']);
        const templeMenu = need(h.takeMenu)();
        need(h.submenu)(templeLabel, templeMenu, !templePresent);
        // 원본의 || 단락 평가 순서와 workshop 모드의 뒤늦은 조회를 유지한다.
        let workshopAllowed = false;
        for (let index = 0; index < state.workshops.length; index += 1) {
            if (need(h.allowed)(state.workshops[index])) { workshopAllowed = true; break; }
        }
        if (workshopAllowed && (this.workshopMode() || state.special || this.editor())) {
            need(h.beginVertical)();
            // 원본 workshop 타입 순서에 대응하는 theme 인자는 3,0,1,2 이다.
            for (let index = 0; index < state.workshops.length; index += 1) this.addBuild(state.workshops[index], !this.editor() && !this.hasTheme((index + 3) % 4), priest, true);
            const label = this.format('Workshop');
            const menu = need(h.takeMenu)();
            need(h.submenu)(label, menu, true);
        }
        const outpost = state.outpost;
        const technology = need(h.currentTechnology)();
        if (technology >= this.pool.capacity) throw new RangeError('사제 메뉴 Technology 범위 오류');
        if (need(h.containedType)(technology, TECHNOLOGY, outpost) || !this.chargeBlocked() || this.editor()) this.addBuild(outpost, false, priest, false);
        const daisPresent = this.player(owner, 0x8c) !== 0 || this.player(owner, 0x94) !== 0;
        this.addBuild(state.dais, daisPresent, priest, false);
        const immobile = Boolean(need(h.immobile)(priest));
        let constructDisabled = immobile;
        if (!constructDisabled) constructDisabled = Boolean(need(h.hasEvent)(priest, BUILDING));
        if (!constructDisabled) {
            const frame = this.frameState(priest);
            constructDisabled = frame >= 10 && frame <= 14;
        }
        const constructLabel = this.format('%sConstruct~.', [immobile ? '~r' : '']);
        const constructMenu = need(h.takeMenu)();
        need(h.submenu)(constructLabel, constructMenu, !constructDisabled);
        if (!this.chargeBlocked() && !need(h.containedType)(priest, PRAYER, state.devastation) && !immobile && !need(h.hasEvent)(priest, BUILDING)) {
            const frame = this.frameState(priest);
            if (!(frame >= 10 && frame <= 14) && !need(h.hasEvent)(priest, BLOCKED)) {
                need(h.beginFancy)();
                need(h.title)(this.format('Prayer\n'));
                need(h.item)(this.format('Devastation'), state.prayCommand, ((state.devastation << 16) | priest) >>> 0, true);
                const prayerLabel = this.format('%sPray~.', ['']);
                const prayerMenu = need(h.takeMenu)();
                need(h.submenu)(prayerLabel, prayerMenu, true);
            }
        }
        need(h.item)(this.format('View Netstorm Knowledge'), state.knowledgeCommand, 0, true);
        need(h.separator)();
        need(h.baseMenu)(priest);
    }
}

// 기존 controlled 메뉴의 생성·작성·스택을 그대로 연결한다. 메뉴 저장소나 수명을 복제하지 않고 기존 API 에만 위임한다. (2026-11-15 WB04-E540)
export function bindPriestMenuControls(hooks, widgets, controls, menus) {
    hooks.beginFancy = () => { widgets.beginFancy(); };
    hooks.beginVertical = () => { widgets.beginVertical(0); };
    hooks.title = (text) => controls.addTitle(text);
    hooks.takeMenu = () => menus.takeCurrent();
    hooks.submenu = (text, child, enabled) => controls.addSubmenu(text, child, enabled, false);
    hooks.item = (text, command, parameter, enabled) => controls.addText(text, command, parameter, true, enabled, false);
    hooks.separator = () => controls.addSeparator();
}

// 같은 월드의 조회와 contained 생성 몸체만 묶으며 실제 커서 소유권/번역은 상위에 남긴다. 모든 처리기는 같은 SID 풀을 써야 한다. (2026-11-15 WB04-E540)
export function bindPriestMenuWorld(hooks, pool, priest, processes, players, carrier, reward, create) {
    if (pool !== priest.pool || pool !== processes.pool || pool !== players.pool || pool !== carrier.pool || pool !== reward.pool || pool !== create.pool) {
        throw new TypeError('사제 메뉴 월드 Pool 연결 오류');
    }
    hooks.immobile = (sid) => priest.immobile(sid);
    hooks.hasEvent = (sid, event) => processes.findEvent(sid, event) !== null;
    hooks.currentTechnology = () => players.currentTechnology();
    hooks.cost = (type) => reward.typeCostUnits(type);
    hooks.containedType = (parent, mask, type) => {
        const found = carrier.findContained(parent, mask, type, 0);
        return found ? read(pool.slot(found), 18, 2) : 0;
    };
    hooks.createPreview = (parent, type) => create.create(parent, type, 0, 0x20, 0x10, 0);
}
