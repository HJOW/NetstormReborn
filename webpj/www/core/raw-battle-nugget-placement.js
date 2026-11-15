// 비미션 보충용 오벨리스크(Nugget) 배치(원본 0040e050)를 현재 모드/청크 seed/실제 임의 배치(0040ded0)에 연결하는 모듈이다(cpppj `o/RawBattleNuggetPlacement.cpp/.h`, 10.78 전용).
// 사용: placeBattleNuggets(rectangle, quantity, state, hooks)
//   rectangle = { left, top, right, bottom }(float 네 값의 값 전달 사본이며 호출자의 사각형을 바꾸지 않는다), quantity = 배치 수량(unsigned 32비트).
//   state = { single, boss, buriedType } 은 00594fa4/00540bc4/00541218 의 현재값이며 각각 { value } 상자·현재 값을 돌려주는 함수·단순 값 중 하나로 받아 원문 위치마다 다시 읽는다.
//   hooks = { singleAssertion(), wallSeconds(), regionZero(), place(random, rectangle, region, quantity, type, force) } — 선택된 경로의 장치만 요구한다(미선택 경로의 장치는 요구하지 않는다).
//     singleAssertion: 원문 `!inSingle` 진단(복귀형)이다. wallSeconds: 현재 벽시계 초(double)이다. regionZero: 현재 ChunkMap 지역 0 의 첫 일치 6BYTE 기록이다.
//     place: 실제 0040ded0 이며 지역 -1·force 1 과 현재 타입을 그대로 받는다(반환값은 원문도 쓰지 않는다).
// 순서: single 이면 진단 → 지역 난수(GameRandom(0), 공유 게임 난수와 별개) → boss 이면 실제 wall seed(RawFortFinishOwner.timeSeed), 아니면 지역0 기록의 byte3*103 → 현재 타입 읽기 → 실제 배치.
// 별도 성공/객체/전역 난수 소비를 합성하지 않는다. 누락한 선택 장치는 앞선 효과를 되돌리지 않고 명시 오류로 알린다.
// 원본 관측 fixture 가 없어 cpppj 소스 계약을 tests/raw-battle-nugget-placement.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E558: 보충 오벨리스크 배치 이식)
import { GameRandom } from './random.js';
import { RawFortFinishOwner } from './raw-fort-finish-owner.js';

// 비boss 지역 seed 의 곱 상수(지역0 기록 byte3 에 곱한다)·그 기록의 원문 길이(6BYTE)와 강제 배치 인자, 지역 -1(전체) 번호이다. (2026-11-15 WB04-E558)
const REGION_SEED_FACTOR = 103;
const REGION_RECORD_SEED_BYTE = 3;
const REGION_RECORD_BYTES = 6;
const ALL_REGIONS = -1;
const FORCE_PLACE = 1;
// 읽기 전용 현재 전역은 { value } 상자·현재 값을 돌려주는 함수·독립 입력의 단순 값 중 하나로 받아 원문 위치마다 다시 읽는다. (2026-11-15 WB04-E558)
const read = (source) => (typeof source === 'function' ? source() : (source !== null && typeof source === 'object' ? source.value : source));

// 선택된 실제 장치가 함수가 아니면 명시 오류를 낸다(미선택 경로는 호출하지 않으므로 요구하지 않는다). (2026-11-15 WB04-E558)
function need(device) {
    if (typeof device !== 'function') throw new Error('전투 오벨리스크 배치의 실제 장치 미연결');
    return device;
}

// 0040e050 전체: 원문 진단 → 현재 boss → 지역 seed → 현재 TYPE → 실제 배치의 순서를 보존한다. (2026-11-15 WB04-E558)
export function placeBattleNuggets(rectangle, quantity, state, hooks) {
    // 단일 모드에서는 이 보충이 쓰이지 않아야 하므로 원문 진단을 먼저 낸다(복귀하면 계속한다).
    if (read(state.single)) need(hooks.singleAssertion)();
    // 공유 게임 난수와 구별되는 원문 지역 난수이다.
    const random = new GameRandom(0);
    // boss 판정은 진단이 값을 바꿔도 이 위치에서 다시 읽는다.
    if (read(state.boss)) random.setState(RawFortFinishOwner.timeSeed(need(hooks.wallSeconds)()));
    else {
        // 비boss 는 현재 ChunkMap 지역 0 기록의 byte3 에 103 을 곱한 값이 seed 이다(기록은 주소/소유자 값이 아니다).
        const record = need(hooks.regionZero)();
        if (record.length !== REGION_RECORD_BYTES) throw new RangeError('보충 지역0 기록은 6바이트여야 합니다.');
        random.setState(((record[REGION_RECORD_SEED_BYTE] & 0xff) * REGION_SEED_FACTOR) >>> 0);
    }
    // 실제 배치 장치를 먼저 선택한 뒤(원문 평가 순서) 현재 타입을 읽고, 사각형은 값 전달 사본으로 넘긴다.
    const place = need(hooks.place);
    const buriedType = read(state.buriedType) >>> 0;
    const copy = { left: Math.fround(rectangle.left), top: Math.fround(rectangle.top), right: Math.fround(rectangle.right), bottom: Math.fround(rectangle.bottom) };
    // 원문의 미사용 반환을 새 성공 분기로 해석하지 않는다.
    place(random, copy, ALL_REGIONS, quantity >>> 0, buriedType, FORCE_PLACE);
}
