// State 정리의 고정 Zone 객체 004bfd60 을 정확히 6 BYTE 쓰기로 옮긴 모듈이다(cpppj `o/RawStateZoneReset.cpp/.h`, 10.78 전용).
// 현재 실제 객체의 DWORD(+0)를 0 으로 쓴 뒤 +4 WORD 를 0 으로 쓴다. Zone 은 State 144 BYTE 블록이나 인접 8 BYTE 전체와 별개이며 +6/+7 은 읽거나 쓰지 않는다.
// 짧은 물리는 해당 store 에서 RangeError 로 거부하고 완료한 prefix 는 보존한다(8 BYTE 전체 0 쓰기로 대체하지 않음). (2026-11-08 WB04-E11: State Zone 초기화 이식으로 추가)

// Zone 의 처음 6 BYTE 를 두 번의 store 폭(4 → 2)대로 0 으로 쓴다. (2026-11-08 WB04-E11)
export function resetStateZone(zone) {
    if (zone.length < 4) throw new RangeError('State Zone의 첫 DWORD 물리 부족');
    // 첫 원문 DWORD store 이다.
    zone.fill(0, 0, 4);
    if (zone.length < 6) throw new RangeError('State Zone의 마지막 WORD 물리 부족');
    // +4 의 WORD 만 쓰고 인접 +6/+7 은 건드리지 않는다.
    zone.fill(0, 4, 6);
}
