// 게임 보조 전역(WB04-C2)을 검사한다.
//  - 초기값: 보호 원본 originals/Netstorm.exe(10.78) 의 PE 섹션 표로 각 원본 주소를 파일 위치로 바꿔 .data 초기값/BSS(0)을 직접 읽어 대조한다.
//    파일을 읽기만 하며 실행하지 않는다. 원본이 없는 환경에서는 이 대조만 건너뛴다.
//  - TYPE 번호 칸: 웹 타입 표(TYPE_LOAD_ORDER·PROCESS_TYPE_NAMES·내장 Form 이름)의 같은 번호 이름과 맞는지 본다.
//  - 폭 검사·자원 비트·일반 월드 준비/정리 되돌리기·장식 배열 대여·인스턴스 독립.
// (2026-11-05 WB04-C2: 세션 장기 상태의 현재 TYPE/모드 전역 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { GAME_AUX_FIELDS, GameAuxState, WORLD_DECORATION_FIELDS } from '../www/core/game-aux-state.js';
import { FIRST_ASSET_TYPE_NUMBER, TYPE_LOAD_ORDER } from '../www/assets/type-table.js';
import { PROCESS_TYPE_NAMES } from '../www/assets/type-flags.js';

// 보호 원본 실행 파일(10.78) 경로이다. (2026-11-05 WB04-C2)
const ORIGINAL_EXE = path.join(REPO_ROOT, 'originals', 'Netstorm.exe');

// 원본 10.78 실행 파일의 SHA-256 이다(현재 저장소 보호 원본). 다른 판본으로 대조하지 않도록 확인한다. (2026-11-05 WB04-C2)
const ORIGINAL_EXE_SHA256 = 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236';

// PE 파일에서 가상 주소의 초기 DWORD 를 읽는 함수를 만든다. 섹션의 파일 자료 밖(초기화되지 않은 BSS)은 0 이다. 어느 섹션에도 없으면 오류이다. (2026-11-05 WB04-C2)
function peReader(bytes) {
    const pe = bytes.readUInt32LE(0x3c);
    assert.equal(bytes.toString('latin1', pe, pe + 4), 'PE\0\0');
    const count = bytes.readUInt16LE(pe + 6);
    const optionalSize = bytes.readUInt16LE(pe + 20);
    const imageBase = bytes.readUInt32LE(pe + 24 + 28);
    const table = pe + 24 + optionalSize;
    // 섹션 표를 읽는다.
    const sections = Array.from({ length: count }, (_, index) => {
        const offset = table + index * 40;
        return { name: bytes.toString('latin1', offset, offset + 8).replace(/\0+$/, ''), virtualSize: bytes.readUInt32LE(offset + 8), virtualAddress: bytes.readUInt32LE(offset + 12), rawSize: bytes.readUInt32LE(offset + 16), rawOffset: bytes.readUInt32LE(offset + 20) };
    });
    return (address, width = 4) => {
        const relative = address - imageBase;
        const section = sections.find((entry) => relative >= entry.virtualAddress && relative < entry.virtualAddress + Math.max(entry.virtualSize, entry.rawSize));
        if (!section) throw new RangeError(`섹션 밖 주소 ${address.toString(16)}`);
        const inside = relative - section.virtualAddress;
        if (inside + width > section.rawSize) return { section: section.name, bytes: Buffer.alloc(width) };
        return { section: section.name, bytes: bytes.subarray(section.rawOffset + inside, section.rawOffset + inside + width) };
    };
}

// 바이트의 SHA-256 16진 문자열이다. (2026-11-05 WB04-C2)
function sha256(bytes) {
    return crypto.createHash('sha256').update(bytes).digest('hex');
}

// 필드 폭의 바이트 수이다. (2026-11-05 WB04-C2)
const WIDTH = Object.freeze({ u32: 4, i32: 4, f32: 4, f64: 8, bool: 1 });

// 바이트를 필드 폭에 맞춰 값으로 해석한다. (2026-11-05 WB04-C2)
function decode(kind, raw) {
    if (kind === 'u32') return raw.readUInt32LE(0);
    if (kind === 'i32') return raw.readInt32LE(0);
    if (kind === 'f32') return raw.readFloatLE(0);
    if (kind === 'f64') return raw.readDoubleLE(0);
    return raw[0] !== 0;
}

// 초기값: 모든 필드와 장식 네 칸이 원본 PE 의 .data 초기값/BSS 0 과 같고, 새 객체의 값도 같다.
test('GameAuxState: 초기값이 원본10.78 PE 의 .data/BSS 와 같다', (t) => {
    const aux = new GameAuxState();
    // 새 객체의 값이 표의 초기값이다.
    for (const field of GAME_AUX_FIELDS) assert.equal(aux[field.name], field.initial, field.name);
    assert.deepEqual([...aux.worldDecorationTypes], WORLD_DECORATION_FIELDS.map((field) => field.initial));
    if (!fs.existsSync(ORIGINAL_EXE)) {
        t.diagnostic('originals/Netstorm.exe 가 없는 환경이라 PE 대조를 건너뜀');
        return;
    }
    const bytes = fs.readFileSync(ORIGINAL_EXE);
    assert.equal(sha256(bytes), ORIGINAL_EXE_SHA256, '보호 원본 10.78 실행 파일');
    const read = peReader(bytes);
    const mismatches = [];
    // 표의 각 주소를 원본 폭으로 읽어 비교한다.
    for (const field of GAME_AUX_FIELDS) {
        const { section, bytes: raw } = read(field.address, WIDTH[field.kind]);
        const value = decode(field.kind, raw);
        if (section !== '.data' || value !== field.initial) mismatches.push(`${field.name}@${field.address.toString(16)}: ${section} ${value} ≠ ${field.initial}`);
    }
    // 장식 네 칸도 같은 방식으로 비교한다.
    for (const field of WORLD_DECORATION_FIELDS) {
        const value = read(field.address).bytes.readUInt32LE(0);
        if (value !== field.initial) mismatches.push(`장식@${field.address.toString(16)}: ${value} ≠ ${field.initial}`);
    }
    assert.deepEqual(mismatches, []);
    // 필드 50개(double 4개는 8바이트 전체를 읽음, WB04-D3 의 Form 종속 전역 3칸 포함)와 장식 4칸이다.
    assert.deepEqual([GAME_AUX_FIELDS.length, WORLD_DECORATION_FIELDS.length], [50, 4]);
});

// TYPE 번호 칸: 웹 타입 표에서 같은 번호의 이름이 표의 대응 이름과 같다(자산 70~, 프로세스 10~, 내장 Form 1·2·3·6). 프로세스 상한 63 은 마지막 프로세스(62)+1 이다.
test('GameAuxState: TYPE 번호가 웹 타입 표의 이름과 맞는다', () => {
    // 내장 Form 네 종의 이름이다(WB04-D3 에서 Gump 3·Content 6 추가).
    const builtin = { 1: 'DependForm', 2: 'ProcessForm', 3: 'GumpForm', 6: 'ContentForm' };
    // 번호의 웹 이름을 찾는다.
    const nameOf = (number) => (number >= FIRST_ASSET_TYPE_NUMBER ? TYPE_LOAD_ORDER[number - FIRST_ASSET_TYPE_NUMBER] : number >= 10 ? PROCESS_TYPE_NAMES[number - 10] : builtin[number]);
    const typed = [...GAME_AUX_FIELDS.filter((field) => field.typeName !== null).map((field) => [field.name, field.initial, field.typeName]),
        ...WORLD_DECORATION_FIELDS.map((field) => [`decoration@${field.address.toString(16)}`, field.initial, field.typeName])];
    assert.equal(typed.length, 28);
    for (const [name, number, typeName] of typed) assert.equal(nameOf(number), typeName, `${name}=${number}`);
    assert.equal(new GameAuxState().processTypeEnd, 10 + PROCESS_TYPE_NAMES.length);
});

// 폭 검사: u32/i32 범위·f32 반올림·f64·bool 형식을 원본 폭으로 저장하고 잘못된 값은 쓰기 전에 거부한다. 자원 DWORD 의 unsigned 비트 읽기/쓰기.
test('GameAuxState: 원본 폭 검사와 자원 비트 해석', () => {
    const aux = new GameAuxState();
    aux.priestType = 0xffffffff;
    assert.equal(aux.priestType, 0xffffffff);
    assert.throws(() => { aux.priestType = -1; }, RangeError);
    assert.throws(() => { aux.priestType = 1.5; }, RangeError);
    assert.equal(aux.priestType, 0xffffffff, '거부된 쓰기는 값을 바꾸지 않는다');
    aux.totalCost = -0x80000000;
    assert.equal(aux.totalCost, -0x80000000);
    assert.throws(() => { aux.totalCost = 0x80000000; }, RangeError);
    aux.lostX = 0.1;
    assert.equal(aux.lostX, Math.fround(0.1));
    aux.geyserDeadline = 0.1;
    assert.equal(aux.geyserDeadline, 0.1);
    assert.throws(() => { aux.lostY = '1'; }, TypeError);
    aux.priestQuarterHp = true;
    assert.equal(aux.priestQuarterHp, true);
    assert.throws(() => { aux.priestQuarterHp = 1; }, TypeError);
    // 자원 DWORD 는 signed 로 저장하고 unsigned 비트로도 읽고 쓴다.
    aux.resourcePool = -1;
    assert.equal(aux.resourcePoolBits, 0xffffffff);
    aux.storeResourcePoolBits(0x80000000);
    assert.equal(aux.resourcePool, -0x80000000);
    assert.throws(() => aux.storeResourcePoolBits(-1), RangeError);
    // 장식 배열은 같은 저장소를 빌려 주며 원소 쓰기는 32비트로 감긴다.
    const decorations = aux.worldDecorationTypes;
    decorations[3] = 186;
    assert.equal(aux.worldDecorationTypes[3], 186);
    assert.equal(aux.worldDecorationTypes, decorations);
    // 새 객체는 독립이다.
    const other = new GameAuxState();
    assert.deepEqual([other.priestType, other.totalCost, other.worldDecorationTypes[3]], [158, 0, 185]);
});

// 일반 월드 준비: 정해진 fresh 필드만 되돌리고 현재 TYPE·Form/예약 번호·기한·군도 밀도는 유지한다. 정리는 손실 좌표만 -10000 으로 쓴다.
test('GameAuxState: 일반 월드 준비와 손실 좌표 정리', () => {
    const aux = new GameAuxState();
    Object.assign(aux, { ordinaryGeysers: 3, totalCost: 50, resourcePool: -7, refundPercent: 40, allowAnyCapture: 1, archipelago: 2, lostX: 5, lostY: 6, geyserType: 123, priestQuarterHp: true });
    Object.assign(aux, { priestType: 200, processFormType: 9, regularProcessType: 47, geyserDeadline: 12.5, archipelagoMinimum: 3, defaultSp: 900 });
    aux.resetForGeneralWorld();
    assert.deepEqual([aux.ordinaryGeysers, aux.totalCost, aux.resourcePool, aux.refundPercent, aux.allowAnyCapture, aux.archipelago, aux.lostX, aux.lostY, aux.geyserType, aux.priestQuarterHp],
        [0, 0, 0, 25, 0, 0, 0, 0, 122, false]);
    assert.deepEqual([aux.priestType, aux.processFormType, aux.regularProcessType, aux.geyserDeadline, aux.archipelagoMinimum, aux.defaultSp], [200, 9, 47, 12.5, 3, 900]);
    aux.resetLostPositionForCleanup();
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, aux.lostX, true);
    assert.equal(view.getUint32(0, true), 0xc61c4000);
    assert.equal(aux.lostY, aux.lostX);
    assert.equal(aux.totalCost, 0, '정리는 손실 좌표만 쓴다');
});
