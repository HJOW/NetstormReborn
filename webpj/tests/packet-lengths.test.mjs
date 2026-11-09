// 10.78 패킷 선언 길이 표 raw-packet-lengths.js(원본 Netstorm.exe 0052ec40, 타입 0~137의 WORD 138개)를 원본 실행 파일의 .data 구역 초기화 값과 한 WORD 도 다르지 않게 대조한다.
//  - 원본 관측: originals/Netstorm.exe(10.78) 를 읽기 전용으로 열어 PE 구역 표로 가상 주소 0052ec40 을 파일 위치로 바꾸고 138개 WORD 를 읽는다(게임 실행 없음, 파일 수정 없음).
//  - 보조 대조: cpppj `o/RawPacketLengths.h` 의 kPatchPacketLengths 선언 텍스트(저장소 안 파일)와 같다.
//  - 계약: 가변 길이 표식 0xffff, 표 밖 타입 거부, 표는 동결.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E232: 패킷 선언 길이 표 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { PATCH_PACKET_LENGTHS, PATCH_PACKET_TYPE_COUNT, VARIABLE_PACKET_LENGTH, patchPacketLength } from '../www/core/raw-packet-lengths.js';

// 원본 표의 가상 주소이다(10.78 Netstorm.exe 의 .data 구역). (2026-11-10 WB04-E232)
const TABLE_ADDRESS = 0x52ec40;

// 읽기 전용으로 연 원본 실행 파일이다. 없으면 PE 대조는 건너뛴다. (2026-11-10 WB04-E232)
const exePath = path.join(REPO_ROOT, 'originals', 'Netstorm.exe');
const exe = fs.existsSync(exePath) ? fs.readFileSync(exePath) : null;

// PE 구역 표를 읽어 가상 주소를 파일 위치로 바꾼다(초기화 값이 파일에 있는 구역만). (2026-11-10 WB04-E232)
function fileOffsetOf(bytes, address) {
    const header = bytes.readUInt32LE(0x3c);
    const sections = bytes.readUInt16LE(header + 6);
    const optionalSize = bytes.readUInt16LE(header + 20);
    const imageBase = bytes.readUInt32LE(header + 24 + 28);
    const rva = address - imageBase;
    for (let index = 0; index < sections; index += 1) {
        const at = header + 24 + optionalSize + index * 40;
        const virtualAddress = bytes.readUInt32LE(at + 12);
        const rawSize = bytes.readUInt32LE(at + 16);
        const rawPointer = bytes.readUInt32LE(at + 20);
        if (rva >= virtualAddress && rva < virtualAddress + rawSize) return rawPointer + (rva - virtualAddress);
    }
    throw new Error('PE 구역에 없는 주소');
}

test('패킷 선언 길이 표: 원본 Netstorm.exe 10.78 의 0052ec40 WORD 138개와 모두 같다', { skip: !exe && 'originals/Netstorm.exe 없음' }, () => {
    const at = fileOffsetOf(exe, TABLE_ADDRESS);
    const original = Array.from({ length: PATCH_PACKET_TYPE_COUNT }, (_, type) => exe.readUInt16LE(at + type * 2));
    assert.deepEqual(Array.from(PATCH_PACKET_LENGTHS), original);
    // 가변 길이와 고정 길이의 개수도 원본 표에서 직접 센 값과 같다.
    assert.equal(original.filter((value) => value === VARIABLE_PACKET_LENGTH).length, PATCH_PACKET_LENGTHS.filter((value) => value === VARIABLE_PACKET_LENGTH).length);
});

test('패킷 선언 길이 표: cpppj RawPacketLengths.h 의 kPatchPacketLengths 선언과 같다', { skip: !fs.existsSync(path.join(REPO_ROOT, 'cpppj', 'src', 'o', 'RawPacketLengths.h')) && 'cpppj 헤더 없음' }, () => {
    const text = fs.readFileSync(path.join(REPO_ROOT, 'cpppj', 'src', 'o', 'RawPacketLengths.h'), 'utf8');
    const body = /kPatchPacketLengths\{([\s\S]*?)\};/.exec(text)[1];
    const declared = [...body.matchAll(/0x([0-9a-fA-F]+)/g)].map((match) => parseInt(match[1], 16));
    assert.deepEqual(Array.from(PATCH_PACKET_LENGTHS), declared);
});

test('패킷 선언 길이 표 계약: 가변 길이 표식 0xffff·표 밖 타입 거부·동결', () => {
    assert.equal(PATCH_PACKET_TYPE_COUNT, 138);
    assert.equal(PATCH_PACKET_LENGTHS.length, 138);
    assert.equal(VARIABLE_PACKET_LENGTH, 0xffff);
    // 알려진 값: 타입 0 은 가변, 16 은 8 BYTE, 27 은 0x26, 137 은 0x10b.
    assert.deepEqual([0, 16, 27, 137].map(patchPacketLength), [0xffff, 8, 0x26, 0x10b]);
    for (const type of [-1, 138, 1.5, NaN, '16']) assert.throws(() => patchPacketLength(type), RangeError);
    assert.ok(Object.isFrozen(PATCH_PACKET_LENGTHS));
    assert.throws(() => { 'use strict'; PATCH_PACKET_LENGTHS[0] = 1; }, TypeError);
});
