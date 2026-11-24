// cpppj CampaignSession_ActualDropIslandSupportAndParticles와 같은 실제 받침 확정·소유자 전파·삭제·파편 수명을 검사한다. (WB04-E1316)
import test from 'node:test';
import assert from 'node:assert/strict';
import { visualSessionScene } from './campaign-visual-session-support.mjs';
import { ShrapnelVisualProcess } from '../www/core/shrapnel-visual-system.js';
import { get, bitsToFloat } from './raw-support.mjs';

// C++ EmptySessionSupport와 같이 현재 spot의 점유 하위 6비트만 읽어 빈 3×3 영역을 찾는다.
function emptySupport(core) {
    // 가장자리 발자국을 피한 행 순서로 후보를 조회한다.
    for (let y = 4; y < 248; y += 1) {
        // 행 안에서는 왼쪽부터 조회한다.
        for (let x = 4; x < 248; x += 1) {
            let empty = true;
            // 점유된 행을 찾으면 다음 후보로 넘어간다.
            for (let dy = 0; dy < 3 && empty; dy += 1) {
                // 실제 세 칸의 spot을 읽고 지형을 합성하지 않는다.
                for (let dx = 0; dx < 3; dx += 1) if (core.spots[(y + dy) * 256 + x + dx] & 0x3f) { empty = false; break; }
            }
            if (empty) return { x, y };
        }
    }
    throw new Error('보호 Session의 빈 3×3 받침 위치 부재');
}

test('CampaignSession: 실제 섬 투하가 받침 11개와 파편 10개 수명을 잇는다', () => {
    const lost = [], notices = [];
    const { session: s } = visualSessionScene({ unitLost: (sid) => lost.push(sid), construction: {
        configInt: () => 1, wallSeconds: () => 100, refresh: () => notices.push('refresh'),
        negativeQuantity: () => { throw new Error('음수 수량'); }, boundsAssertion: () => { throw new Error('범위 진단'); },
    } });
    try {
        assert.throws(() => s.confirmSupport(4, 4), /설치 전/);
        s.load(); const c = s.core(), point = emptySupport(c);
        assert.equal(s.confirmSupport(point.x, point.y), true);
        const packet = s.construction().lastPacket(); assert.equal(packet.length, 45); assert.equal(packet[26], 9);
        let right = -10000, bottom = -10000;
        // 확정된 실제 9칸에서 오른쪽 아래 기준점을 읽는다.
        for (let index = 0; index < 9; index += 1) {
            const sid = get(packet, 27 + index * 2, 2), raw = c.pool.slot(sid);
            right = Math.max(right, bitsToFloat(get(raw, 14))); bottom = Math.max(bottom, bitsToFloat(get(raw, 18)));
        }
        s.world().setSupportOwner(right, bottom, 1);
        const family = [], counts = new Map([[157, 0], [94, 0], [95, 0]]);
        // 이번 확정/소유자 전파가 만든 실제 부품만 센다.
        for (const { sid, object } of s.world().objects()) {
            if (object.owner !== 1 || object.x < right - 2 || object.x > right || object.y < bottom - 2 || object.y > bottom || !counts.has(object.type)) continue;
            counts.set(object.type, counts.get(object.type) + 1); family.push(sid);
        }
        assert.deepEqual([...counts.values()], [9, 1, 1]);
        const source = s.world().create(75, 0); assert.ok(source >= c.pool.layout.serverFirst); assert.equal(c.processState.server, true);
        c.owner.set(source, 1); s.world().pop(source, right, bottom, 0);
        const sp = s.sp(1); assert.equal(s.world().destroy(source, 0), true); assert.equal(c.pool.slot(source)[11] & 3, 3);
        // 소유자 전파된 부품은 모두 실제 삭제되어야 한다.
        for (const sid of family) assert.equal(c.pool.slot(sid)[11] & 3, 3);
        let particles = 0;
        // 별도 기록이 아닌 현재 표시 등록표와 Host의 실제 파편 파생 객체를 센다.
        for (const token of s.particles().rendering.slots) {
            const fields = s.particles().rendering.lookup(token);
            if (fields && c.processes.fromForm(fields.form & 0xffff) instanceof ShrapnelVisualProcess) particles += 1;
        }
        assert.equal(particles, 10); assert.equal(s.sp(1), sp); assert.ok(lost.includes(source));
        assert.deepEqual([c.destroy.preDepth, c.destroy.postDepth, c.bookkeeping.depth], [0, 0, 0]);
    } finally { s.dispose(); }
    assert.throws(() => s.construction(), /반납/);
});
