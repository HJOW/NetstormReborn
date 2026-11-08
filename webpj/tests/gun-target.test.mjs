// Gun 표적 raw-gun-target.js(RawGunTarget)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: guntarget-x86.tsv 10.78(originals) 1727행 — 입력 18열(검색/MayTarget/MayShoot/MayFire/CannonRay/Footprint 종류·원본 타입·방향 모드·테마·사거리 비트·후보 좌표 비트·extra·flags1/2·수량·소유자·동맹 허용·친구 발사·프레임·장면 종류·진단 출력·후보 SID)
//    → 결과 SID 또는 판정, 사정선/발자국 네 float 비트, 진단 출력 요청(D:대상여부:네 float 비트). 같은 입력을 값형 동맹 배열과 현재 Player 조회기(allyMaskRead/playerFlagsRead) 두 방식으로 재생한다. CD 10.72 행은 웹 범위 밖이다.
//  - 계약: rawGunTarget_bindings_and_missing_effect(진단 출력 경계 누락 거부·플레이어 bit20 이면 ray 진단 출력 생략). 보호 원본 SHP 자산의 48 사정선(GunTarget_ActualCannonAssetRay48)은 SHP 자산 시험 장치가 선행이라 이번 범위 밖이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E82: Gun 표적 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash, SQUID_HASH_SCALES } from '../www/core/squid-hash.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { RawGunTarget } from '../www/core/raw-gun-target.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E82)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'guntarget-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// fixture 의 같은 source Pop 좌표와 현재 중심, 판본당 독립 행 수이다. 판단 결과를 만드는 상수는 아니다. (2026-11-08 WB04-E82)
const SOURCE_X = 50.75;
const SOURCE_Y = 52.5;
const CENTER_X = 50.25;
const CENTER_Y = 52.0;
const ROWS = 1727;

// 예외가 나는지 본다. (2026-11-08 WB04-E82)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 새 Gun 전역 상태(독립 기본값)이다. (2026-11-08 WB04-E82)
function freshState() {
    return {
        friendlyFire: false, checking: false, showRanges: false,
        sunCannonType: 128, sunFenceType: 136, windArcherType: 140, thunderArcherType: 148,
        windHalfAngle: 30, scaleX: 16, scaleY: 11,
        allyMasks: new Array(9).fill(0), playerFlags: new Array(9).fill(0), angles: {}
    };
}

// 새 타입 레코드/검색 자료/SHP 의 기본값이다. (2026-11-08 WB04-E82)
const freshType = () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0, hotspotX: 0, hotspotY: 0 });
const freshTarget = () => ({ range: 0, theme: 0, directionFrames: [0, 0, 0, 0], heightExtension: 0 });
const freshShape = () => ({ loaded: false, frameCount: 0, frames: [] });

// 실제 공유 raw 풀·해시·타입/SHP 와 진단 효과 사건을 묶은 장면이다. (2026-11-08 WB04-E82)
class GunTargetScene {
    // 원본 client 할당/반납과 server/예측 Take 로 고정 native 입력 번호를 확보한다.
    constructor() {
        this.pool = new SidPool(24000);
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, freshType);
        this.targetTypes = Array.from({ length: 188 }, freshTarget);
        this.shapes = Array.from({ length: 188 }, freshShape);
        this.state = freshState();
        this.events = [];
        const first = this.pool.layout.serverFirst;
        // 마지막 free 슬롯은 원본의 예약 꼬리로 남기며 그 직전까지만 일반 할당한다.
        for (let sid = 5; sid < first - 1; sid += 1) assert.equal(this.pool.allocate(2), sid);
        // 미사용 6번을 반납해 새 예약 꼬리로 옮기면 이전 마지막 슬롯도 정상 allocate 가 가능하다.
        this.pool.release(6);
        assert.equal(this.pool.allocate(2), first - 1);
        const factory = new SquidFactory(this.pool, this.types);
        // 고정 native 번호의 서버/예측 수신은 실제 take 계약을 사용한다.
        for (const sid of [6000, 14999, 15000, 15050, 15060, 15070]) if (sid >= first) assert.equal(factory.take(86, sid), sid);
    }

    // 독립 native 입력을 실제 chain/head 에 같은 순서로 등록한다. Pop 전체를 증명하는 준비 함수가 아니다.
    object(sid, type, extra, xb, yb, width, height, extension, level) {
        const raw = this.pool.allocated(sid);
        raw.fill(0);
        raw[10] = type;
        raw[40] = extra;
        put(raw, 14, xb);
        put(raw, 18, yb);
        this.types[type].footX = width;
        this.types[type].footY = height;
        this.targetTypes[type].heightExtension = extension;
        const scale = SQUID_HASH_SCALES[level];
        const x = Math.trunc(Math.trunc(bitsToFloat(xb)) / scale);
        const y = Math.trunc(Math.trunc(bitsToFloat(yb)) / scale);
        put(raw, 4, this.hash.cell(level, x, y), 2);
        this.hash.setCell(level, x, y, sid);
    }

    // 입력 칸의 각 값만 옮기며 기대 판단은 native fixture 에서 읽는다.
    prepare(input) {
        assert.equal(input.length, 18);
        this.events = [];
        this.hash.reset();
        this.types.forEach((_, index) => { this.types[index] = freshType(); });
        this.targetTypes.forEach((_, index) => { this.targetTypes[index] = freshTarget(); });
        this.shapes.forEach((_, index) => { this.shapes[index] = freshShape(); });
        this.state = freshState();
        // native 는 매 입력의 전체 풀을 0 으로 준비하므로 비등록 후보도 이전 타입을 남기지 않는다.
        for (const sid of [60, 5999, 6000, 14999, 15000, 15050, 15060, 15070]) this.pool.allocated(sid).fill(0);
        const scene = input[15];
        const sid = input[17];
        const width = scene ? 3 : 1;
        const height = scene ? 2 : 1;
        const extension = scene === 2 ? 3 : 0;
        // scene3 만 후보 입력 순서를 뒤집어 동일 점수의 원본 첫 후보 보존을 검사한다.
        if (input[0] === 0 && scene === 3) {
            this.object(15070, 88, 0, floatToBits(50.75), floatToBits(56.5), 2, 2, 1, 2);
            this.object(15060, 87, 0, floatToBits(55.75), floatToBits(52.5), 1, 1, 0, 1);
            this.object(sid, 86, input[7], input[5], input[6], width, height, extension, 1);
        } else {
            this.object(sid, 86, input[7], input[5], input[6], width, height, extension, 1);
            if (input[0] === 0) {
                this.object(15060, 87, 0, floatToBits(53.75), floatToBits(52.5), 1, 1, 0, 1);
                this.object(15070, 88, 0, floatToBits(50.75), floatToBits(56.5), 2, 2, 1, 2);
            }
        }
        // 모든 source 타입은 같은 독립 metadata 를 받으며 특수 group 만 다르다.
        for (const number of [71, 128, 129, 140, 148, 83]) {
            const type = this.types[number];
            type.group = (number === 128 || number === 129) ? 1 : number === 83 ? 3 : 0;
            type.footX = 1;
            type.footY = 1;
            type.hotspotX = scene ? 2 : 0;
            type.hotspotY = scene ? -3 : 0;
            this.targetTypes[number].range = bitsToFloat(input[4]);
            this.targetTypes[number].theme = input[3] | 0;
            this.targetTypes[number].directionFrames = [0, 1, 2, 3];
            const shape = this.shapes[number];
            shape.loaded = true;
            shape.frameCount = 4;
            // native 가 header+20/+22 에 설치한 signed short 포구 입력을 정확한 필드로 옮긴다.
            shape.frames = Array.from({ length: 4 }, (_, index) => ({ muzzleX: scene ? 3 * index - 4 : 0, muzzleY: scene ? index - 2 : 0 }));
        }
        // 수량/owner 는 현재 raw 슬롯이고 후보 flags 는 현재 타입 표에서 다시 읽는다.
        for (const target of [sid, 15060, 15070]) {
            const raw = this.pool.allocated(target);
            raw[34] = input[11];
            put(raw, 26, input[10], 4);
            this.types[raw[10]].flags1 = input[8];
            this.types[raw[10]].flags2 = target === sid ? input[9] : target === 15060 ? 0x100000 : 0;
        }
        this.state.friendlyFire = input[13] !== 0;
        this.state.showRanges = input[16] !== 0;
        this.state.allyMasks[1] = (1 << 1) | (input[12] ? 1 << input[11] : 0);
    }

    // 원본 진단 rectangle 출력 경계만 기록하고 실제 후보/기하 결과를 대체하지 않는다.
    hooks() {
        return {
            drawRange: (rect, target) => {
                this.events.push(`D:${target ? 1 : 0}:${[rect.left, rect.top, rect.right, rect.bottom].map(floatToBits).join(',')}`);
            }
        };
    }

    // 출력 목록의 원본 순서를 세미콜론 문자열로 만든다.
    eventsText() { return this.events.length === 0 ? '-' : this.events.join(';'); }

    // 현재 장면으로 표적 모듈을 만든다.
    logic() { return new RawGunTarget(this.pool, this.hash, this.types, this.targetTypes, this.shapes, this.state, this.hooks()); }
}

// 사정선/발자국의 네 float 비트를 저장된 native 결과 형식으로 만든다. (2026-11-08 WB04-E82)
const rectBits = (rect) => [rect.left, rect.top, rect.right, rect.bottom].map(floatToBits).join(',');

// 같은 원본 입력을 값형 준비 또는 현재 Player 조회기로 재생한다. 기대값은 native fixture 를 유지한다. (2026-11-08 WB04-E82)
function replay(currentPlayers) {
    const scene = new GunTargetScene();
    let compared = 0;
    let maskReads = 0;
    let flagReads = 0;
    // 각 고유 원본 입력은 두 정밀도에서 같은 결과임이 별도 native 보고서에 기록되어 있다.
    for (const row of rows) {
        assert.equal(row.length, 5);
        const input = row[1].split(',').map(Number);
        scene.prepare(input);
        const masks = scene.state.allyMasks.slice();
        const flags = scene.state.playerFlags.slice();
        if (currentPlayers) {
            scene.state.allyMaskRead = (owner) => { maskReads += 1; return masks[owner]; };
            scene.state.playerFlagsRead = (owner) => { flagReads += 1; return flags[owner]; };
            // fallback 이 잘못 사용되면 기존 native 출력과 달라지는 반대 입력을 남긴다.
            scene.state.allyMasks.fill(0xffffffff);
            scene.state.playerFlags.fill(0x20);
        }
        const logic = scene.logic();
        const query = { x: SOURCE_X, y: SOURCE_Y, centerX: CENTER_X, centerY: CENTER_Y, type: input[1], direction: input[2] | 0, owner: 1, frame: input[14] | 0 };
        const target = input[17];
        let result = 0;
        let rect = { left: 0, top: 0, right: 0, bottom: 0 };
        switch (input[0]) {
        case 0: result = logic.findTarget(query.x, query.y, query.type, query.direction, query.owner); break;
        case 1: result = Number(logic.mayTarget(query, target)); break;
        case 2: result = Number(logic.mayShoot(query, target)); break;
        case 3: result = Number(logic.mayFire(query, target)); break;
        case 4: rect = logic.cannonRay(query, query.direction); break;
        case 5: rect = logic.footprint(target); break;
        default: throw new Error('Gun fixture kind 오류');
        }
        const label = `입력 ${row[1]}`;
        assert.equal(result, Number(row[2]), label);
        assert.equal(rectBits(rect), row[3], label);
        assert.equal(scene.eventsText(), row[4], label);
        compared += 1;
    }
    assert.equal(compared, ROWS);
    if (currentPlayers) assert.ok(maskReads > 0 && flagReads > 0);
}

test('GunTarget: 원본 10.78 1727입력의 검색 순서·후보 필터·사거리/관측각·사정선·진단 출력이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    replay(false);
});

test('GunTarget: 같은 입력을 현재 Player 조회기(동맹 마스크/참여 플래그)로 재생해도 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    replay(true);
});

test('GunTarget: 진단 출력 경계 누락을 거부하고 플레이어 bit20 이면 ray 진단 출력만 생략하며 판본/크기를 구별한다', () => {
    const scene = new GunTargetScene();
    scene.prepare([4, 128, 0, 0, floatToBits(10.5), floatToBits(55.75), floatToBits(52.5), 0, 0x10, 0, 2, 2, 0, 0, 1, 0, 1, 15050]);
    const missing = new RawGunTarget(scene.pool, scene.hash, scene.types, scene.targetTypes, scene.shapes, scene.state);
    const query = { x: SOURCE_X, y: SOURCE_Y, centerX: CENTER_X, centerY: CENTER_Y, type: 128, direction: 0, owner: 1, frame: 1 };
    assert.ok(throws(() => missing.cannonRay(query, 0)));
    const logic = scene.logic();
    assert.equal(logic.pool, scene.pool);
    // 플레이어 bit20 이면 ray 진단 출력만 생략하며 검색/판정은 그대로 실행된다.
    scene.state.playerFlags[1] = 0x20;
    scene.events = [];
    logic.cannonRay(query, 0);
    assert.equal(scene.events.length, 0);
    assert.ok(throws(() => new RawGunTarget({ edition: 'cd1072' }, scene.hash, scene.types, scene.targetTypes, scene.shapes, scene.state)));
    assert.ok(throws(() => new RawGunTarget(scene.pool, scene.hash, scene.types.slice(1), scene.targetTypes, scene.shapes, scene.state)));
    assert.ok(throws(() => logic.mayShoot({ ...query, owner: 9 }, 15050)));
});
