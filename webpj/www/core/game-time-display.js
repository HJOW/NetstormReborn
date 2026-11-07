// 전투 시간 표시 토글 키(T/t, 004d9862)와 CRT 전투 시계의 초 계산(004a68b0)을 옮긴 모듈이다(cpppj `client/GameTimeDisplay.cpp/.h`, 10.78 전용). 게임 정지 시계와 별개인 원문 CRT clock 의 signed DWORD 계산을 보존한다.
//  - dispatchGameTimeKey(event, holder, paused): 키 뗌이거나 수정키를 지운 코드가 'T'/'t' 가 아니면 소비하지 않는다(false). 그 밖에는 현재 정지(paused())를 읽어 정지가 아니면 표시 DWORD(holder.value)의 비영 전체를 0, 0 을 1 로 바꾼다. 수정키(Shift/Ctrl)는 막지 않는다.
//    정지 조회가 없으면 거부한다. 선행 모달 거부는 호출자(Uber 부모) 책임이다.
//  - readGameTimeSeconds(stamp, origin, clock): clock() 호출 뒤에 현재 원점(origin())을 읽고 stamp.value = clock 값으로 쓴 다음, 감긴 signed 차이(now − base)를 1000 으로 0 쪽 절삭한 정수 초를 돌려준다. clock 예외 때 이미 쓴 stamp/origin 은 되돌리지 않는다.
// stamp 는 { value }, origin 은 현재 원점 getter, clock 은 CRT clock 밀리초 getter 이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E34: 게임 시간 표시 이식으로 추가)
import { InputCode } from './input-queue.js';

// 수정키 제거 마스크이다. (2026-11-08 WB04-E34)
const KEY_MASK = 0x03ffffff;

// 모달을 지난 T/t 누름만 현재 정지 뒤에 표시 DWORD 로 반영한다. (2026-11-08 WB04-E34)
export function dispatchGameTimeKey(event, holder, paused) {
    const key = (event.code & KEY_MASK) >>> 0;
    if ((event.code & InputCode.release) !== 0 || (key !== 0x54 && key !== 0x74)) return false;
    if (typeof paused !== 'function') throw new TypeError('게임 시간 키의 현재 정지 조회 미연결');
    if (!paused()) holder.value = holder.value === 0 ? 1 : 0;
    return true;
}

// 실제 CRT clock 호출 뒤 현재 원점을 읽고 감긴 signed 밀리초를 정수 초로 바꾼다. (2026-11-08 WB04-E34)
export function readGameTimeSeconds(stamp, origin, clock) {
    if (typeof clock !== 'function') throw new TypeError('게임 시간의 실제 CRT clock 미연결');
    const now = clock() >>> 0;
    const base = origin() >>> 0;
    stamp.value = now;
    // 0 쪽 절삭이며 -0 이 아닌 0 을 돌려준다.
    return (((now - base) | 0) / 1000) | 0;
}
