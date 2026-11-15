// 일반 Sprite 의 정기 4794b0 을 옮긴 모듈이다(cpppj `o/RawSpriteRegular.cpp/.h`, 10.78 전용): 현재 프레임 선저장·현재 TYPE +114(frameCount)·signed IDIV·실제 Frame.Set 순서를 복원한다.
// 사용: const next = runSpriteRegular(pool, types, setter, parent, payload) — setter 는 같은 풀의 `SquidFrame`(squid-frame.js), types 는 188칸 TYPE 레코드({ frameCount, flags2, … }), payload 는 float 이다.
//   부모의 현재 프레임 DWORD(+36)를 먼저 저장하고(32비트 전체) 현재 TYPE 의 frameCount 로 (프레임 + 1) 의 부호 있는 나머지를 구해 `setter.set(parent, 나머지, 0)` 으로 지정한 뒤 원래 payload 를 그대로 반환한다.
//   event/count 와 무관하다. frameCount 가 0 이거나 (프레임 + 1 이 −2^31 이고 frameCount 가 −1)이면 원본 IDIV 오류이므로 어떤 지정도 하기 전에 거부한다.
// 다른 파생의 프레임 진행/방향 나머지와 구별하여 원문 4794b0 한 번의 signed 나머지를 수행한다. 원본 주소는 판본/원문 기록이며 호스트 함수 포인터로 실행하지 않는다.
// 기준선 뒤 cpppj 변경 없음(기준선 `225553df` 와 HEAD 소스 동일, 2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E529: 4-D 구획 RawSpriteRegular 이식)

// 4ad6c0 이전의 현재 프레임과 이후 실제 지정/표시 순서를 유지한다. 풀·TYPE 표·Frame 지정기가 같은 10.78 계약이 아니면 거부한다. (2026-11-15 WB04-E529)
export function runSpriteRegular(pool, types, setter, parent, payload) {
    if (pool?.edition !== 'patch1078' || setter?.pool !== pool || types?.length !== 188) throw new TypeError('Sprite Regular의 같은PATCH/188TYPE/Frame 계약 오류');
    const raw = pool.slot(parent);
    // 원문은 전체 DWORD 프레임을 먼저 선저장한다.
    const current = (raw[36] | (raw[37] << 8) | (raw[38] << 16) | (raw[39] << 24)) >>> 0;
    const type = raw[10];
    if (type >= types.length) throw new RangeError('Sprite Regular의 현재TYPE 범위 오류');
    const divisor = types[type].frameCount | 0;
    const next = (current + 1) | 0;
    if (!divisor || (next === -2147483648 && divisor === -1)) throw new RangeError('Sprite 원본 IDIV 오류');
    setter.set(parent, (next % divisor) | 0, 0);
    return payload;
}
