// 초기 지형 도장(RawTerrainStamp)의 실제 생성·소유자·공간·지역 쓰기를 같은 raw 표면 월드에 연결하는 훅 묶음이다(cpppj `client/RawTerrainWorldBinding.cpp/.h`, 10.78 전용).
// 사용: const hooks = makeTerrainStampWorldHooks(world, stampState, (kind) => 진단, { chunkRegion: (cx, cy) => 지역, assertion: (kind) => 진단 });
//   new RawTerrainStamp(core.pool, core.types, core.frames, map, random, crt, stampState, hooks).stamp(0x78).
//   원본 ChunkMap 조회(chunkRegion)와 stamp 진단(assertion), 지역 setter 진단은 호출자가 공급하고, 나머지 네 훅(create/setOwner/pop/setRegion)은 월드의 실제 몸체로 바꾼다.
// 이전에 검증한 도장 몸체를 다시 쓰지 않고 현재 월드의 `create`/`pop` 과 가상 소유자 분배(`makeOwnerDispatch`)·지역 setter(`setSquidRegion`)에 연결한다.
// world/state/진단 대상은 반환한 훅과 RawTerrainStamp 보다 오래 살아야 한다. 이 모듈은 DOM 을 쓰지 않는다. 기준선 뒤 cpppj 변경은 없다(3.6절 ①).
// (2026-11-11 WB04-E301: 4-B RawTerrainWorldBinding 이식)
import { makeOwnerDispatch } from './raw-bridge-connect.js';
import { setSquidRegion } from './raw-squid-region.js';

// 첫 생성 이전에 외부 지역 조회/진단을 확인하고 같은 월드 Core 의 실제 소비자만 설치한다. effects 는 복사하며 호출자 객체를 바꾸지 않는다. (2026-11-11 WB04-E301)
export function makeTerrainStampWorldHooks(world, state, regionDiagnostic, effects) {
    // 누락된 지역 조회/진단은 묵시 기본값으로 대신하지 않고 월드에 아무것도 만들기 전에 거부한다.
    if (!effects?.chunkRegion || !effects.assertion || !regionDiagnostic) throw new TypeError('초기 지형 월드의 지역 조회/진단 연결 누락');
    const core = world.core();
    const hooks = { ...effects };
    // 월드 Factory 의 실제 생성 몸체이다(flags 는 지도의 makePredictable).
    hooks.create = (type, flags) => world.create(type, flags);
    // 본섬과 파생 받침/종유석의 실제 vtable 소유자 규칙을 같은 가변 전역에 분배한다.
    hooks.setOwner = makeOwnerDispatch(core.pool, core.bridgeState, (sid, player) => core.owner.set(sid, player));
    // 월드의 실제 Pop(공간 해시·후처리·표시)이다.
    hooks.pop = (sid, x, y, flags) => world.pop(sid, x, y, flags);
    // 지면 객체의 지역 번호를 원문 setter(unsigned 변환, 현재 noRegion)로 쓴다.
    hooks.setRegion = (sid, region) => setSquidRegion(core.pool, core.types, sid, region >>> 0, state.noRegion >>> 0, regionDiagnostic);
    return hooks;
}
