// 구름 표시 raw-kencloud-render.js(updateKencloud/markKencloudDirty/cleanKencloudDirty/kencloudRowDirty/drawKencloud)를 원본 10.78 관측 944행과 cpppj 계약으로 대조한다.
//  - 원본 관측: kencloud-render-x86.tsv — 진입점(update/mark/clean/query/draw/alternate)·캐시·speed·dirty 패턴·전체/부분 요청·면적·원점·하위 변화(mutation, 명시 중단 포함)로
//    완료 종류(return 또는 중단 경계), 조회 결과, 2054 DWORD 전체 물리의 FNV-1a 64비트, 최종 clip, speed, 하위 호출 순서(trace)가 같다. 두 x87 정밀도 열(639/895)의 행을 모두 소비한다.
//  - 계약: 자연 생략은 하위 장치를 요구하지 않고, 실제 순서의 물리 오류는 앞선 효과를 유지하며, 행 -1 은 같은 cache DWORD 의 인접 alias 를 읽는다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E119: 구름 표시 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { cleanKencloudDirty, drawKencloud, kencloudRowDirty, markKencloudDirty, updateKencloud } from '../www/core/raw-kencloud-render.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E119)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'kencloud-render-x86.tsv')) ?? [];

// 독립 원문의 명시 생성/변경 주소이다. (2026-11-08 WB04-E119)
const FIRST = 0x12001000;
const SECOND = 0x12051000;

// 명시 경계 중단을 정상 return 과 구별하는 표식이다. (2026-11-08 WB04-E119)
class Stop {
    // 중단한 경계 이름이다.
    constructor(cause) { this.cause = cause; }
}

// 원시 unsigned DWORD 의 십진 입력이다. (2026-11-08 WB04-E119)
const numbers = (values) => values.join(',');

// 같은 little endian 2054 DWORD 전체의 FNV-1a 64비트를 16진 16자리로 만든다(64비트 곱을 32비트 두 쪽으로 계산). (2026-11-08 WB04-E119)
function hash(state) {
    let lo = 0x84222325;
    let hi = 0xcbf29ce4;
    // 모든 DWORD 의 낮은 바이트부터 섞는다.
    for (const word of state) {
        for (let shift = 0; shift < 32; shift += 8) {
            lo = (lo ^ ((word >>> shift) & 0xff)) >>> 0;
            // 소수 0x100000001b3 곱: value*0x1b3 + (value << 40).
            const product = lo * 0x1b3;
            const carry = Math.floor(product / 4294967296);
            const nextHi = (hi * 0x1b3 + carry + ((lo << 8) >>> 0)) % 4294967296;
            lo = product % 4294967296;
            hi = nextHi;
        }
    }
    return hi.toString(16).padStart(8, '0') + lo.toString(16).padStart(8, '0');
}

// 명시 하위 효과만 구현하는 장치이다(부모 계산은 제품 몸체를 호출한다). (2026-11-08 WB04-E119)
class Device {
    // 같은 packed 물리와 기존 clip, 명시 현재 속도·경계 순번과 관측 문자열이다.
    state = new Uint32Array(2054);
    clip = [0xfffffff7, 0xfffffff9, 900, 700];
    speed = 0;
    steps = 0;
    clips = 0;
    blits = 0;
    mutation = '';
    trace = [];

    // 진입 순서만 기록한다. (2026-11-08 WB04-E119)
    event(value) { this.trace.push(value); }

    // 원문 도구와 같은 명시 하위 입력이다. (2026-11-08 WB04-E119)
    mutate(label) {
        const mutation = this.mutation;
        if (mutation === `${label}-stop`) throw new Stop(label);
        if (mutation === `${label}-speed2`) this.speed = 2;
        else if (mutation === `${label}-speed0`) this.speed = 0;
        else if (mutation === `${label}-cache22`) this.state[515] = SECOND;
        else if (mutation === `${label}-all0`) this.state[2051] = 0;
        else if (mutation === `${label}-all1`) this.state[2051] = 1;
    }

    // 실제 잠금/복사 장치는 별도 실제 Screen 검사로 대조하므로 여기서는 인자와 순서만 기록한다. (2026-11-08 WB04-E119)
    hooks() {
        return {
            build: () => { this.event('BUILD'); this.state[515] = FIRST; this.mutate('build'); },
            step: () => { this.steps += 1; this.event('STEP'); this.mutate(`step${this.steps}`); },
            clip: () => this.clip.slice(),
            setClip: (value) => { this.clips += 1; this.event(`CLIP:${numbers(value)}`); this.clip = value.slice(); this.mutate(`clip${this.clips}`); },
            lock: (alternate) => { this.event(`LOCK:${alternate ? 1 : 0}`); this.mutate('lock'); },
            unlock: (alternate) => { this.event(`UNLOCK:${alternate ? 1 : 0}`); this.mutate('unlock'); },
            blit: (args) => { this.blits += 1; this.event(`BLIT:${numbers(args)}`); this.mutate(`blit${this.blits}`); }
        };
    }
}

test('KencloudRender: 원본 10.78 944행의 전체 물리·clip·속도·반환·호출 순서가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    let count = 0;
    // 고정 fixture 행을 정규화하거나 중복 제거하지 않는다.
    for (const fields of rows) {
        const args = fields[1].split(',');
        assert.ok(fields.length === 8 && args.length === 13);
        const device = new Device();
        // 같은 현재 물리 초기 입력이다.
        for (let index = 0; index < device.state.length; index += 1) device.state[index] = 0x81000000 + index;
        const number = (index) => Number(args[index]) >>> 0;
        const cache = number(1) ? FIRST : 0;
        const pattern = number(3);
        device.speed = number(2);
        device.mutation = args[12];
        // 같은 이전/현재 행이다.
        for (let index = 0; index < 512; index += 1) {
            device.state[index] = (pattern === 2 || (pattern === 1 && (index === 0 || index === 8 || index === 511))) ? 1 : 0;
            device.state[516 + index] = (pattern === 2 || (pattern === 1 && (index === 1 || index === 8 || index === 511))) ? 1 : 0;
        }
        device.state[512] = cache + 262144;
        device.state[513] = cache + 261632;
        device.state[514] = cache + 512;
        device.state[515] = cache;
        device.state[2051] = number(4);
        device.state[2052] = number(5);
        device.state[2053] = 9;
        // 같은 인자 DWORD 를 보존한다.
        const area = [0, 1, 2, 3, 4, 5].map((index) => number(index + 6));
        const hooks = device.hooks();
        const globals = { state: device.state, get speed() { return device.speed; } };
        let result = 0;
        let completion = 'return';
        try {
            if (args[0] === 'update') updateKencloud(globals, hooks);
            else if (args[0] === 'mark') markKencloudDirty(device.state);
            else if (args[0] === 'clean') cleanKencloudDirty(device.state);
            else if (args[0] === 'query') result = kencloudRowDirty(device.state, area[0], area[1]);
            else drawKencloud(globals, area, hooks, args[0] === 'alternate');
        } catch (error) {
            if (!(error instanceof Stop)) throw error;
            completion = error.cause;
        }
        const actual = [fields[0], fields[1], completion, String(result), hash(device.state), numbers(device.clip), String(device.speed), device.trace.join(';')].join('\t');
        assert.equal(actual, fields.join('\t'), `구름 표시 행 ${count} ${fields[1]}`);
        count += 1;
    }
    assert.equal(count, 944);
});

test('KencloudRender: 자연 생략은 하위를 요구하지 않고 실제 순서의 물리 오류는 앞선 효과를 유지한다', () => {
    // 면적 0(곱의 하위 32비트)과 기존 캐시 + 속도 0 은 어떤 장치도 요구하지 않는다.
    const empty = { state: new Uint32Array(0), speed: 0 };
    drawKencloud(empty, [0, 0, 65536, 65536, 0, 0], {}, true);
    const cache = new Uint32Array(516);
    cache[515] = 1;
    updateKencloud({ state: cache, speed: 0 }, {});
    // 짧은 물리는 첫 store 뒤 부분 요청 쓰기에서 거부되고 앞선 쓰기는 남는다.
    const shortState = new Uint32Array(2052);
    assert.throws(() => markKencloudDirty(shortState), RangeError);
    assert.equal(shortState[2051], 1);
    // 행 -1 은 같은 cache DWORD 의 인접 alias 를 읽는다.
    const device = new Device();
    device.state[515] = 7;
    assert.equal(kencloudRowDirty(device.state, 0xffffffff, 0), 1);
    device.state[515] = 0;
    assert.equal(kencloudRowDirty(device.state, 0xffffffff, 0), 0);
    // 선택된 장치가 없으면 도달 시점에 거부한다.
    assert.throws(() => updateKencloud({ state: new Uint32Array(2054), speed: 1 }, {}), Error);
    assert.throws(() => drawKencloud({ state: new Uint32Array(2054), speed: 0 }, [0, 0, 10, 10, 0, 0], {}), Error);
});
