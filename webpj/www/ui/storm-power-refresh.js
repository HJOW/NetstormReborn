// 현재 serial의 실제 예산(Storm Power) Gump를 원문 Push/Attach 순서로 다시 붙인다(원문 00464020/0043dad0, cpppj client/RawStormPowerRefresh.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 금액/SP/Idle/문자열을 다시 계산하지 않고 실제 예산 Gump의 재부착만 수행한다. 숫자 토큰은 같은 공통 등록표(GumpCreateState.entries)의 노드에만 해석하며
// 노드 조회(hooks.node)는 같은 저장소의 순수 주소 조회여야 한다. 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: refreshStormPowerGump(state.entries, serial, { node: (token) => nodes.get(token), push: (token) => tree.push(token), attach: (token, x, y, a, b) => tree.attach(token, x, y, a, b) }).
// (2026-11-14 WB04-E437: RawMenuControls 선행 SP 표시 재부착 이식)
import { GUMP_REGISTRY_CAPACITY } from './gump-create.js';

// 원문 노드 플래그의 dead 비트다. 이 비트가 켜진 노드는 serial 이 같아도 찾지 않는다.
const DEAD_FLAG = 0x10000;

// 같은 등록표의 생존 토큰을 현재 주소로 읽으며 누락된 장치를 해당 조회 위치에서 거부한다(logic_error→Error, out_of_range→RangeError).
function currentNode(node, token) {
    if (typeof node !== 'function') throw new Error('현재 Gump serial의 실제 주소 조회 누락');
    const value = node(token);
    if (!value) throw new RangeError('현재 Gump serial의 생존 노드 누락');
    return value;
}

// 00464020: 등록 1..999의 현재 슬롯에서 serial 이 같고 dead 가 아닌 첫 Gump 를 앞에서부터 찾아 토큰을 돌려준다. 없으면 0 이다.
// 루트 0칸은 건너뛰며 비어 있지 않은 슬롯마다 node 를 조회한다(찾기 전까지의 조회에서 노드가 없으면 RangeError). entries 가 1000칸 미만이면 TypeError 다.
export function findCurrentGumpSerial(entries, serial, node) {
    if (!entries || entries.length < GUMP_REGISTRY_CAPACITY) throw new TypeError('현재 Gump serial의 실제1000슬롯 누락');
    const wanted = serial >>> 0;
    // 루트 0칸을 뺀 각 현재 슬롯을 앞에서부터 조회한다.
    for (let index = 1; index < GUMP_REGISTRY_CAPACITY; index += 1) {
        const token = entries[index] >>> 0;
        if (!token) continue;
        const current = currentNode(node, token);
        if ((current.serial >>> 0) === wanted && !(current.flags & DEAD_FLAG)) return token;
    }
    return 0;
}

// 0043dad0: 현재 serial 조회 → 깊이 보관 → Push → 현재 좌표 Attach(0/0) → 정상 복귀 뒤 같은 깊이 복원이다.
// hooks = { node(token) → 노드|null, push(token) (원문 가상+18), attach(token, x, y, 0, 0) (원문 가상+20) }. 못 찾으면 아무 장치도 부르지 않는다.
// Push/Attach 가 던지면 깊이는 복원하지 않는다(원문은 두 가상 호출이 정상 복귀한 뒤에만 한 DWORD 를 되돌린다).
export function refreshStormPowerGump(entries, serial, hooks) {
    const token = findCurrentGumpSerial(entries, serial, hooks.node);
    if (!token) return;
    const current = currentNode(hooks.node, token);
    const depth = current.depth;
    if (typeof hooks.push !== 'function') throw new Error('SP 표시 재부착의 실제 Push 누락');
    hooks.push(token);
    if (typeof hooks.attach !== 'function') throw new Error('SP 표시 재부착의 실제 Attach 누락');
    // Push 뒤의 현재 좌표를 그대로 읽어 넘긴다(Push 가 위치를 바꾸었을 수 있다).
    hooks.attach(token, current.x, current.y, 0, 0);
    current.depth = depth;
}
