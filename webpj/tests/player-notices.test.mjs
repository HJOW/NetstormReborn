// Player 링크 정리(0048fcd0)·프레임 끝 대기 결과 알림(00490da0)·Core 기술 합계(0042b210)·AI 피해 통지(00416760)를 원본 10.78 관측으로 대조한다(WB04-E8,
// cpppj PlayerLinksCleanupTests·PlayerNotificationTests·CampaignNoticeTests 이식).
//  - player-links-cleanup-x86.tsv 162행(정상 138·중단 18·NULL 물리 6): 반환 구분, 현재 base/owner 전역, 두 Player 1620 BYTE 전체, 반납 인자, 현재 base 재조회와 free 순서.
//  - player-notices-x86.tsv 118행: 전체 9기록·owner·부분 효과·호출 순서. core-technology-weight-x86.tsv 92행: 결과 BYTE·Core 마스크/rank 재조회·TYPE 개수 변경·getter 순서.
//  - player-damage-notice-x86.tsv 144행: 입력별 호출 여부와 세 인자, Player 기록 불변.
//  - 계약: 실제 반납 소유자 수명, 선택하지 않은 장치는 요구하지 않음, 부분 효과 보존, 현재 모드/AI 필드 재조회.
// (2026-11-08 WB04-E8: Player 정리·알림 묶음 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { cleanupPlayerLinks } from '../www/core/raw-player-links-cleanup.js';
import { coreTechnologyWeight, notifyPendingPlayerResult } from '../www/core/raw-player-notification.js';
import { RawPlayerDamageNotice } from '../www/core/raw-player-damage-notice.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E8)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const linkRows = fixture('player-links-cleanup-x86.tsv');
const noticeRows = fixture('player-notices-x86.tsv');
const weightRows = fixture('core-technology-weight-x86.tsv');
const damageRows = fixture('player-damage-notice-x86.tsv');

// 입력 DWORD 를 32비트로 감아 읽고 소문자 hex 로 표시한다. (2026-11-08 WB04-E8)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 선택 free 의 명시 중단을 실제 물리 계약 오류와 구별한다. (2026-11-08 WB04-E8)
class LinksStop extends Error {}

// 부모 기대가 아닌 명시 초기 물리/현재 base/free 외부 장치만 제공한다(cpppj LinksDevice). (2026-11-08 WB04-E8)
class LinksDevice {
    constructor(input) {
        assert.equal(input.length, 6);
        const seed = Number(input[0]);
        const masks = [Number(input[1]), Number(input[2])];
        this.current = Number(input[3]);
        this.stop = Number(input[4]);
        this.mutation = input[5];
        this.owner = 7;
        this.calls = 0;
        this.tokens = [];
        this.trace = [];
        this.blocks = [new Uint8Array(1620), new Uint8Array(1620)];
        // 다른 미작성 필드도 모두 원시 입력과 같이 만든다.
        for (let block = 0; block < 2; block += 1) {
            for (let byte = 0; byte < 1620; byte += 1) this.blocks[block][byte] = (seed + block * 53 + byte * 17) & 0xff;
            // 토큰은 호스트 포인터로 해석하지 않는다.
            for (let player = 0; player < 9; player += 1) put(this.blocks[block], player * 180 + 104, (masks[block] & (1 << player)) ? 0x21000000 + block * 0x1000 + player * 0x10 : 0);
        }
    }

    // 원문의 매 현재 base 읽기와 동일하게 조회하고 별도 사본으로 동기화하지 않는다.
    players() {
        this.trace.push(`R:${this.current}`);
        return this.current === 2 ? new Uint8Array(0) : this.blocks[this.current];
    }

    // 원문의 같은 free 인자/현재값 변경/재진입만 제공하며 실제 CRT free 완료를 주장하지 않는다.
    release(token) {
        this.trace.push(`F:${token}`);
        this.tokens.push(token);
        this.calls += 1;
        if (this.calls === this.stop) throw new LinksStop();
        const separator = this.mutation.indexOf('-');
        if ((separator < 0 ? this.mutation : this.mutation.slice(0, separator)) !== `f${this.calls}`) return;
        const action = this.mutation.slice(separator + 1);
        if (action === 'baseB') this.current = 1;
        else if (action === 'baseA') this.current = 0;
        else if (action === 'null') this.current = 2;
        else if (action === 'owner8') this.owner = 8;
        else if (action === 'pointerA') put(this.blocks[0], 104, 0x21234567);
        else if (action === 'nextA') put(this.blocks[0], 180 + 104, 0);
        else if (action === 'reenter') {
            this.trace.push('RE:begin');
            cleanupPlayerLinks(this.hooks());
            this.trace.push('RE:end');
        } else assert.fail(`Player 링크 미해석 명시 효과 ${action}`);
    }

    // 실제 부모에 같은 현재 물리/자식을 연결한다.
    hooks() { return { players: () => this.players(), release: (token) => this.release(token) }; }
}

// 실제 PE 정상 138/중단 18/NULL 6 의 전체 물리/현재 전역/trace 를 모두 대조한다.
test('Player 링크 정리: 원본10.78 162행(정상 138·중단 18·NULL 6)', { skip: !linkRows && '원본 fixture 없음' }, () => {
    let count = 0;
    const statuses = { normal: 0, stop: 0, 'fault-null': 0 };
    for (const values of linkRows) {
        assert.equal(values.length, 8);
        const device = new LinksDevice(values[1].split(','));
        let status = 'normal';
        try { cleanupPlayerLinks(device.hooks()); } catch (error) {
            if (error instanceof LinksStop) status = 'stop';
            else if (error instanceof RangeError && device.current === 2) status = 'fault-null';
            else throw error;
        }
        count += 1;
        statuses[status] += 1;
        const label = `행 ${count} ${values[1]}`;
        assert.equal(status, values[2], `${label} 상태`);
        assert.equal(`${device.current},${device.owner}`, values[3], `${label} 전역`);
        assert.equal(hex(device.blocks[0]) + hex(device.blocks[1]), values[4], `${label} Player`);
        assert.equal(device.tokens.length ? device.tokens.join(',') : '-', values[5], `${label} 반납 입력`);
        assert.equal(device.trace.join(';'), values[6], `${label} 호출 순서`);
    }
    assert.deepEqual([count, statuses.normal, statuses.stop, statuses['fault-null']], [162, 138, 18, 6]);
});

// 실제 반납은 선택 토큰의 유일한 소유자가 수행하고 부모가 Player 슬롯만 뒤에서 0 으로 쓴다.
test('Player 링크 정리: 소유한 할당을 반납하고 0 토큰은 건너뛴다', () => {
    const players = new Uint8Array(1620);
    const allocations = new Map();
    let releases = 0;
    // 각 토큰이 소유한 352 BYTE 할당을 만든다.
    for (let owner = 0; owner < 9; owner += 1) {
        const token = owner + 1;
        allocations.set(token, new Uint8Array(352));
        put(players, owner * 180 + 104, token);
        put(players, owner * 180 + 156, 17);
        put(players, owner * 180 + 8, 21);
        put(players, owner * 180, 0x89abcdef);
    }
    const hooks = { players: () => players, release: (token) => {
        assert.ok(allocations.has(token));
        assert.equal(get(players, (token - 1) * 180 + 104), token, '반납 시점에는 Player 슬롯이 아직 지워지지 않음');
        allocations.delete(token);
        releases += 1;
    } };
    cleanupPlayerLinks(hooks);
    assert.equal(allocations.size, 0);
    assert.equal(releases, 9);
    // Player+0 의 별도 할당은 반납하지 않고 세 필드만 0 이 된다.
    for (let owner = 0; owner < 9; owner += 1) {
        assert.deepEqual([get(players, owner * 180 + 104), get(players, owner * 180 + 156), get(players, owner * 180 + 8), get(players, owner * 180)], [0, 0, 0, 0x89abcdef]);
    }
    // 모든 동적 필드가 0 이면 미선택 release 를 선행 요구하지 않는다.
    cleanupPlayerLinks({ players: () => players });
    assert.equal(releases, 9);
});

// 필수 free/물리 오류는 앞선 선택 필드 0 을 보존하고 뒤의 pointer 0/다음 기록을 앞당기지 않는다.
test('Player 링크 정리: 반납 장치 누락과 짧은 물리는 앞선 효과를 남긴다', () => {
    const players = new Uint8Array(1620).fill(0xa5);
    put(players, 104, 1);
    assert.throws(() => cleanupPlayerLinks({ players: () => players }), /실제 장치 미연결/);
    assert.deepEqual([get(players, 156), get(players, 8), get(players, 104), players[180 + 156]], [0, 0, 1, 0xa5]);
    let releases = 0;
    assert.throws(() => cleanupPlayerLinks({ players: () => players.subarray(0, 160), release: (token) => { assert.equal(token, 1); releases += 1; } }), RangeError);
    // 두 번째 record 의 첫 store 는 짧은 host 뷰에서 실패한다.
    assert.deepEqual([releases, get(players, 104), players[180 + 156]], [1, 0, 0xa5]);
});

// 실제 490da0/48fc60 의 118입력은 전체 9기록/owner/부분 효과까지 일치해야 한다.
test('Player 알림: 원본10.78 대기 결과 118행(전체 9기록·owner·부분 효과)', { skip: !noticeRows && '원본 fixture 없음' }, () => {
    class Stop extends Error {}
    let cases = 0;
    for (const f of noticeRows) {
        assert.equal(f.length, 14);
        // 원문 이웃 기록/미사용 BYTE 까지 고정한 입력이다.
        const players = new Uint8Array(9 * 180);
        for (let index = 0; index < players.length; index += 1) players[index] = (index * 37 + 11) & 0xff;
        put(players, 180 + 0xa0, number(f[3]));
        put(players, 180 + 0x9c, number(f[4]));
        players[180 + 0x71] = number(f[5]) & 0xff;
        let owner = 1;
        let events = 0;
        const trace = [];
        // 효과 적용 전 진입이다.
        const event = (value) => { trace.push(value); events += 1; if (events === number(f[8])) throw new Stop(); };
        // 명시 하위 응답의 현재 데이터 변경이다.
        const mutate = (point) => { if (number(f[9]) === point) { owner = 2; put(players, 180 + 0xa0, 125); } };
        const hooks = {
            global: (address) => {
                switch (address) {
                    case 0x595344: return number(f[1]);
                    case 0x594fbc: return number(f[2]);
                    case 0x540c70: return owner;
                    case 0x5411cc: return 93;
                    case 0x5412c0: return 154;
                    default: throw new Error('알림 검사 예상 밖 전역');
                }
            },
            players: () => players,
            technology: () => { event('T'); return number(f[1]) ? get(players, owner * 180 + 0x9c) : 0; },
            weight: (core) => { event(`W,${hex(core)}`); mutate(1); return number(f[6]) & 0xff; },
            contained: (parent, mask, type, index) => { event(`F,${parent},${mask},${type},${index}`); mutate(2); return number(f[7]); },
            tell: (text) => { event(`N,${text}`); mutate(3); },
            grant: (type, current) => event(`G,${type},${current}`),
            refresh: () => event('S'),
        };
        let completed = true;
        try { notifyPendingPlayerResult(hooks); } catch (error) { if (error instanceof Stop) completed = false; else throw error; }
        const label = `행 ${cases} ${f[0]}`;
        assert.equal(completed, f[10] === '1', `${label} 완료`);
        assert.equal(owner, number(f[11]), `${label} owner`);
        assert.equal(hex(players), f[12], `${label} 전체 Player`);
        assert.equal(trace.join(';'), f[13], `${label} 호출 순서`);
        cases += 1;
    }
    assert.equal(cases, 118);
});

// 실제 42b210 의 92입력은 결과 BYTE/현재 Core 마스크·rank/타입 수·getter 순서를 비교한다.
test('Core 기술 합계: 원본10.78 92행(결과·재조회·타입 수 변경·getter 순서)', { skip: !weightRows && '원본 fixture 없음' }, () => {
    let cases = 0;
    for (const f of weightRows) {
        assert.equal(f.length, 9);
        // 타입 표/현재 개수와 실제 10 BYTE 를 독립 입력으로 만든다.
        const core = new Uint8Array(10);
        core[5] = number(f[2]) & 0xff;
        put(core, 6, number(f[1]));
        const types = [];
        if (f[3] !== '-') {
            for (const value of f[3].split(';')) {
                const comma = value.indexOf(',');
                types.push({ bit: Number(value.slice(0, comma)), level: Number(value.slice(comma + 1)) });
            }
        }
        let count = types.length;
        let calls = 0;
        const trace = [];
        const hooks = {
            count: () => count,
            type: (index) => {
                trace.push(`Y,${index}`);
                calls += 1;
                if (calls === 1) {
                    if (f[4] === '1') { put(core, 6, 0x80000000); core[5] = 7; }
                    if (f[4] === '2') count = 1;
                }
                assert.ok(index >= 0 && index < types.length, 'TYPE 조회 범위');
                return types[index];
            },
        };
        const label = `행 ${cases} ${f[0]}`;
        assert.equal(coreTechnologyWeight(core, hooks), number(f[5]), `${label} 결과`);
        assert.equal(hex(core.subarray(5)), f[6], `${label} Core 끝 5BYTE`);
        assert.equal(count, number(f[7]) | 0, `${label} 타입 수`);
        assert.equal(trace.join(';'), f[8], `${label} getter 순서`);
        cases += 1;
    }
    assert.equal(cases, 92);
});

// 원문의 선행 자연 반환은 선택되지 않은 큰 게임 장치를 요구하지 않는다.
test('Player 알림: 선택하지 않은 장치는 요구하지 않는다', () => {
    let reads = 0;
    notifyPendingPlayerResult({ global: () => { reads += 1; return 0; } });
    assert.equal(reads, 1);
    const core = new Uint8Array(10);
    core[5] = 43;
    // 마스크 0 은 타입 개수/표를 읽지 않는다.
    assert.equal(coreTechnologyWeight(core, {}), 43);
    assert.throws(() => coreTechnologyWeight(new Uint8Array(9), {}), RangeError);
});

// 실제 __thiscall 경계에서 얻은 144개 입력/호출 여부/세 인자를 전수 대조한다.
test('Player 피해 통지: 원본10.78 144행(호출 여부·세 인자·기록 불변)', { skip: !damageRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of damageRows) {
        assert.equal(row.length, 9);
        const owner = number(row[0]) | 0;
        const mode = { challenge: number(row[1]) !== 0 };
        const players = new Uint8Array(9 * 0xb4).fill(0x46);
        put(players, Math.max(owner, 0) * 0xb4 + 0xb0, number(row[2]));
        const before = players.slice();
        let calls = 0;
        let observed = [0, 0, 0];
        const notice = new RawPlayerDamageNotice(players, mode, (token, victim, attacker) => { observed = [token, victim, attacker]; calls += 1; });
        notice.notice(owner, number(row[3]) & 0xffff, number(row[4]) & 0xffff);
        const label = `행 ${count} ${row.join(',')}`;
        assert.equal(calls, number(row[5]), `${label} 호출`);
        assert.deepEqual(observed, [number(row[6]), number(row[7]), number(row[8])], `${label} 인자`);
        assert.deepEqual(players, before, `${label} 기록 불변`);
        count += 1;
    }
    assert.equal(count, 144);
});

// 비영 AI 는 실제 몸체 없이는 실패하며 현재 모드/AI 필드의 재조회와 범위 경계를 확인한다.
test('Player 피해 통지: 현재 모드·AI 재조회와 누락 거부', () => {
    const players = new Uint8Array(9 * 0xb4);
    const mode = { challenge: false };
    const notice = new RawPlayerDamageNotice(players, mode);
    notice.notice(1, 20000, 20001);
    put(players, 0xb4 + 0xb0, 0x12345678);
    // challenge 이면 범위 밖 owner 도 검사 전에 반환한다.
    mode.challenge = true;
    notice.notice(39, 0, 0);
    mode.challenge = false;
    assert.throws(() => notice.notice(1, 20000, 20001), /00415460 미연결/);
    put(players, 0xb4 + 0xb0, 0);
    notice.notice(1, 20000, 20001);
    assert.throws(() => notice.notice(9, 0, 0), RangeError);
    assert.throws(() => new RawPlayerDamageNotice(new Uint8Array(10), mode), RangeError);
});
