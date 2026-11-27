// CP1375 ClientMain의 InitFullTime을 웹 이벤트 루프의 비동기 대기로 연결한다. 게임 프레임/RNG를 실행하지 않는다.

// 00461090이time32에서 감산하는 원본 기준 초다. 현재 시간 자체는 ClientTimeSource에서 읽는다.
export const CLIENT_CLOCK_EPOCH = 0x32c85780;
// Native Sleep(1)에 대응해 브라우저 이벤트 루프를 양보한다. UI 스레드의 바쁜 대기를 만들지 않는다.
const yieldMillisecond = () => new Promise((resolve) => setTimeout(resolve, 1));

// 비고정 모드는 UTC초가 바뀐 뒤 밀리초/현재초 순서로 읽는다. 취소되면null이며 실제 앱의 세대 검사가 결과를 버린다.
export async function prepareClientClockOrigin(source, { wait = yieldMillisecond, cancelled = () => false } = {}) {
    if (!source || typeof source.milliseconds !== 'function' || typeof source.unixSeconds !== 'function' || typeof wait !== 'function' || typeof cancelled !== 'function') throw new TypeError('Client 초기 시계의 실제 공급원/대기/취소 연결 필요');
    if (cancelled()) return null;
    let originMilliseconds = source.milliseconds();
    if (source.fixedEnabled) return { originMilliseconds, baseSeconds: 0, initialCapture: false };
    const previousSecond = source.unixSeconds();
    // 현재UTC초가 달라지는 순간까지 양보하며, 오래된 앱 수명으로 복귀하면 더 이상 시계를 읽지 않는다.
    while (source.unixSeconds() === previousSecond) { await wait(); if (cancelled()) return null; }
    originMilliseconds = source.milliseconds();
    return { originMilliseconds, baseSeconds: (source.unixSeconds() - CLIENT_CLOCK_EPOCH) >>> 0, initialCapture: true };
}
