// 장식·가장자리 팜의 Regular raw-decoration-regular.js(runDecorationRegular)를 원본 10.78 관측 630입력과 cpppj `DecorationRegularTests` 계약으로 대조한다(WB04-E527).
//  - 원본 관측 decoration-regular-x86.tsv 630행: 입력 7 DWORD(사건·count·payload 비트·서버 권한·최대 HP·현재 HP·최대 HP 효과의 가상 변경)에서 반환 비트·선택한 효과 순서(M 최대 HP·H/h 피격·D/d 삭제·B damageable)·
//    최종 슬롯 50BYTE 전체를 대조한다. payload 는 NaN 페이로드(0x7fc01234)와 −0 을 포함한다.
//  - 계약: 권한 없는 종료와 비0 사건은 최대 HP/삭제/피격을 앞당기지 않고, 서버 권한에서 미연결 삭제는 명시 거부하며, CD 10.72 풀은 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E527)
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { runDecorationRegular } from '../www/core/raw-decoration-regular.js';

// 원본 관측 630행이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E527)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'decoration-regular-x86.tsv'));

// float 단정도 값의 비트를 읽고 쓰는 변환기이다. (2026-11-15 WB04-E527)
const view = new DataView(new ArrayBuffer(4));
const floatToBits = (value) => { view.setFloat32(0, value, true); return view.getUint32(0, true); };
const bitsToFloat = (bits) => { view.setUint32(0, bits >>> 0, true); return view.getFloat32(0, true); };
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 부모 50 의 같은 물리 슬롯에 원본 입력 바이트를 채우고 선택된 효과를 기록하는 장면이다(cpppj DecorationScene). (2026-11-15 WB04-E527)
class DecorationScene {
    constructor() {
        this.pool = new SidPool(24000, { server: true });
        // 부모 SID 50 까지 실제 할당해 같은 물리 슬롯을 쓴다.
        for (let sid = 5; sid <= 50; sid += 1) assert.equal(this.pool.allocate(2), sid);
        // 입력이 상태 BYTE 까지 0xa5 로 채우므로(할당 확인이 실패) 할당 직후의 살아 있는 뷰를 한 번만 얻어 계속 쓴다.
        this.view = this.pool.allocated(50);
        this.boss = false;
        this.row = [];
        this.trace = '';
    }

    // 부모 50 의 실제 슬롯이다. 슬롯 사본이 아니라 살아 있는 할당 뷰를 쓴다.
    raw() { return this.view; }

    // 원본 입력 바이트만 재배치한다: 0xa5 채움 + VT 0x11010000 + TYPE 100 + 현재 HP.
    prepare(input) {
        this.row = input.split(',').map(Number);
        this.raw().fill(0xa5);
        put(this.raw(), 0, 0x11010000);
        this.raw()[10] = 100;
        put(this.raw(), 26, this.row[5]);
        this.boss = this.row[3] !== 0;
        this.trace = '';
    }

    // 현재 효과 문자열을 원문 관찰 형식으로 보존한다.
    event(value) { this.trace = this.trace ? `${this.trace};${value}` : value; }

    // 원본과 같은 큰 경계만 제공하며 HP 감소 판단은 대체하지 않는다.
    hooks() {
        return {
            maxHitPoints: (sid) => {
                assert.equal(sid, 50);
                this.event('M:50');
                if (this.row[6]) {
                    put(this.raw(), 0, 0x11020000);
                    put(this.raw(), 26, 77);
                }
                return this.row[4] | 0;
            },
            hitPoints: (sid, amount, owner, attacker) => {
                assert.equal(sid, 50);
                this.event(`${get(this.raw(), 0) === 0x11020000 ? 'h' : 'H'}:${amount >>> 0}:${owner}:${attacker}`);
            },
            destroy: (sid, flags) => {
                assert.equal(sid, 50);
                this.event(`${get(this.raw(), 0) === 0x11020000 ? 'd' : 'D'}:${flags}`);
            },
            damageable: (sid, event, count, payload) => {
                assert.equal(sid, 50);
                this.event(`B:${event}:${count}:${floatToBits(payload)}`);
                return 0.375;
            },
        };
    }
}

test('장식 Regular: 원본 10.78 630입력의 반환·효과 순서·최종 슬롯 50BYTE 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, 630);
    const scene = new DecorationScene();
    let count = 0;
    // 모든 독립 입력의 원문 효과와 최종 50BYTE 를 비교한다.
    for (const fields of rows) {
        assert.equal(fields.length, 4);
        scene.prepare(fields[0]);
        const result = runDecorationRegular(scene.pool, scene.boss, 50, scene.row[0], scene.row[1], bitsToFloat(scene.row[2]), scene.hooks());
        const trace = scene.trace === '' ? '-' : scene.trace;
        assert.equal(floatToBits(result), Number(fields[1]), fields[0]);
        assert.equal(trace, fields[2], fields[0]);
        assert.equal(hex(scene.raw()), fields[3], fields[0]);
        count += 1;
    }
    assert.equal(count, 630);
});

test('장식 Regular 계약: 권한 없는 종료와 비0 사건은 최대 HP/삭제/피격을 앞당기지 않고 미연결 삭제·CD 풀은 거부한다', () => {
    const pool = new SidPool(24000, { server: true });
    // 권한이 없으면 count 3 이상의 종료는 어떤 효과도 요구하지 않고 −1 을 반환한다.
    assert.equal(runDecorationRegular(pool, false, 0, 0, 3, 9, {}), -1);
    // 비0 사건은 damageable 만 요구한다.
    assert.equal(runDecorationRegular(pool, false, 0, 1, 0, 9, { damageable: () => 0.125 }), 0.125);
    // 서버 권한에서 삭제 하위가 없으면 명시 거부한다. boss 는 함수여도 호출 시점의 현재 값을 읽는다.
    assert.throws(() => runDecorationRegular(pool, true, 0, 0, 3, 9, { damageable: () => 0.125 }), /선택된 실제 하위 미연결/);
    let current = false;
    assert.equal(runDecorationRegular(pool, () => current, 0, 0, 5, 9, {}), -1);
    current = true;
    const destroyed = [];
    assert.equal(runDecorationRegular(pool, () => current, 7, 0, 5, 9, { destroy: (sid, flags) => { destroyed.push([sid, flags]); } }), -1);
    assert.deepEqual(destroyed, [[7, 0]]);
    // signed count: 0xffffffff 는 −1 이라 종료가 아니고 최대 HP 를 요구한다.
    assert.throws(() => runDecorationRegular(pool, true, 0, 0, 0xffffffff, 9, { damageable: () => 0 }), /선택된 실제 하위 미연결/);
    // 10.72 풀은 거부한다.
    assert.throws(() => runDecorationRegular({ edition: 'cd1072' }, true, 0, 0, 3, 9, {}), TypeError);
});
