// 기존 정상 지형·저장 객체·SP·Meta·Graph 를 한 실제 월드의 Fort 적재 부모(RawFortWorldLoad)의 필수 경계에 연결하는 훅 묶음이다(cpppj `client/RawFortWorldLoadBinding.cpp/.h`, 10.78 전용).
// 사용: const hooks = makeFortWorldLoadBinding(world, normal, objects, money, meta, moneyStream, selectMeta, metaStream, baseHooks); new RawFortWorldLoad(pool, chunks, islands, frames, piece, owner, noRegion, hooks).load(0).
//   moneyStream(fort, owner)/selectMeta(fort, owner, technology)/metaStream(fort, technology)은 매 호출의 현재 Template 와 같은 전역을 기존 소비자에 선택해야 한다(Money 선택은 wrapper 의 깊이 증가/진단 뒤 지연 호출).
//   generate = 정상 지형 상위(`normal.generateOriginal`), money = `money.loadSelected`, rebuildGraph = 월드 Graph 재구성이고 objects/badges/technology 는 `makeFortWorldChildrenHooks` 가 연결한다.
// 군도/Island0 Populate/TypeNames 전신/부모 getter/UI 는 미복원 범위를 명시한 기존 필수 훅(baseHooks)으로 남는다. Graph 의 현재 전체 풀 제한은 보존하며 Form 이 있는 미지원 월드를 성공으로 통과시키지 않는다.
// 기준선 뒤 cpppj 변경은 없다(3.6절 ①). CD(10.72) 장면은 웹 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E303: 4-B RawFortWorldLoadBinding 이식)
import { makeFortWorldChildrenHooks } from './raw-fort-world-load.js';

// 누락된 효과나 서로 다른 풀을 부모 실행 전에 거부하고, 반환 훅의 모든 선택 경계 수명은 호출자가 보장한다. (2026-11-11 WB04-E303)
export function makeFortWorldLoadBinding(world, normal, objects, money, meta, moneyStream, selectMeta, metaStream, baseHooks) {
    const core = world.core();
    if (normal.pool !== core.pool || typeof moneyStream !== 'function') throw new TypeError('Fort 부모의 실제 정상 월드/현재 Money 선택 누락');
    const hooks = makeFortWorldChildrenHooks(core.pool, objects, meta, selectMeta, metaStream, baseHooks);
    // 정상 지형 상위(004c1510)는 builder 의 owner/경계와 현재 선택 지역을 그대로 받는다.
    hooks.generate = (builder, region) => { normal.generateOriginal(builder.owner, builder.bounds, region); };
    // 실제 Money 선택은 wrapper 의 깊이 증가/진단 뒤에 수행한다(JS 인수 평가로 먼저 선택하지 않는다).
    hooks.money = (fort, owner) => { money.loadSelected(() => moneyStream(fort, owner)); };
    // Graph 0 재구성은 월드의 실제 Graph 이다.
    hooks.rebuildGraph = (resetAll) => { core.graph.rebuild(resetAll); };
    return hooks;
}
