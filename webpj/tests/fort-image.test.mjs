// 현재 Template 섹션의 SaveImage 직렬화(cpppj RawFortImage, 원본 004bf520 → 0044d640/004bcd80)의 웹 대응 `core/raw-fort-image.js` 를 검사한다(4-B, WB04-E277).
//  - 고정 관측 `fort-image-packets-x86.tsv` 중 이미지 행 `I` 116개: 합성 payload 패턴(0·가변·32696·32695 바이트)과 실제 보호 Fort 8개(Early Missions 여섯 + 캠페인 1-1/1-2)의 현재 섹션을 입력해
//    이미지 전체 바이트와 외부 읽기/진단 순서(타입 개수 읽기 G, 진단 types/section/stream)를 대조하고 논리 끝 밖 고정 할당이 0 으로 남았는지 확인한다.
//    같은 파일의 `S`(SendFort 93)·`F`(FinalizeMissionFortPacket 10) 행은 4-C 로그인 패킷(RawMissionLoginPackets) 몫이다.
//  - 손 시험(cpppj FortImage_CurrentProtectedSections · PhysicalCapacityAndRequiredConsumers): 현재 raw 섹션 변경이 반영되고 해석 필드로 되돌리지 않으며 이미지를 다시 읽으면 같은 섹션이 나오고,
//    엄격한 '<' 진단은 정확히 32768 인 쓰기를 막지 않고 초과만 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E277)
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';
import { readData } from './helpers.mjs';
import { ARCHIVE_PATH } from '../www/assets/game-data.js';
import { TaffArchive } from '../www/assets/taff.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { TypeTable } from '../www/assets/type-table.js';
import { createFortTypes } from '../www/assets/type-flags.js';
import { parseFort } from '../www/assets/fort.js';
import { FORT_IMAGE_ASSERTION, FORT_IMAGE_CAPACITY, FortImageData, RawFortImage } from '../www/core/raw-fort-image.js';

// 저장소 안의 고정 관측 파일이다. (2026-11-10 WB04-E277)
const FIXTURE = resolve(dirname(fileURLToPath(import.meta.url)), '../../cpppj/tests/fixtures/fort-image-packets-x86.tsv');

// Early Missions 여섯 개와 캠페인 1-1/1-2 의 실제 loadFort 파일명이다(profile 100..107). (2026-11-10 WB04-E277)
const FORT_NAMES = ['BridgeTheGap', 'SecretWorkshop', 'CaptureThePriest', 'TacticalCombat', 'SubtleDefense', 'RawPower', 'TheWarBegins', 'MasterOfWhirligigs'];

// 실제 보호 자료를 읽는 아카이브와 타입 표이다. (2026-11-10 WB04-E277)
const archive = new TaffArchive(readData(ARCHIVE_PATH));
const types = createFortTypes(TypeTable.fromArchive(archive, decodeOriginalText));

// 소문자 16진수 문자열. (2026-11-10 WB04-E277)
function hex(bytes) {
    return bytes.length === 0 ? '-' : Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');
}

// 원본에 준 현재 섹션 입력만 만든다(이미지의 기대 헤더·길이는 계산하지 않는다). (2026-11-10 WB04-E277)
function fortInput(profile, seed, count, flag) {
    let fort;
    if (profile >= 100) {
        fort = parseFort(archive.read(`\\d\\${FORT_NAMES[profile - 100]}.fort`), types);
        if (seed & 1) {
            while (fort.sections.length < 13) fort.sections.push(new Uint8Array(0));
            fort.sections[12] = Uint8Array.of(0xa0, profile & 255, seed & 255, 0, 0xfe);
        }
    } else {
        fort = { sections: [], flag: 0 };
        for (let index = 0; index < count; index += 1) {
            const size = profile === 0 ? 0 : profile === 1 ? (index * 7 + seed) % 17 : profile === 2 && index === 34 ? 32696 : profile === 3 && index === 34 ? 32695 : 0;
            // BYTE 패턴은 각 현재 stream 의 독립 입력이다.
            fort.sections.push(Uint8Array.from({ length: size }, (_, byte) => (byte * 29 + index * 13 + seed) & 255));
        }
    }
    fort.flag = flag & 255;
    return fort;
}

// 논리 끝 뒤 전체 고정 할당이 0 으로 남았는지 본다. (2026-11-10 WB04-E277)
function checkAllocation(image) {
    assert.equal(image.storage.length, FORT_IMAGE_CAPACITY);
    assert.ok(image.end <= image.storage.length);
    assert.ok(image.storage.subarray(image.end).every((byte) => byte === 0));
}

test('Fort 이미지: 원본 이미지 행 116개의 전체 바이트와 타입 개수 읽기·진단 순서가 같다', () => {
    const rows = readFileSync(FIXTURE, 'utf8').split(/\r?\n/).filter((line) => line !== '' && !line.startsWith('kind\t')).map((line) => line.split('\t'));
    let compared = 0;
    for (const row of rows) {
        if (row[0] !== 'I') continue;
        assert.equal(row.length, 7);
        const input = row[1].split(',').map(Number);
        const fort = fortInput(input[0], input[1], input[2], input[3]);
        const trace = [];
        const hooks = {
            typeCount: () => { trace.push(`G,${input[4]}`); return input[4]; },
            assertion: (kind) => trace.push(kind === FORT_IMAGE_ASSERTION.typeCount ? 'A,types' : kind === FORT_IMAGE_ASSERTION.sectionCapacity ? 'A,section' : 'A,stream'),
        };
        const image = new RawFortImage('patch1078', fort, hooks).save();
        checkAllocation(image);
        assert.equal(hex(image.bytes()), row[2], row[1]);
        assert.equal(trace.join(';') || '-', row[6], row[1]);
        compared += 1;
    }
    assert.equal(compared, 116);
});

test('Fort 이미지: 현재 raw 섹션 변경이 반영되고 해석 필드로 되돌리지 않으며 다시 읽으면 같은 섹션이다', () => {
    for (let profile = 100; profile < 108; profile += 1) {
        const fort = fortInput(profile, 0, 35, 0);
        const hooks = { typeCount: () => 188 };
        const original = new RawFortImage('patch1078', fort, hooks).save();
        // 해석된 Money 필드를 바꿔도 현재 raw 섹션만 쓰므로 같은 이미지이다.
        fort.money = 999999;
        const unchanged = new RawFortImage('patch1078', fort, hooks).save();
        assert.ok(Buffer.from(original.storage).equals(Buffer.from(unchanged.storage)) && original.end === unchanged.end);
        fort.sections[12] = Uint8Array.of(0xa0, profile, 1, 0, 0xfe);
        const changed = new RawFortImage('patch1078', fort, hooks).save();
        assert.ok(!Buffer.from(changed.storage).equals(Buffer.from(original.storage)));
        checkAllocation(changed);
        const parsed = parseFort(changed.bytes(), types);
        assert.equal(parsed.sections.length, fort.sections.length);
        parsed.sections.forEach((section, index) => assert.ok(Buffer.from(section).equals(Buffer.from(fort.sections[index])), `${FORT_NAMES[profile - 100]} #${index}`));
    }
});

test('Fort 이미지: 엄격한 < 진단은 정확히 32768 인 쓰기를 막지 않고 초과만 거부하며 필수 소비자를 요구한다', () => {
    const fort = fortInput(2, 0, 35, 0);
    const assertions = [];
    const hooks = { typeCount: () => 188, assertion: (kind) => assertions.push(kind) };
    const exact = new RawFortImage('patch1078', fort, hooks).save();
    assert.deepEqual([exact.end, assertions], [32768, [FORT_IMAGE_ASSERTION.sectionCapacity]]);
    // 한 바이트 더 많으면 물리 범위를 넘어 거부한다.
    fort.sections[34] = Uint8Array.from([...fort.sections[34], 1]);
    assert.throws(() => new RawFortImage('patch1078', fort, hooks).save(), /payload의 물리 범위/);
    // 길이 필드 위치 자체가 용량 끝(32766)에 정확히 닿으면 쓸 수 있고(섹션 진단만), 한 바이트라도 넘으면 stream 진단 뒤 거부한다.
    const edge = { sections: [new Uint8Array(32696)], flag: 0 };
    const edgeAssertions = [];
    const edgeImage = new RawFortImage('patch1078', edge, { typeCount: () => 188, assertion: (kind) => edgeAssertions.push(kind) }).save();
    assert.deepEqual([edgeImage.end, edgeAssertions], [32768, [FORT_IMAGE_ASSERTION.sectionCapacity]]);
    edge.sections[0] = new Uint8Array(32697);
    edgeAssertions.length = 0;
    assert.throws(() => new RawFortImage('patch1078', edge, { typeCount: () => 188, assertion: (kind) => edgeAssertions.push(kind) }).save(), /길이 필드의 물리 범위/);
    assert.equal(edgeAssertions.at(-1), FORT_IMAGE_ASSERTION.streamCapacity);
    // 현재 타입 개수는 signed 로 비교한다: 0x80000000 은 음수라 진단이 없다.
    const negative = [];
    new RawFortImage('patch1078', { sections: [], flag: 0 }, { typeCount: () => 0x80000000, assertion: (kind) => negative.push(kind) }).save();
    assert.deepEqual(negative, []);
    // 진단 소비자가 필요한 위치에서 없으면 거부한다(타입 개수 256 이상).
    const empty = { sections: [], flag: 0 };
    assert.throws(() => new RawFortImage('patch1078', empty, { typeCount: () => 256 }).save(), /진단 미연결/);
    // 10.78 전용, 타입 개수 장치 필수, 섹션은 35개 이하.
    assert.throws(() => new RawFortImage('cd1072', empty, hooks), TypeError);
    assert.throws(() => new RawFortImage('patch1078', empty, {}), TypeError);
    assert.throws(() => new RawFortImage('patch1078', { sections: new Array(36).fill(new Uint8Array(0)), flag: 0 }, hooks), TypeError);
    // 변조된 논리 끝은 배열 밖을 빌리지 않는다.
    assert.throws(() => new FortImageData(new Uint8Array(0), 1).bytes(), RangeError);
    // flag 0xfe 만 둘째 바이트로 남고, 선택 훅이 있으면 그 값과 현재 섹션을 쓴다.
    const flagged = new RawFortImage('patch1078', { sections: [], flag: 0xfe }, hooks).save();
    assert.deepEqual([flagged.storage[0], flagged.storage[1], flagged.end], [0x46, 0xfe, 2 + 35 * 2]);
    const viaHooks = new RawFortImage('patch1078', { sections: [], flag: 0 }, { ...hooks, signatureFlag: () => 0xfe, section: (index) => Uint8Array.of(index) }).save();
    assert.deepEqual([viaHooks.storage[1], viaHooks.end, viaHooks.storage[2], viaHooks.storage[4]], [0xfe, 2 + 35 * 3, 3, 0]);
});
