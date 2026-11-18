// 31h의 객체 조회·소유자 변경·삭제를 현재 Session 월드에 전달한다.
// 사용: executeDeveloperObjects(text, owner, trace) → 처리한 명령이면 true, 다른 명령이면 false.
import { parseDeveloperCommand, readDeveloperIntegers } from './raw-campaign-developer-settings.js';
import { setRawSquidHitPoints } from './raw-squid-word.js';

// 원본 signed 입력을 현재 Pool의 물리 SID로 검사한다.
function sid(pool, value) {
    if (!Number.isInteger(value) || value < 0 || value > 65535 || value >= pool.capacity) throw new RangeError('개발자 명령의 현재 SID 물리 범위');
    return value;
}

// SID 0은 건너뛰고 나머지는 현재 좌표를 실제 화면 강조 장치로 전달한다.
function highlight(owner, target) {
    if (!target) return;
    const raw = owner.core.pool.slot(target), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
    if (typeof owner.services.developerHighlight !== 'function') throw new Error('개발자 명령의 실제 화면 강조 미연결');
    owner.services.developerHighlight(view.getFloat32(14, true), view.getFloat32(18, true));
}

// 원본 서버·클라이언트 분기와 외부 효과 순서를 그대로 실행한다. (2026-11-18 WB04-E744)
export function executeDeveloperObjects(text, owner, trace) {
    const { key, rest } = parseDeveloperCommand(text), core = owner.core;
    // 원본 수준29 로그는 도달한 위치에서만 추적 장치를 요구한다.
    const log = (message) => { if (typeof trace !== 'function') throw new Error('개발자 명령의 실제 추적 장치 미연결'); trace(29, message); };
    if (key === 'dump' || key === 'd') {
        const value = readDeveloperIntegers(rest, 1)[0] ?? 0;
        if (!value) return true;
        const target = sid(core.pool, value); highlight(owner, target);
        if (typeof owner.services.developerTraceObject !== 'function') throw new Error('개발자 dump의 실제44dab0 미연결');
        owner.services.developerTraceObject(target); return true;
    }
    if (key === 'setowner' || key === 'so') {
        const values = readDeveloperIntegers(rest, 2);
        if (values.length !== 2) throw new TypeError('개발자 소유자 명령의 원문 미정 정수 입력');
        const target = sid(core.pool, values[0]), player = (values[1] << 24) >> 24;
        if (core.processState.server) {
            owner.world.unpop(target, 0); owner.ownership.setOwner(target, player >>> 0); owner.world.repop(target, 8); return true;
        }
        highlight(owner, target); log(`requested owner on ${values[0]} set to ${player}\n`); return true;
    }
    if (key === 'setlocalhitpoints' || key === 'slhp') {
        const values = readDeveloperIntegers(rest, 2);
        if (values.length !== 2) { log('bad command.\n'); return true; }
        const target = sid(core.pool, values[0]);
        owner.world.unpop(target, 0); setRawSquidHitPoints(core.pool, target, values[1]); owner.world.repop(target, 0); highlight(owner, target);
        const raw = core.pool.slot(target), current = new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getInt32(26, true);
        log(`sid ${values[0]} changed to ${current} hp.  (max for type = ${owner.hp.maxHitPoints(target)})\n`); return true;
    }
    if (key === 'setabstract' || key === 'sa') {
        const values = readDeveloperIntegers(rest, 2);
        if (values.length !== 2) { log('bad command.\n'); return true; }
        const target = sid(core.pool, values[0]); owner.world.unpop(target, 0);
        let raw = core.pool.allocated(target);
        if (!(raw[11] & 4)) owner.world.storage.assertDisplay('isVoid()', 'c:\\ns\\o\\squid.h', 0x282);
        raw = core.pool.allocated(target); raw[40] = (raw[40] ^ ((raw[40] ^ values[1]) & 1)) & 0xff;
        owner.world.repop(target, 0); return true;
    }
    if (key === 'applydamage' || key === 'ad') {
        const values = readDeveloperIntegers(rest, 2);
        if (values.length !== 2 || values[0] < 0) { log('bad command\n'); return true; }
        if (typeof owner.developerDamage !== 'function') throw new Error('개발자 피해의 실제 가상98 미연결');
        owner.developerDamage(sid(core.pool, values[0]), values[1] >>> 0, 0, 0); return true;
    }
    if (key === 'destroy' || key === 'de') {
        if (!core.processState.server) return true;
        const values = readDeveloperIntegers(rest, 1);
        if (values.length !== 1) return true;
        const target = sid(core.pool, values[0]);
        if (!(core.pool.slot(target)[11] & 1)) owner.world.destroy(target, 0);
        return true;
    }
    return false;
}
