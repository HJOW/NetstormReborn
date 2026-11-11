// 로그인 sendFort(0x2a)의 예약·선언·전송과 길이 observer(cpppj `o/RawMissionLoginPackets`, 10.78 004a7240·004df590·0040a540)의 웹 대응 `core/raw-mission-login-packets.js` 를 검사한다(4-C, WB04-E317).
//  - 고정 관측 `fort-image-packets-x86.tsv` 중 sendFort 행 `S` 93개와 길이 observer 행 `F` 10개: 합성 payload 패턴(0·가변·32696·32695 바이트)과 실제 보호 Fort 8개(Early Missions 여섯 + 캠페인 1-1/1-2)의 현재 섹션을 입력해
//    예약(image+20) 전체 BYTE·observer 전후·최종 wire(image+17)와 외부 읽기 순서(Template 조회 C, 길이 H, 로그 L, 누락 진단, 송신 S, 타입 개수 G)를 대조한다. 같은 파일의 이미지 행 `I` 116개는 앞 단위(fort-image.test.mjs)가 맡는다.
//  - 손 시험(cpppj `FortImagePackets_SelectedBoundaries`): Template 없음은 자연 false, 선택한 소비자 누락은 그 위치에서 거부, 구성 검사, 물리 범위 거부, 예약 tail 보존.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E317)
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
import { FORT_IMAGE_ASSERTION } from '../www/core/raw-fort-image.js';
import { MissionFortPacket, RawMissionLoginPackets, finalizeMissionFortPacket, makeMissionFortPacket } from '../www/core/raw-mission-login-packets.js';

// 저장소 안의 고정 관측 파일이다. (2026-11-11 WB04-E317)
const FIXTURE = resolve(dirname(fileURLToPath(import.meta.url)), '../../cpppj/tests/fixtures/fort-image-packets-x86.tsv');

// Early Missions 여섯 개와 캠페인 1-1/1-2 의 실제 loadFort 파일명이다(profile 100..107). (2026-11-11 WB04-E317)
const FORT_NAMES = ['BridgeTheGap', 'SecretWorkshop', 'CaptureThePriest', 'TacticalCombat', 'SubtleDefense', 'RawPower', 'TheWarBegins', 'MasterOfWhirligigs'];

// 실제 보호 자료를 읽는 아카이브와 타입 표이다. (2026-11-11 WB04-E317)
const archive = new TaffArchive(readData(ARCHIVE_PATH));
const types = createFortTypes(TypeTable.fromArchive(archive, decodeOriginalText));

// 소문자 16진수 문자열(빈 관찰은 fixture 의 단일 '-')이다. (2026-11-11 WB04-E317)
const hex = (bytes) => (bytes.length === 0 ? '-' : Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join(''));

// little endian DWORD/WORD 를 읽고 쓴다. (2026-11-11 WB04-E317)
const get = (bytes, offset, width = 4) => { let value = 0; for (let index = 0; index < width; index += 1) value |= bytes[offset + index] << (8 * index); return value >>> 0; };
function put(bytes, offset, value) {
    for (let index = 0; index < 4; index += 1) bytes[offset + index] = (value >>> (8 * index)) & 0xff;
}

// 원본에 준 현재 섹션 입력만 만든다(이미지의 기대 헤더·길이는 계산하지 않는다). (2026-11-11 WB04-E317)
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

// S/F 행 재생: 열은 kind/input(13 숫자)/image/before/after/wire/trace 이다. (2026-11-11 WB04-E317)
test('미션 로그인 패킷: 원본 sendFort 93행과 길이 observer 10행의 예약·wire·읽기 순서', () => {
    const rows = readFileSync(FIXTURE, 'utf8').split(/\r?\n/).filter((line) => line !== '' && !line.startsWith('kind\t')).map((line) => line.split('\t'));
    let compared = 0;
    let sends = 0;
    let observers = 0;
    // 각 행의 현재 Template·Player·효과 변경은 다음 행에 누적하지 않는다.
    for (const row of rows) {
        if (row[0] === 'I') continue;
        assert.equal(row.length, 7);
        const input = row[1].split(',').map(Number);
        assert.equal(input.length, 13);
        const first = fortInput(input[0], input[1], input[2], input[3]);
        const second = fortInput(input[0], input[1] + 3, input[2], (input[3] ^ 0xfe) & 0xff);
        const players = new Uint8Array(1620);
        players[input[6] * 180 + 0x58] = input[9];
        let current = input[11] ? first : null;
        const trace = [];
        const event = (text) => { trace.push(text); };
        const imageHooks = {
            typeCount: () => { event(`G,${input[4]}`); return input[4]; },
            assertion: (kind) => { event(kind === FORT_IMAGE_ASSERTION.typeCount ? 'A,types' : kind === FORT_IMAGE_ASSERTION.sectionCapacity ? 'A,section' : 'A,stream'); },
        };
        let imageText = '-';
        let before = '-';
        let after = '-';
        let wire = '-';
        if (row[0] === 'F') {
            // 길이 observer 단독: 20바이트 0xa5 예약의 크기 DWORD 를 입력하고 bool 인자에 따라 선언 WORD 를 확정한다.
            const packet = new Uint8Array(20).fill(0xa5);
            put(packet, 12, input[8]);
            finalizeMissionFortPacket(packet, input[12] !== 0, () => { event('A,packet'); });
            before = hex(packet);
            observers += 1;
        } else {
            const reads = {
                current: (owner) => { assert.equal(owner, input[6]); event(current ? 'C,1' : 'C,0'); return current; },
                initialLength: () => { event(`H,${input[10]}`); if (input[12] === 3) players[input[6] * 180 + 0x58] = 0x7f; return input[10] & 0xffff; },
            };
            const hooks = {
                log: (owner, target) => {
                    assert.equal(owner, input[6]);
                    assert.equal(target >>> 0, input[7]);
                    event('L');
                    if (input[12] === 1) current = second;
                    else if (input[12] === 2) current = null;
                },
                missingTemplate: () => { event('A,template'); assert.equal(input[12], 2); current = second; },
                // 송신 장치는 공개 hook 이 wire 보다 3BYTE 큰 실제 mutable 예약을 빌리는 경계이며, 실제 observer 를 부르고 선언 wire 와 미작성 꼬리를 각각 대조한다.
                send: (target, bytes) => {
                    event(`S,${target >>> 0}`);
                    const imageSize = get(bytes, 12);
                    assert.equal(bytes.length, imageSize + 20);
                    imageText = hex(bytes.subarray(17, 17 + imageSize));
                    before = hex(bytes);
                    finalizeMissionFortPacket(bytes, true);
                    after = hex(bytes);
                    assert.equal(get(bytes, 2, 2), imageSize + 17);
                    wire = hex(bytes.subarray(0, get(bytes, 2, 2)));
                },
            };
            const packets = new RawMissionLoginPackets(players, reads, imageHooks, hooks);
            assert.equal(packets.sendFort(input[6], input[7] | 0, input[8]), input[11] !== 0);
            sends += 1;
        }
        assert.deepEqual([imageText, before, after, wire, trace.length ? trace.join(';') : '-'], [row[2], row[3], row[4], row[5], row[6]], `행 ${compared} (${row[0]},${row[1]})`);
        compared += 1;
    }
    assert.deepEqual([compared, sends, observers], [103, 93, 10]);
});

// 현재 Template 없음은 자연 반환이고 선택된 실제 소비자 누락은 그 원문 위치에서 거부한다. 구성·물리 범위 계약도 확인한다. (2026-11-11 WB04-E317)
test('미션 로그인 패킷: 선택된 경계의 누락과 물리 범위', () => {
    const fort = fortInput(0, 0, 35, 0);
    const players = new Uint8Array(1620);
    let current = null;
    let reads = 0;
    const input = { current: () => { reads += 1; return current; }, initialLength: () => 0 };
    const image = { typeCount: () => 188 };
    const absent = new RawMissionLoginPackets(players, input, image, {});
    assert.equal(absent.sendFort(1, 9, 0), false);
    assert.equal(reads, 1);
    // Template 이 있으면 로그 장치가 없는 위치에서 거부한다(그 전에 한 번 더 읽었음).
    current = fort;
    assert.throws(() => absent.sendFort(1, 9, 0), /실제 선택 소비자 미연결/);
    // 송신 장치가 없으면 이미지 작성 뒤 거부하고, owner 9 이상은 거부한다.
    const noSend = new RawMissionLoginPackets(players, input, image, { log: () => {} });
    assert.throws(() => noSend.sendFort(1, 9, 0), /실제 선택 소비자 미연결/);
    assert.throws(() => noSend.sendFort(9, 9, 0), RangeError);
    assert.throws(() => noSend.sendFort(-1, 9, 0), RangeError);
    // 구성: 현재 조회/길이/타입 개수/Player9 크기가 없으면 거부한다.
    assert.throws(() => new RawMissionLoginPackets(players, { ...input, current: undefined }, image, {}), TypeError);
    assert.throws(() => new RawMissionLoginPackets(players, { ...input, initialLength: undefined }, image, {}), TypeError);
    assert.throws(() => new RawMissionLoginPackets(players, input, {}, {}), TypeError);
    assert.throws(() => new RawMissionLoginPackets(new Uint8Array(1619), input, image, {}), TypeError);
    assert.throws(() => new RawMissionLoginPackets([], input, image, {}), TypeError);
    // 길이 observer: 거짓이면 물리 범위를 보지 않고 반환, 참이면 17바이트 미만을 거부하고, 크기 -1 은 진단 장치가 필요하다.
    finalizeMissionFortPacket(new Uint8Array(0), false);
    assert.throws(() => finalizeMissionFortPacket(new Uint8Array(0), true), RangeError);
    const sentinel = new Uint8Array(17);
    put(sentinel, 12, 0xffffffff);
    assert.throws(() => finalizeMissionFortPacket(sentinel, true), /크기 진단 미연결/);
    // 진단이 복귀하면 현재 하위 WORD 를 다시 읽어 +0x11 을 선언 WORD 에 쓴다(DWORD 사본이 아님).
    finalizeMissionFortPacket(sentinel, true, () => { put(sentinel, 12, 0x00010005); });
    assert.equal(get(sentinel, 2, 2), 0x16);
    // 예약 packet: 변조된 헤더/선언 길이는 wire 로 빌리지 않는다.
    assert.throws(() => new MissionFortPacket(new Uint8Array(0)).bytes(), RangeError);
    assert.throws(() => new MissionFortPacket(new Uint8Array(4)).bytes(), RangeError);
    const reserved = makeMissionFortPacket(2, 7, 0x55, Uint8Array.of(1, 2, 3), 0);
    // 초기 길이는 최소 4, 헤더/필드 위치와 0xff 예약(끝 3BYTE 포함)을 확인한다.
    assert.equal(hex(reserved.storage), '012a0400' + '02000000' + '07000000' + '03000000' + '55' + '010203' + 'ffffff');
    assert.equal(reserved.storage.length, 3 + 20);
    reserved.storage[2] = 0x40;
    assert.throws(() => reserved.bytes(), RangeError);
    reserved.storage[2] = 0x13;
    reserved.storage[3] = 0;
    assert.equal(reserved.bytes().length, 0x13);
    // 선언 길이가 예약 길이와 같으면(23) 전체를 빌릴 수 있고, 하나 크면 거부한다.
    reserved.storage[2] = 23;
    assert.equal(reserved.bytes().length, 23);
    reserved.storage[2] = 24;
    assert.throws(() => reserved.bytes(), RangeError);
    assert.equal(makeMissionFortPacket(1, 0, 0, new Uint8Array(0), 9).storage[2], 9);
    // 초기 길이는 WORD 로 취급한다: 0x10001 은 WORD 1 이라 최소 4 로 올린다.
    assert.deepEqual(Array.from(makeMissionFortPacket(1, 0, 0, new Uint8Array(0), 0x10001).storage.subarray(2, 4)), [4, 0]);
    assert.throws(() => makeMissionFortPacket(1, 0, 0, new Uint8Array(32769), 0), RangeError);
    assert.doesNotThrow(() => makeMissionFortPacket(1, 0, 0, new Uint8Array(32768), 0));
});
