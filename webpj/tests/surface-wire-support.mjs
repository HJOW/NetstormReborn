// 공용 Squid 송신의 실제 Pop 진입을 관찰하되 원문 writer 실행은 계속하는 검사 전용 spy다(WB04-E594).
// 제품의 임시 bindSquidTransmission 성공 훅을 대신하지 않으며 같은 실제 장기 송신기를 끝까지 호출한다.
export function observeSquidPop(world, requests) {
    const wire = world.storage.transmission;
    const original = wire.pop;
    const observed = (sid, flags) => { requests.push([sid, flags]); return original.call(wire, sid, flags); };
    wire.pop = observed;
    return { release: () => { if (wire.pop === observed) wire.pop = original; } };
}
