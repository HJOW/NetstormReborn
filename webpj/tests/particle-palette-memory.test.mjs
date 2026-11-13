// 입자 색 표 저장소 particle-palette-memory.js(ParticlePaletteMemory, adapt)를 검사한다(cpppj `ParticleRenderingTests.cpp` 의 ParticlePaletteMemory_ActualAllocationAndAdjacentRead 에 대응).
// 원본의 "요청 크기 그대로 확보 + 바로 뒤 DWORD 읽기" 를 웹은 OS 힙 대신 명시 인접 입력(기본 관찰값 1)으로 대체했으므로, 이 시험은 실제 OS 메모리 읽기가 아닌
// 확보 전 거부·색수 11 과 snapshot 길이 12·앞 11개 색 보존·인접 입력·재확보 교체·잘못된 입력 거부·실패 시 이전 배열 보존을 확인한다. 실제 게임·브라우저 실행이 아니다.
// (2026-11-13 WB04-E390: 5-A RawCampaignParticles 와 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { ParticlePaletteMemory, OBSERVED_ADJACENT_DWORD } from '../www/render/particle-palette-memory.js';

// 앞 11개는 원본 GIFCLOUD 생성자 결과이고 마지막은 재할당 전의 명시 검사 표식이다(cpppj kPaletteSeed). (2026-11-13 WB04-E390)
const SEED = [217, 226, 62, 56, 51, 200, 221, 218, 218, 197, 196, 0xcafebabe];

test('ParticlePaletteMemory: 확보 전 조회는 거부하고 색수 11 확보 뒤 snapshot 은 색 11개와 인접 DWORD 하나이다(ActualAllocationAndAdjacentRead)', () => {
    const memory = new ParticlePaletteMemory();
    assert.equal(memory.count, 0);
    assert.equal(memory.address, 0);
    assert.throws(() => memory.snapshot(), /생성 전 조회/);
    // 12개를 먼저 확보한 뒤 앞 11개로 다시 확보해도 이전 표식은 남지 않는다.
    memory.assign(SEED);
    assert.equal(memory.count, 12);
    memory.assign(SEED.slice(0, 11));
    assert.equal(memory.count, 11);
    assert.equal(memory.address, 2);
    const actual = memory.snapshot();
    assert.equal(actual.length, 12);
    for (let index = 0; index < 11; index += 1) assert.equal(actual[index], SEED[index]);
    // 인접 DWORD 는 마지막 색(196)이나 0xcafebabe 가 아니라 기본 관찰값 1 이다.
    assert.equal(actual[11], OBSERVED_ADJACENT_DWORD);
    assert.equal(OBSERVED_ADJACENT_DWORD, 1);
    assert.ok(actual instanceof Uint32Array);
    // 같은 확보 세대의 snapshot 은 같은 배열이고, 입력 배열을 바꿔도 확보된 색은 변하지 않는다.
    assert.equal(memory.snapshot(), actual);
    const input = [1, 2, 3];
    memory.assign(input);
    input[0] = 99;
    assert.deepEqual(Array.from(memory.snapshot()), [1, 2, 3, OBSERVED_ADJACENT_DWORD]);
});

test('ParticlePaletteMemory: 인접 DWORD 는 명시 입력이고 범위 밖 입력은 이전 배열을 보존한 채 거부한다', () => {
    // 인접 입력은 DWORD 정수만 받는다.
    for (const bad of [-1, 0x100000000, 1.5, NaN, '1', null]) assert.throws(() => new ParticlePaletteMemory(bad), RangeError, String(bad));
    for (const good of [0, 0xffffffff, 0xabcdef01]) assert.equal(new ParticlePaletteMemory(good).count, 0);
    const memory = new ParticlePaletteMemory(0xabcdef01);
    memory.assign([5, 6]);
    assert.deepEqual(Array.from(memory.snapshot()), [5, 6, 0xabcdef01]);
    const before = memory.snapshot();
    const generation = memory.address;
    // 빈 배열, 256 초과, 색 DWORD 범위 밖, 배열이 아닌 입력은 모두 거부되고 이전 표를 그대로 둔다.
    assert.throws(() => memory.assign([]), /요청 수 오류/);
    assert.throws(() => memory.assign(new Array(257).fill(1)), /요청 수 오류/);
    assert.throws(() => memory.assign(null), /요청 수 오류/);
    assert.throws(() => memory.assign([1, -1]), /DWORD 범위 오류/);
    assert.throws(() => memory.assign([1, 0x100000000]), /DWORD 범위 오류/);
    assert.throws(() => memory.assign([1, 0.5]), /DWORD 범위 오류/);
    // 첫 항목의 잘못된 값도 거부한다(검사는 모든 항목을 앞에서부터 본다).
    assert.throws(() => memory.assign([-1, 1]), /DWORD 범위 오류/);
    assert.throws(() => memory.assign([0.5]), /DWORD 범위 오류/);
    assert.equal(memory.snapshot(), before);
    assert.equal(memory.address, generation);
    // 경계 길이 1과 256 은 허용한다.
    memory.assign([7]);
    assert.equal(memory.count, 1);
    memory.assign(new Array(256).fill(3));
    assert.equal(memory.count, 256);
    assert.equal(memory.snapshot().length, 257);
    assert.equal(memory.snapshot()[256], 0xabcdef01);
});
