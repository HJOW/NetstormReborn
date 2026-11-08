// 사제 보호막 조회 raw-priest-forcefield.js(RawPriestForcefield·PriestForcefieldState·makePriestForcefieldHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestforcefield-x86.tsv 10.78(originals) 2976행 — Scene 입력(좌표·소유자·상태·extra·보호막 TYPE·등록 여부·후보 목록)에서 Find(반환 SID)와 Pre(사제 삭제 준비: 목록 압축·보호막 삭제/Carrier 호출 기록)의
//    결과·개수·목록 전체·호출 기록·일곱 슬롯 전체. CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
//  - 계약: priest_forcefield_guards_and_read_only_lookup(손상 사제/좌표/타입, 다른 풀, 타입 표 크기는 보호 예외, 조회는 읽기 전용).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E63: 사제 보호막 조회 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { PRIEST_TYPE, PRIEST_VTABLE } from '../www/core/raw-priest-owner.js';
import { PriestPostPopState } from '../www/core/raw-priest-post-pop.js';
import { RawPriestPreDestroy } from '../www/core/raw-priest-pre-destroy.js';
import { RawPriestForcefield, PriestForcefieldState, makePriestForcefieldHooks } from '../www/core/raw-priest-forcefield.js';

// 원본 관측 파일에서 Scene 입력과 10.78 Find/Pre 행을 읽는다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E63)
const file = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priestforcefield-x86.tsv');
function loadFixture() {
    if (!fs.existsSync(file)) return null;
    const scenes = new Map();
    const rows = [];
    for (const line of fs.readFileSync(file, 'utf8').split(/\r?\n/)) {
        if (line.length === 0 || line[0] === '#') continue;
        const row = line.split('\t');
        if (row[0] === 'Scene') scenes.set(row[1], row.slice(2));
        else if (row[1] === 'originals') rows.push(row);
    }
    return { scenes, rows };
}
const fixture = loadFixture();

// 합성 raw 입력 번호(50 이 사제), 판본당 관찰 행 수, 공통 가상 표이다. (2026-11-08 WB04-E63)
const IDS = [50, 60, 61, 62, 63, 64, 65];
const ROWS = 2976;
const COMMON_VTABLE = 0x11010000;

// 예외가 나는지 본다. (2026-11-08 WB04-E63)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E63)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 모든 입력 슬롯을 미리 확보하고 free 상태 표본도 이 원래 메모리 범위에서 준비하는 장면이다. (2026-11-08 WB04-E63)
function forcefieldScene() {
    const pool = new SidPool(24000, { server: false });
    for (let id = 5; id <= 65; id += 1) assert.equal(pool.allocate(2), id);
    const types = Array.from({ length: 188 }, () => ({ footX: 0, footY: 0, flags1: 0, flags2: 0 }));
    const scene = { pool, hash: new SquidHash(), types, forcefield: new PriestForcefieldState(), priests: new PriestPostPopState(), events: [] };
    // node 필드: sid/type/state/extra/x/y/footX/footY/owner/level. 판단 결과는 계산하지 않는다.
    scene.putNode = (fields, registered) => {
        assert.equal(fields.length, 10);
        const sid = Number(fields[0]);
        const raw = pool.physical(sid);
        raw.fill(0);
        const number = Number(fields[1]);
        put(raw, 0, sid === 50 ? PRIEST_VTABLE : COMMON_VTABLE);
        raw[10] = number;
        raw[11] = Number(fields[2]);
        raw[40] = Number(fields[3]);
        raw[34] = Number(fields[8]);
        put(raw, 14, Number(fields[4]));
        put(raw, 18, Number(fields[5]));
        types[number].footX = Number(fields[6]);
        types[number].footY = Number(fields[7]);
        if (registered) {
            const x = bitsToFloat(Number(fields[4]));
            const y = bitsToFloat(Number(fields[5]));
            const level = Number(fields[9]);
            put(raw, 4, scene.hash.bucket(level, x, y), 2);
            scene.hash.setBucket(level, x, y, sid);
        }
    };
    // 장면 입력만 재현한다. 보호막 검색 결과와 목록 압축은 여기서 만들지 않는다.
    scene.prepare = (fields) => {
        assert.equal(fields.length, 8);
        scene.hash.reset();
        scene.events = [];
        for (const sid of IDS) pool.physical(sid).fill(0);
        scene.putNode(['50', '158', fields[3], fields[4], fields[0], fields[1], '1', '1', fields[2], '0'], Number(fields[6]) !== 0);
        // 등록 순서만 복원하고 first SID 는 실제 finder 가 결정하게 둔다.
        for (const node of fields[7].split(';')) scene.putNode(node.split(','), true);
        scene.forcefield.type = Number(fields[5]);
        scene.priests.priests.entries = [50, 60, 50, 61, 50, 62];
        scene.priests.priests.count = 6;
    };
    // 미복원 파생 삭제 효과만 원본 실행기와 같은 인자로 기록한다.
    scene.record = (marker, sid, flags) => scene.events.push(`${marker}:${sid}:${flags}`);
    scene.hooks = () => ({
        destroyForcefield: (sid, flags) => scene.record('D', sid, flags),
        validateCarrier: () => {},
        carrierPre: (sid, flags) => scene.record('C', sid, flags)
    });
    scene.entries = () => scene.priests.priests.entries.join(',');
    scene.slots = () => IDS.map((sid) => `${sid}:${hex(pool.slot(sid))}`).join(';');
    return scene;
}

test('PriestForcefield: 원본 10.78 2976행의 조회·사제 삭제 준비 결과·목록·호출 기록·슬롯 전체가 같다', { skip: !fixture && '원본 fixture 없음' }, () => {
    const scene = forcefieldScene();
    let count = 0;
    // 각 행을 독립적으로 초기화한다. 조회 객체에는 입력 타입 발자국을 복사한다.
    for (const row of fixture.rows) {
        assert.equal(row.length, 9);
        scene.prepare(fixture.scenes.get(row[2]));
        const lookup = new RawPriestForcefield(scene.pool, scene.hash, scene.types, scene.forcefield);
        if (row[0] === 'Find') assert.equal(lookup.find(50), Number(row[4]), `행 ${count}`);
        else new RawPriestPreDestroy(scene.pool, scene.priests, makePriestForcefieldHooks(scene.pool, lookup, scene.hooks())).preDestroy(50, Number(row[3]));
        assert.equal(scene.priests.priests.count, Number(row[5]), `행 ${count}`);
        assert.equal(scene.entries(), row[6], `행 ${count}`);
        assert.equal(scene.events.length === 0 ? '-' : scene.events.join(';'), row[7], `행 ${count}`);
        assert.equal(scene.slots(), row[8], `행 ${count}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('PriestForcefield: 손상 사제/좌표/타입 및 다른 풀은 보호 예외로 거부하고 조회는 읽기 전용이다', { skip: !fixture && '원본 fixture 없음' }, () => {
    const scene = forcefieldScene();
    scene.prepare(fixture.scenes.values().next().value);
    const lookup = new RawPriestForcefield(scene.pool, scene.hash, scene.types, scene.forcefield);
    const before = scene.slots();
    const list = scene.entries();
    assert.ok(throws(() => lookup.find(0)) && throws(() => lookup.find(24000)));
    assert.ok(scene.slots() === before && scene.entries() === list);
    const raw = scene.pool.physical(50);
    raw[11] = 1;
    assert.ok(throws(() => lookup.find(50)));
    raw[11] = 8;
    assert.ok(throws(() => lookup.find(50)));
    raw[11] = 0;
    put(raw, 0, COMMON_VTABLE);
    assert.ok(throws(() => lookup.find(50)));
    put(raw, 0, PRIEST_VTABLE);
    const old = get(raw, 14);
    // 유한한 signed int 범위를 넘는 입력은 임의 포인터/정수 감김으로 복제하지 않는다.
    for (const coordinate of [Number.NaN, Number.POSITIVE_INFINITY, 2147483648]) {
        put(raw, 14, floatToBits(coordinate));
        assert.ok(throws(() => lookup.find(50)));
    }
    put(raw, 14, old);
    assert.ok(throws(() => makePriestForcefieldHooks(new SidPool(24000, { server: false }), lookup, scene.hooks())));
    assert.ok(throws(() => new RawPriestForcefield(scene.pool, scene.hash, scene.types.slice(0, 100), scene.forcefield)));
    scene.forcefield.type = 256;
    assert.ok(lookup.find(50) === 0 && scene.events.length === 0);
    assert.equal(PRIEST_TYPE, 158);
});
