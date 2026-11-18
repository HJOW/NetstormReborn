// 원본 10.78 43ef10의 다리 공급·생산 재고 및 비용 변경을 현재 내용물과 두 시계에 적용한다.
// 사용: new RawProductionMaintenance(pool, typeCount, finder, globals, panel, reads, hooks).run(). Session과 UI 연결은 별도다.
import { RawContainedFinder } from './raw-contained-finder.js';

// 재고 옵션 0~2의 원본 벽시각 간격이다.
const STOCK_INTERVALS = Object.freeze([30, 15, 8]);
// 비정렬 원본 WORD 또는 DWORD만 읽는다.
function read(raw, offset, width = 2) { const view = new DataView(raw.buffer, raw.byteOffset + offset, width); return width === 2 ? view.getUint16(0, true) : view.getUint32(0, true); }
// 원본 _ftol의 signed64 절삭 뒤 하위 signed DWORD를 반환한다.
function truncate(value) {
    if (!Number.isFinite(value) || value >= 9223372036854775808 || value < -9223372036854775808) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 패널보다 오래 사는 원본 생산 전역과 별도의 테스트 전용 다리 순서이다.
export class ProductionMaintenanceGlobals {
    // TYPE별 재고 벽시각 종료값이다.
    stockUntil = [];
    // 다리 공급 순번과 재고 활성화 여부다.
    sequence = 0; stockEnabled = 1;
    // 빠른 다리 옵션과 강제 패턴, 현재 다리 TYPE이다.
    quickBridges = 0; forcedPattern = 0; bridgeType = 82;
    // 생산 루트·자금 식별자와 현재 생산 정책이다.
    rootSerial = 0; budgetSerial = 0; policyFirst = 0; policyByte = 0;
    // 배경 TYPE·프레임과 생산 패널 프레임, 채팅 알림 식별자다.
    backgroundType = 0; backgroundFrame = 0; panelFrame = 2; chatAlertSerial = 0;
    // 재고 상한과 다리 공급 간격이다.
    stockLimit = 1; bridgeDelay = 1;
    // 테스트 전용 조각 목록과 실제 소비 횟수다.
    testBridgePatterns = []; testBridgeDraws = 0;
    // 테스트 전용 순서를 완전히 검증한 뒤 교체한다. 빈 문자열과 off는 해제한다.
    configureTestBridgePatterns(text) {
        const parsed = [];
        if (text !== '' && text !== 'off') {
            // 각 쉼표 필드는 양쪽 ASCII 공백만 제거하고 0~25 정수로 해석한다.
            for (const raw of text.split(',')) {
                const field = raw.replace(/^ +| +$/g, '');
                if (!/^\d+$/.test(field) || Number(field) >= 26) throw new RangeError('테스트 다리 조각은 0~25의 쉼표 목록 또는 off여야 합니다');
                parsed.push(Number(field));
            }
        }
        this.testBridgePatterns = parsed; this.testBridgeDraws = 0;
    }
    // 테스트 전용 조각이 있을 때만 소비한다. 0도 명시된 번호다.
    nextTestBridgePattern() {
        if (!this.testBridgePatterns.length) return null;
        const value = this.testBridgePatterns[this.testBridgeDraws % this.testBridgePatterns.length]; this.testBridgeDraws += 1; return value;
    }
}

// 현재 Combat 패널의 공급 기한과 이전 자금·신전·슬롯을 보관한다.
export class ProductionMaintenancePanel {
    // 패널 생성 여부와 다리 공급 벽시각 기한이다.
    initialized = false; bridgeUntil = 0;
    // 이전 자금, 다리 슬롯 한도 및 이전 신전 SID다.
    oldMoney = 0; slots = 0; temple = 0;
}

// 같은 SID 풀·검색 상태·현재 전역과 명시 장치의 수명을 빌린다.
export class RawProductionMaintenance {
    // 같은 월드와 생산 전역 및 현재 장치를 보관한다.
    #pool; #typeCount; #finder; #globals; #panel; #reads; #hooks;
    // 원본 연결 누락은 설치 시 거부한다.
    constructor(pool, typeCount, finder, globals, panel, reads, hooks) {
        const readNames = ['active', 'technology', 'templateTechnology', 'temple', 'editor', 'generatorTimer', 'stockRate', 'artificial'];
        const hookNames = ['wallSeconds', 'gameSeconds', 'money', 'touchDeck', 'pattern', 'create', 'destroy', 'allowed', 'cost', 'refresh'];
        if (pool?.edition !== 'patch1078' || readNames.some((key) => typeof reads?.[key] !== 'function') || hookNames.some((key) => typeof hooks?.[key] !== 'function')) throw new TypeError('생산 유지의 실제 연결 누락');
        this.#pool = pool; this.#typeCount = typeCount; this.#finder = finder; this.#globals = globals; this.#panel = panel; this.#reads = reads; this.#hooks = hooks;
    }
    // 부모 0도 허용하지만 실제 풀 밖의 DWORD는 거부한다.
    #parent(value) { if (!Number.isInteger(value) || value < 0 || value >= this.#pool.capacity) throw new RangeError('생산 유지 현재 부모 범위'); return value; }
    // 실제 내용물 연결과 현재 전역을 원본 순서대로 한 번 갱신한다.
    run() {
        const pool = this.#pool, g = this.#globals, panel = this.#panel, r = this.#reads, h = this.#hooks;
        if (!r.active() || !r.technology()) return;
        if (!g.stockUntil.length) g.stockUntil = new Array(this.#typeCount).fill(0);
        if (g.stockUntil.length !== this.#typeCount) throw new Error('생산 TYPE별 시계 크기 변경');
        const parent = this.#parent(r.templateTechnology()), count = new RawContainedFinder(pool, this.#finder); let total = 0, single = 0;
        // 템플릿 기술의 다리 전체와 한 칸 조각 수를 각각 센다.
        for (let sid = count.begin(parent); sid; sid = count.next()) { const raw = pool.slot(sid); if (read(raw, 20) & 0x40) { total += 1; if (!read(raw, 12)) single += 1; } }
        const room = total < panel.slots;
        if (!r.editor() && panel.temple && !r.temple()) {
            const remove = new RawContainedFinder(pool, this.#finder);
            // 신전 소실 시 현재 기술에 연결된 다리 내용물을 삭제한다.
            for (let sid = remove.begin(this.#parent(r.technology())); sid; sid = remove.next()) if (read(pool.slot(sid), 20) & 0x40) h.destroy(sid);
            panel.temple = 0;
        }
        panel.temple = r.temple(); h.touchDeck();
        if (!r.artificial(parent) && (r.editor() || r.temple()) && (panel.bridgeUntil <= h.wallSeconds() || !r.generatorTimer())) {
            panel.bridgeUntil = h.wallSeconds() + Math.fround(g.bridgeDelay);
            if (room) {
                let pattern = h.pattern(); const before = g.sequence | 0; g.sequence = (g.sequence + 1) >>> 0;
                if (before % 5 === 0 && !single) pattern = 0;
                if (g.forcedPattern) pattern = g.forcedPattern;
                const specified = g.nextTestBridgePattern(); if (specified !== null) pattern = specified;
                h.create(parent, g.bridgeType, pattern, 0x40, 0x10, 0);
            }
        }
        let teleportChanged = false, bridgeChanged = false; const contents = new RawContainedFinder(pool, this.#finder); let sid = contents.begin(this.#parent(r.technology()));
        // 다음 링크를 실제 검색기에서 관리하며 현재 내용물의 타이머와 재고를 순서대로 갱신한다.
        while (sid) {
            let raw = pool.allocated(sid);
            if ((read(raw, 20) & 0x40) && !r.editor() && (read(raw, 31, 4) | 0) <= truncate(h.gameSeconds() * 10) && !(read(raw, 20) & 0x100)) {
                if (g.quickBridges) { if (!raw[30]) { raw[30] = 2; bridgeChanged = true; } }
                else if (!raw[30]) { raw[30] = 1; new DataView(raw.buffer, raw.byteOffset + 31, 4).setUint32(0, (60 - truncate(h.gameSeconds() * -10)) >>> 0, true); bridgeChanged = true; }
                else if (raw[30] === 1) { raw[30] = 2; bridgeChanged = true; }
            }
            raw = pool.allocated(sid);
            if (read(raw, 20) & 8) {
                const kind = read(raw, 18); if (kind >= this.#typeCount) throw new RangeError('생산 유지 내용물 종류 범위');
                if (g.stockEnabled && g.stockUntil[kind] <= h.wallSeconds()) {
                    let next = raw[30] + 1; if (g.stockLimit && g.stockLimit < next) next = g.stockLimit; raw[30] = next;
                    const rate = r.stockRate(); if (rate >= STOCK_INTERVALS.length) throw new RangeError('생산 재고 옵션 범위');
                    g.stockUntil[kind] = STOCK_INTERVALS[rate] + h.wallSeconds(); if (r.editor()) g.stockUntil[kind] = h.wallSeconds(); teleportChanged = true;
                }
                const money = h.money();
                if (money !== panel.oldMoney) {
                    if (!h.allowed(kind)) { const removed = sid; sid = contents.next(); h.destroy(removed); continue; }
                    const cost = h.cost(kind); if ((cost <= panel.oldMoney) !== (Math.fround(cost) <= h.money())) teleportChanged = true;
                }
            }
            sid = contents.next();
        }
        panel.oldMoney = truncate(h.money()); if (teleportChanged) h.refresh(1); if (bridgeChanged) h.refresh(2);
    }
}
