// 현재 Client 전역과 고정 Player 물리를 구별해 CoreData를 저장한다(225553df, WB04-E633).
// 사용: writeFortCoreData(slot, globals, hooks). globals.seed()는 같은 DWORD의 { value } 참조를 반환한다.
// 진단 복귀 뒤 현재 전역을 다시 읽는 원본 Template.cpp 행 번호다.
export const FORT_CORE_WRITE_ASSERTION = Object.freeze({ missingTemplate: 0x45f, inMetamap: 0x460, wrongPlayer: 0x467 });
// 선택된 하위 경계만 실제 도달 위치에서 요구한다.
function need(device) { if (typeof device !== 'function') throw new Error('CoreWrite의 선택된 실제 장치 누락'); return device; }
// 보관한 Player 할당의 실제 지정 범위만 빌린다.
function physical(bytes, offset, width) { if (!(bytes instanceof Uint8Array) || offset > bytes.length || width > bytes.length - offset) throw new RangeError('CoreWrite의 고정Player 물리 범위'); return bytes.subarray(offset, offset + width); }
// 고정 Player의 현재 unsigned BYTE/WORD를 하위 호출 직전에 다시 읽는다.
function unsigned(bytes, offset, width) { const field = physical(bytes, offset, width); return width === 1 ? field[0] : new DataView(field.buffer, field.byteOffset, 2).getUint16(0, true); }
// 004beb30: 진단→post/editor 자연 반환→Create→고정 Player/현재 owner→refresh→일반/seed/SP 쓰기다.
export function writeFortCoreData(slot, globals, hooks) {
    slot |= 0; if (!need(hooks.manager)(slot)) need(hooks.assertion)(FORT_CORE_WRITE_ASSERTION.missingTemplate);
    if (need(globals.metamap)()) need(hooks.assertion)(FORT_CORE_WRITE_ASSERTION.inMetamap);
    if (need(globals.missionPost)()) return; if (need(globals.editor)()) return;
    const manager = need(hooks.manager)(slot), stream = need(hooks.create)(manager, 'CoreData');
    const player = slot >= 1 && slot <= 8 ? slot : 0, players = need(globals.players)(), offset = player * 180;
    if (player !== need(globals.playerOwner)()) need(hooks.assertion)(FORT_CORE_WRITE_ASSERTION.wrongPlayer);
    need(hooks.refresh)(); const write = need(hooks.write); write(stream, physical(players, offset + 0x6c, 10));
    const seed = need(globals.seed)(); if (seed == null || !('value' in Object(seed))) throw new Error('CoreWrite의 실제 seed DWORD 참조 누락');
    if ((seed.value & 0xffff) === 0) {
        need(hooks.prepareRandom)(); let low; // 준비가 seed를 바꿔도 첫 Next를 생략하지 않는다.
        do { // 반환 low16이 비영일 때까지 같은 난수만 소비하고 seed 상위 WORD를 보존한다.
            low = need(hooks.nextRandom)(0x10000) & 0xffff; seed.value = ((seed.value & 0xffff0000) | low) >>> 0;
        } while (!low);
    }
    const local = seed.value >>> 0, bytes = new Uint8Array([local & 255, (local >>> 8) & 255]); need(hooks.write)(stream, bytes);
    const rank = unsigned(players, offset + 0x70, 1); need(hooks.sp)(stream, Math.fround(rank));
    const first = unsigned(players, offset + 0x6c, 2); need(hooks.sp)(stream, Math.fround(first));
    const second = unsigned(players, offset + 0x6e, 2); need(hooks.sp)(stream, Math.fround(second));
}
