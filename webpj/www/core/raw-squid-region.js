// 실제 표면/다리의 섬 번호(region) 지정 몸체(패치 004acd20)를 옮긴 모듈이다(cpppj `o/RawSquidRegion.cpp/.h`).
// 프레임·소유자·Graph 는 바꾸지 않고, 현재 TYPE 의 genus 를 확인한 뒤 허용 표면의 raw+8 WORD 에 region 하위 16비트만 저장한다.
//  - 섬/다리/3x3 섬 genus(0x01000006) 중 아무것도 없는 현재 TYPE → 패치 진단 UnsupportedGenus 후 저장하지 않는다.
//  - 다리(genus 비트 4)는 현재 noRegion DWORD 만 받을 수 있으며 항상 기존 WORD 를 보존한다. region 이 noRegion 과 다르면 패치 진단 BridgeRegion 후 저장하지 않는다.
// 진단 콜백이 없으면 해당 분기를 거부한다(진단 복귀 뒤 원본처럼 region 쓰기를 생략). TYPE 번호가 표 밖이면 RangeError 이다.
// 근거: cpppj 기준선 1f00a25a. 10.78 전용이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E2: 섬 번호 지정 이식으로 추가)

// 패치판의 두 원본 assert 원인이다. (2026-11-08 WB04-E2)
export const SQUID_REGION_ASSERT = Object.freeze({ unsupportedGenus: 'unsupportedGenus', bridgeRegion: 'bridgeRegion' });

// 슬롯 필드 위치(TYPE, 섬 WORD)와 표면 genus 묶음, 다리 비트이다. (2026-11-08 WB04-E2)
const TYPE = 10, REGION = 8;
const SURFACE_GENUS = 0x01000006, BRIDGE = 4;

// 패치 004acd20: 허용 표면의 raw+8 WORD 에 region 하위 16비트를 저장한다. types 는 현재 TYPE 표, diagnostic(종류) 은 패치 진단 복귀 함수(없으면 진단 분기 거부)이다.
// 사용: setSquidRegion(pool, types, sid, region, noRegion, (kind) => …). (2026-11-08 WB04-E2)
export function setSquidRegion(pool, types, sid, region, noRegion, diagnostic = null) {
    if (pool?.edition !== 'patch1078') throw new TypeError('섬 번호 지정은 10.78 전용');
    const type = pool.slot(sid)[TYPE];
    if (type >= types.length) throw new RangeError('섬 번호 지정 타입 범위 오류');
    const genus = types[type].flags2 >>> 0;
    const value = region >>> 0;
    if ((genus & SURFACE_GENUS) === 0 || (genus & BRIDGE) !== 0) {
        if ((genus & SURFACE_GENUS) === 0 || value !== (noRegion >>> 0)) {
            if (typeof diagnostic !== 'function') throw new Error('섬 번호 지정 원본 진단 경계 누락');
            diagnostic((genus & SURFACE_GENUS) === 0 ? SQUID_REGION_ASSERT.unsupportedGenus : SQUID_REGION_ASSERT.bridgeRegion);
        }
        return;
    }
    const target = pool.allocated(sid);
    target[REGION] = value & 0xff;
    target[REGION + 1] = (value >>> 8) & 0xff;
}

// 패치 004acda0: 섬 번호 getter 이다(기준선 뒤 cpppj 9cb0547f 에서 추가된 몸체를 WB04-E2 에서 반영). 현재 TYPE 의 genus 에 섬/다리/3x3 섬이 없으면 진단(UnsupportedGenus) 뒤에도
// 현재 noRegion 을 반환하고(진단 경계가 없으면 거부), 다리는 raw WORD 를 읽지 않고 noRegion 을 반환하며, 그 밖에는 raw+8 WORD 를 돌려준다. TYPE 번호가 표 밖이면 RangeError 이다.
// 사용: getSquidRegion(pool, types, sid, noRegion, (kind) => …). (2026-11-08 WB04-E2)
export function getSquidRegion(pool, types, sid, noRegion, diagnostic = null) {
    if (pool?.edition !== 'patch1078') throw new TypeError('섬 번호 getter는 10.78 전용');
    const raw = pool.slot(sid);
    if (raw[TYPE] >= types.length) throw new RangeError('섬 번호 getter의 현재 TYPE 범위');
    const genus = types[raw[TYPE]].flags2 >>> 0;
    if (!(genus & SURFACE_GENUS)) {
        if (typeof diagnostic !== 'function') throw new Error('섬 번호 getter의 원문 genus 진단 미연결');
        diagnostic(SQUID_REGION_ASSERT.unsupportedGenus);
        return noRegion >>> 0;
    }
    return (genus & BRIDGE) ? noRegion >>> 0 : (raw[REGION] | (raw[REGION + 1] << 8));
}
