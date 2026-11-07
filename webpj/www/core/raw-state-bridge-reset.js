// State 정리의 소유자 색 표(0043b540)와 다리 붕괴 스캔(00422490)을 원문의 현재 물리에서 초기화하는 모듈이다(cpppj `o/RawStateBridgeReset.cpp/.h`, 10.78 전용).
//  - resetStateOwnerColors: 앞 9 DWORD 만 0..8 로 쓴다. 열째 칸·이웃·현재 TYPE·프레임은 보존하며 9칸 미만이면 어떤 prefix 도 쓰기 전에 거부한다.
//  - resetStateBridgeDecayScan: 현재 float 지연(period)과 double 게임 시각(now)을 먼저 더해(now 를 float 로 줄이지 않음) 기한을 계산한 뒤 first=15000·last=23001·cursor=15000·next=기한 순으로 쓴다.
// 큰 State 부모나 프레임 순회는 새로 실행하지 않는다. (2026-11-08 WB04-E11: State 다리 초기화 이식으로 추가)

// 소유자 색 표의 앞 9 칸을 0..8 로 쓴다. (2026-11-08 WB04-E11)
export function resetStateOwnerColors(colors) {
    if (colors.length < 9) throw new RangeError('State 소유자 색 표의 실제 9DWORD 부족');
    // 원문은 뒤의 열째 칸을 포함하지 않는다.
    for (let owner = 0; owner < 9; owner += 1) colors[owner] = owner;
}

// 다리 붕괴 스캔을 초기화한다. scan 은 { first, last, cursor, next(double) } 이고 period 는 float 로 읽은 지연, now 는 double 게임 시각이다. (2026-11-08 WB04-E11)
export function resetStateBridgeDecayScan(scan, period, now) {
    // 원문 fld/fadd 는 첫 DWORD store 보다 앞이며 now 를 float 로 줄이지 않는다.
    const next = period + now;
    scan.first = 15000;
    scan.last = 23001;
    scan.cursor = 15000;
    scan.next = next;
}
