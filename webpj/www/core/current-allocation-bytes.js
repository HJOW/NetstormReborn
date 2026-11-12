// 원본 생성자가 초기화하지 않는 바이트의 "현재 할당 입력" 공급 지점을 옮긴 모듈이다(cpppj `client/CurrentAllocationBytes.h`, 헤더만, 기준선 뒤 변경 없음).
// 원문(C++)은 같은 크기의 `malloc` 을 하나 받아 그 내용(초기화되지 않은 실제 힙 바이트)을 복사한 뒤 즉시 반납해, new 가 초기화하지 않는 공장 120BYTE 등에 현재 CRT 할당의 실제 입력이 남게 한다.
// 웹(adapt): JavaScript 의 새 `Uint8Array` 는 항상 0 이고 비결정적 힙 내용을 읽을 방법도 없다. 그래서 **입력을 덮어쓰지 않고 새 할당의 0 을 그대로 입력으로 쓴다**(결정적이며 원문의 "기본 0 초기화를 추가하지 않는다"는 의도를
// 어기지 않는다: 이 호출이 값을 만들어 쓰지 않는다). 검사는 `ClientPlayerRecords` 의 `allocationBytes` 인자로 명시 입력(예: 0x79 채우기)을 주입해 미초기화 입력의 흐름을 확인한다.
// 사용: currentAllocationBytes(bytes) — 이 호출은 bytes 를 바꾸지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E332: 4-D CurrentAllocationBytes 웹 적응)

// 새 할당의 현재 내용을 입력으로 그대로 둔다(웹은 0). 비어 있거나 Uint8Array 가 아니면 형 오류이다. (2026-11-12 WB04-E332)
export function currentAllocationBytes(bytes) {
    if (!(bytes instanceof Uint8Array)) throw new TypeError('현재 할당 입력은 Uint8Array 여야 합니다.');
}
