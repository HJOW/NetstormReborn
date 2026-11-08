// 스톤 타워 표적 통지 raw-gun-blocker.js(RawGunBlocker)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: gun-blocker-x86.tsv 690행 — 입력 18열(수집/통지 종류·group 모드·총 flags/extra·동맹·친구 발사·총 방향 오프셋·현재 표적 없음 마스크·기존 표적 SID·추상 플래그·용량·역순·타워 좌표 비트·타워 발자국·확장 높이·boss)
//    → 수집 목록(쉼표 SID, 중복 보존)과 기존 Gun 으로 보낸 통지 순서(`N:총:타워`), gun-blocker-boundary-x86.tsv 6행(최대 반경 20 의 아래/동일/위). 모든 원본 일반 finder 순서·엄격한 점수 비교·목록 용량을 실제 raw 월드에서 재생한다.
//  - 계약: GunBlocker_AiCallerCapacityThirtyKeepsNativeOrder(AI 의 30개 목록에서도 포화 전 후보 순서/내용이 같음).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E87: 스톤 타워 표적 통지 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { RawGunTarget } from '../www/core/raw-gun-target.js';
import { RawGunBlocker } from '../www/core/raw-gun-blocker.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E87)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const rows = readRows(path.join(fixtures, 'gun-blocker-x86.tsv'));
const boundaryRows = readRows(path.join(fixtures, 'gun-blocker-boundary-x86.tsv'));

// 독립 입력의 기존 총 번호, 네 캐논 ray 와 가까운 발사기의 원점, 총 방향 기본값, 독립 행 수이다. (2026-11-08 WB04-E87)
const GUNS = Object.freeze([15050, 15051, 15052, 15053, 15054]);
const POSITIONS = Object.freeze([[49, 20], [49, 80], [20, 49], [80, 49], [49, 45]]);
const MODES = Object.freeze([4, 0, 2, 6, 0]);
const ROWS = 690;

const freshType = () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0, hotspotX: 0, hotspotY: 0 });
const freshTarget = () => ({ range: 0, theme: 0, directionFrames: [0, 0, 0, 0], heightExtension: 0 });
const freshShape = () => ({ loaded: false, frameCount: 0, frames: [] });

// 원본 입력의 같은 raw 슬롯·공간 해시·타입과 외부 기존 Gun 자료이다. (2026-11-08 WB04-E87)
class GunBlockerScene {
    // 고정 서버 입력을 정상 take 로 준비하며 임의 SID 를 client 번호로 바꾸지 않는다.
    constructor() {
        this.pool = new SidPool(24000);
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, freshType);
        this.targetTypes = Array.from({ length: 188 }, freshTarget);
        this.shapes = Array.from({ length: 188 }, freshShape);
        this.targetingState = { friendlyFire: false, checking: false, showRanges: false, sunCannonType: 128, sunFenceType: 136, windArcherType: 140, thunderArcherType: 148, windHalfAngle: 30, scaleX: 16, scaleY: 11, allyMasks: new Array(9).fill(0), playerFlags: new Array(9).fill(0), angles: {} };
        this.process = { boss: true, friendlyFire: false, allies: Array.from({ length: 9 }, () => new Array(9).fill(false)) };
        this.state = { cannonRange: 70, archerRange: 20, stoneTowerType: 78 };
        this.input = [];
        this.notices = [];
        const factory = new SquidFactory(this.pool, this.types);
        // 총/현재 표적/새 타워의 원본 번호를 한 번 확보한다.
        for (const sid of [15050, 15051, 15052, 15053, 15054, 15060, 15061, 15070]) assert.equal(factory.take(100, sid), sid);
    }

    // free/dead 독립 입력도 쓰도록 상태와 무관한 물리 슬롯을 빌리며, 원본 준비와 동일한 TYPE/슬롯/버킷 head 삽입이고 제품 Pop 전체를 대체하는 증거로 세지 않는다.
    object(sid, type, flags, extra, x, y, width, height, extension) {
        const raw = this.pool.physical(sid);
        raw.fill(0);
        raw[10] = type;
        raw[11] = flags;
        raw[40] = extra;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        this.types[type].footX = width;
        this.types[type].footY = height;
        this.targetTypes[type].heightExtension = extension;
        const cx = Math.trunc(Math.trunc(x) / 2);
        const cy = Math.trunc(Math.trunc(y) / 2);
        put(raw, 4, this.hash.cell(1, cx, cy), 2);
        this.hash.setCell(1, cx, cy, sid);
    }

    // 한 원본 입력을 준비하며 이전 행의 어떤 판단 결과도 빌리지 않는다.
    prepare(encoded) {
        this.input = encoded.split(',').map(Number);
        assert.equal(this.input.length, 18);
        this.notices = [];
        this.hash.reset();
        this.types = Array.from({ length: 188 }, freshType);
        this.targetTypes = Array.from({ length: 188 }, freshTarget);
        this.process = { boss: this.input[17] !== 0, friendlyFire: this.input[5] !== 0, allies: Array.from({ length: 9 }, () => new Array(9).fill(false)) };
        this.process.allies[1][2] = this.input[4] !== 0;
        this.pool.physical(0).fill(0);
        const input = this.input;
        // 원본 입력 순서의 역순도 같은 버킷에 머리 삽입한다.
        for (let order = 0; order < GUNS.length; order += 1) {
            const index = input[11] ? GUNS.length - 1 - order : order;
            const type = 100 + index;
            this.object(GUNS[index], type, input[2], input[3], POSITIONS[index][0], POSITIONS[index][1], 1, 1, 0);
            this.pool.physical(GUNS[index])[34] = 2;
            this.types[type].group = input[1] < 2 ? input[1] : index === 4 ? 0 : 1;
        }
        this.object(15060, 90, 0, 0, 49, 150, 2, 2, input[16] | 0);
        this.object(15061, 91, 0, 0, 48, 48, 2, 2, input[16] | 0);
        this.object(15070, 78, 0, 0, bitsToFloat(input[12]), bitsToFloat(input[13]), input[14] | 0, input[15] | 0, 0);
        this.pool.physical(15070)[34] = 1;
        this.types[90].flags2 = input[9] ? 0x100000 : 0;
        this.types[91].flags2 = this.types[90].flags2;
    }

    // 원본 실행기의 외부 기존 Gun/방향/포구/통지만 같은 독립 자료로 공급한다.
    hooks() {
        return {
            currentTarget: (sid) => {
                const index = GUNS.indexOf(sid);
                if (index < 0) return undefined;
                if (this.input[7] & (1 << index)) return undefined;
                return this.input[8];
            },
            mode: (sid) => {
                const index = GUNS.indexOf(sid);
                return index < 0 ? 0 : (MODES[index] + this.input[6]) % 8;
            },
            muzzle: (sid) => {
                const raw = this.pool.slot(sid);
                return [bitsToFloat(get(raw, 14)), bitsToFloat(get(raw, 18))];
            },
            setTarget: (gun, blocker) => { this.notices.push(`N:${gun}:${blocker}`); }
        };
    }

    // 실제 제품 수집/통지를 실행하고 원본 결과 문자열과 같은 형식으로 반환한다.
    run() {
        const target = new RawGunTarget(this.pool, this.hash, this.types, this.targetTypes, this.shapes, this.targetingState);
        const logic = new RawGunBlocker(this.pool, this.hash, this.types, this.targetTypes, target, this.process, this.state, this.hooks());
        let list = '';
        if (this.input[0]) logic.notify(15070);
        else list = logic.collect(1, [bitsToFloat(this.input[12]), bitsToFloat(this.input[13])], this.input[10]).join(',');
        const events = this.notices.join(';');
        return [list === '' ? '-' : list, events === '' ? '-' : events];
    }
}

test('GunBlocker: 원본 10.78 690입력의 네 방향·거리 비교·동맹/추상·빈 현재 표적·목록 포화와 통지 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new GunBlockerScene();
    let count = 0;
    let positive = 0;
    // 기대값을 재계산하지 않고 원본 두 출력과 매 행을 직접 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 3);
        scene.prepare(row[0]);
        const actual = scene.run();
        assert.deepEqual(actual, [row[1], row[2]], `입력 ${row[0]}`);
        count += 1;
        if (actual[0] !== '-' || actual[1] !== '-') positive += 1;
    }
    assert.ok(count === ROWS && positive > 100);
});

test('GunBlocker: AI 호출자의 30개 목록에서도 포화 전 후보 순서/내용이 원본과 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new GunBlockerScene();
    let checked = false;
    // 원본 20개 한도에 못 미치는 다섯 Gun 장면에서 비어 있지 않은 수집 입력을 찾는다.
    for (const row of rows) {
        scene.prepare(row[0]);
        if (scene.input[0] || scene.input[10] !== 20 || row[1] === '-') continue;
        scene.input[10] = 30;
        assert.deepEqual(scene.run(), [row[1], row[2]]);
        checked = true;
        break;
    }
    assert.ok(checked);
});

test('GunBlocker: 최대 반경 20 에 정확히 닿는 후보는 포함하고 그보다 먼 후보는 제외하는 원본 경계 6행이 같다', { skip: !boundaryRows && '원본 fixture 없음' }, () => {
    const scene = new GunBlockerScene();
    let count = 0;
    // 아래/동일/위의 실제 원본 결과만 읽고 제품 구현으로 기대값을 만들지 않는다.
    for (const row of boundaryRows) {
        assert.equal(row.length, 3);
        scene.prepare(row[0]);
        assert.deepEqual(scene.run(), [row[1], row[2]]);
        count += 1;
    }
    assert.equal(count, 6);
});

test('GunBlocker: boss 가 아니면 통지하지 않고 필수 경계 누락과 판본/풀 혼합을 거부한다', () => {
    const scene = new GunBlockerScene();
    scene.prepare('1,0,0,0,0,0,0,0,15060,0,20,0,1111883776,1115865088,3,3,0,1');
    scene.process.boss = false;
    assert.deepEqual(scene.run(), ['-', '-']);
    const target = new RawGunTarget(scene.pool, scene.hash, scene.types, scene.targetTypes, scene.shapes, scene.targetingState);
    const logic = new RawGunBlocker(scene.pool, scene.hash, scene.types, scene.targetTypes, target, scene.process, scene.state, {});
    assert.throws(() => logic.collect(1, [49, 49]));
    scene.process.boss = true;
    assert.throws(() => logic.notify(15070));
    assert.throws(() => new RawGunBlocker({ edition: 'cd1072' }, scene.hash, scene.types, scene.targetTypes, target, scene.process, scene.state, {}));
    assert.throws(() => new RawGunBlocker(scene.pool, scene.hash, scene.types, scene.targetTypes.slice(1), target, scene.process, scene.state, {}));
    assert.throws(() => logic.collect(9, [49, 49]));
});
