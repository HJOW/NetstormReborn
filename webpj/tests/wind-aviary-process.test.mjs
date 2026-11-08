// 데빌메이커 생산 프로세스 raw-wind-aviary-process.js(RawWindAviaryProcess·WindAviaryProcess)와 원주 위치 raw-wind-circle.js(windCirclePosition)를 실제 PE 관측 fixture 로 대조한다.
//  - wind-aviary-frame-x86.tsv 1,639행: F 1,607행(상태 1/2/5/7/11/12/13·실제 4/6/26초 경과·가감속·서버/boss/편집/표시 유무의 전체 80바이트·세 슬롯·효과 순서)과 P 32행(후처리: 편집/친군 사격/서버/최초 비트 조합). 열 = 종류, 입력, 호출 뒤 80바이트 hex, 슬롯 50/60/70 hex, 효과 trace.
//  - wind-circle-frame-x86.tsv 288행: 실제 중심/원주/헤더를 실행한 상태 12 의 전체 80바이트/세 슬롯/Pop 좌표(헤더 callback 이 본체/부모를 바꾸는 재진입 포함). wind-aviary-destroy-x86.tsv 880행: 45c090 의 Form 삭제(정상 슬롯/주소 끝/bit31 별칭/진단 재진입과 사용하지 않는 flags).
//  - 각 행은 입력만 설치하고 모듈이 계산한다. 이전 행의 메모리가 섞이지 않도록 매 행 새 장면을 만든다. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E185: 데빌메이커 프로세스 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { WindAviaryProcessFields } from '../www/core/raw-wind-aviary-state.js';
import { windCirclePosition } from '../www/core/raw-wind-circle.js';
import { RawWindAviaryProcess, WindAviaryProcess, findAttachedProcess } from '../www/core/raw-wind-aviary-process.js';

// 실제 PE 관측 fixture 이다. (2026-11-09 WB04-E185)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const frameRows = (readRows(path.join(FIXTURES, 'wind-aviary-frame-x86.tsv')) ?? []).filter((row) => row[0] === 'F' || row[0] === 'P');
const circleRows = readRows(path.join(FIXTURES, 'wind-circle-frame-x86.tsv')) ?? [];
const destroyRows = readRows(path.join(FIXTURES, 'wind-aviary-destroy-x86.tsv')) ?? [];
// 부모/임시 플라이어/Form 의 원본 순서이다. (2026-11-09 WB04-E185)
const IDS = [50, 60, 70];

// 예외가 나는지 본다. (2026-11-09 WB04-E185)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E185)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원문과 같은 입력/명시 효과만 준비하며 결과 계산은 실제 제품 Frame 에 맡긴다. (2026-11-09 WB04-E185)
class WindScene {
    // 독립 원문과 같은 현재 입력만 설치한다.
    constructor(kind, input) {
        this.pool = new SidPool(24000, { server: false });
        this.fields = new WindAviaryProcessFields();
        this.fort = false;
        this.friendly = 0;
        this.server = true;
        this.checking = 0;
        this.boss = true;
        this.editor = 0;
        this.processType = 59;
        this.flyerType = 138;
        this.aviaryType = 139;
        this.now = 100;
        this.trace = [];
        this.fullCircle = false;
        this.circleMutation = 0;
        this.circleHeader = { walkStepY: 0, height: 0, hotspotY: 0 };
        this.circleTypes = Array.from({ length: 188 }, () => ({ footX: 0, footY: 0 }));
        // 세 실제 슬롯까지 확보한다.
        for (;;) {
            const sid = this.pool.allocate(2);
            if (sid === 70) break;
            if (!sid || sid > 70) throw new Error('Wind SID 입력 준비 실패');
        }
        // 세 슬롯의 전체 50바이트를 원문 입력 그대로 설치한다.
        for (const id of IDS) {
            const raw = this.pool.allocated(id);
            raw.fill(0xa5);
            put(raw, 0, 0x1240f000);
            put(raw, 4, 0, 2);
            put(raw, 6, id === 50 ? 70 : 0, 2);
            put(raw, 8, 0, 2);
            raw[10] = id === 50 ? 139 : id === 60 ? 138 : 59;
            raw[11] = 0;
            put(raw, 12, 0, 2);
            put(raw, 14, floatToBits(id + 0.25));
            put(raw, 18, floatToBits(id + 0.5));
            raw[34] = 1;
            raw[40] = 0;
            put(raw, 36, 0);
        }
        this.fields.bytes.fill(0xa5);
        put(this.fields.bytes, 0, 0x509dfc);
        this.fields.bytes[8] = 0x10;
        this.double(16, 100);
        put(this.fields.bytes, 24, 1);
        this.double(32, 99);
        this.double(40, 100);
        put(this.fields.bytes, 48, 0);
        put(this.fields.bytes, 56, 0x509ddc);
        put(this.fields.bytes, 60, 77);
        put(this.fields.bytes, 64, 50);
        put(this.fields.bytes, 68, 4);
        put(this.fields.bytes, 72, 2);
        if (kind === 'F') {
            put(this.fields.bytes, 24, Number(input[0]));
            this.double(40, 100 - Number(input[1]));
            this.double(32, Number(input[2]));
            put(this.fields.bytes, 48, Number(input[3]));
            this.server = Number(input[4]) !== 0;
            this.boss = Number(input[5]) !== 0;
            this.pool.allocated(50)[10] = Number(input[6]) & 0xff;
            this.editor = Number(input[7]);
            this.pool.allocated(50)[40] = Number(input[8]) & 0xff;
            put(this.pool.allocated(50), 36, Number(input[9]) >>> 0);
        } else {
            this.server = Number(input[0]) !== 0;
            this.fort = Number(input[1]) !== 0;
            this.friendly = Number(input[2]);
            this.fields.bytes.fill(0xa5);
            put(this.pool.allocated(50), 6, 0, 2);
        }
        this.logic = new RawWindAviaryProcess(this.pool, {
            fort: () => this.fort, friendlyFire: () => this.friendly, server: () => this.server, boss: () => this.boss, editor: () => this.editor,
            checking: () => this.checking, now: () => this.now, processType: () => this.processType, flyerType: () => this.flyerType, aviaryType: () => this.aviaryType
        }, this.hooks());
    }

    // 원문 double 의 비트 입력이다. (2026-11-09 WB04-E185)
    double(at, value) {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, value, true);
        put(this.fields.bytes, at, view.getUint32(0, true));
        put(this.fields.bytes, at + 4, view.getUint32(4, true));
    }

    // 원본 호출 순서를 보존한다. (2026-11-09 WB04-E185)
    event(value) { this.trace.push(value); }

    // 명시 큰 효과는 원문 입력의 정해진 결과만 돌려준다. (2026-11-09 WB04-E185)
    hooks() {
        return {
            findProcess: (sid, type) => findAttachedProcess(this.pool, sid, type),
            allocate: () => { this.event('Allocate,80'); const f = new WindAviaryProcessFields(); f.bytes.fill(0xa5); return f; },
            attach: (f, parent, type) => { this.event(`Attach,${type},${parent},8`); this.fields = f; },
            damageable: (sid, flags) => { this.event(`Base,${flags}`); },
            assertion: (line) => { this.event(`Assert,${line}`); },
            setFrame: (sid, frame) => { this.event(`Frame,${sid},${frame}`); put(this.pool.allocated(sid), 36, frame >>> 0); },
            create: (type) => { this.event(`Create,${type}`); return 60; },
            setOwner: (sid, owner) => {
                this.event(`Owner,${owner}`);
                this.pool.allocated(sid)[34] = owner & 0xff;
                if (this.fullCircle && this.circleMutation === 1) this.pool.allocated(sid)[10] = 139;
                if (this.fullCircle && this.circleMutation === 2) this.pool.allocated(50)[10] = 139;
            },
            height: () => { this.event('Height'); return 44.0; },
            footY: () => 3.0,
            pop: (sid, x, y) => { this.event(`Pop,${floatToBits(x)},${floatToBits(y)}`); },
            createFlyerProcess: (sid, parent) => { this.event('Allocate,104'); this.event(`Process,${sid},${parent},1082130432,1056964608`); },
            bindFlyer: (sid, home) => { this.event(`Bind,${sid},${home}`); },
            circlePosition: (parent, flyer, current) => {
                if (this.fullCircle) {
                    return windCirclePosition(this.pool, this.circleTypes, parent, flyer, current, (sid, abstract, frame) => {
                        this.event(`Header,${this.pool.slot(sid)[10]},${abstract},${frame}`);
                        if (this.circleMutation === 3) {
                            put(this.fields.bytes, 68, 7);
                            put(this.fields.bytes, 72, 1);
                            put(this.fields.bytes, 64, 51);
                            put(this.pool.allocated(50), 14, floatToBits(61.75));
                            put(this.pool.allocated(50), 18, floatToBits(62.5));
                        }
                        return this.circleHeader;
                    });
                }
                const radius = get(current.bytes, 68) | 0;
                const index = get(current.bytes, 72) | 0;
                this.event('SpecialHeight');
                this.event('Center');
                this.event(`Circle,${radius},${index}`);
                return [Math.fround(61.25 - 1.399999976158142), Math.fround(44.0 * 0.09090909361839294 - 3.0 + 62.5)];
            },
            finish: () => { this.event('Finish,0'); }
        };
    }

    // 세 슬롯 전체의 원본 순서이다. (2026-11-09 WB04-E185)
    objects() { return IDS.map((sid) => hex(this.pool.slot(sid))).join(''); }
}

// 상태마다 독립 입력을 재생하여 이전 행의 메모리가 섞이지 않게 한다. (2026-11-09 WB04-E185)
function compareWind(kind, expectedCount) {
    let seen = 0;
    // 해당 범위의 원본 행만 순서대로 재생한다.
    for (const row of frameRows) {
        if (row[0] !== kind) continue;
        const input = row[1].split(',');
        const scene = new WindScene(kind, input);
        seen += 1;
        if (kind === 'F') scene.logic.frame(scene.fields);
        else scene.logic.postPop(50, Number(input[3]));
        const trace = scene.trace.length ? scene.trace.join(';') : '-';
        assert.equal(hex(scene.fields.bytes), row[2], `Wind ${kind} 80바이트 ${row[1]}`);
        assert.equal(scene.objects(), row[3], `Wind ${kind} 슬롯 ${row[1]}`);
        assert.equal(trace, row[4], `Wind ${kind} 효과 ${row[1]}`);
    }
    assert.equal(seen, expectedCount);
}

test('데빌메이커 프레임: 실제 4/6/26초와 전체 상태·가감속·표시 유무 1,607행의 80바이트/세 슬롯/효과 순서를 실제 PE 와 대조한다', () => { compareWind('F', 1607); });
test('데빌메이커 후처리: 편집/친군 사격/서버/최초 비트 조합 32행의 80바이트/세 슬롯/효과 순서를 실제 PE 와 대조한다', () => { compareWind('P', 32); });

test('데빌메이커 원주 프레임: 실제 중심/원주/헤더를 실행한 상태 12 의 288행 전체 80바이트/세 슬롯/Pop 좌표를 대조한다', () => {
    assert.equal(circleRows.length, 288);
    circleRows.forEach((row, index) => {
        const a = row[0].split(',');
        const scene = new WindScene('F', '12,0,99,0,1,1,133,0,0,0'.split(','));
        scene.fullCircle = true;
        scene.circleMutation = Number(a[9]);
        put(scene.fields.bytes, 68, Number(a[0]));
        put(scene.fields.bytes, 72, Number(a[1]));
        put(scene.pool.allocated(50), 14, Number(a[2]));
        put(scene.pool.allocated(50), 18, Number(a[3]));
        scene.pool.allocated(60)[40] = Number(a[10]) & 0xff;
        scene.circleTypes[133].footX = Number(a[4]) | 0;
        scene.circleTypes[133].footY = Number(a[5]) | 0;
        scene.circleTypes[139].footX = (Number(a[4]) | 0) + 2;
        scene.circleTypes[139].footY = (Number(a[5]) | 0) + 1;
        scene.circleHeader = { walkStepY: bitsToFloat(Number(a[6])), height: Number(a[7]) | 0, hotspotY: Number(a[8]) | 0 };
        scene.logic.frame(scene.fields);
        assert.equal(hex(scene.fields.bytes), row[1], `원주 80바이트 행 ${index}`);
        assert.equal(scene.objects(), row[2], `원주 슬롯 행 ${index}`);
        assert.equal(scene.trace.join(';'), row[3], `원주 효과 행 ${index}`);
    });
});

test('데빌메이커 Form 삭제: 정상 슬롯/주소 끝/bit31 별칭/진단 재진입과 사용하지 않는 flags 880행을 실제 PE 와 대조한다', () => {
    assert.equal(destroyRows.length, 880);
    destroyRows.forEach((row, index) => {
        assert.equal(row.length, 4);
        const a = row[0].split(',').map((value) => Number(value) >>> 0);
        assert.equal(a.length, 7);
        const pool = new SidPool(24000, { server: false });
        const fields = new WindAviaryProcessFields();
        fields.bytes.fill(0xa5);
        put(fields.bytes, 48, a[1]);
        // 예약 1 과 free 입력을 원문대로 보존하는 네 슬롯(첫/정상/중간/마지막 주소 순서)의 명시 50바이트이다.
        const ids = [1, 60, 127, 23999];
        for (const id of ids) {
            const raw = pool.physical(id);
            raw.fill(0);
            put(raw, 0, 0x12200000);
            raw[10] = a[3] & 0xff;
            raw[11] = a[2] & 0xff;
            raw[34] = 2;
        }
        let boss = a[0] !== 0;
        const effects = [];
        const event = (name, values) => { effects.push([name, ...values].join(':')); };
        // 원문 진단 경계가 바꾼 현재 상태를 이후 코드가 어떻게 읽는지 대조한다.
        const hooks = {
            assertion: (line) => {
                event('A', [line]);
                if (a[6] === 1) { put(fields.bytes, 48, 0); pool.physical(60)[11] = 2; }
                if (a[6] === 2) put(pool.physical(60), 0, 0x12200100);
                if (a[6] === 3) boss = false;
            },
            destroy: (sid, flags) => {
                event(get(pool.slot(sid), 0) === 0x12200100 ? 'K2' : 'K', [sid, flags]);
                pool.physical(sid)[11] |= 3;
            }
        };
        const logic = new RawWindAviaryProcess(pool, {
            fort: () => false, friendlyFire: () => 0, server: () => true, boss: () => boss, editor: () => 0, checking: () => 0, now: () => 0, processType: () => 59, flyerType: () => a[4], aviaryType: () => 139
        }, hooks);
        logic.formDestroy(fields);
        const raws = ids.map((id) => hex(pool.slot(id))).join('');
        assert.equal(effects.length ? effects.join(';') : '-', row[1], `삭제 효과 행 ${index} ${row[0]}`);
        assert.equal(hex(fields.bytes), row[2], `삭제 80바이트 행 ${index}`);
        assert.equal(raws, row[3], `삭제 슬롯 행 ${index}`);
    });
});

test('데빌메이커 프로세스 래퍼: 프레임 전달·부착 필드 기록·빈 통지·0x74 writer 와 후속 정리를 확인한다', () => {
    const frames = [];
    const destroyed = [];
    const logic = { frame: (f) => frames.push(f), formDestroy: (f) => destroyed.push(f) };
    const fields = new WindAviaryProcessFields();
    fields.bytes.fill(0xa5);
    const process = new WindAviaryProcess(logic, fields);
    process.runFrame();
    process.onFormDestroy(123);
    process.notify(1, 2, 3);
    assert.ok(frames.length === 1 && frames[0] === fields && destroyed.length === 1 && process.fields === fields);
    // 부착 전 전송도 같은 Host 필드(Form/부모 0)를 기록한다.
    const packet = new Uint8Array(64);
    const length = process.writePacket(packet, true, true);
    assert.ok(length >= 9 && packet[0] === 1 && packet[1] === 0x74 && get(fields.bytes, 60) === 0 && get(fields.bytes, 64) === 0);
    assert.equal(process.writePacket(new Uint8Array(64), false, false, 2, 0x75) >= 9, true);
});

test('데빌메이커/원주/프로세스 탐색: PATCH 전용·현재 전역 조회기·원주 경계·체인 오류를 거부한다', () => {
    const pool = new SidPool(24000, { server: false });
    const globals = { fort: () => false, friendlyFire: () => 0, server: () => true, boss: () => true, editor: () => 0, checking: () => 0, now: () => 0, processType: () => 59, flyerType: () => 138, aviaryType: () => 139 };
    assert.ok(throws(() => new RawWindAviaryProcess({}, globals, {})));
    for (const name of Object.keys(globals)) assert.ok(throws(() => new RawWindAviaryProcess(pool, { ...globals, [name]: undefined }, {})));
    const fields = new WindAviaryProcessFields();
    assert.ok(throws(() => windCirclePosition(pool, [], 50, 60, fields, () => ({}))));
    assert.ok(throws(() => windCirclePosition(pool, Array.from({ length: 188 }, () => ({ footX: 0, footY: 0 })), 50, 60, fields, undefined)));
    // 선택된 하위 누락은 도달한 위치에서 거부한다.
    const logic = new RawWindAviaryProcess(pool, globals, {});
    assert.ok(throws(() => logic.postPop(50, 1)));
    assert.ok(throws(() => logic.flyerDied(fields, undefined)));
    // 손상된 순환 체인은 풀 크기로 제한해 거부한다.
    const first = pool.allocate(2);
    pool.allocated(first).fill(0);
    put(pool.allocated(first), 6, first, 2);
    put(pool.allocated(first), 4, first, 2);
    assert.ok(throws(() => findAttachedProcess(pool, first, 999)));
    assert.equal(findAttachedProcess(pool, 1, 59), 0);
});
