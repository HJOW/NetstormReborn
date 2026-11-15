// 공통 객체·Damageable·회수 메뉴의 원본 조건과 명령 인자를 보존하는 모듈이다(cpppj `client/RawObjectMenu.cpp/.h`, 10.78 전용 — 원문 004b1270 객체 메뉴·0044cbd0 Damageable 메뉴·0044ca00 회수 항목).
// 사용:
//   const menu = new RawObjectMenu(pool, types, state, hooks);   // pool = SidPool(10.78), types = 타입 레코드 188개(`flags2`·`cost`), state = createObjectMenuState(), hooks = 아래.
//   menu.open(sid) → 편집/치트 항목·도움말·SetOwner·소유자 Player 하위 메뉴(004b1270).  menu.openDamageable(sid) → 내 객체의 가상 개수 조회→회수 항목 뒤 공통 객체 메뉴(0044cbd0).
//   menu.addSalvage(sid, type, count) → 현재 슬롯의 조건과 명시 타입/개수를 각각 보존해 회수 항목을 만든다(0044ca00).
//   hooks = { hasHitPoints(sid), count(sid) → 가상+80 개수, salvageRefund(sid, type, count) → signed 환불, format(키, 인자[]) → 번역/printf, itemCount(), separator(),
//             item(문자열, 명령, 인자(숫자 또는 정적 문자열)), submenu(문자열, 토큰), setConfig(키, 값), ownerSubscriber(owner) → subscriber DWORD, playerMenu(subscriber, true, false) → 토큰 }.
//   bindObjectMenuControls(hooks, controls, menus, config, encodeString) / bindObjectMenuIdentity(hooks, identity) — 기존 controlled 메뉴·설정·PlayerFinder 에 연결한다.
// 문자열 인자(SetOwner/DenySalvage/SalvageVortex/SalvageInf)는 원문 정적 수명이라 80칸 스크립트 고리의 임시 토큰으로 대체하지 않고 encodeString 이 같은 상수 표의 DWORD 로 공급한다.
// 모드·소유자·회수 금지는 사용 시점마다 현재 값을 다시 읽는다(환불/메뉴 콜백이 바꿀 수 있다). 도달한 미연결 효과는 성공으로 대체하지 않는다(Error '미연결').
// 원본 관측: `object-menu-x86.tsv` 의 10.78(originals) 행 — base(Open)·damage(OpenDamageable)·salvage(AddSalvage) — 을 tests/raw-object-menu.test.mjs 가 대조한다(CD 10.72 판본 행은 웹 범위 밖).
// 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E541: 객체 메뉴 RawObjectMenu 이식)

// 두 보호 PE 의 동일 의미 전역이며 메뉴 작성 중에도 현재 값을 다시 읽는다. 읽기 함수가 있으면 값형 독립 입력보다 우선한다. helpWord 의 null 은 0054db38 의 NULL 포인터이다. (2026-11-15 WB04-E541)
export function createObjectMenuState(overrides = {}) {
    return {
        localOwner: 1, // 00540c70 의 로컬 번호이다.
        editor: false, special: false, chargeBlocked: false, // 005c85a4/0055a480/00594fa4 의 모드이다.
        denySalvage: 0, // 00595078 의 현재 금지 DWORD 이다.
        growingRainBlocker: 150, nugget: 154, suppressedSalvage: 122, // 5412b0/5412c0/541240 이다.
        hurtCommand: 0x030000e2, destroyCommand: 0x03000015, makeYoursCommand: 0x030000af, // 원본 직접 객체 명령이다.
        helpCommand: 0x02000017, tellCommand: 0x02000095, salvageCommand: 0x0200005d, // 원본 도움말/문자열 안내/회수 명령이다.
        helpWord: null, // 0054db38 의 NULL 포인터와 실제 문자열을 구별한다.
        helpWordRead: null, // 실제 Client 의 0054db38 현재 캐시를 항목 작성 위치에서 읽는다(없으면 값형 helpWord).
        localOwnerRead: null, denySalvageRead: null, // 실제 세션의 현재 소유자/회수 금지 전역이다.
        editorRead: null, specialRead: null, chargeBlockedRead: null, // 현재 DWORD/모드를 값형 fixture 와 구별하여 직접 조회한다.
        ...overrides,
    };
}

// 미연결 실제 효과를 빈 메뉴나 성공으로 바꾸지 않는다. (2026-11-15 WB04-E541)
function need(effect) {
    if (typeof effect !== 'function') throw new Error('실제 객체 메뉴 하위 미연결');
    return effect;
}

// 같은 Pool/TYPE 를 조회하는 세 원본 몸체이다. Carrier/Priest 는 마지막 기본 메뉴 호출에 이 객체를 연결한다. 참조와 훅은 처리기보다 오래 살아야 한다. (2026-11-15 WB04-E541)
export class RawObjectMenu {
    // 풀의 판본(10.78)과 실제 TYPE 표의 크기(188)를 확인한다.
    constructor(pool, types, state, hooks) {
        if (pool.edition !== 'patch1078' || types.length !== 188) throw new TypeError('객체 메뉴 실제 TYPE 표 크기 오류');
        this.pool = pool;
        this.types = types;
        this.state = state;
        this.hooks = hooks;
    }

    // getter 가 있으면 같은 현재 Player 번호를 원본 읽기 시점에 조회한다.
    localOwner() { return (this.state.localOwnerRead ? this.state.localOwnerRead() : this.state.localOwner) >>> 0; }
    // 원본 편집 전역의 각 소비 시점을 합치지 않는다.
    editor() { return Boolean(this.state.editorRead ? this.state.editorRead() : this.state.editor); }
    // 호출 중 변경된 특수 모드도 다음 분기에 반영한다.
    special() { return Boolean(this.state.specialRead ? this.state.specialRead() : this.state.special); }
    // 현재 미션 전역을 별도 스냅숏으로 고정하지 않는다.
    chargeBlocked() { return Boolean(this.state.chargeBlockedRead ? this.state.chargeBlockedRead() : this.state.chargeBlocked); }
    // 환불/메뉴 작성 뒤 같은 현재 회수 금지 DWORD 를 재조회한다.
    denySalvage() { return (this.state.denySalvageRead ? this.state.denySalvageRead() : this.state.denySalvage) >>> 0; }

    // 원본 물리 SID 를 조회한다. free/dead 를 임의로 생략하지 않는다(살아 있는 span 처럼 접근 시점마다 현재 슬롯을 읽는다).
    raw(sid) {
        if (!sid || sid >= this.pool.capacity) throw new RangeError('객체 메뉴 SID 범위 오류');
        return this.pool.slot(sid);
    }

    // 호출 시점의 TYPE 인자를 읽는다.
    type(type) {
        if (type >= this.types.length) throw new RangeError('객체 메뉴 TYPE 범위 오류');
        return this.types[type];
    }

    // +22 의 BYTE(소유자)를 현재 슬롯에서 읽는다.
    owner(sid) { return this.raw(sid)[34]; }

    // 번역된 고정 서식만 허용하고 NUL 뒤를 버리며 원본 256바이트 스택 버퍼 초과는 예외로 거부한다.
    format(key, args = []) {
        let text = need(this.hooks.format)(key, args);
        const nul = text.indexOf('\0');
        if (nul >= 0) text = text.slice(0, nul);
        if (text.length >= 256) throw new RangeError('객체 메뉴 원본 서식 버퍼 초과');
        return text;
    }

    // 004b1270: 편집/치트·도움말·SetOwner·소유자 Player 하위 메뉴이다.
    open(sid) {
        const h = this.hooks;
        const state = this.state;
        this.raw(sid);
        if (this.editor()) need(h.item)('Destroy', state.destroyCommand, sid);
        else if (this.special() && this.chargeBlocked()) {
            if (need(h.hasHitPoints)(sid)) need(h.item)('CHEAT Hurt Me!', state.hurtCommand, sid);
            need(h.item)('CHEAT Destroy', state.destroyCommand, sid);
            need(h.item)('CHEAT Make Yours', state.makeYoursCommand, sid);
        }
        // 앞선 항목 작성 효과 뒤의 현재 0054db38 캐시를 이 위치에서 읽는다.
        const help = state.helpWordRead ? state.helpWordRead() : state.helpWord;
        if (help === null || help === undefined) throw new Error('원본 객체 메뉴 helpWord NULL');
        need(h.item)(help, state.helpCommand, this.raw(sid)[10]);
        if (this.editor() || this.special()) {
            const label = this.format('CHEAT Set Owner');
            need(h.item)(label, state.tellCommand, 'SetOwner');
        }
        if (this.owner(sid)) {
            const subscriber = need(h.ownerSubscriber)(this.owner(sid));
            // 10.78 은 Player 하위 메뉴 생성 전에 번역한다.
            const label = this.format('Player');
            const menu = need(h.playerMenu)(subscriber, true, false);
            need(h.submenu)(label, menu);
        }
    }

    // 0044cbd0: 내 객체의 가상 개수→회수 뒤 공통 객체 메뉴이다. 가상 조회 콜백이 슬롯의 타입을 바꿀 수 있으므로 10.78 의 타입 snapshot 시점(개수 조회 전)을 지킨다.
    openDamageable(sid) {
        if (this.owner(sid) === this.localOwner()) {
            const type = this.raw(sid)[10];
            const count = need(this.hooks.count)(sid);
            this.addSalvage(sid, type, count);
        }
        this.open(sid);
    }

    // 0044ca00: raw 타입 조건과 명시 타입의 flags2 를 구분한다. 음수 환불은 DWORD 부호 반전 후 원본 %d 로 출력한다.
    addSalvage(sid, type, count) {
        const h = this.hooks;
        const state = this.state;
        if (this.owner(sid) !== this.localOwner()) return;
        const currentType = this.raw(sid)[10];
        const current = this.type(currentType);
        if (!(current.flags2 & 0x200) && currentType !== state.growingRainBlocker && current.cost === 0) return;
        if (type === state.nugget || this.editor()) return;
        const refund = need(h.salvageRefund)(sid, type, count) | 0;
        const amount = refund < 0 ? ((0 - (refund >>> 0)) | 0) : refund;
        const label = this.format(refund < 0 ? 'Salvage costs %d~[IfortGump.3]' : 'Salvage gains %d~[IfortGump.3]', [amount]);
        const flags = this.type(type).flags2;
        if (need(h.itemCount)()) need(h.separator)();
        if (this.denySalvage()) { need(h.item)(label, state.tellCommand, 'DenySalvage'); return; }
        need(h.setConfig)('global.salvageSid', String(sid));
        if (flags & 0x200) { need(h.item)(label, state.tellCommand, 'SalvageVortex'); return; }
        if (flags & 0x4000) { need(h.item)(label, state.tellCommand, 'SalvageInf'); return; }
        // 10.78 만 현재 슬롯의 타입이 회수 생략 타입이면 항목을 만들지 않는다.
        if (this.raw(sid)[10] === state.suppressedSalvage) return;
        need(h.item)(label, state.salvageCommand, sid);
    }
}

// 정적 문자열 인자는 실제 명령 소비자가 해석하는 같은 상수 표의 DWORD 로 공급한다. menus.itemCount()/controls.addSeparator()/addText()/addSubmenu()/config.set() 에 위임한다. (2026-11-15 WB04-E541)
export function bindObjectMenuControls(hooks, controls, menus, config, encodeString) {
    hooks.itemCount = () => menus.itemCount();
    hooks.separator = () => controls.addSeparator();
    hooks.item = (label, command, parameter) => {
        const word = typeof parameter === 'number' ? parameter : need(encodeString)(parameter);
        controls.addText(label, command, word, true, true, false);
    };
    hooks.submenu = (label, child) => controls.addSubmenu(label, child, true, false);
    hooks.setConfig = (key, value) => config.set(key, value);
}

// 네이티브 정수 식별자를 문자열/호스트 주소로 재해석하지 않는다. 같은 현재 PlayerFinder 를 통해 실제 owner→subscriber DWORD 를 연결한다. (2026-11-15 WB04-E541)
export function bindObjectMenuIdentity(hooks, identity) {
    hooks.ownerSubscriber = (owner) => identity.subscriberForOwner(owner);
}
