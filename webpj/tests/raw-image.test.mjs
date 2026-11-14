// 동적 Image(RawImage)의 bitmap 소비·VFX 1.10 shape 인코딩·깊은 복사·소멸을 기준선 소스 계약과 손 계산 바이트로 검증한다. (2026-11-14 WB04-E440)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(RawImage.cpp)에서 옮겼고 인코딩은 웹 ShapeDatabase 로 되풀이 해독해 원래 픽셀과 대조한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { RawImage } from '../www/ui/raw-image.js';
import { ShapeDatabase } from '../www/assets/shapes.js';

const make = (width, height, pixels, crop = false) => { const image = new RawImage(); image.convert({ width, height, pixels: Uint8Array.from(pixels) }, crop); return image; };
const header = (heightMinus, widthMinus, left, top, right, bottom) => [0x31, 0x2e, 0x31, 0x30, 1, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, heightMinus & 255, heightMinus >> 8 & 255, widthMinus & 255, widthMinus >> 8 & 255, heightMinus & 255, heightMinus >> 8 & 255, widthMinus & 255, widthMinus >> 8 & 255,
    ...[left - widthMinus, top - heightMinus, right - widthMinus, bottom - heightMinus].flatMap((value) => [value & 255, value >> 8 & 255, value >> 16 & 255, value >>> 24])];

test('convert: 3×2 비트맵은 40바이트 머리(표식·프레임 수 1·표 16·좌표)와 행별 skip/repeat/literal/종결 토큰이 된다', () => {
    const image = make(3, 2, [0, 5, 5, 7, 0, 0]);
    // 머리: 높이-1=1, 폭-1=2, 불투명 상자 왼쪽 0·위 0·오른쪽 2·아래 1 → 좌표 DWORD 는 (왼쪽-2, 위-1, 오른쪽-2, 아래-1) = (-2, -1, 0, 0).
    assert.deepEqual(Array.from(image.shape()), [...header(1, 2, 0, 0, 2, 1), 1, 1, 4, 5, 0, 3, 7, 0]); assert.deepEqual([image.width(), image.height(), image.shapeSize(), image.bits().length], [3, 2, 48, 0]);
    assert.deepEqual(Array.from(image.shape().subarray(24, 40)), [0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0]);
});

test('convert: crop 은 오른쪽/아래의 마지막 nonzero 까지만 남기고 왼쪽/위쪽 여백은 자르지 않으며 전부 0 이면 1×1 이다', () => {
    const pixels = [0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0]; // 4×4: (1,1)=9, (3,2)=4
    const cropped = make(4, 4, pixels, true); assert.deepEqual([cropped.width(), cropped.height()], [4, 3]);
    const full = make(4, 4, pixels, false); assert.deepEqual([full.width(), full.height()], [4, 4]); assert.equal(full.shapeSize() > 40, true);
    // 불투명 상자는 같지만 머리의 높이/폭 필드가 crop 크기를 보존해 shape 가 다르다.
    assert.notDeepEqual(Array.from(cropped.shape()), Array.from(full.shape()));
    const empty = make(5, 4, new Array(20).fill(0), true); assert.deepEqual([empty.width(), empty.height(), empty.shapeSize()], [1, 1, 40]);
    // 빈 상자의 sentinel(INT_MAX/-INT_MAX)이 DWORD 로 감긴다: 왼쪽/위 0x7fffffff, 오른쪽/아래 0x80000001.
    assert.deepEqual(Array.from(empty.shape()), [...header(0, 0, 0, 0, 0, 0).slice(0, 24), 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0x7f, 1, 0, 0, 0x80, 1, 0, 0, 0x80]);
    // 한 줄 전체가 0 인 위쪽/왼쪽 여백은 상자 안 skip 으로만 남는다.
    const lone = make(3, 3, [0, 0, 0, 0, 7, 0, 0, 0, 0], true); assert.deepEqual([lone.width(), lone.height()], [2, 2]);
});

test('인코딩 토큰: 255 단위 skip·127 단위 repeat/literal·세 번 같은 색 직전 literal 분리·중간 pair 는 literal 에 남는다', () => {
    const rowTokens = (row) => { const image = make(row.length, 1, row, false); return Array.from(image.shape().subarray(40)); };
    // 불투명 상자 밖의 앞 투명은 인코딩하지 않는다. 상자 안 skip 300 → (1,255),(1,45).
    assert.deepEqual(rowTokens([0, 0, 5, ...new Array(300).fill(0), 9, 0]), [3, 5, 1, 255, 1, 45, 3, 9, 0]);
    // 같은 색 130 → repeat 127 + 3, 색 5. 끝의 투명은 버린다.
    assert.deepEqual(rowTokens([...new Array(130).fill(5), 0, 0]), [254, 5, 6, 5, 0]);
    // 서로 다른 130 색 → literal 127 + 3(각 payload 그대로).
    const alternating = Array.from({ length: 130 }, (_, index) => (index % 2 ? 7 : 4)); const literal = rowTokens(alternating);
    assert.deepEqual(literal.slice(0, 3), [255, 4, 7]); assert.deepEqual(literal.slice(128), [7, 7, 4, 7, 0]); // 나머지 인덱스 127..129 의 값은 7,4,7 이다. assert.equal(literal.length, 133);
    assert.deepEqual(rowTokens([1, 2, 3, 3, 3, 4]), [5, 1, 2, 6, 3, 3, 4, 0]); assert.deepEqual(rowTokens([1, 2, 2, 3]), [9, 1, 2, 2, 3, 0]);
    // 첫 pair 는 길이 2 부터 repeat 다. 바깥 pair 를 literal 안에서 만나면 세 번 이상일 때만 끊는다.
    assert.deepEqual(rowTokens([6, 6]), [4, 6, 0]); assert.deepEqual(rowTokens([6, 7, 7, 8]), [9, 6, 7, 7, 8, 0]);
});

test('ShapeDatabase 되풀이 해독: 인코딩한 불투명 사각형이 원래 픽셀/투명 위치와 같다', () => {
    const width = 6; const height = 5; const pixels = new Array(width * height).fill(0);
    for (const [x, y, value] of [[1, 1, 3], [2, 1, 3], [3, 1, 3], [4, 1, 9], [0, 2, 8], [5, 2, 8], [2, 3, 4], [3, 3, 2], [1, 4, 6]]) pixels[y * width + x] = value;
    const image = make(width, height, pixels, false); const database = new ShapeDatabase(Uint8Array.from(image.shape())); const decoded = database.decode(0, 0);
    // 불투명 상자는 x 0..5·y 1..4 → 6×4. 해독 행렬과 원본 행렬의 해당 구간이 같다.
    assert.deepEqual([decoded.width, decoded.height], [6, 4]);
    for (let y = 0; y < decoded.height; y += 1) for (let x = 0; x < decoded.width; x += 1) {
        const source = pixels[(y + 1) * width + x]; const index = y * decoded.width + x;
        assert.equal(decoded.opacity[index], source ? 255 : 0, `${x},${y}`); if (source) assert.equal(decoded.indices[index], source, `${x},${y}`);
    }
});

test('copyFrom/destroy: 깊은 복사·소멸은 폭/높이/버퍼를 지우되 shapeSize 는 보존하며 자기 복사는 먼저 파괴하므로 빈 이미지다', () => {
    const source = make(3, 2, [0, 5, 5, 7, 0, 0]); const copy = new RawImage(); copy.copyFrom(source);
    assert.deepEqual([copy.width(), copy.height(), copy.shapeSize()], [3, 2, 48]); assert.deepEqual(Array.from(copy.shape()), Array.from(source.shape())); assert.notEqual(copy.shape(), source.shape());
    // 복사본은 독립이다: 원본을 파괴해도 복사본은 남는다.
    source.destroy(); assert.deepEqual([source.width(), source.height(), source.shape().length, source.shapeSize()], [0, 0, 0, 48]); assert.equal(copy.shape().length, 48); source.destroy(); assert.equal(source.shapeSize(), 48);
    // 자기 복사: Destroy 가 먼저이므로 폭/높이/shape 가 0 이고 shapeSize 만 남는다.
    const self = make(3, 2, [0, 5, 5, 7, 0, 0]); self.copyFrom(self); assert.deepEqual([self.width(), self.height(), self.shape().length, self.shapeSize()], [0, 0, 0, 48]);
    // shape 가 없는 원본을 복사하면 이쪽의 이전 shapeSize 가 그대로 남는다.
    const target = make(3, 2, [0, 5, 5, 7, 0, 0]); target.copyFrom(new RawImage()); assert.deepEqual([target.width(), target.height(), target.shape().length, target.shapeSize()], [0, 0, 0, 48]);
});

test('convert 입력 검사: 폭/높이가 양수가 아니거나 픽셀 수가 맞지 않으면 TypeError 이고 입력 배열은 바꾸지 않는다', () => {
    for (const bitmap of [{ width: 0, height: 2, pixels: new Uint8Array(0) }, { width: 2, height: 0, pixels: new Uint8Array(0) }, { width: -2, height: -1, pixels: new Uint8Array(2) }, { width: 2, height: 2, pixels: new Uint8Array(3) }, { width: 2, height: 2, pixels: new Uint8Array(5) }]) assert.throws(() => new RawImage().convert(bitmap, true), { name: 'TypeError', message: /bitmap 크기 오류/ });
    const input = Uint8Array.from([0, 3, 3, 0]); new RawImage().convert({ width: 2, height: 2, pixels: input }, true); assert.deepEqual(Array.from(input), [0, 3, 3, 0]);
});

test('기본 상태와 경계: 새 Image 는 모두 0/빈 버퍼이고 1×N/N×1 도 유효하며 높이/폭 16비트 상위 바이트까지 쓴다', () => {
    const fresh = new RawImage(); assert.deepEqual([fresh.width(), fresh.height(), fresh.shapeSize(), fresh.shape().length, fresh.bits().length], [0, 0, 0, 0, 0]);
    for (const [width, height] of [[1, 1], [1, 3], [3, 1]]) { const image = make(width, height, new Array(width * height).fill(2), false); assert.deepEqual([image.width(), image.height()], [width, height]); }
    // 폭 300 → 폭-1=299 = 0x012b(낮은 바이트 먼저), 높이 300 → 같은 값이 높이 칸에 들어간다.
    const wide = make(300, 1, [...new Array(299).fill(0), 5]); assert.deepEqual(Array.from(wide.shape().subarray(16, 24)), [0, 0, 43, 1, 0, 0, 43, 1]);
    const tall = make(1, 300, [...new Array(299).fill(0), 5]); assert.deepEqual(Array.from(tall.shape().subarray(16, 24)), [43, 1, 0, 0, 43, 1, 0, 0]);
});

test('인코딩 끝/crop 경계: 행 끝의 세 번 같은 색은 literal 을 끊고, crop 은 첫 행/첫 열의 nonzero 도 센다', () => {
    const rowTokens = (row) => Array.from(make(row.length, 1, row, false).shape().subarray(40));
    assert.deepEqual(rowTokens([1, 2, 3, 3, 3]), [5, 1, 2, 6, 3, 0]);
    // 첫 행에만 nonzero 가 있어도 폭은 거기서 정해지고, 첫 열에만 있어도 높이는 거기서 정해진다.
    const first = make(4, 1, [0, 0, 5, 0], true); assert.deepEqual([first.width(), first.height()], [3, 1]);
    const column = make(3, 4, [0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0], true); assert.deepEqual([column.width(), column.height()], [1, 3]);
    // 소멸 뒤 bitmap/shape 버퍼는 모두 비어 있다.
    const image = make(3, 2, [0, 5, 5, 7, 0, 0]); image.destroy(); assert.deepEqual([image.bits().length, image.shape().length], [0, 0]);
});
