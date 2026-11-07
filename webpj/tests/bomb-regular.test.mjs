// 주문 효과 Regular raw-bomb-regular.js(RawBombRegular)를 독립 원본 관측 bomb-regular-x86.tsv 741행으로 대조한다.
// 입력 13개 DWORD: [event, count, payload 비트, 시전 시간, 운반체 SID, 권한, 상태 BYTE, 대상 수, 변형, 프로토콜, 선언 길이, 현재 owner, 첫 프레임].
// 변형 1~8 은 통지/프레임/애니메이션/대상 검색/배포/수신/해제 콜백이 현재 TYPE·권한·패킷 헤더·owner·상태를 바꾸는 명시 입력이다.
// 반환 float 비트·효과 문자열(호출 순서와 인자 DWORD, 배포·수신 패킷 hex)·부모 raw 50바이트를 원본과 비교하며 기대값은 계산하지 않는다. (2026-11-08 WB04-E54: RawBombRegular 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { RawBombRegular } from '../www/core/raw-bomb-regular.js';
import { SidPool } from '../www/core/sid-pool.js';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, floatToBits, put, readRows } from './raw-support.mjs';

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E54)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 입력 십진수를 원본 하위 32비트 수로 읽는다. (2026-11-08 WB04-E54)
const unsigned = (text) => Number(BigInt.asUintN(32, BigInt(text)));

// 부모 50 을 가진 장면이다. 원문 입력만 설치하며 기대 시간/패킷/반환값은 계산하지 않는다. (2026-11-08 WB04-E54)
class Scene {
    pool = new SidPool(24000, { server: false });
    types = Array.from({ length: 188 }, () => ({ firstFrame: 0, castTime: 0 }));
    state = { boss: 0, localOwner: 1, effect: { message: 0x5f, protocol: 1, length: 10 } };
    input = [];
    trace = '';

    // 부모 슬롯의 쓰기 가능한 50바이트이다.
    raw() {
        return this.pool.physical(50).subarray(0, 50);
    }

    // 효과 인자의 DWORD 비트와 순서를 원문 표현으로 기록한다.
    event(name, values) {
        if (this.trace) this.trace += ';';
        this.trace += name;
        // 각 원본 인자의 폭을 보존한다.
        for (const value of values) this.trace += `:${value >>> 0}`;
    }

    // 원문 입력 13칸을 signed 변환 없이 DWORD 로 읽어 부모/TYPE/전역을 설치한다.
    prepare(text) {
        this.input = text.split(',').map(unsigned);
        this.trace = '';
        assert.equal(this.input.length, 13);
        const raw = this.raw();
        raw.fill(0);
        raw[10] = 168;
        raw[11] = this.input[6] & 0xff;
        put(raw, 12, this.input[4], 2);
        this.types[168] = { firstFrame: this.input[12] | 0, castTime: this.input[3] | 0 };
        this.types[169] = { firstFrame: 99, castTime: 1 };
        this.state.boss = this.input[5];
        this.state.localOwner = this.input[11];
        this.state.effect = { message: 0x5f, protocol: this.input[9], length: this.input[10] };
    }

    // 큰 외부 효과와 재진입 입력만 원본과 같게 연결한다.
    hooks() {
        const input = this.input;
        return {
            // QA 통지는 현재 TYPE 를 바꿀 수 있으며 프레임은 그 뒤 TYPE 를 사용해야 한다.
            notice: (sid, code, value) => {
                this.event('notice', [sid, code, value]);
                if (input[8] === 1) this.raw()[10] = 169;
            },
            // 초기 프레임 효과 뒤 권한 변경을 동일하게 공급한다.
            frame: (sid, frame, flags) => {
                this.event('frame', [sid, frame, flags]);
                put(this.raw(), 36, frame, 4);
                if (input[8] === 2) this.state.boss = 0;
            },
            // 반복 애니메이션의 현재 TYPE/권한 변경만 공급한다.
            animate: (sid, step, flags) => {
                this.event('animate', [sid, step, flags]);
                if (input[8] === 3) this.raw()[10] = 169;
                if (input[8] === 4) this.state.boss = 0;
            },
            // 실제 명시 확보 경계의 크기를 기록한다.
            allocate: (bytes) => {
                this.event('allocate', [bytes]);
                return true;
            },
            // 기대 대상을 계산하지 않고 고정 입력 목록(번호 60..)을 반환한다.
            targets: (sid, out) => {
                this.event('targets', [sid, out.length]);
                for (let index = 0; index < input[7]; index += 1) out[index] = 60 + index;
                if (input[8] === 5) {
                    this.state.effect.protocol = 3;
                    this.state.effect.length = 6;
                    this.state.localOwner = 513;
                }
                return input[7];
            },
            // 배포 전 전체 버퍼와 배포 뒤 로컬 수신의 변경을 구별한다.
            broadcast: (packet) => {
                this.event('broadcast', []);
                this.trace += `:${hex(packet)}`;
                if (input[8] === 6) {
                    this.state.localOwner = 769;
                    put(packet, 10, 222, 2);
                }
            },
            // 수신 효과의 dead 변경은 뒤의 Regular 반환에 반영되어야 한다.
            receive: (owner, connection, packet) => {
                this.event('receive', [owner, connection]);
                this.trace += `:${hex(packet)}`;
                if (input[8] === 7) this.raw()[11] = 2;
            },
            // 해제 효과의 dead 변경은 이미 캡처한 반환에 영향을 주지 않는다.
            release: (bytes) => {
                this.event('release', [bytes]);
                if (input[8] === 8) this.raw()[11] = 2;
            },
        };
    }
}

test('bomb-regular: 초기화·시전 시간 비교·대상 패킷 배포→로컬 수신→해제 순서와 재진입을 원본 741행과 대조한다', () => {
    const scene = new Scene();
    const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'bomb-regular-x86.tsv'));
    let count = 0;
    // 매 행의 독립 입력으로 반환/효과/전체 raw 를 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 7);
        scene.prepare(row[0]);
        const regular = new RawBombRegular(scene.pool, scene.types, scene.state, scene.hooks());
        const actual = floatToBits(regular.handle(50, scene.input[0], scene.input[1], bitsToFloat(scene.input[2])));
        assert.equal(actual, unsigned(row[1]), `Bomb 입력 ${row[0]} 반환`);
        assert.equal(scene.trace || '-', row[2], `Bomb 입력 ${row[0]} 효과`);
        assert.equal(hex(scene.raw()), row[3], `Bomb 입력 ${row[0]} 슬롯`);
        // 두 x87 정밀도 관측이 같은 행만 들어 있다.
        assert.equal(row[1], row[4]);
        assert.equal(row[2], row[5]);
        assert.equal(row[3], row[6]);
        count += 1;
    }
    assert.equal(count, 741);
});

test('bomb-regular: 미도달 경계는 요구하지 않고 확보 실패/누락/초과는 거부하며 예외에도 해제한다', () => {
    const scene = new Scene();
    scene.prepare('0,0,1065353216,1,0,0,0,0,0,1,10,1,17');
    // 비권한 초기화는 프레임 이외의 하위를 사용하지 않는다.
    const hooks = { frame() {} };
    const lazy = new RawBombRegular(scene.pool, scene.types, scene.state, hooks);
    assert.equal(lazy.pool, scene.pool);
    assert.equal(lazy.handle(50, 0, 0, 1.0), 1.0);
    scene.state.boss = 1;
    scene.types[168].castTime = -1;
    assert.throws(() => lazy.handle(50, 0, 0, 1.0), TypeError);
    // 확보 실패를 효과/배포 성공으로 바꾸지 않는다.
    const failed = new RawBombRegular(scene.pool, scene.types, scene.state, { ...hooks, allocate: () => false });
    assert.throws(() => failed.handle(50, 0, 0, 1.0), /확보 실패/);
    // 확보 뒤 대상 검색 누락도 해제해야 한다.
    let releases = 0;
    const withRelease = { ...hooks, allocate: () => true, release: (bytes) => { assert.equal(bytes, 400); releases += 1; } };
    assert.throws(() => new RawBombRegular(scene.pool, scene.types, scene.state, withRelease).handle(50, 0, 0, 1.0), TypeError);
    assert.equal(releases, 1);
    // 손상된 하위 반환이 210바이트 버퍼를 넘지 않도록 거부한다.
    assert.throws(() => new RawBombRegular(scene.pool, scene.types, scene.state, { ...withRelease, targets: () => 101 }).handle(50, 0, 0, 1.0), RangeError);
    assert.equal(releases, 2);
    // 구성 오류와 생성 시점 훅 사본.
    assert.throws(() => new RawBombRegular({ edition: 'original1037' }, scene.types, scene.state, {}), TypeError);
    assert.throws(() => new RawBombRegular(scene.pool, scene.types.slice(1), scene.state, {}), TypeError);
    const mutable = { frame() {} };
    const copied = new RawBombRegular(scene.pool, scene.types, scene.state, mutable);
    mutable.frame = undefined;
    scene.state.boss = 0;
    assert.equal(copied.handle(50, 0, 0, 1.0), 1.0);
});
