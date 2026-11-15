// 원본 10.78 Factory(공장)의 생산/지식 하위 메뉴와 기본 Damageable 메뉴를 같은 controlled 트리에 작성하는 모듈이다(cpppj `client/RawFactoryMenu.cpp/.h`, 10.78 전용 — 원문 454c00 Open·4540d0 현재 생산·4752b0 지식·454040 슬롯·4735c0 마나 문자).
// 사용:
//   const menu = new RawFactoryMenu(pool, types, contained, excluded, state, hooks);   // pool = SidPool(10.78), types = 타입 레코드 188개(`theme`·`flags2`), contained = ContainedFinderState, excluded = SquidPostPopList(52f7e0),
//   //   state = createFactoryMenuState()(타입·명령·제외 전역), hooks = 아래.
//   menu.open(factory) → 내 일반 Factory(Outpost 제외)만 현재 생산·지식을 작성하고 어떤 분기에서도 기본 메뉴(baseMenu)를 마지막에 호출한다.
//   menu.currentProduction(factory) → 현재 생산 하위 메뉴를 작성하고 distinct 종류 기준 남은 슬롯 수를 반환한다.   menu.slots(count) → 454040 의 0 번역/음수 빈 값/양수 ? 아이콘 문자열.
//   formatFactoryMana(text, active, manaType = 103) → 4735c0 의 ASCII 마나 문자를 그림자/활성 프레임 토큰으로 바꾼다(점은 출력/활성 개수 제외).
//   hooks = { localOwner(), currentTechnology(), editor(), technologyType(parent, index) → 종류 WORD(0 이면 끝), capacity(sid), level(sid), allowed(type), inProduction(type), cost(type) → 정수, money(owner) → float,
//             description(type), mana(type), helpWord(), format(키, 인자[]), beginFancy(), title(문자열), header(문자열), takeMenu() → 토큰, submenu(문자열, 토큰), item(문자열, 명령, 인자(숫자 또는 정적 문자열), include, enabled), baseMenu(sid) }.
//   bindFactoryMenuControls(hooks, widgets, controls, menus, encodeString) — 기존 controlled 메뉴/정적 문자열 표에 위임한다.
// 지식 후보는 전체 지식 창과 같은 원본 70바이트 행·공유 CRT 정렬(ui/knowledge-sort.js)을 쓰며, 생산 중인 행만 숨기고 금지 목록은 include, 슬롯 부족은 enabled 에만 적용한다. 선택한 경계의 미연결은 거부한다.
// 원본 관측: `factory-menu-x86.tsv` 76행(Open 39·Mana 32·Slots 5)을 tests/raw-factory-menu.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E545: Factory 메뉴 RawFactoryMenu 이식)
import { RawContainedFinder } from './raw-contained-finder.js';
import { sortKnowledgeRecords } from '../ui/knowledge-sort.js';

// 실제 명령·제외 TYPE 전역이며 다른 종류의 생산 금지 목록과 합치지 않는다. (2026-11-15 WB04-E545)
export function createFactoryMenuState(overrides = {}) {
    return {
        outpost: 126, manaType: 103, // 541250 과 5411f4 의 원본 기본 타입이다.
        helpCommand: 0x02000017, researchCommand: 0x03000080, emptyCommand: 0x02000089, upgradeCommand: 0x03000032, // 실제 542754/5428f8/542918/5427c0 이다.
        excluded: [158, 133, 100, 101, 102, 84, 122, 121, 138, 144, 150, 73, 123, 124, 125, 85, 86, 87, 88, 126], // 원본 5400d0 의 제외 전역 순서이다.
        ...overrides,
    };
}

// 선택된 실제 경계가 미연결이면 빈 성공 대신 즉시 알린다. (2026-11-15 WB04-E545)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('Factory 메뉴 실제 조회/작성 미연결');
    return hook;
}

// 비정렬 원문 DWORD/WORD 를 낮은 바이트부터 읽는다(이웃 필드를 섞지 않고 명시 폭만). (2026-11-15 WB04-E545)
function read(raw, offset, width = 4) {
    let value = 0;
    // 명시 폭의 바이트만 조립한다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (index * 8);
    return value >>> 0;
}

// 현재 실제 마나 초기화 표는 11문자만 쓰고 그 밖의 ASCII 는 BSS 0 이다. 원본 C 문자열의 첫 NUL 에서 멈추며 연속 점은 개수에 넣지 않는다. (4735c0)
export function formatFactoryMana(text, active, manaType = 103) {
    // 실제 540ad8 문자열의 물리 그림 순서이다.
    const letters = 'wrtsWRTS.?>';
    let result = '';
    let count = 0;
    // 각 문자를 원본 순서로 그림 프레임 토큰으로 바꾼다.
    for (const letter of text) {
        if (letter === '\0') break;
        if (letter === '.') continue;
        if (letter.charCodeAt(0) >= 128) throw new RangeError('마나 원본 signed ASCII 표 범위');
        const found = letters.indexOf(letter);
        let frame = found < 0 ? 0 : found;
        if (count < active) frame += 11;
        result += `~[I${manaType}.${frame}]`;
        count += 1;
    }
    if (result.length >= 250) throw new RangeError('원본 마나 문자열 250바이트 진단');
    return result;
}

// 원문 454c00/4540d0/4752b0 의 원본 순서·지식 정렬·슬롯 계산만 수행하며 생산 명령 실행은 별도이다. (2026-11-15 WB04-E545)
export class RawFactoryMenu {
    // 생성 중 게임 상태/메뉴를 바꾸지 않고 단일 참조만 보관한다(10.78 풀과 188개 TYPE 필요).
    constructor(pool, types, contained, excluded, state, hooks) {
        if (pool.edition !== 'patch1078' || types.length !== 188) throw new TypeError('Factory 메뉴 10.78 실제 저장소 필요');
        this.pool = pool;
        this.types = types;
        this.contained = contained;
        this.excluded = excluded;
        this.state = state;
        this.hooks = hooks;
    }

    // 같은 +a8/+ac 를 가진 두 원본 Factory 표(0x509148/0x509210)만 받는다. 접근 시점마다 현재 슬롯을 읽는다.
    raw(sid) {
        if (!sid || sid >= this.pool.capacity) throw new RangeError('Factory 메뉴 SID 범위');
        const raw = this.pool.slot(sid);
        const table = read(raw, 0);
        if (table !== 0x509148 && table !== 0x509210) throw new Error('Factory 메뉴 파생 가상 표 미복원');
        return raw;
    }

    // 번역/조회 후에도 실제 타입 번호를 현재 표에서 읽는다.
    type(type) {
        if (type >= this.types.length) throw new RangeError('Factory 메뉴 TYPE 범위');
        return this.types[type];
    }

    // 실제 번역 서식의 첫 NUL/원본 스택 버퍼 한도(256바이트)를 유지한다.
    format(key, args = []) {
        let result = need(this.hooks.format)(key, args);
        const zero = result.indexOf('\0');
        if (zero >= 0) result = result.slice(0, zero);
        if (result.length >= 256) throw new RangeError('Factory 메뉴 원본 서식 256바이트 한도');
        return result;
    }

    // 0 은 번역된 None 이고 음수는 원본 repeat 가 빈 문자열을 만드는 별도 경로이다(원본은 양수/음수에서도 먼저 None 번역을 조회한다). (454040)
    slots(count) {
        const none = this.format('None');
        if (!count) return none;
        if (count < 0) return '';
        if (count >= 256) throw new RangeError('Factory 슬롯 원본 256바이트 버퍼');
        return formatFactoryMana('?'.repeat(count), -1, this.state.manaType);
    }

    // 내 일반 Factory 만 특수 메뉴를 붙이고 Damageable 부모는 항상 마지막에 실행한다. (454c00)
    open(factory) {
        if (this.raw(factory)[34] === (need(this.hooks.localOwner)() >>> 0) && this.raw(factory)[10] !== this.state.outpost) this.available(factory);
        need(this.hooks.baseMenu)(factory);
    }

    // 같은 종류가 중복돼도 항목은 각각 작성하며 남은 슬롯은 처음 한 번만 줄인다. (4540d0)
    currentProduction(factory) {
        const h = this.hooks;
        const state = this.state;
        need(h.beginFancy)();
        need(h.title)(this.format('Current Production\n'));
        let remaining = need(h.capacity)(factory) | 0;
        const seen = new Array(255).fill(false);
        let written = 0;
        const finder = new RawContainedFinder(this.pool, this.contained);
        // 실제 Form 순서와 flags16 을 유지한다. 금지된 종류도 슬롯은 차지한다.
        for (let sid = finder.begin(factory); sid; sid = finder.next()) {
            const raw = this.pool.slot(sid);
            if (!(raw[20] & 0x10)) continue;
            const type = read(raw, 18, 2);
            this.type(type);
            if (type >= seen.length) throw new RangeError('Factory 생산 중복 표 범위');
            if (need(h.allowed)(type)) {
                need(h.item)(this.format('%s~i%s%s~.', ['', '', need(h.description)(type)]), state.helpCommand, type, true, true);
                written += 1;
            }
            if (!seen[type]) { remaining -= 1; seen[type] = true; }
        }
        if (!written) need(h.header)(this.format('No Production Yet.\n'));
        need(h.item)(need(h.helpWord)(), state.helpCommand, 'researchHelp', true, true);
        const label = this.format('View Current Production');
        const child = need(h.takeMenu)();
        need(h.submenu)(label, child);
        return remaining;
    }

    // 기술 후보를 원래 순서로 만든 뒤 실제 CRT 비교/교환으로 정렬한다. (4752b0)
    available(factory) {
        const h = this.hooks;
        const state = this.state;
        const remaining = this.currentProduction(factory);
        need(h.beginFancy)();
        need(h.title)(this.format('Knowledge Available\n'));
        need(h.header)(this.format('Production Slots Available: %s\n', [this.slots(remaining)]));
        const technology = need(h.currentTechnology)();
        if (technology >= this.pool.capacity) throw new RangeError('Factory 현재 기술 부모 범위');
        const records = [];
        // 원본은 첫 0 종류에서 끝나며 중복 기술도 별도 행으로 보존한다.
        for (let index = 0; ; index += 1) {
            const type = need(h.technologyType)(technology, index);
            if (!type) break;
            const record = this.type(type);
            if (state.excluded.includes(type)) continue;
            const factoryTheme = this.type(this.raw(factory)[10]).theme | 0;
            if (factoryTheme !== (record.theme | 0) && (factoryTheme !== 3 || !(record.flags2 & 0x800))) continue;
            const row = { type, available: remaining >= 1, mana: need(h.mana)(type), description: need(h.description)(type), inProduction: Boolean(need(h.inProduction)(type)) };
            // 원본 두 strcpy 의 첫 NUL 경계이다.
            const manaEnd = row.mana.indexOf('\0');
            const descriptionEnd = row.description.indexOf('\0');
            if (manaEnd >= 0) row.mana = row.mana.slice(0, manaEnd);
            if (descriptionEnd >= 0) row.description = row.description.slice(0, descriptionEnd);
            if (row.mana.length > 9 || row.description.length > 81) throw new RangeError('Factory 원본 70바이트 지식 행 범위');
            records.push(row);
            if (records.length > 200) throw new RangeError('Factory 원본 지식 후보 200행 범위');
        }
        sortKnowledgeRecords(records, this.types);
        let any = false;
        // 생산 중인 행만 숨긴다. 금지 목록은 include, 슬롯 부족은 enabled 에만 적용한다.
        for (const row of records) {
            if (row.inProduction) continue;
            any = true;
            const label = this.format('%s%s', [row.description, '']);
            const blocked = this.excluded.items();
            const include = !blocked.includes(row.type);
            need(h.item)(label, state.researchCommand, ((row.type << 16) | factory) >>> 0, include, row.available);
        }
        if (!any) need(h.item)(this.format('All Knowledge is in Production'), state.emptyCommand, 0, true, false);
        const label = this.format('Put Knowledge into Production');
        const child = need(h.takeMenu)();
        need(h.submenu)(label, child);
        if ((need(h.level)(factory) | 0) < 3) {
            const cost = need(h.cost)(this.raw(factory)[10]) | 0;
            const money = need(h.money)(need(h.localOwner)());
            const affordable = Boolean(need(h.editor)()) || cost <= money;
            need(h.item)(this.format('Upgrade costs %s%d~[IfortGump.3]', [affordable ? '' : '~r', cost]), state.upgradeCommand, factory, true, true);
        }
    }
}

// 기존 controlled 스택과 수명이 같은 정적 인자 토큰만 사용한다. encodeString 은 정적 문자열 인자를 같은 상수 표의 DWORD 로 바꾼다. (2026-11-15 WB04-E545)
export function bindFactoryMenuControls(hooks, widgets, controls, menus, encodeString) {
    hooks.beginFancy = () => { widgets.beginFancy(); };
    hooks.title = (text) => controls.addTitle(text);
    hooks.header = (text) => controls.addHeader(text);
    hooks.takeMenu = () => menus.takeCurrent();
    hooks.submenu = (text, child) => controls.addSubmenu(text, child, true, false);
    hooks.item = (text, command, parameter, include, enabled) => {
        const token = typeof parameter === 'number' ? parameter : need(encodeString)(parameter);
        controls.addText(text, command, token, include, enabled, false);
    };
}
