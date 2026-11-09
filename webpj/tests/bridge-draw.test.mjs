// 다리 캡처의 실제 가상 몸체 RawBridgeDraw(기본 프레임 getter·기본 0 수명 숫자)를 실제 ShapeToBuffer 위에서 원본 10.78 관측으로 대조한다
// (cpppj `BridgeCollapseTests.cpp` 의 BridgeCollapse_NativeWrapperAndAllOwnerFrames 이식, WB04-E245).
//  - bridge-collapse-x86.tsv: wrapper 3행(종류 2)과 보호 60프레임 × 소유자 0/9색 캡처 600행(extra 0). bridge-extra-capture-x86.tsv: 실제 extra 2 인 600행.
//    각 행은 raw 슬롯 50(다리 vtable·TYPE 82·소유자·프레임·extra)과 표준 소유자 색표 입력만 설치하고, 캡처 메타데이터 16바이트와 전체 픽셀을 비교한다. 기대 열은 비교에만 쓴다.
//  - 거부·경계 계약: 다른 가상 표 거부, 프레임 DWORD 의 signed 해석, lifeNumbers 비0 의 미복원 거부, 10.78 이 아닌 풀 거부.
// TYPE 기준점은 원문 입력대로 0,0 이다(웹 TYPE 레코드는 기준점 0). 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E245)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { buildStandardOwnerColorMaps } from '../www/assets/owner-colors.js';
import { ShrapnelKind } from '../www/core/raw-shrapnel-frame.js';
import { RawBridgeDraw, createBridgeDrawState } from '../www/core/raw-bridge-draw.js';
import { RawShapeToBuffer, createShapeToBufferState } from '../www/render/raw-shape-to-buffer.js';
import { REPO_ROOT } from './helpers.mjs';
import { put, readRows } from './raw-support.mjs';
import { clientPool, hexOrDash as hex, realCaptureAssets } from './capture-assets-support.mjs';

// 원본 x86 관측이다. 파일이 없으면 검사를 건너뛴다. (2026-11-10 WB04-E245)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const collapseRows = readRows(path.join(FIXTURES, 'bridge-collapse-x86.tsv'));
const extraRows = readRows(path.join(FIXTURES, 'bridge-extra-capture-x86.tsv'));

// 다리 타입 번호와 원본 다리 클래스 가상 표 기록값이다. 기록값은 모듈 상수가 아닌 원문 리터럴로 설치해 상수 오타를 잡는다. (2026-11-10 WB04-E245)
const BRIDGE = 82;
const BRIDGE_VTABLE = 0x5034c8;

// 16진 문자열 → 바이트 배열이다(홀수 길이 거부). (2026-11-10 WB04-E245)
function decode(text) {
    assert.equal(text.length % 2, 0);
    return Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16));
}

// 예외가 발생하는지 본다. (2026-11-10 WB04-E245)
function throws(action, kind = Error) {
    try { action(); } catch (error) { return error instanceof kind; }
    return false;
}

test('RawBridgeDraw: 다리 wrapper 3행과 보호 60프레임 × 소유자의 캡처 1200행이 원본과 같다', { skip: (!collapseRows || !extraRows) && '원본 fixture 없음' }, () => {
    const assets = realCaptureAssets();
    assert.equal(assets.shapes[BRIDGE].frameCount, 60);
    // 동결 원문의 명시 TYPE 기준점 0,0 입력이다(웹 레코드는 기준점 0).
    const types = assets.types.slice();
    types[BRIDGE] = { ...types[BRIDGE], hotspotX: 0, hotspotY: 0 };
    const pool = clientPool();
    const drawState = createBridgeDrawState();
    const bridgeDraw = new RawBridgeDraw(pool, drawState);
    // wrapper 행은 앞에, 별도로 동결한 실제 extra 2 의 600행은 'E' 표식을 붙여 뒤에 읽는다.
    const rows = [...collapseRows, ...extraRows.map((row) => ['E', ...row])];
    const maps = buildStandardOwnerColorMaps(BRIDGE, BRIDGE);
    const tokens = new Uint32Array(188);
    tokens[BRIDGE] = 1;
    let captures = 0;
    let wrappers = 0;
    // 같은 명시 입력을 설치한 뒤 저장된 출력만 기대값으로 쓴다.
    for (const row of rows) {
        if (row[0] === 'W') {
            assert.equal(row.length, 3);
            assert.equal(ShrapnelKind.Variant2, Number(row[2]));
            wrappers += 1;
            continue;
        }
        assert.equal(row.length, 6);
        assert.ok(row[0] === 'C' || row[0] === 'E');
        const owner = Number(row[2]);
        const color = Number(row[3]);
        const raw = pool.allocated(50);
        raw.fill(0);
        put(raw, 0, BRIDGE_VTABLE);
        raw[10] = BRIDGE;
        raw[40] = row[0] === 'E' ? 2 : 0;
        raw[34] = owner;
        put(raw, 36, Number(row[1]));
        let selected = 0;
        const state = createShapeToBufferState({
            addTypeHotspot: true,
            ownerTypeMaps: tokens,
            // 소유자 색표는 소유자가 0 이 아닐 때만 한 번 고른다.
            ownerColors: (type, currentOwner) => {
                assert.deepEqual([type, currentOwner], [BRIDGE, owner]);
                selected += 1;
                return maps[color];
            },
            mainFrame: (sid) => bridgeDraw.mainFrame(sid),
            additional: (sid, image, x, y) => bridgeDraw.additional(sid, x, y),
        });
        const image = new RawShapeToBuffer(pool, types, assets.shapes, assets.database, state).capture(50);
        const metadata = Buffer.alloc(16);
        [image.width, image.height, image.hotspotX, image.hotspotY].forEach((value, index) => metadata.writeInt32LE(value, index * 4));
        assert.equal(hex(metadata), row[4], `메타데이터 ${row[0]} frame ${row[1]} owner ${owner}`);
        assert.equal(hex(image.pixels), row[5], `픽셀 ${row[0]} frame ${row[1]} owner ${owner}`);
        assert.equal(selected, owner ? 1 : 0);
        captures += 1;
    }
    assert.deepEqual([captures, wrappers], [1200, 3]);
});

// 기본 프레임 getter 는 다리 가상 표만 받고 프레임 DWORD 를 signed 로 돌려주며, 수명 숫자가 켜지면 빈 성공으로 넘기지 않는다.
test('RawBridgeDraw: 가상 표·signed 프레임·수명 숫자·판본 계약', () => {
    const pool = clientPool();
    const state = createBridgeDrawState();
    assert.deepEqual(state, { lifeNumbers: 0 });
    const draw = new RawBridgeDraw(pool, state);
    const raw = pool.allocated(50);
    raw.fill(0);
    // 다른 가상 표(0)는 거부한다.
    assert.ok(throws(() => draw.mainFrame(50)));
    put(raw, 0, BRIDGE_VTABLE);
    put(raw, 36, 59);
    assert.equal(draw.mainFrame(50), 59);
    // 프레임 DWORD 는 signed int32 로 읽는다(ShapeToBuffer 가 -1 을 "그리지 않음"으로 쓴다).
    put(raw, 36, 0xffffffff);
    assert.equal(draw.mainFrame(50), -1);
    put(raw, 36, 0x80000000);
    assert.equal(draw.mainFrame(50), -0x80000000);
    // 현재 프레임을 캐시하지 않는다: 다음 호출은 바뀐 슬롯을 읽는다.
    put(raw, 36, 7);
    assert.equal(draw.mainFrame(50), 7);
    // 기본 0 은 인자를 읽지 않고 반환하며, 같은 전역을 바꾸면 다음 호출이 그 값을 읽어 거부한다.
    assert.doesNotThrow(() => draw.additional());
    state.lifeNumbers = 1;
    assert.ok(throws(() => draw.additional(50, 0, 0)));
    // 10.78 풀이 아니면 만들 수 없다.
    assert.ok(throws(() => new RawBridgeDraw({ edition: 'cd1072' }, state), TypeError));
    assert.ok(throws(() => new RawBridgeDraw(null, state), TypeError));
});
