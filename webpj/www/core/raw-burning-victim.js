// 연소 세 부모(423d50 캡처·424110 연소점·424530 프레임)의 공통 앞단을 옮긴 모듈이다(cpppj `o/RawBurningVictim.cpp/.h`, 10.78 전용).
// 현재 SID 를 두 번 같은 live 로 진단(isSidRobust)하고, 두 번째 진단이 통과한 뒤의 현재 번호를 이후 가상 표/HP 진단에 쓰는 슬롯으로 고정한다. 이후 캡처/HP 계산/입자 이동 몸체는 호출자가 이어서 실행한다.
//  - runBurningVictimParent(current, entry, checking, hooks): checking 이 비영이면 첫 robust 가 실패할 때 `isSidRobust(burningSid)` 진단(.\Burning.cpp, 캡처 0x6f/연소점 0x91/프레임 0x11f)을 올리고 정상 복귀하면 계속한다.
//    두 번째 robust(현재 번호를 다시 읽음)가 실패하면 null(조기 반환)이다. 통과하면 그 번호를 고정하고, 캡처만 가상 표 비영(`*((long**)&s)!=0`, 0x77)을 확인하며, 세 부모 모두 HP 보유 여부
//    (캡처 `s.hasHitPoints()` 0x7f / 연소점 `burningVictim.hasHitPoints()` 0x98 / 프레임 `s.hasHitPoints()` 0x126)를 확인하고 실패하면 진단을 올린 뒤 정상 복귀 시 고정한 번호를 반환한다.
//  - makeBurningVictimHooks(pool, types, state): 원문의 비가상 HP/가상 표 조회와 현재 진단을 같은 풀에 연결한다.
// `current` 와 `checking` 은 호출마다 현재 값을 읽는 getter 함수이다(C++ 의 const Sid& / 비소유 RawCheckingFlag 뷰에 해당; 진단 callback 이 값을 바꿔도 다음 읽기가 그것을 본다).
// 선택하지 않은 장치는 요구하지 않으며 도달한 장치의 누락은 빈 성공이 아니라 오류이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E76: 연소 피해 대상 앞단 이식으로 추가)
import { isSidRobust } from './raw-sid-robust.js';

// 세 부모 입구의 종류이다. 캡처 423d50·연소점 424110·프레임 424530 이다. (2026-11-08 WB04-E76)
export const BurningVictimEntry = Object.freeze({ Capture: 0, Sources: 1, Frame: 2 });

// 진단에 올리는 원문 파일 이름과 부모별 줄 번호, 캡처의 가상 표 진단 줄이다. (2026-11-08 WB04-E76)
const BURNING_CPP = '.\\Burning.cpp';
const ROBUST_LINES = Object.freeze([0x6f, 0x91, 0x11f]);
const HIT_POINT_LINES = Object.freeze([0x7f, 0x98, 0x126]);
const VTABLE_LINE = 0x77;
// Dais 형 TYPE 의 flags2 비트와 일반 자산의 flags1 HP 비트, Dais 의 extra 바이트 위치이다. (2026-11-08 WB04-E76)
const DAIS_FLAG = 0x400000;
const HIT_POINT_FLAG = 0x10;
const EXTRA = 40;

// 선택된 장치를 빈 성공으로 처리하지 않는다. checking 0 은 첫 진단 장치에 접근하지 않는다. (2026-11-08 WB04-E76)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('연소 피해 대상의 선택된 원문 장치 미연결');
    return hook;
}

// current/checking getter 를 확인한다. (2026-11-08 WB04-E76)
function getter(value, name) {
    if (typeof value !== 'function') throw new TypeError(`연소 피해 대상의 ${name} 는 현재 값 getter 함수여야 합니다`);
    return value;
}

// 번호/TYPE/dead 를 사전에 걸러 원문의 실제 조회를 생략하지 않으며, 두 번째 robust 가 통과한 현재 번호를 반환한다. 빈 반환(null)은 두 번째 실제 robust 0 이며 예외/진단 중단과 구별한다. (2026-11-08 WB04-E76)
export function runBurningVictimParent(current, entry, checking, hooks) {
    const read = getter(current, 'current');
    const gate = getter(checking, 'checking');
    // 같은 조건이라도 부모별 원문 줄 번호를 유지한다.
    const robustLine = ROBUST_LINES[entry];
    if (robustLine === undefined) throw new RangeError('연소 피해 대상의 부모 종류 범위');
    if (gate() && !need(hooks.robust)(read(), true)) need(hooks.assertion)('isSidRobust(burningSid)', BURNING_CPP, robustLine);
    if (!need(hooks.robust)(read(), true)) return null;
    // 두 번째 조회가 바꾼 현재 번호를 여기서만 고정하며 이후 진단은 이 슬롯을 유지한다.
    const selected = read();
    if (entry === BurningVictimEntry.Capture && !need(hooks.vtable)(selected)) need(hooks.assertion)('*((long**)&s)!=0', BURNING_CPP, VTABLE_LINE);
    if (!need(hooks.hasHitPoints)(selected)) {
        need(hooks.assertion)(entry === BurningVictimEntry.Sources ? 'burningVictim.hasHitPoints()' : 's.hasHitPoints()', BURNING_CPP, HIT_POINT_LINES[entry]);
    }
    return selected;
}

// state = { checking()→현재 DWORD(독립 기본 0), robust?(sid, live), assertion?(조건, 파일, 줄) } 이다. robust 가 없으면 원문 번호 판정만 쓰며 checking 이 비영이면 미연결 상세 장치를 선택 지점에서 거부한다.
// 같은 pool/types 의 현재 슬롯에서 가상 표 비영 여부와 HP 보유 여부를 읽는다. (2026-11-08 WB04-E76)
export function makeBurningVictimHooks(pool, types, state) {
    if (pool?.edition !== 'patch1078') throw new TypeError('연소 피해 대상은 PATCH 전용입니다');
    const checking = state?.checking ?? (() => 0);
    return {
        // 진단 상태는 호출마다 새로 만든다. 독립 checking 0 은 원문 번호 판정만 사용한다.
        robust: (sid, live) => {
            if (state?.robust) return state.robust(sid, live);
            return isSidRobust(sid, live ? 1 : 0, { lastRobust: 0, type: 0 }, { checking });
        },
        // 현재 DWORD 의 비영 여부만 같은 네 바이트에서 읽는다.
        vtable: (sid) => {
            const raw = pool.slot(sid);
            return (raw[0] | raw[1] | raw[2] | raw[3]) !== 0;
        },
        // Dais 는 extra bit0, 일반 자산은 현재 flags1 bit4(0x10)이다. 현재 TYPE 이 표 밖이면 거부한다.
        hasHitPoints: (sid) => {
            const raw = pool.slot(sid);
            const type = raw[10];
            if (type >= types.length) throw new RangeError('연소 피해 대상의 현재 TYPE 범위');
            const record = types[type];
            return (record.flags2 & DAIS_FLAG) !== 0 ? (raw[EXTRA] & 1) !== 0 : (record.flags1 & HIT_POINT_FLAG) !== 0;
        },
        // 독립 장치도 선택 진단을 빈 성공으로 처리하지 않는다.
        assertion: (condition, source, line) => {
            if (state?.assertion) state.assertion(condition, source, line);
            else throw new Error(`원문 연소 진단 ${source}:${line}: ${condition}`);
        }
    };
}
