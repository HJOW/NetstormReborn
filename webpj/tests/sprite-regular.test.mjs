// 일반 Sprite 정기 raw-sprite-regular.js(runSpriteRegular)를 원본 10.78 관측 147행과 cpppj `SpriteRefresh_IndependentPe225Inputs` 의 정기 부분 계약으로 대조한다(WB04-E529).
//  - 원본 관측 sprite-refresh-x86.tsv 225행 중 정기(모드 R) 147행: 현재 프레임 DWORD·frameCount·payload 비트에서 IDIV 오류 여부·BYTE 35 보존·최종 프레임 DWORD·반환 payload 비트·Frame.Set 의 옛/새 표시 두 번 중
//    두 번째 표시 시점의 프레임(`S:<signed 프레임>`) 순서를 대조한다. 같은 파일의 표시(모드 D) 78행은 `NativeMetaSpriteRefresh`(메타 Sprite 표시, 웹 범위 밖 MP/메타 단위)의 몫이라 소비하지 않는다.
//  - 계약: 다른 풀/TYPE 표/지정기 거부, TYPE 범위, frameCount 0 은 어떤 지정도 하기 전에 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E529)
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { runSpriteRegular } from '../www/core/raw-sprite-regular.js';

// 원본 관측 225행이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E529)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'sprite-refresh-x86.tsv'));

// 단정도 float 와 비트 변환이다. (2026-11-15 WB04-E529)
const view = new DataView(new ArrayBuffer(4));
const bitsToFloat = (bits) => { view.setUint32(0, bits >>> 0, true); return view.getFloat32(0, true); };
const floatToBits = (value) => { view.setFloat32(0, value, true); return view.getUint32(0, true); };

// 일반 Sprite 부모와 같은 풀의 Frame 지정기를 만들고 표시 두 번 중 두 번째의 프레임만 기록한다(원문 전체 Set 경계 한 번으로 정규화). (2026-11-15 WB04-E529)
function makeScene(current, divisor) {
    const pool = new SidPool(24000, { server: false });
    const sid = pool.allocate(2);
    const raw = pool.allocated(sid);
    raw.fill(0);
    raw[10] = 161;
    raw[35] = 0x77;
    put(raw, 36, current);
    const types = Array.from({ length: 188 }, () => ({ flags2: 0, frameCount: 0 }));
    types[161].frameCount = divisor | 0;
    types[161].flags2 = 4;
    const events = [];
    let updates = 0;
    const hooks = {
        frameSize: () => { throw new Error('Sprite 명시 Frame.Set 경계의 프레임 크기 호출'); },
        unpop: () => { throw new Error('Sprite 명시 Frame.Set 경계의 Unpop 호출'); },
        pop: () => { throw new Error('Sprite 명시 Frame.Set 경계의 Pop 호출'); },
        update: (parent, flags) => {
            assert.ok(parent === sid && flags === 0);
            updates += 1;
            if (updates === 2) events.push(`S:${get(raw, 36) | 0}`);
        },
    };
    return { pool, sid, raw, types, events, setter: new SquidFrame(pool, types, hooks) };
}

test('Sprite 정기: 원본 10.78 관측 147행의 IDIV 오류·BYTE35·프레임·반환 payload·지정 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, 225);
    let regular = 0;
    for (const row of rows) {
        if (row[0] === 'D') continue;
        const scene = makeScene(Number(row[11]), Number(row[12]));
        let error = false;
        let result = 0;
        try {
            result = runSpriteRegular(scene.pool, scene.types, scene.setter, scene.sid, bitsToFloat(Number(row[13])));
        } catch (caught) {
            assert.ok(caught instanceof RangeError, String(caught));
            error = true;
        }
        const label = `행 ${regular}: ${row.slice(11, 14).join(',')}`;
        assert.equal(error, Number(row[14]) !== 0, label);
        assert.equal(scene.raw[35], Number(row[15]), label);
        assert.equal(get(scene.raw, 36), Number(row[16]), label);
        assert.equal(scene.events.length === 0 ? '-' : scene.events.join(';'), row[18], label);
        if (!error) assert.equal(floatToBits(result), Number(row[17]) >>> 0, label);
        regular += 1;
    }
    assert.equal(regular, 147);
});

test('Sprite 정기 계약: 다른 풀·잘못된 TYPE 표·지정기 거부와 frameCount 0 의 사전 거부', () => {
    const scene = makeScene(5, 8);
    const other = makeScene(5, 8);
    assert.throws(() => runSpriteRegular(scene.pool, scene.types, other.setter, scene.sid, 1), TypeError);
    assert.throws(() => runSpriteRegular(scene.pool, scene.types.slice(1), scene.setter, scene.sid, 1), TypeError);
    assert.throws(() => runSpriteRegular({ edition: 'cd1072' }, scene.types, scene.setter, scene.sid, 1), TypeError);
    // 정상: (5 + 1) % 8 = 6 을 지정하고 payload 를 그대로 반환한다.
    assert.equal(runSpriteRegular(scene.pool, scene.types, scene.setter, scene.sid, 0.5), 0.5);
    assert.equal(get(scene.raw, 36), 6);
    // frameCount 0 은 지정하기 전에 거부하고 프레임은 그대로이다.
    const zero = makeScene(5, 0);
    assert.throws(() => runSpriteRegular(zero.pool, zero.types, zero.setter, zero.sid, 1), RangeError);
    assert.equal(get(zero.raw, 36), 5);
    assert.deepEqual(zero.events, []);
    // 음수 나머지는 부호 있는 IDIV 처럼 피제수의 부호를 따른다: 프레임 −3 → next −2, divisor 3 → −2.
    const negative = makeScene(0xfffffffd, 3);
    runSpriteRegular(negative.pool, negative.types, negative.setter, negative.sid, 1);
    assert.equal(get(negative.raw, 36) | 0, -2);
});
