// 공통 postPop 몸체(004b0d30)의 현재 조회·저장 genus·진단 복귀·효과 순서를 원문 그대로 분배하는 모듈이다(cpppj `o/RawSquidPostPopParent.cpp/.h`, 10.78 전용).
// 같은 물리 슬롯을 쓰며 상위의 가상 분배나 Activate 깊이 증가를 대신하지 않는다(깊이는 호출자가 이미 증가시켰고 이 몸체가 끝에서 한 번 줄인다). 도달한 실제 하위만 요구하며 전체 사전 검사를 호출하지 않는다.
// 순서: suppressed(메타맵)가 참이면 raw/TYPE/판본을 보기 전에 깊이만 줄이고 끝(0 에서 줄이면 32비트로 감김) → 슬롯의 첫 TYPE genus(flags2)를 읽어 영역 군(0x50444208)이고 그래프가 비활성이면 그래프 단계 전체를 건너뜀(그래프 활성을 다시 조회하지 않음) →
//   영역 군이고 flags&3 이 있고 noGraph(0x2000000)가 아니면 region(sid) → 그래프가 활성이면: (flags&0x203)==0 이거나 noGraph 일 때는 현재 TYPE 이 표면(flags1 0x800)이고 flags&1 이며 noGraph 이면 resetGraph,
//   그 밖에는 flags&0x800 이면 진단(!(popFlags&pfDESTROY_GRAPH), Squid.cpp 0x80b; checking 무관, 정상 복귀 가능) 후 현재 TYPE 이 표면이면 add → 저장 genus(flags2)를 따로 저장한 뒤
//   flags&1 이고 현재 extra(+40)에 비트 1/8 이 없으면 장부: cost → 현재 TYPE 이 공급원(flags1 0x10000000)이면 provider → 저장 genus 가 공장 군(0x4200)이면 ownerFactory(현재 owner, sid), factory(sid) → aiNotice(현재 owner, sid)
//   → 저장한 owner 를 먼저 읽은 뒤 현재 로컬 owner 를 읽어 같으면 localCount(현재 TYPE) → globalCount(현재 TYPE) → recheckSelection → 깊이 감소.
// 각 하위 호출 뒤에 슬롯의 TYPE/owner/extra 와 전역을 다시 읽는다. 하위 실패 때 이미 일어난 효과와 증가된 깊이는 되돌리지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E24: 공통 postPop 부모 이식으로 추가)

// 영역 군 genus 비트와 그래프 억제 비트이다. (2026-11-08 WB04-E24)
const POST_REGION = 0x50444208;
const POST_NO_GRAPH = 0x2000000;

// 도달한 장치만 요구한다. (2026-11-08 WB04-E24)
function need(device) {
    if (typeof device !== 'function') throw new Error('공통 후처리의 선택된 실제 장치 미연결');
    return device;
}

// pool 은 10.78 SidPool, state 는 { depth }(읽고 쓰는 u32), hooks = { suppressed, graphsEnabled, type(number) → { flags1, flags2 }, region, add, resetGraph, assertion(식, 파일, 줄), cost, provider, factory,
// ownerFactory(owner, sid), aiNotice(owner, sid), localOwner, localCount(type), globalCount(type), recheckSelection } 이다. (2026-11-08 WB04-E24)
export function runSquidPostPopParent(pool, sid, flags, state, hooks) {
    const decrease = () => { state.depth = ((state.depth >>> 0) - 1) >>> 0; };
    if (need(hooks.suppressed)()) { decrease(); return; }
    if (pool?.edition !== 'patch1078') throw new TypeError('현재 공통 후처리는 10.78 전용');
    // 슬롯은 callback 뒤에도 같은 물리를 다시 읽는다.
    const raw = pool.physical(sid);
    const firstGenus = need(hooks.type)(raw[10]).flags2 >>> 0;
    // 원문의 LAB_004b0e4c 직행을 보존한다.
    const skipGraph = (firstGenus & POST_REGION) !== 0 && !need(hooks.graphsEnabled)();
    if (!skipGraph) {
        if ((firstGenus & POST_REGION) !== 0 && (flags & 3) !== 0 && (flags & POST_NO_GRAPH) === 0) need(hooks.region)(sid);
        if (need(hooks.graphsEnabled)()) {
            if ((flags & 0x203) === 0 || (flags & POST_NO_GRAPH) !== 0) {
                if (((need(hooks.type)(raw[10]).flags1 >>> 0) & 0x800) !== 0 && (flags & 1) !== 0 && (flags & POST_NO_GRAPH) !== 0) need(hooks.resetGraph)(sid);
            } else {
                if ((flags & 0x800) !== 0) need(hooks.assertion)('!(popFlags&pfDESTROY_GRAPH)', '\\Ns\\O\\Squid.cpp', 0x80b);
                if (((need(hooks.type)(raw[10]).flags1 >>> 0) & 0x800) !== 0) need(hooks.add)(sid);
            }
        }
    }
    const savedGenus = need(hooks.type)(raw[10]).flags2 >>> 0;
    if ((flags & 1) !== 0 && (raw[40] & 9) === 0) {
        need(hooks.cost)(sid);
        if (((need(hooks.type)(raw[10]).flags1 >>> 0) & 0x10000000) !== 0) need(hooks.provider)(sid);
        if ((savedGenus & 0x4200) !== 0) {
            need(hooks.ownerFactory)(raw[34], sid);
            need(hooks.factory)(sid);
        }
        need(hooks.aiNotice)(raw[34], sid);
        // 004b0f5b 의 owner 읽기를 004b0f5f 의 로컬 owner 읽기보다 먼저 수행한다.
        const savedOwner = raw[34];
        const localOwner = need(hooks.localOwner)();
        if (savedOwner === (localOwner >>> 0)) need(hooks.localCount)(raw[10]);
        need(hooks.globalCount)(raw[10]);
        need(hooks.recheckSelection)();
    }
    decrease();
}
