// Player 전체 초기화(004910a0: 이전 공장·Player 해제 → 1620 BYTE 할당 → 9기록 초기화)를 원본 10.78 관측으로 대조한다(WB04-E7, cpppj PlayerInitializationTests 이식).
//  - player-initialization-x86.tsv 202행(독립 입력 101개 × x87 제어값 027f/037f): 정상 174·NULL 물리 실패 8·외부 중단 20의 반환, 전역 8개, 세 Player 물리 전체,
//    방향표 81 DWORD, 채팅 4800 BYTE, 채팅 이벤트, 호출·전역 재조회 순서를 대조한다. 동맹·Chat 하위는 이식한 RawPlayerParticipation.setAlliance 와 RawChatChannels 가 실제로 수행한다.
//  - 계약: 선택하지 않은 free 는 요구하지 않고 선택 누락/짧은 DWORD 는 앞선 실제 효과를 남긴다.
// cpppj 의 ClientCoreDataGlobals 연동 검사(PlayerInitialization_CoreDetachCacheBeforeReleaseAndNewAliasAfterAllocation)는 client Core 전역 단위를 옮길 때 대조한다.
// (2026-11-08 WB04-E7: Player 전체 초기화 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { initializePlayerRecords } from '../www/core/raw-player-initialization.js';
import { PlayerParticipationState, RawPlayerParticipation } from '../www/core/raw-player-participation.js';
import { CHAT_CHANNEL_CALL, RawChatChannels } from '../www/core/raw-chat-channels.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. 첫 열이 control 인 줄은 머리글이다. (2026-11-08 WB04-E7)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'player-initialization-x86.tsv'));

// 세 Player 물리 식별자(opaque32)와 명시 할당 장치가 발급하는 공장 블록 토큰 기준이다. (2026-11-08 WB04-E7)
const BASES = [0x12200000, 0x12201000, 0x12202000];
const NEW_FACTORY_BASE = 0x12310000;

// 원문 입력 DWORD 를 읽는다. (2026-11-08 WB04-E7)
const number = (text) => Number(BigInt(text) & 0xffffffffn);

// 소문자 hex 로 표시한다(기대 상태를 계산하지 않음). (2026-11-08 WB04-E7)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 외부 중단과 NULL 물리 실패를 구별하는 명시 오류이다. (2026-11-08 WB04-E7)
class InitStop extends Error {}
class InitNull extends RangeError {
    constructor() { super('초기화 입력의 NULL 물리'); }
}

// 할당 주소/외부 입력/실제 동맹·Chat 하위만 제공한다. Player 부모 계산은 제품에 맡긴다(cpppj InitScene). (2026-11-08 WB04-E7)
class InitScene {
    // 기대 결과 없이 원본과 같은 세 초기 물리/이름/포인터/표 패턴을 구성한다.
    constructor(input) {
        const fields = input.split(',');
        assert.equal(fields.length, 10);
        const seed = number(fields[0]);
        const old = number(fields[1]);
        const mask = number(fields[4]);
        const hiddenMask = number(fields[8]);
        this.allocated = number(fields[2]);
        this.player = { playerKnown: number(fields[3]), battleOwner: 1 };
        this.nullNew = number(fields[5]);
        this.stop = fields[6];
        this.mutation = fields[7];
        this.my = number(fields[9]);
        assert.ok(old <= 3 && this.allocated <= 3, 'Player 초기화 입력 base 번호');
        this.base = old === 3 ? 0 : BASES[old];
        this.fortCount = 0x12345678;
        this.lastPlayer = 0x89abcdef;
        this.current = BASES[0] + 180 + 0x6c;
        this.active = 0xa5a5a5a5;
        this.calls = [0, 0, 0, 0, 0];
        this.trace = [];
        this.chatEvents = [];
        this.onFreePlayers = null;
        this.blocks = [0, 1, 2].map(() => new Uint8Array(1620));
        // 이전/새 주소의 stale BYTE 를 각각 독립 준비한다.
        for (let block = 0; block < 3; block += 1) {
            const bytes = this.blocks[block];
            for (let index = 0; index < bytes.length; index += 1) bytes[index] = (seed + block * 53 + index * 17) & 0xff;
            // 명시 공장 포인터/이름/subscriber 만 입력으로 쓴다.
            for (let owner = 0; owner < 9; owner += 1) {
                const offset = owner * 180;
                put(bytes, offset, (mask & (1 << owner)) ? 0x12408000 + block * 0x2000 + owner * 0x100 : 0);
                bytes.fill(65 + owner, offset + 12, offset + 44);
                const name = 'OldName\0';
                for (let position = 0; position < name.length; position += 1) bytes[offset + 12 + position] = name.charCodeAt(position);
                put(bytes, offset + 44, 700 + owner + block * 100);
            }
        }
        // 기존 81 DWORD 의 BYTE 패턴은 동맹 기대 결과가 아니다.
        this.allies = new Uint32Array(81);
        for (let index = 0; index < 81; index += 1) {
            let value = 0;
            for (let byte = 0; byte < 4; byte += 1) value = (value | (((seed + (index * 4 + byte) * 7) & 0xff) << (byte * 8))) >>> 0;
            this.allies[index] = value;
        }
        this.hidden = new Uint8Array(9);
        for (let index = 0; index < 9; index += 1) this.hidden[index] = (hiddenMask >>> index) & 1;
        // 기존 Chat 입력 장치의 stale BYTE 와 실제 빈 Subscriber/NUL 뒤 stale 이름·flags 이다.
        this.chat = new Uint8Array(4800);
        for (let index = 0; index < this.chat.length; index += 1) this.chat[index] = (index * 37) & 0xff;
        for (let index = 0; index < 100; index += 1) {
            const offset = index * 48;
            this.chat.fill(65 + (index % 26), offset, offset + 40);
            const name = 'OldName\0';
            for (let position = 0; position < name.length; position += 1) this.chat[offset + position] = name.charCodeAt(position);
            this.chat[offset + 40] = (index * 13 + 0x95) & 0xff;
            put(this.chat, offset + 44, 0);
        }
    }

    // 현재 같은 물리 주소를 해석하며 NULL 과 다른 물리 범위 위반을 구별한다.
    memory(address, size) {
        if (address < 1620) throw new InitNull();
        // 현재 실제 세 할당에서 요청한 영역만 빌린다.
        for (let index = 0; index < 3; index += 1) {
            if (address >= BASES[index] && address + size <= BASES[index] + this.blocks[index].length) return this.blocks[index].subarray(address - BASES[index], address - BASES[index] + size);
        }
        throw new RangeError('Player 초기화의 미등록 실제 물리');
    }

    // 같은 현재 슬롯의 원문 load/store 만 trace 에 표시한다.
    globals() {
        const scene = this;
        return {
            get fortCount() { return scene.fortCount; },
            set fortCount(value) { scene.fortCount = value; },
            get lastLoginPlayer() { return scene.lastPlayer; },
            set lastLoginPlayer(value) { scene.lastPlayer = value; },
            playerBase: () => { this.trace.push(`R:${this.base}`); return this.base; },
            writePlayerBase: (address) => { this.trace.push(`W:${address}`); this.base = address; },
        };
    }

    // callee 진입 관측이며 원문에 없는 기본 성공이나 부모 결과를 계산하지 않는다.
    boundary(kind, name, argument, mask = 0) {
        this.calls[kind] += 1;
        const count = this.calls[kind];
        this.trace.push(kind === 4 ? `A:${argument}:${mask}` : `${name}:${argument}`);
        if (this.stop === `${name}${count}`) throw new InitStop();
        return count;
    }

    // 명시 외부 장치의 현재 base/필드 변경과 실제 부모 재진입이다.
    mutate(boundary) {
        const dash = this.mutation.indexOf('-');
        if (dash < 0 || this.mutation.slice(0, dash) !== boundary) return;
        const action = this.mutation.slice(dash + 1);
        this.trace.push(`M:${action}`);
        if (action === 'baseB') this.base = BASES[1];
        else if (action === 'baseNull') this.base = 0;
        else if (action === 'row64') put(this.memory(this.base + 0x64, 4), 0, 0x12345678);
        else if (action === 'known1') this.player.playerKnown = 1;
        else if (action === 'reenter') {
            this.trace.push('RE:begin');
            initializePlayerRecords(this.globals(), this.hooks());
            this.trace.push('RE:end');
        } else assert.fail(`Player 초기화의 미해석 명시 변경 ${action}`);
    }

    // 실제 기존 방향별 동맹/Chat 몸체를 같은 현재 Player/표/기록에 연결한다.
    alliance(owner, mask) {
        const count = this.boundary(4, 'alliance', owner, mask);
        this.mutate(`alliance${count}`);
        const channels = new RawChatChannels(this.chat, () => this.my, (call, subscriber) => {
            this.chatEvents.push(call === CHAT_CHANNEL_CALL.nameChanged ? `N,${subscriber}` : 'R');
        });
        const state = new PlayerParticipationState();
        state.localOwner = this.player.battleOwner;
        state.playerKnown = this.player.playerKnown;
        const body = new RawPlayerParticipation(this.memory(this.base, 1620), this.allies, this.hidden, state, {
            clearChat: (channel) => channels.clear(channel),
            addChat: (subscriber, name, channel) => { channels.addToChannel(subscriber, name, channel); },
            assertion: () => { throw new Error('Player 초기화 입력의 예상 밖 동맹 진단'); },
        });
        body.setAlliance(owner, mask);
    }

    // 실제 명시 allocator/free 와 이미 검증한 동맹 자식을 반환한다.
    hooks() {
        return {
            memory: (address, size) => this.memory(address, size),
            freeFactory: (token) => { const count = this.boundary(0, 'freeFactory', token); this.mutate(`freeFactory${count}`); },
            freePlayers: (token) => {
                const count = this.boundary(1, 'freePlayers', token);
                this.onFreePlayers?.(token);
                this.mutate(`freePlayers${count}`);
            },
            mallocPlayers: (size) => {
                assert.equal(size, 1620);
                const count = this.boundary(2, 'malloc', size);
                this.mutate(`malloc${count}`);
                return this.allocated === 3 ? 0 : BASES[this.allocated];
            },
            newFactory: (size) => {
                assert.equal(size, 120);
                const count = this.boundary(3, 'new', size);
                this.mutate(`new${count}`);
                return count === this.nullNew ? 0 : NEW_FACTORY_BASE + (count - 1) * 0x100;
            },
            alliance: (owner, mask) => this.alliance(owner, mask),
        };
    }

    // 정상/외부 중단/NULL 물리 실패만 원문과 같은 이름으로 분류한다.
    run() {
        try {
            initializePlayerRecords(this.globals(), this.hooks());
            return 'normal';
        } catch (error) {
            if (error instanceof InitStop) return 'stop';
            if (error instanceof InitNull) return 'fault-null';
            throw error;
        }
    }

    // 원문 출력과 같은 unsigned DWORD 순서이다.
    globalsText() {
        return [this.base, this.fortCount, this.lastPlayer, this.player.playerKnown, this.player.battleOwner, this.my, this.current, this.active].join(',');
    }

    // 같은 세 Player 물리 전체를 출력한다.
    playersHex() { return hex(this.blocks[0]) + hex(this.blocks[1]) + hex(this.blocks[2]); }

    // 실제 81 DWORD 를 원문 little endian BYTE 로 표시한다(동맹 기대는 계산하지 않음).
    alliesHex() {
        const bytes = new Uint8Array(324);
        for (let index = 0; index < 81; index += 1) put(bytes, index * 4, this.allies[index]);
        return hex(bytes);
    }

    // 같은 이벤트/효과 trace 를 원문 구분자로 표시한다.
    static join(values) { return values.length ? values.join(';') : '-'; }
}

// 전체 정상/중단/NULL/재진입과 실제 동맹·Chat 결과를 각각 대조한다.
test('Player 전체 초기화: 원본10.78 202행(독립 입력 101개 × 두 제어값)', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    let normal = 0;
    let fault = 0;
    let stopped = 0;
    const inputs = new Set();
    for (const row of rows) {
        if (row[0] === 'control') continue;
        assert.equal(row.length, 10);
        const scene = new InitScene(row[1]);
        const label = `행 ${count} ${row[1]}`;
        assert.equal(scene.run(), row[2], `${label} 상태`);
        assert.equal(scene.globalsText(), row[3], `${label} 전역`);
        assert.equal(scene.playersHex(), row[4], `${label} Player 물리`);
        assert.equal(scene.alliesHex(), row[5], `${label} 방향표`);
        assert.equal(hex(scene.chat), row[6], `${label} 채팅 저장소`);
        assert.equal(InitScene.join(scene.chatEvents), row[7], `${label} 채팅 이벤트`);
        assert.equal(InitScene.join(scene.trace), row[8], `${label} 호출 순서`);
        assert.ok((row[0] === '027f' || row[0] === '037f') && number(row[9]) > 0);
        if (row[2] === 'normal') normal += 1;
        else if (row[2] === 'fault-null') fault += 1;
        else { assert.equal(row[2], 'stop'); stopped += 1; }
        inputs.add(row[1]);
        count += 1;
    }
    assert.deepEqual([count, inputs.size, normal, fault, stopped], [202, 101, 174, 8, 20]);
});

// 선택하지 않은 free 는 요구하지 않으며 선택 누락/짧은 DWORD 는 앞선 실제 효과를 남긴다.
test('Player 전체 초기화: 필요 장치 누락은 앞선 부분 효과를 남긴다', () => {
    const missing = new InitScene('165,3,2,0,511,0,none,none,0,4294967295');
    const hooks = missing.hooks();
    delete hooks.freeFactory;
    delete hooks.freePlayers;
    delete hooks.mallocPlayers;
    assert.throws(() => initializePlayerRecords(missing.globals(), hooks), /실제 장치 미연결/);
    assert.deepEqual([missing.fortCount, missing.lastPlayer, missing.base, missing.calls[0], missing.calls[1], missing.calls[3]], [0, 0, 0, 0, 0, 0]);
    const shortStore = new InitScene('165,3,2,0,511,0,none,none,0,4294967295');
    const before = shortStore.blocks[2].slice();
    const shortHooks = shortStore.hooks();
    shortHooks.memory = (address, size) => shortStore.memory(address, size).subarray(0, 3);
    assert.throws(() => initializePlayerRecords(shortStore.globals(), shortHooks), RangeError);
    assert.deepEqual([shortStore.base, shortStore.fortCount, shortStore.lastPlayer, shortStore.calls[2], shortStore.calls[3]], [BASES[2], 0, 0, 1, 0]);
    assert.deepEqual(shortStore.blocks[2], before);
});

// 이전 Player 해제 callback 은 아홉 번의 공장 해제가 끝난 뒤 이전 base 를 받으며, 반납 뒤 base 슬롯은 NULL 이었다가 새 주소가 된다.
test('Player 전체 초기화: 해제 순서와 base 슬롯 전이', () => {
    const scene = new InitScene('165,0,2,0,511,0,none,none,0,4294967295');
    const seen = [];
    scene.onFreePlayers = (token) => { seen.push([token, scene.calls[0]]); };
    assert.equal(scene.run(), 'normal');
    assert.deepEqual(seen, [[BASES[0], 9]]);
    assert.equal(scene.base, BASES[2]);
    assert.equal(scene.fortCount, 0);
    assert.equal(scene.lastPlayer, 0);
    // 새 Player 영역의 각 행은 공장 토큰·30·0·0·127 과 자기 동맹 bit 를 가진다.
    for (let owner = 0; owner < 9; owner += 1) {
        const row = scene.blocks[2].subarray(owner * 180, (owner + 1) * 180);
        assert.equal(row[4], 30);
        assert.equal(row[0x78], 127);
        assert.equal(row[0x54] | (row[0x55] << 8), 1 << owner);
    }
    // 이전 base 가 NULL 이면 해제 효과 없이 할당만 한다.
    const fresh = new InitScene('165,3,2,0,511,0,none,none,0,4294967295');
    assert.equal(fresh.run(), 'normal');
    assert.deepEqual([fresh.calls[0], fresh.calls[1], fresh.calls[2], fresh.calls[3], fresh.calls[4]], [0, 0, 1, 9, 9]);
});
