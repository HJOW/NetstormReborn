// 대본 DestroyAt 의 위치 문자열 해석(0044c540)과 점 검색·삭제 루프(0044c790, Damageable.cpp)를 옮긴 모듈이다(cpppj `o/RawDestroyAt.cpp/.h`, 10.78 전용).
//  - parseDestroyAtLocation(source, lookup): 첫 NUL 앞 문자열에서 '(' 가 맨 앞에 있을 때만 건너뛰고 앞 공백을 새로 지우지 않는다. 구분자 순서는 x → ')' → '#' → (타입 이름 '/' 반경) → 소유자(개행)이다.
//    x/y/반경은 atol(configParseLong, signed), 타입은 lookup(이름)의 signed 반환을 DWORD 로, 소유자는 atol 의 DWORD 이다. lookup 이 없으면 토큰을 읽은 뒤 거부한다.
//  - destroyObjectsAt(source, hooks): 권한(boss)이 없으면 진단 839 를 부른 뒤 권한을 다시 읽어 없으면 끝낸다. 위치를 해석해 x/y 를 float 저장·절삭 두 번으로 정수화한 점 (x,y,x,y)을 begin 으로 검색하고, 반환된 현재 항목마다(SID 0 이거나 현재 풀 상한 이상이면 끝)
//    타입(0 이면 전체)·분류 비트(0x7c77ef00)·소유자(0 이면 전체)를 raw 의 늦은 읽기로 판정해 맞으면 destroy(sid, 0x400000)를 호출하고 삭제 뒤 next() 로 다음 항목을 조회한다. 반경은 해석하되 원문처럼 무시한다.
// 선택된 장치의 누락은 그 위치에서 거부하고 앞선 삭제는 되돌리지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E30: DestroyAt 부모 이식으로 추가)
import { configParseLong } from '../assets/config.js';
import { readRawTextToken } from './raw-text-token.js';

// 원문 44c8a1/44c8b6/44c79f 의 비교/호출 값이다: 삭제 대상 분류 비트, 삭제 플래그, 권한 진단 줄. (2026-11-08 WB04-E30)
const DESTROY_GENUS = 0x7c77ef00;
const DESTROY_FLAGS = 0x400000;
const BOSS_ASSERTION = 839;

// 호출되지 않는 하위는 요구하지 않고 선택된 효과의 누락만 거부한다. (2026-11-08 WB04-E30)
function need(device) {
    if (typeof device !== 'function') throw new Error('DestroyAt 의 실제 장치 미연결');
    return device;
}

// 원문의 int → float32 → ftol → float32 → ftol 저장을 DWORD 감김과 함께 두 번 적용한다. (2026-11-08 WB04-E30)
function pointCoordinate(value) {
    let result = value | 0;
    // 두 float 저장 사이의 signed DWORD 값을 보존한다.
    for (let pass = 0; pass < 2; pass += 1) result = Number(BigInt.asIntN(32, BigInt(Math.trunc(Math.fround(result)))));
    return result;
}

// 반환은 { x, y, type, radius, owner } 이다. source 는 원본 바이트 문자열, lookup(이름) 은 타입 이름의 signed 번호(미등록 -1)이다. (2026-11-08 WB04-E30)
export function parseDestroyAtLocation(source, lookup) {
    const nul = source.indexOf('\0');
    const text = nul < 0 ? source : source.slice(0, nul);
    const x = readRawTextToken(text, text.length > 0 && text[0] === '(' ? 1 : 0, 'x');
    const y = readRawTextToken(text, x.next, ')');
    const tail = readRawTextToken(text, y.next, '#');
    const type = readRawTextToken(tail.text, 0, '/');
    const owner = readRawTextToken(text, tail.next, '\n');
    return {
        x: configParseLong(x.text),
        y: configParseLong(y.text),
        type: need(lookup)(type.text) >>> 0,
        radius: configParseLong(tail.text.slice(type.next)),
        owner: configParseLong(owner.text) >>> 0,
    };
}

// hooks = { boss(), assertion(줄), parse(문자열) → 위치, begin(영역) → 첫 SID, next(), capacity(), slot(sid) → 현재 물리 50바이트, genus(sid), destroy(sid, 플래그) } 이다. (2026-11-08 WB04-E30)
export function destroyObjectsAt(source, hooks) {
    if (!need(hooks.boss)()) {
        need(hooks.assertion)(BOSS_ASSERTION);
        if (!need(hooks.boss)()) return;
    }
    const spot = need(hooks.parse)(source);
    const x = pointCoordinate(spot.x);
    const y = pointCoordinate(spot.y);
    let sid = need(hooks.begin)({ left: x, top: y, right: x, bottom: y });
    // finder 가 이미 반환한 현재 항목에 별도 free/dead/선택/소유자 자격을 추가하지 않는다.
    for (;;) {
        if (!sid || sid >= need(hooks.capacity)()) return;
        const raw = need(hooks.slot)(sid);
        if (raw.length < 35) throw new RangeError('DestroyAt 의 현재 물리 슬롯 크기 오류');
        if ((!spot.type || raw[10] === spot.type) && (need(hooks.genus)(sid) & DESTROY_GENUS) !== 0 && (!spot.owner || raw[34] === spot.owner)) need(hooks.destroy)(sid, DESTROY_FLAGS);
        sid = need(hooks.next)();
    }
}
