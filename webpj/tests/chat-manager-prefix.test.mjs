// Chat manager 랭크 prefix raw-chat-manager-prefix.js(0042e000 생성·0042e050 초기화·0042d620 setter·0042f480 formatter·0042f540 wrapper)를 원본 10.78 관측과 cpppj `ChatManagerPrefixTests` 계약으로 대조한다.
//  - 원본 관측 chat-manager-prefix-x86.tsv 644행(정상 524·CPU fault 94·명시 중단 26, 두 x87 관측): 입력 13칸(진입점·seed·현재 manager·활성·랭크·subscriber·할당 실패 프로필·아이콘 개수·최대 랭크·반복·Config 길이·변이)에서
//    상태(현재 manager 번호·subscriber·할당 횟수)·외부 출력 256 BYTE·두 manager 0xa98 BYTE 전체·두 내부 0x80 BYTE 전체·하위 호출 trace 가 한 바이트도 다르지 않다(미작성 필드 포함).
//  - 계약: 현재 manager/활성이 0 인 자연 관문은 Core·formatter·버퍼 장치가 없어도 복귀, 실사용 CRT 어댑터는 같은 실제 메모리에 쓰고 기존 사용자 경계를 덮어쓰지 않는다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E229: Chat manager 랭크 prefix 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import {
    CHAT_MANAGER_BUFFER_BYTES, CHAT_MANAGER_BYTES, ChatManagerDivideError, appendChatRankIcon, bindChatManagerMemoryCopy, chatPrefixIcons, constructChatManager, copyChatManagerFormat,
    initializeChatManager, refreshChatManagerRankPrefix, setChatManagerRankPrefix,
} from '../www/core/raw-chat-manager-prefix.js';

// 저장된 독립 기계어 출력을 한 번만 읽는다(11칸: 대조용 FPCW 열·입력·상태·...·trace·명령어 수). (2026-11-10 WB04-E229)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'chat-manager-prefix-x86.tsv'));

// 격리 PE 의 실제 두 manager 할당 토큰과 내부 0x80 할당은 manager 와 별개로 보존한다. (2026-11-10 WB04-E229)
const MANAGERS = [0x12001000, 0x12004000];
const BUFFERS = [0x12008000, 0x12009000];

// 소문자 hex 로 표시한다(NUL 뒤 미작성 BYTE 까지 전체). (2026-11-10 WB04-E229)
const hex = (bytes) => Array.from(bytes, (value) => value.toString(16).padStart(2, '0')).join('');

// 하위 callback 의 명시 중단만 나타내며 물리/CPU fault 와 구별한다. (2026-11-10 WB04-E229)
class ChatStop {}

// 부모 기대를 계산하지 않고 같은 현재 전역·실제 할당과 명시 하위 효과만 제공한다. (2026-11-10 WB04-E229)
class ChatDevice {
    constructor(values) {
        assert.equal(values.length, 13);
        this.managers = [new Uint8Array(CHAT_MANAGER_BYTES), new Uint8Array(CHAT_MANAGER_BYTES)]; // 모든 미작성 필드까지 관측하는 실제 manager 물리이다.
        this.buffers = [new Uint8Array(CHAT_MANAGER_BUFFER_BYTES), new Uint8Array(CHAT_MANAGER_BUFFER_BYTES)]; // 생성자가 초기화하지 않는 실제 내부 물리이다.
        this.output = new Uint8Array(256); // 직접 formatter 의 외부 출력과 wrapper 의 실제 지역 출력은 다른 주소이다.
        this.temporary = new Uint8Array(256);
        this.wrapperOutput = new Uint8Array(256);
        this.core = new Uint8Array(10); // 원문 wrapper 가 unsigned BYTE+4 만 읽는 명시 하위 입력이다.
        this.source = [];
        this.calls = new Map();
        this.trace = [];
        this.allocations = 0;
        this.mode = Number(values[0]);
        const seed = Number(values[1]);
        const selected = Number(values[2]); // 0 은 NULL 이고 1/2 는 서로 다른 실제 할당이다.
        this.current = selected === 0 ? 0 : MANAGERS[selected - 1];
        this.active = Number(values[3]);
        this.rank = Number(values[4]);
        this.subscriber = Number(values[5]) >>> 0;
        this.profile = Number(values[6]);
        this.minor = Number(values[7]);
        this.maximum = Number(values[8]);
        this.repeat = Number(values[9]);
        this.frontLength = Number(values[10]);
        this.backLength = Number(values[11]);
        this.mutation = values[12];
        this.seed = seed;
        for (let object = 0; object < 2; object += 1) for (let byte = 0; byte < CHAT_MANAGER_BYTES; byte += 1) this.managers[object][byte] = (seed + byte * 17 + object * 59) & 0xff; // 미작성 전체 BYTE 의 독립 입력이다.
        for (let buffer = 0; buffer < 2; buffer += 1) for (let byte = 0; byte < CHAT_MANAGER_BUFFER_BYTES; byte += 1) this.buffers[buffer][byte] = (seed + byte * 17 + buffer * 71 + 13) & 0xff; // 내부 new 가 이 물리를 덮지 않는지 확인한다.
        this.output.fill(seed & 0xff);
        this.core[4] = this.rank & 0xff;
        if (this.mode === 2) {
            this.source = new Uint8Array(this.rank + 1).fill(0x41);
            this.source[this.rank] = 0;
            if (this.mutation === 'alias-forward' || this.mutation === 'alias-backward') {
                const offset = this.mutation === 'alias-forward' ? 0x243 : 0x245; // 같은 manager 안의 순방향 겹침을 실제 주소로 제공한다.
                this.managers[0].set([0x41, 0x42, 0x43, 0x44, 0], offset); // 원문에 먼저 쓴 NUL 포함 입력이다.
            }
        }
    }

    // 호출 경계 앞의 실제 인자를 기록하고 종류별 번호를 반환한다. (2026-11-10 WB04-E229)
    event(kind, text) {
        this.trace.push(text);
        const count = (this.calls.get(kind) ?? 0) + 1;
        this.calls.set(kind, count);
        return `${kind}${count}`;
    }

    // 부모 밖 하위 장치가 바꾸는 같은 현재 전역만 수정한다. (2026-11-10 WB04-E229)
    mutate(boundary) {
        const split = this.mutation.indexOf('-'); // alias 입력은 실제 source 로 처리하므로 이 관문을 통과하지 않는다.
        if (split === -1 || this.mutation.slice(0, split) !== boundary) return;
        const action = this.mutation.slice(split + 1); // 이 호출에 지정한 하위 효과이다.
        if (action === 'stop') throw new ChatStop();
        if (action === 'manager0') this.current = 0;
        else if (action === 'manager1') this.current = MANAGERS[0];
        else if (action === 'manager2') this.current = MANAGERS[1];
        else if (action === 'subscriber9') this.subscriber = 9;
        else if (action === 'maximum0') this.maximum = 0;
        else if (action === 'maximumNegative') this.maximum = -1;
        else if (action === 'minor3') this.minor = 3;
        else throw new Error('Chat 미해석 변경');
    }

    // 토큰은 host 포인터로 축소하지 않고 실제 소유 배열의 같은 뷰를 빌린다. (2026-11-10 WB04-E229)
    memory(token) {
        for (let index = 0; index < 2; index += 1) if (token === MANAGERS[index]) return this.managers[index]; // 같은 할당 주소만 허용한다.
        for (let index = 0; index < 2; index += 1) if (token === BUFFERS[index]) return this.buffers[index]; // 내부 할당은 다른 실제 배열이다.
        throw new RangeError('Chat 실제 NULL/미등록 할당');
    }

    // PE 의 가상 주소를 fixture 와 같은 식별자로 표현한다. (2026-11-10 WB04-E229)
    static normalized(address) {
        if (address === 0) return '0';
        for (let index = 0; index < 2; index += 1) if (address >= MANAGERS[index] && address - MANAGERS[index] < CHAT_MANAGER_BYTES) return `M${index + 1}+${address - MANAGERS[index]}`; // offset 도 실제 인자이다.
        for (let index = 0; index < 2; index += 1) if (address === BUFFERS[index]) return `B${index + 1}`; // 내부 할당 주소이다.
        return `0x${(address >>> 0).toString(16)}`; // NULL+offset 같은 범위 밖 인자도 호출 진입 trace 에 남긴다.
    }

    // 실제 current 슬롯의 logical 번호와 살아 있는 전역·할당 시도를 출력한다. (2026-11-10 WB04-E229)
    state() {
        const index = this.current === 0 ? 0 : this.current === MANAGERS[0] ? 1 : this.current === MANAGERS[1] ? 2 : -1; // fixture 식별자는 포인터 사본 소유자가 아니다.
        return `${index},${this.subscriber},${this.allocations}`;
    }

    // trace 를 생략 없이 같은 TSV 표현으로 작성한다. 호출이 없을 때만 '-' 이다. (2026-11-10 WB04-E229)
    traceText() { return this.trace.length ? this.trace.join(';') : '-'; }

    // 부모 분기를 구현하지 않은 명시 할당/설정/CRT/그림 개수 장치를 제공한다. (2026-11-10 WB04-E229)
    hooks() {
        const device = this;
        const normalized = ChatDevice.normalized;
        return {
            allocate: (amount) => {
                let result = 0; // 실패 입력도 별도 반환하며 뒤의 진단/부분 쓰기를 부모에 맡긴다.
                if (amount === CHAT_MANAGER_BYTES) { device.allocations += 1; result = (device.profile & 1) ? 0 : MANAGERS[(device.allocations - 1) % 2]; }
                else if (amount === CHAT_MANAGER_BUFFER_BYTES) result = (device.profile & 2) ? 0 : BUFFERS[(device.allocations === 0 ? 0 : device.allocations - 1) % 2];
                else throw new Error('Chat 예상 밖 할당 크기');
                device.mutate(device.event('new', `N:${amount}:${normalized(result)}`));
                return result;
            },
            memory: (token) => device.memory(token),
            assertion: (line) => {
                const text = line === 220 ? 'address != NULL' : line === 898 ? 'TheChatSystem != NULL' : line === 2474 ? 'maxMinor > 0' : null; // PE 가 넘긴 실제 진단 문자열이다.
                if (!text) throw new Error('Chat 예상 밖 진단');
                device.mutate(device.event('assert', `A:${line}:${text}`));
            },
            registerCommand: (name, callback, flags) => { device.mutate(device.event('command', `C:${name}:${normalized(callback)}:${flags}`)); },
            readConfig: (configKey) => {
                const ordinal = (device.calls.get('read') ?? 0) % 2; // 각 초기화의 front/back 지역 입력이며 반복에도 2개씩 진행한다.
                const length = ordinal === 0 ? device.frontLength : device.backLength; // 누락 ReadString 은 지역 첫 NUL 을 보존한다.
                const boundary = device.event('read', `R:${configKey}:${length}`);
                const value = length >= 0 ? (ordinal === 0 ? 'F' : 'G').repeat(length) : null; // 실제 지역 사본은 뒤의 다른 Config 호출에도 변하지 않는다.
                device.mutate(boundary);
                return value;
            },
            copyFormat: (token, offset, text, count) => {
                const boundary = device.event('strncpy', `P:${normalized((token + offset) >>> 0)}:${count}`); // NULL 인자도 CRT 진입 뒤 물리 오류로 남긴다.
                const bytes = device.memory(token);
                if (offset > bytes.length) throw new RangeError('Chat CRT offset');
                copyChatManagerFormat(bytes.subarray(offset), text, count);
                device.mutate(boundary);
            },
            maximumRank: () => { const value = device.maximum; device.mutate(device.event('max', `X:${value}`)); return value; },
            minorCount: () => { const value = device.minor; device.mutate(device.event('minor', `I:${value}`)); return value; },
            appendIcon: (target, kind, value) => {
                appendChatRankIcon(target, kind, value);
                device.mutate(device.event('append', `F:~[Iicon.${kind}%d]:${value}`));
            },
            core: () => { device.mutate(device.event('core', 'K')); return device.core; },
        };
    }

    // 공개 본문만 선택하며 중단 이후 재호출이나 상태 복구를 하지 않는다. (2026-11-10 WB04-E229)
    run() {
        const device = this;
        const hooks = this.hooks(); // 같은 장치/현재 슬롯은 재초기화 사이에도 유지한다.
        try {
            for (let invocation = 0; invocation < this.repeat; invocation += 1) { // 원문과 같이 새 지역 미정 입력만 매 호출 공급한다.
                this.temporary.fill(this.seed & 0xff);
                const globals = { current: { get value() { return device.current; }, set value(next) { device.current = next >>> 0; } }, subscriber: () => device.subscriber, coreDataActive: () => device.active };
                if (this.mode === 0) assert.equal(constructChatManager(MANAGERS[0], hooks), MANAGERS[0]);
                else if (this.mode === 1) initializeChatManager(globals, hooks);
                else if (this.mode === 2) {
                    let text = this.source; // 일반 입력은 별도 실제 source 이다.
                    if (this.mutation === 'alias-forward' || this.mutation === 'alias-backward') text = this.managers[0].subarray(this.mutation === 'alias-forward' ? 0x243 : 0x245);
                    setChatManagerRankPrefix(this.managers[0], text);
                } else if (this.mode === 3) {
                    const text = chatPrefixIcons(this.output, this.temporary, this.rank, hooks);
                    assert.ok(text.buffer === this.output.buffer && text.byteOffset === this.output.byteOffset && text.length === 256);
                } else if (this.mode === 4) refreshChatManagerRankPrefix(globals, hooks, this.wrapperOutput, this.temporary); // 원문 wrapper 는 자기 stack 에 출력하므로 fixture 의 외부 OUTPUT 은 그대로이다.
                else throw new Error('Chat fixture 진입점');
            }
            return 'normal';
        } catch (error) {
            if (error instanceof ChatStop) return 'stop';
            if (error instanceof RangeError || error instanceof ChatManagerDivideError) return 'fault';
            throw error;
        }
    }
}

test('Chat manager 랭크 prefix: 원본 10.78 644행(정상 524·fault 94·중단 26)의 상태·출력·두 manager 전체 물리·내부 버퍼·trace 가 모두 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    const statuses = { normal: 0, fault: 0, stop: 0 };
    for (const values of rows) {
        assert.equal(values.length, 11);
        assert.ok(values[0] === '639' || values[0] === '895');
        const device = new ChatDevice(values[1].split(','));
        const status = device.run();
        count += 1;
        statuses[status] += 1;
        const label = `행 ${count} 입력 ${values[1]}`;
        assert.equal(status, values[2], `${label} status`);
        assert.equal(device.state(), values[3], `${label} state`);
        assert.equal(hex(device.output), values[4], `${label} output256`);
        assert.equal(hex(device.managers[0]), values[5], `${label} managerA`);
        assert.equal(hex(device.managers[1]), values[6], `${label} managerB`);
        assert.equal(hex(device.buffers[0]), values[7], `${label} bufferA`);
        assert.equal(hex(device.buffers[1]), values[8], `${label} bufferB`);
        assert.equal(device.traceText(), values[9], `${label} trace`);
    }
    assert.equal(count, 644);
    assert.deepEqual(statuses, { normal: 524, fault: 94, stop: 26 });
});

test('Chat manager 랭크 prefix 계약: 현재 manager 나 활성이 0 인 자연 분기는 선택하지 않은 장치 없이도 복귀한다', () => {
    const state = { current: 0, subscriber: 7, active: 1 }; // 최초 manager 미생성과 활성 관문을 독립 구별한다.
    const globals = { current: { get value() { return state.current; }, set value(next) { state.current = next; } }, subscriber: () => state.subscriber, coreDataActive: () => state.active };
    refreshChatManagerRankPrefix(globals, {}, new Uint8Array(0), new Uint8Array(0));
    assert.ok(state.current === 0 && state.subscriber === 7 && state.active === 1);
    state.current = 0x1234;
    state.active = 0;
    refreshChatManagerRankPrefix(globals, {}, new Uint8Array(0), new Uint8Array(0));
    assert.ok(state.current === 0x1234 && state.subscriber === 7 && state.active === 0);
});

test('Chat manager 랭크 prefix 계약: 실사용 CRT 어댑터는 같은 실제 바이트를 빌리고 기존 사용자 경계는 덮어쓰지 않는다', () => {
    const bytes = new Uint8Array(80).fill(0xa5); // NUL padding 밖 물리 보존도 확인한다.
    let memoryReads = 0;
    let customCalls = 0; // 관측용 횟수이며 부모 알고리즘을 흉내 내지 않는다.
    const hooks = { memory: (token) => { memoryReads += 1; if (token !== 17) throw new RangeError('검사 token'); return bytes; } };
    let bound = bindChatManagerMemoryCopy(hooks); // bytes 사본이 아니라 같은 물리를 빌린다.
    bound.copyFormat(17, 20, 'AB', 60);
    assert.ok(memoryReads === 1 && bytes[19] === 0xa5 && bytes[20] === 0x41 && bytes[21] === 0x42);
    assert.ok(bytes.subarray(22).every((byte) => byte === 0));
    hooks.copyFormat = () => { customCalls += 1; }; // 사용자 callback 은 현재값 재조회/계측을 유지한다.
    bound = bindChatManagerMemoryCopy(hooks);
    bound.copyFormat(0, 0, 'unused', 1);
    assert.ok(customCalls === 1 && memoryReads === 1);
    hooks.copyFormat = undefined;
    bound = bindChatManagerMemoryCopy(hooks);
    assert.throws(() => bound.copyFormat(0, 20, 'AB', 60), RangeError); // 선택된 잘못된 실제 토큰만 도달 위치에서 거부한다.
    assert.equal(memoryReads, 2);
});

test('Chat manager 랭크 prefix 계약: formatter 는 캡처 divisor·idiv fault·물리 범위·마지막 DWORD 까지의 256 BYTE 복사를 지킨다', () => {
    const calls = [];
    // 최대 랭크 getter 가 호출마다 다른 값을 주는 장치이다(매 비교에서 다시 읽는 것을 확인한다).
    const makeHooks = (maximums, minor) => {
        let index = 0;
        return {
            maximumRank: () => maximums[Math.min(index++, maximums.length - 1)],
            minorCount: () => minor,
            assertion: (line) => { calls.push(`A${line}`); },
            appendIcon: (target, kind, value) => { calls.push(`${kind}${value}`); appendChatRankIcon(target, kind, value); },
        };
    };
    const text = (bytes) => String.fromCharCode(...bytes.subarray(0, bytes.indexOf(0)));
    // 정상: 랭크 7, 최대 31, 아이콘 3 → 몫 2 → C1, 나머지 1 → B1.
    const output = new Uint8Array(256).fill(0x55);
    const temporary = new Uint8Array(256).fill(0x66);
    temporary[252] = 0x11; temporary[253] = 0x22; temporary[254] = 0x33; temporary[255] = 0x44;
    assert.equal(text(chatPrefixIcons(output, temporary, 7, makeHooks([31], 3))), '~[Iicon.C1]~[Iicon.B1]');
    assert.deepEqual(Array.from(output.subarray(252)), [0x11, 0x22, 0x33, 0x44]); // 마지막 DWORD 도 복사된다.
    assert.deepEqual(calls, ['C1', 'B1']);
    // 음수 랭크는 아이콘 없이 temporary 를 그대로 복사한다.
    calls.length = 0;
    const plain = new Uint8Array(256).fill(0x77);
    assert.equal(chatPrefixIcons(plain, Uint8Array.from({ length: 256 }, (_, i) => i), -1, makeHooks([31], 3)).length, 256);
    assert.ok(calls.length === 0 && plain[0] === 0 && plain[255] === 255);
    // 진단 뒤 divisor 는 다시 읽지 않는다: 아이콘 개수 0 이면 진단 뒤 idiv fault, INT_MIN / -1 도 fault 이며 아이콘은 만들지 않는다.
    calls.length = 0;
    assert.throws(() => chatPrefixIcons(new Uint8Array(256), new Uint8Array(256), 5, makeHooks([31], 0)), ChatManagerDivideError);
    assert.deepEqual(calls, ['A2474']);
    calls.length = 0;
    assert.throws(() => chatPrefixIcons(new Uint8Array(256), new Uint8Array(256), 5, makeHooks([100, -2147483648, -2147483648], -1)), ChatManagerDivideError);
    assert.deepEqual(calls, ['A2474']);
    // 최대 랭크가 중간에 음수로 줄어 랭크가 음수가 되면 몫은 0 이하라 C 없이 B(음수 나머지)만 붙는다.
    calls.length = 0;
    const clamped = new Uint8Array(256);
    assert.equal(text(chatPrefixIcons(clamped, new Uint8Array(256), 10, makeHooks([100, -7, -7], 3))), '~[Iicon.B-1]');
    assert.deepEqual(calls, ['B-1']);
    // 물리 범위: 256 미만 temporary 는 쓰기 전에 거부하고(temporary[0] 불변), 256 미만 output 은 formatter 뒤 복사에서 거부한다.
    const shortTemporary = new Uint8Array(255).fill(0x66);
    assert.throws(() => chatPrefixIcons(new Uint8Array(256), shortTemporary, -1, makeHooks([31], 3)), RangeError);
    assert.equal(shortTemporary[0], 0x66);
    assert.throws(() => chatPrefixIcons(new Uint8Array(255), new Uint8Array(256), -1, makeHooks([31], 3)), RangeError);
});

test('Chat manager 랭크 prefix 계약: setter·append·strncpy·wrapper 의 물리 범위는 쓰기 전/중에 거부하고 지역 C문자열은 첫 NUL 에서 끊긴다', () => {
    // setter: NUL 이 없는 source 는 소진될 때까지 쓴 뒤 거부한다(앞선 쓰기 보존).
    const manager = new Uint8Array(CHAT_MANAGER_BYTES);
    assert.throws(() => setChatManagerRankPrefix(manager, Uint8Array.from([1, 2, 3])), RangeError);
    assert.deepEqual(Array.from(manager.subarray(0x244, 0x248)), [1, 2, 3, 0]);
    // append: NUL 없는 대상과 서식이 들어가지 않는 대상은 거부하고 아무것도 쓰지 않는다.
    const noNul = new Uint8Array(32).fill(0x41);
    assert.throws(() => appendChatRankIcon(noNul, 'B', 1), RangeError);
    assert.ok(noNul.every((value) => value === 0x41));
    const tight = new Uint8Array(12); // "~[Iicon.B1]" 은 NUL 포함 12 BYTE 가 필요하다.
    tight[3] = 0;
    tight.fill(0x42, 0, 3);
    assert.throws(() => appendChatRankIcon(tight, 'B', 1), RangeError);
    assert.deepEqual(Array.from(tight.subarray(0, 4)), [0x42, 0x42, 0x42, 0]);
    const exact = new Uint8Array(12);
    appendChatRankIcon(exact, 'B', 1);
    assert.equal(String.fromCharCode(...exact.subarray(0, 11)), '~[Iicon.B1]');
    // strncpy: count 가 대상보다 크면 쓰기 전에 거부하고, 첫 NUL 뒤 문자는 복사하지 않고 NUL 로 채운다.
    const target = new Uint8Array(5).fill(0x7a);
    assert.throws(() => copyChatManagerFormat(target, 'abc', 6), RangeError);
    assert.ok(target.every((value) => value === 0x7a));
    copyChatManagerFormat(target, 'AB CD', 5);
    assert.deepEqual(Array.from(target), [0x41, 0x42, 0, 0, 0]);
    // wrapper: 현재 Core 가 5 BYTE 미만이면 formatter 에 도달하기 전에 거부한다.
    const state = { current: MANAGERS[0], active: 1 };
    const globals = { current: { get value() { return state.current; }, set value(next) { state.current = next; } }, subscriber: () => 0, coreDataActive: () => state.active };
    let formatterCalls = 0;
    const hooks = { core: () => new Uint8Array(4), maximumRank: () => { formatterCalls += 1; return 1; } };
    assert.throws(() => refreshChatManagerRankPrefix(globals, hooks, new Uint8Array(256), new Uint8Array(256)), RangeError);
    assert.equal(formatterCalls, 0);
});

