// 폭발 주문의 추가 그림과 큰 효과를 같은 World/피격/다리 몸체에 연결한다(cpppj client/RawBombBlastBindings, 225553df, WB04-E583).
// 사용: drawDevastationAdditional(pool, sid, x, y, draw)와 makeCampaignBombBlastHooks(world, hits, coordinates).

// 같은41e3d0 추가 그림을 공유하는 실제16VT다. 타입 번호나 비슷한 그림으로 자격을 넓히지 않는다.
const DEVASTATION_TABLES = new Set([0x503188, 0x5026a0, 0x502778, 0x502840, 0x502908, 0x502b70, 0x5029d0, 0x502aa8, 0x5030c0, 0x502448, 0x502370, 0x502510, 0x502ff8, 0x5025d8, 0x502e68, 0x502f30]);

// 원문41e3d0의 현재 TYPE/프레임/입력 좌표를 평문 Draw 소비자에 전달한다. free/dead/abstract 필터를 추가하지 않는다.
export function drawDevastationAdditional(pool, sid, x, y, draw) {
    const raw = pool.slot(sid); const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength); const table = view.getUint32(0, true); const frame = view.getInt32(36, true);
    if (!DEVASTATION_TABLES.has(table) || typeof draw !== 'function') throw new TypeError('Devastation 추가 그림 표/소비자 오류');
    draw(raw[10], frame, x | 0, y | 0);
}

// 세 실제 소유자의 같은 Pool을 확인하고, 현재 객체와 값 캡처한 피해 함수를 원문 순서로 연결한다.
export function makeCampaignBombBlastHooks(world, hits, coordinates) {
    const core = world.core();
    if (hits?.pool !== core.pool || coordinates?.pool !== core.pool) throw new TypeError('폭발 실제 월드 Pool 불일치');
    const damage = hits.damageHooks({}).damage; const bridge = core.bridgeEvents;
    return {
        // 이 계열502370의 가상b4는 기본 반경 getter다. 다른 표는 원문 오류다.
        radius: (parent, table) => { if ((table >>> 0) !== 0x502370) throw new TypeError('폭발 반경 가상 표 불일치'); return coordinates.defaultRadius(parent); },
        // 기존 가상98 분배를 값으로 보관한다. TYPE별 피해 몸체를 다시 구현하지 않는다.
        damage: (target, _table, amount, owner, parent) => { if (typeof damage !== 'function') throw new Error('폭발 실제 가상 피해 미연결'); damage(target, amount | 0, owner & 0xff, parent); },
        // 원문이 무시하는 bool 반환을 새 성공 관문으로 쓰지 않는다.
        destroy: (target, _table, flags) => { world.destroy(target, flags >>> 0); },
        // 실제 Dude 반응/공유 난수/충돌 효과를 같은 단일 Hit 인스턴스에서 실행한다.
        ground: (x, y) => { hits.impact.ground(Math.fround(x), Math.fround(y)); },
        // 실제421f70이 감싼 끝 칸 변환을 현재 표면 처리기에 전달한다.
        edge: (target, x, y) => { bridge.convertEnd(target, Math.fround(x), Math.fround(y)); },
    };
}
