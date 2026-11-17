// 같은 게임 난수를 소비해 Money의 float32 네 BYTE를 기록한다(225553df, WB04-E633).
// 사용: saveFortMoney(stream, random, write). 현재 SP 저장소를 읽거나 초기화하지 않는다.
// 선택된 장치만 실제 도달 위치에서 요구해 앞선 Create/난수 소비를 보존한다.
function need(device) { if (typeof device !== 'function') throw new Error('Fort Money 저장 실제 경계 미연결'); return device; }
// 004832c0: Next(1010)의 signed 정수에 333.0f를 더하고 같은 stream에 일반 쓰기를 한다.
export function saveFortMoney(stream, random, write) {
    const value = Math.fround((random.next(0x3f2) | 0) + 333);
    const bytes = new Uint8Array(4); new DataView(bytes.buffer).setFloat32(0, value, true);
    need(write)(stream, bytes);
}
// 004beec0: slot을 clamp하지 않고 현재 manager의 새 Money stream을 한 번만 만든다.
export function snapshotFortMoney(slot, random, hooks) {
    const manager = need(hooks.manager)(slot | 0); const stream = need(hooks.create)(manager, 'Money');
    saveFortMoney(stream, random, hooks.write);
}
