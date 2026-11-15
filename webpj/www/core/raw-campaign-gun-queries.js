// 캠페인의 현재 Gun 조회를 같은 World/모드/표적/피해 시각 처리기에 연결한다(cpppj client/RawCampaignGunQueries, 225553df, PATCH 전용).
// 사용: makeCampaignGunQueryHooks(core, mode, target, hit, state, firingIntervals, 기존 훅). 큰 감시/발사/애니메이션 장치는 기존 훅에 보존한다.
// 입력 표·모듈·전역은 반환한 함수보다 오래 살아야 하며 값을 복사한 별도 표적 월드를 만들지 않는다. (2026-11-16 WB04-E565)
import { checkRawObjectForms, diagnoseRawGunIslandOwners } from './raw-checking-diagnostics.js';

// 정렬되지 않은 현재 슬롯 DWORD를 원문 바이트 그대로 읽는다. (2026-11-16 WB04-E565)
function dword(raw, offset) { return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0; }

// 실제 표적 중심의 float32 저장 비트를 DWORD 문맥에 넣는다. (2026-11-16 WB04-E565)
function floatBits(value) { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); }

// 원문 함수 포인터 묶음을 값으로 합성하고 각 소비 시점의 슬롯·TYPE·시각을 읽는다. (2026-11-16 WB04-E565)
export function makeCampaignGunQueryHooks(core, mode, target, hit, state, intervals, hooks = {}) {
    // RawWorldCore는 참조 멤버를 가진 값 인수다. 외부 묶음 재대입과 실제 하위 상태 변경을 구별한다.
    core = { ...core };
    if (mode?.pool !== core.pool || target?.pool !== core.pool || hit?.pool !== core.pool || intervals.length !== core.types.length) throw new TypeError('캠페인 Gun query의 같은 Pool/TYPE 자료 불일치');
    return {
        ...hooks,
        // 원문 Form 진단의 거짓 결과는 성공/자동 복구로 바꾸지 않는다.
        checkForms: (sid) => { if (!checkRawObjectForms(core.pool, sid)) throw new Error('Gunprocess.cpp parent.checkForms() 원본 진단'); },
        // C++ 인수 평가처럼 현재 Footprint를 먼저 구하고 순수 진단 안에서 checking을 다시 읽는다.
        validateIslandOwners: (sid) => {
            diagnoseRawGunIslandOwners(core.pool, core.hash, core.types, sid, core.neighbors.footprint(sid),
                () => (typeof state.checking === 'function' ? state.checking() : state.checking), () => core.bridgeState.noIslandType,
                (line) => { throw new Error(`Gunprocess.cpp noIsland 소유자 원본 진단 줄=${line}`); });
        },
        // 두 번째 owner BYTE는 원문에서 읽지 않는다.
        targetingMode: (sid) => mode.get(sid),
        // C++ float/DWORD/BYTE 인수 폭과 방향 DWORD의 signed 재해석을 보존한다.
        findTarget: (x, y, type, direction, owner) => target.findTarget(Math.fround(x), Math.fround(y), type >>> 0, direction | 0, owner & 0xff),
        // 같은 실제 시간 WORD setter와 공간 통지를 사용한다.
        refreshRange: (sid) => { hit.setTime(sid, state.now); },
        // raw→현재 중심→owner BYTE→여덟 DWORD 순서다. 좌표 원문 비트는 NaN payload까지 산술 변환 없이 보존한다.
        shotContext: (sid) => {
            const raw = core.pool.physical(sid); const center = target.center(sid); const owner = raw[34];
            return [dword(raw, 14), dword(raw, 18), floatBits(center[0]), floatBits(center[1]), raw[10], mode.get(sid) >>> 0, owner, dword(raw, 36)];
        },
        // 실제 float 표의 현재 칸을 double로 승격한다. JS 배열 입력도 span<float>와 같은 저장 폭으로 읽는다.
        firingInterval: (sid) => {
            const type = core.pool.slot(sid)[10];
            if (type >= intervals.length) throw new RangeError('캠페인 현재 발사간격 TYPE 범위');
            return Math.fround(intervals[type]);
        },
    };
}
