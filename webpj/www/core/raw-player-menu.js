// 10.78 Player 메뉴(004c9700)를 실제 Identity/Player/동맹/사제 상태와 controlled 메뉴에 연결하는 모듈이다(cpppj `client/RawPlayerMenu.cpp/.h`, 10.78 전용).
// 사용:
//   const menu = new RawPlayerMenu(identity, alliances, hidden, priestStates, state, hooks);
//     identity = RawPlayerIdentity(같은 Player 9×180·metadata 40×460·현재 battle/editor/metamap/subscriber/localOwner 조회기), alliances = 방향별 81 DWORD, hidden = 9 BYTE, priestStates = 9 DWORD(ownerState),
//     state = createPlayerMenuState()(명령 전역과 두 정적 asksend 버퍼), hooks = 아래.
//   menu.open(subscriber) → 메뉴를 작성해 현재 스택에서 꺼낸 토큰(owner 0 이면 0).  menu.textParameter(token) → 정적 토큰이 가리키는 현재 asksend 문자열(버퍼 갱신 뒤에도 같은 토큰 의미).
//   formatPlayerMenuText(pattern, values) → 고정 %s/%d/%% 서식(256BYTE 한도, 번역은 호출자가 먼저 실행). bindPlayerMenuControls(hooks, widgets, controls, menus) / bindObjectPlayerMenu(hooks, players) — 기존 메뉴 처리기/객체 메뉴 연결.
//   hooks = { mission(), fort(), allowSend(), allowOffer(), allowBreak() — 00594fa4/00594fa0/0052f7ed·eb·ec 현재 상태, assertion(PlayerMenuAssertion), parseAddress(문자열, 0) — 004e0cd0, reliability(index) — 00478310,
//             ignored(subscriber), whispered(subscriber) → 하위 BYTE, money(owner) → float(0040ec50), translate(키), print(패턴, 인자[]) — 번역 없는 고정 서식,
//             beginFancy(), beginVertical(), separator(), title(문자열), header(문자열), takeMenu() → 토큰, submenu(문자열, 토큰), item(문자열, 명령, 인자, include, enabled), checked(check, 문자열, 명령, 인자) }.
//   도달한 미연결 효과를 빈 목록/성공으로 바꾸지 않는다(Error '미연결'). 모드·소유자·SP·동맹·이름은 사용 시점마다 현재 값을 다시 읽고 번역은 뒤 인자 조회보다 먼저 실행한다.
// 원본 관측: `player-menu-x86.tsv` 292입력(Player 부모 전체의 반환·정적 208바이트·모든 조회/GUI 효과)을 tests/raw-player-menu.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E544: Player 메뉴 RawPlayerMenu 이식)
import { RawPlayerIdentity } from './raw-player-identity.js';

// 원본이 진단 후 계속하는 사제 번호/메타맵 번호 조회이다. (2026-11-15 WB04-E544)
export const PlayerMenuAssertion = Object.freeze({ PriestOwner: 0, MetamapNumber: 1 });

// 두 정적 asksend 버퍼(104BYTE)와 현재 명령 전역이며 버퍼 주소의 의미는 호스트 포인터가 아닌 opaque DWORD 로 유지한다. (2026-11-15 WB04-E544)
export function createPlayerMenuState(overrides = {}) {
    return {
        sendMission: new Uint8Array(104), sendLobby: new Uint8Array(104), // 005cad98/005cae00 의 현재 NUL 문자열이다.
        missionTextToken: 0x005cad98, lobbyTextToken: 0x005cae00, // 호스트 주소로 역참조하지 않는 원본 정적 식별자이다.
        viewInfo: 0x0200009b, findPriest: 0x020000b8, tell: 0x02000095, // 실제 Player 정보/사제 찾기/안내 명령이다.
        ignore: [0x020000ee, 0x020000ef], whisper: [0x02000027, 0x02000101], // 상태 0/비영의 원본 토글 명령이다.
        send: [0x02000120, 0x02000108, 0x020000ec, 0x02000100, 0x0200012b], // 300/700/1000/X/All 순서이다.
        requestAlliance: 0x020000fb, breakAlliance: 0x020000fa, // 실제 동맹 요청/해제 명령이다.
        ...overrides,
    };
}

// 도달한 미연결 효과를 빈 목록/성공으로 바꾸지 않는다. (2026-11-15 WB04-E544)
function need(effect) {
    if (typeof effect !== 'function') throw new Error('실제 Player 메뉴 하위 미연결');
    return effect;
}

// 원본 메뉴의 실제 고정 %s/%d/%% 만 처리한다. 번역은 호출자의 원본 조회 위치에서 수행한다. values 는 문자열 또는 int32 이다. (2026-11-15 WB04-E544)
export function formatPlayerMenuText(pattern, values) {
    let result = '';
    let argument = 0;
    // 각 원본 형식과 인자를 순서대로 소비하고 정의되지 않은 서식은 거부한다.
    for (let index = 0; index < pattern.length; index += 1) {
        if (pattern[index] !== '%') { result += pattern[index]; continue; }
        index += 1;
        if (index >= pattern.length) throw new Error('실제 객체 메뉴 서식 끝 오류');
        if (pattern[index] === '%') { result += '%'; continue; }
        if (argument >= values.length) throw new Error('실제 객체 메뉴 서식 인자 부족');
        if (pattern[index] === 's') {
            if (typeof values[argument] !== 'string') throw new TypeError('실제 객체 메뉴 %s 인자 형식 오류');
            result += values[argument];
            argument += 1;
        } else if (pattern[index] === 'd') {
            if (typeof values[argument] !== 'number') throw new TypeError('실제 객체 메뉴 %d 인자 형식 오류');
            result += String(values[argument] | 0);
            argument += 1;
        } else throw new Error('실제 객체 메뉴 미복원 서식');
    }
    if (argument !== values.length || result.length >= 256) throw new RangeError('실제 객체 메뉴 원본 256BYTE 서식 계약 오류');
    return result;
}

// 정렬되지 않은 원본 WORD/DWORD 를 낮은 바이트부터 읽는다(원본 little endian 폭 유지). (2026-11-15 WB04-E544)
function read(bytes, offset, width = 4) {
    if (offset + width > bytes.length) throw new RangeError('Player 메뉴 필드 범위 오류');
    let value = 0;
    // 정렬되지 않은 원문 바이트를 낮은 순서로 조립한다.
    for (let index = 0; index < width; index += 1) value += bytes[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}

// 원본 정상 NUL 문자열만 제공한다(NUL 이 없으면 거부). (2026-11-15 WB04-E544)
function text(bytes, offset, capacity) {
    if (offset + capacity > bytes.length) throw new RangeError('Player 메뉴 문자열 범위 오류');
    let end = offset;
    while (end < offset + capacity && bytes[end] !== 0) end += 1;
    if (end === offset + capacity) throw new RangeError('Player 메뉴 NUL 없는 문자열');
    let result = '';
    // 바이트 그대로의 이진 문자열을 만든다.
    for (let index = offset; index < end; index += 1) result += String.fromCharCode(bytes[index]);
    return result;
}

// 원본 strcpy 처럼 NUL 까지 쓰며 정적 버퍼 뒤 바이트를 지우지 않는다. (2026-11-15 WB04-E544)
function store(target, value) {
    if (value.length >= target.length) throw new RangeError('Player 메뉴 정적 버퍼 초과');
    // 바이트 하나씩 복사한다.
    for (let index = 0; index < value.length; index += 1) target[index] = value.charCodeAt(index) & 0xff;
    target[value.length] = 0;
}

// 10.78 Player 부모 전체의 메뉴 구성 처리기이다. 큰 의존 몸체는 명시 경계이며 실제 메뉴 명령 실행과 구분한다. (2026-11-15 WB04-E544)
export class RawPlayerMenu {
    // Identity 와 같은 기록/현재 전역을 빌리고 9×9 방향 동맹/9 숨김/9 사제 상태도 복사하지 않는다.
    constructor(identity, alliances, hidden, priestStates, state, hooks) {
        if (alliances.length !== 81 || hidden.length !== 9 || priestStates.length !== 9) throw new TypeError('Player 메뉴 PATCH 공유 상태 오류');
        this.identity = identity;
        this.alliances = alliances;
        this.hidden = hidden;
        this.priestStates = priestStates;
        this.state = state;
        this.hooks = hooks;
    }

    // 현재 battle/metamap/editor/subscriber/localOwner 는 Identity 와 같은 전역 getter 로 읽고 mission 은 별도 00594fa4 이다. (2026-11-15 WB04-E544)
    battle() { return Boolean(need(this.identity.currentInputs.battle)()); }
    mission() { return Boolean(need(this.hooks.mission)()); }
    meta() { return Boolean(need(this.identity.currentInputs.metamap)()); }
    editor() { return Boolean(need(this.identity.currentInputs.editor)()); }
    mySubscriber() { return need(this.identity.currentInputs.subscriber)() >>> 0; }
    localOwner() { return need(this.identity.currentInputs.localOwner)() >>> 0; }
    // 실제 getter 를 반복 호출하는 원본 위치를 합치지 않는다.
    owner(subscriber) { return this.identity.ownerForSubscriber(subscriber); }

    // 현재 같은 9 Player 의 원문 180BYTE 를 읽는다(살아 있는 보기). (2026-11-15 WB04-E544)
    player(owner) {
        if (owner >= 9) throw new RangeError('Player 메뉴 owner 범위 오류');
        return this.identity.playerRecords.subarray(owner * 180, owner * 180 + 180);
    }

    // 현재 같은 40 metadata 기록(460BYTE)을 읽는다. (2026-11-15 WB04-E544)
    metadata(index) {
        const meta = this.identity.metaRecords;
        if (meta.length !== 40 * 460 || index >= 40) throw new Error('Player 메뉴 metadata 미연결/범위 오류');
        return meta.subarray(index * 460, index * 460 + 460);
    }

    // 숨김 BYTE 는 9개뿐이며 범위 밖 조회는 원본 무효 메모리라 호스트 예외로 거부한다. (2026-11-15 WB04-E544)
    hiddenAt(index) {
        if (index >= 9) throw new RangeError('Player 메뉴 숨김 범위 오류');
        return this.hidden[index];
    }

    // 0048f550 은 metamap 진단 뒤 현재 포인터의 번호 하위 BYTE 를 반환한다. (2026-11-15 WB04-E544)
    number(owner) {
        this.player(owner);
        if (this.meta()) need(this.hooks.assertion)(PlayerMenuAssertion.MetamapNumber);
        return owner & 255;
    }

    // 004922e0 의 Player flags 0x22·ownerState 2 와 무효 번호 진단이다. (2026-11-15 WB04-E544)
    dead(owner) {
        if (owner < 1 || owner > 8) { need(this.hooks.assertion)(PlayerMenuAssertion.PriestOwner); return true; }
        return !(this.player(owner)[0x50] & 0x22) || this.priestStates[owner] === 2;
    }

    // 00492340 의 유효/참여 조건과 ownerState 1 을 그대로 유지한다. (2026-11-15 WB04-E544)
    captured(owner) {
        if (owner < 1 || owner > 8) { need(this.hooks.assertion)(PlayerMenuAssertion.PriestOwner); return true; }
        return Boolean((this.player(owner)[0x50] & 0x22) && this.priestStates[owner] === 1);
    }

    // 00425e90 은 편집 검사보다 SP 를 먼저 읽으며 NaN 을 지불 가능으로 취급하지 않는다. (2026-11-15 WB04-E544)
    canAfford(amount) {
        const sp = need(this.hooks.money)(this.localOwner());
        return this.editor() || amount <= sp;
    }

    // 번역된 고정 서식과 원본 256바이트/NUL 한도이다. (2026-11-15 WB04-E544)
    format(key, args = []) { return this.print(need(this.hooks.translate)(key), args); }

    // 원본에 번역 호출이 없는 asksend 및 조회가 사이에 있는 서식도 같은 출력 한도를 사용한다. (2026-11-15 WB04-E544)
    print(pattern, args = []) {
        let result = need(this.hooks.print)(pattern, args);
        const nul = result.indexOf('\0');
        if (nul >= 0) result = result.slice(0, nul);
        if (result.length >= 256) throw new RangeError('Player 메뉴 256바이트 서식 초과');
        return result;
    }

    // 현재 정적 토큰을 최신 문자열로 해석한다. 임시 문자열 고리로 수명을 바꾸지 않는다. (2026-11-15 WB04-E544)
    textParameter(token) {
        const state = this.state;
        const bytes = token === state.missionTextToken ? state.sendMission : token === state.lobbyTextToken ? state.sendLobby : null;
        if (bytes === null) throw new RangeError('Player 메뉴 문자열 토큰 오류');
        return text(bytes, 0, bytes.length);
    }

    // 원본 세 송금 경로의 include/enabled 와 반복 현재 owner/SP 조회를 유지한다. (2026-11-15 WB04-E544)
    sendMoney(subscriber, owner) {
        const h = this.hooks;
        const state = this.state;
        need(h.beginVertical)();
        const mission = this.mission();
        const asksend = this.print('asksend(%d)', [this.owner(subscriber) | 0]);
        store(mission ? state.sendMission : state.sendLobby, asksend);
        let hidden = false;
        if (!mission) {
            this.owner(this.mySubscriber());
            hidden = this.hiddenAt(owner) !== 0;
        }
        // 실제 지불 가능 색 조회 순서이다.
        const amounts = [300, 700, 1000, 1, 1];
        // 원본 고정 다섯 서식이다.
        const labels = ['Send %s300~[IfortGump.3]', 'Send %s700~[IfortGump.3]', 'Send %s1000~[IfortGump.3]', 'Send %s X~[IfortGump.3]', 'Send %s All~[IfortGump.3]'];
        // 금액은 색만 바꾸며 행 활성화는 현재 연결/허용 플래그의 별도 계산이다.
        for (let index = 0; index < amounts.length; index += 1) {
            const enabled = Boolean((!hidden || index === 4) && (this.player(owner)[0x50] & 0x21) && need(h.allowSend)());
            const affordable = this.canAfford(amounts[index]);
            const label = this.format(labels[index], [affordable ? '' : '~r']);
            if (mission && index === 4) need(h.item)(label, state.tell, state.missionTextToken, 1, enabled ? 1 : 0);
            else {
                const current = this.owner(subscriber);
                need(h.item)(label, state.send[index], current, hidden ? 0 : 1, enabled ? 1 : 0);
            }
        }
        const label = this.format('Send Money');
        const child = need(h.takeMenu)();
        need(h.submenu)(label, child);
        if (!need(h.fort)()) need(h.separator)();
    }

    // 전체 원본 분기에서 현재 Player/모드와 앞서 저장한 번호를 구분한다. owner 0 이면 0 이다. (004c9700)
    open(subscriber) {
        const h = this.hooks;
        const state = this.state;
        let owner = this.owner(subscriber);
        if (!owner) return 0;
        const name = this.mission() ? text(this.player(owner), 0xc, 32) : this.identity.nameForSubscriber(subscriber);
        need(h.beginFancy)();
        const title = this.format('Player %s\n', [name]);
        need(h.title)(title);
        if (this.battle() && this.player(owner)[0x30]) need(h.parseAddress)(text(this.player(owner), 0x30, 32), 0);
        if (this.meta()) need(h.parseAddress)(text(this.metadata(owner), 0x1a0, 44), 0);
        if (!this.editor()) {
            if (this.battle()) {
                owner = this.owner(subscriber);
                if (!this.mission()) need(h.header)(this.format((this.player(owner)[0x50] & 1) ? 'Status: ~iOnline~.\n' : 'Status: ~r~EDISCONNECTED~.\n'));
                const condition = this.format(this.dead(owner) ? 'Dead' : this.captured(owner) ? 'Captured' : 'Alive');
                need(h.header)(this.format('Priest: ~i%s~.\n', [condition]));
                let allied = false;
                // 현재 대상→후보와 후보→대상의 두 방향을 별도로 읽는다.
                for (let candidate = 1; candidate < 9; candidate += 1) {
                    if (read(this.player(candidate), 0x2c) && candidate !== owner && this.alliances[owner * 9 + candidate]) {
                        const pattern = need(h.translate)('Allied with ~i%s~.');
                        let line = this.print(pattern, [text(this.player(candidate), 0xc, 32)]);
                        if (!this.alliances[candidate * 9 + owner]) line += this.format('(One-Way)');
                        line += '\n';
                        if (line.length >= 256) throw new RangeError('Player 메뉴 동맹 서식 초과');
                        need(h.header)(line);
                        allied = true;
                    }
                }
                if (!allied) need(h.header)(this.format('Allied with nobody.\n'));
            }
            let chatItem = false;
            if (!this.mission() || subscriber === this.mySubscriber()) {
                this.identity.addressForSubscriber(subscriber);
                if (this.meta()) {
                    const finder = new Uint8Array(60);
                    RawPlayerIdentity.initialize(finder, 1);
                    // 실제 Finder 의 subscriber DWORD 만 넣고 같은 조회 몸체로 현재 metadata 번호를 찾는다.
                    for (let byte = 0; byte < 4; byte += 1) finder[4 + byte] = (subscriber >>> (8 * byte)) & 0xff;
                    if (this.identity.resolve(finder) && read(finder, 0)) {
                        const index = read(finder, 0);
                        const entry = this.metadata(index);
                        const version = need(h.translate)('Version: ~i%d.%d~.\n');
                        need(h.header)(this.print(version, [read(entry, 0xb0, 2), read(entry, 0xb2, 2)]));
                        const reliability = need(h.translate)('Reliability: ~i%d%%~.\n');
                        const score = need(h.reliability)(index);
                        need(h.header)(this.print(reliability, [score | 0]));
                        const games = Math.min(read(entry, 0x54, 2), 0x7fbc);
                        need(h.header)(this.format('Games Played: ~i%d~.\n', [games]));
                        if (this.meta() || !this.mission()) need(h.item)(this.format('View Player Info'), state.viewInfo, subscriber, subscriber, 1);
                    }
                }
                const ignored = Boolean(need(h.ignored)(subscriber));
                const whispered = need(h.whispered)(subscriber) & 0xff;
                if (subscriber !== this.mySubscriber()) {
                    const command = state.ignore[ignored ? 1 : 0];
                    need(h.checked)(ignored ? 1 : 0, this.format('Ignore chat'), command, subscriber);
                    chatItem = true;
                    if (subscriber !== this.mySubscriber()) {
                        const token = state.whisper[whispered ? 1 : 0];
                        need(h.checked)(whispered, this.format('Whisper To'), token, subscriber);
                    }
                }
            }
            if (this.battle()) {
                owner = this.owner(subscriber);
                if (chatItem) need(h.separator)();
                const local = this.owner(this.mySubscriber());
                this.player(local);
                const label = this.format('Find Priest!');
                const enabled = !this.dead(owner);
                need(h.item)(label, state.findPriest, owner, 1, enabled ? 1 : 0);
                if (subscriber !== this.mySubscriber() && !this.hiddenAt(this.number(local))) this.sendMoney(subscriber, owner);
                if (this.battle() && !need(h.fort)()) {
                    const me = this.owner(this.mySubscriber());
                    this.player(me);
                    this.owner(this.mySubscriber());
                    const target = this.owner(subscriber);
                    if (subscriber !== this.mySubscriber() && !this.hiddenAt(this.number(me))) {
                        const alliedNow = (read(this.player(this.localOwner()), 0x54) & ((1 << (target & 31)) >>> 0)) !== 0;
                        const allianceEnabled = Boolean(need(alliedNow ? h.allowBreak : h.allowOffer)()) && !this.hiddenAt(target);
                        const label2 = this.format(alliedNow ? 'Break Alliance' : 'Request Alliance');
                        need(h.item)(label2, alliedNow ? state.breakAlliance : state.requestAlliance, target, 1, allianceEnabled ? 1 : 0);
                    }
                }
            }
        }
        return need(h.takeMenu)();
    }
}

// 실제 controlled 메뉴/스택에 원본 포함/활성 조건을 그대로 전달한다. (2026-11-15 WB04-E544)
export function bindPlayerMenuControls(hooks, widgets, controls, menus) {
    hooks.beginFancy = () => { widgets.beginFancy(); };
    hooks.beginVertical = () => { widgets.beginVertical(); };
    hooks.separator = () => controls.addSeparator();
    hooks.title = (value) => controls.addTitle(value);
    hooks.header = (value) => controls.addHeader(value);
    hooks.takeMenu = () => menus.takeCurrent();
    hooks.item = (value, command, parameter, include, enabled) => controls.addText(value, command, parameter, include !== 0, enabled !== 0, false);
    hooks.checked = (checked, value, command, parameter) => controls.addChecked(checked !== 0, value, command, parameter, true, true);
    hooks.submenu = (value, child) => controls.addSubmenu(value, child, true, false);
}

// 실제 Player 메뉴의 유일한 입력은 subscriber DWORD 이다. 원본 호출자의 여분 두 인자는 사용하지 않는다. (2026-11-15 WB04-E544)
export function bindObjectPlayerMenu(hooks, players) {
    hooks.playerMenu = (subscriber) => players.open(subscriber);
}
