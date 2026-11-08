// 복원된 프레임 지정(SquidFrame)의 외부 훅을 실제 SHP 표시·일반 Pop/Unpop 모듈로 연결하는 모듈이다(cpppj `o/SquidFrameBinding.cpp/.h` 의 MakeSquidFrameHooks, 10.78 전용).
// 제자리 경로는 old/new update, 단계 이동 경로는 실제 Unpop/Pop 과 그 안의 update 를 쓴다. Pop/Unpop 은 같은 해시/spot 을 사용해야 하고 둘 다 같은 display 를 연결해야 한다.
// 연결을 만들기 전에 타입·풀·표시 연결을 확인하며 원본 주소를 호스트에서 호출하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E59: 객체 프레임 연결 이식으로 추가)

// 슬롯의 TYPE 번호 위치이다. (2026-11-08 WB04-E59)
const TYPE_OFFSET = 10;

// pool 은 SidPool, types 는 188칸 TYPE 표(SquidDisplay 와 같은 내용), display 는 SquidDisplay, unpop/pop 은 같은 풀·해시·spot·display 를 쓰는 SquidUnpop/SquidPop 이다.
//   반환은 SquidFrame 훅 묶음 { frameSize, update, unpop, pop } 이다. 표/풀/표시 불일치는 구성 때 거부한다. (2026-11-08 WB04-E59)
export function makeSquidFrameHooks(pool, types, display, unpop, pop) {
    display.validateFrameBinding(types);
    pop.validateFrameBinding(pool, display, unpop);
    // 현재 슬롯의 TYPE 번호로 레코드를 고른다. 범위 밖은 쓰기 전에 거부한다.
    const recordOf = (sid, message) => {
        const number = pool.slot(sid)[TYPE_OFFSET];
        if (number >= types.length) throw new RangeError(message);
        return types[number];
    };
    return {
        // 현재 프레임의 실제 SHP 칸 크기가 다음 해시 단계 판단에 쓰인다.
        frameSize: (sid, frame) => display.frameSize(pool.slot(sid), frame),
        // 가상 +0x88/+0x8c 공통 경로를 실제 변경 영역 계산기로 이어 준다.
        update: (sid, flags) => display.update(pool.slot(sid), flags),
        // 옛 프레임으로 공간과 옛 표시 영역을 해제한다.
        unpop: (sid, flags) => {
            unpop.unpop(sid, recordOf(sid, '프레임 Unpop 타입 번호 범위 오류'), flags);
        },
        // Unpop 뒤 현재 위치에 flags 로 다시 등록한다. 10.78 의 실제 표시기는 Pop 의 spot 처리 뒤에 현재 SHP 를 읽으므로 칸 크기 인자는 0 이다.
        pop: (sid, x, y, flags) => {
            pop.pop(sid, recordOf(sid, '프레임 Pop 타입 번호 범위 오류'), 0, 0, x, y, flags);
        }
    };
}
