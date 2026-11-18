// 전투 자원 프레임(원본 00486bc0)의 스펠 소비·SP 주기 지급·일반 간헐천 보충을 옮긴 모듈이다(cpppj `o/RawBattleResourceFrame.cpp/.h`, 10.78 전용).
// 사용: runBattleResourceFrame(globals, hooks)
//   globals = { aux, loginEnd, now, boss, battle, single, editor, owner, options, players }
//     aux: 같은 보조 전역 객체로 { spellCount(u32, 00595090), spellDeadline(f64, 005950d0), incomePrimed(u32, 00595124), geyserFrameFlag(u32), geyserDeadline(f64, b8), secondaryGeyserDeadline(f64, c8), ordinaryGeysers(u32), resourcePool(i32) } 의 읽기/쓰기 속성이다.
//       `game-aux-state.js` 의 같은 장기 칸을 실제 소비자에서 빌릴 수 있다.
//     loginEnd·now(프레임 고정 게임 시각, double)·boss·battle·single·editor·owner 는 읽기 전용 전역으로 { value } 상자·현재 값을 돌려주는 함수·단순 값 중 하나이며 각 효과 뒤 원문 위치에서 다시 읽는다.
//     options: 현재 87바이트 전투 옵션(Uint8Array), players(): 현재 Player 9행(180바이트 기록, 1620바이트)을 돌려주는 함수이다.
//   hooks = { replenish(), income(owner, localOwner, amount), predictable(), wallSeconds(), budget(), density(), spacing(), whole(), partition(cell), count(rectangle), geyserCount(), typeCost(), choose(count), populate(quantity, random, rectangle, options) }
//     replenish: 실제 485e40 스펠 보충, income: 6ch/종류 2/서버 9 송신, predictable: 설정 predictableGeysers==1, wallSeconds: 지역 TimeSeed 용 벽시각, budget/density/spacing: 현재 원문 옵션 getter(미리 캡처한 기본값을 쓰지 않음),
//     whole/partition: 원문 전체/3×3 분할 사각형, count(rectangle): 현재 TYPE flags1 finder 의 수, geyserCount: 실제 목록 개수, typeCost: TYPE+c4(float), choose(count): 후보 둘 이상일 때만 공유 난수를 한 번 소비,
//     populate(quantity, random, rectangle, { excludePlayers, allowLand, allowVoid }): 실제 Pool/MayPlace/현재 자원/최근 목록 생성이다.
// 순서: 로그인 중이면 자연 반환 → 스펠 만료 한 번 소비(보스면 보충, 개수 감산 뒤 +20초 기한) → SP 주기 만료(비전투면 선택 초기화, 보스·전투·전원 로그인·주기 비영이면 최초는 1 만 쓰고 이후 숨김 아닌 구독자에게 지급) → 일반 간헐천 보충(옵션 18 이 2 면 3×3 구역 목표 개수와 부족 구역 하나, 1 이면 전체 한 개 조건).
// 한 만료당 한 번만 소비/생성하고 새 프레임/Player/Ready/권한을 합성하지 않는다. 읽기 전용 전역은 각 원문 위치에서 다시 읽고, 선택된 외부 효과만 요구하며 누락은 앞선 효과를 유지한 채 명시 오류이다.
// 원본 관측 fixture 가 없어 cpppj 소스 계약을 tests/raw-battle-resource-frame.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E560: 전투 자원 프레임 이식)
import { GameRandom } from './random.js';
import { RawFortFinishOwner } from './raw-fort-finish-owner.js';
import { battleGeyserPeriod, battleIncomeAmount, battleIncomePeriod, BATTLE_OPTIONS_BYTES } from './raw-battle-option-slots.js';

// Player 기록 폭·수·구독자 DWORD 위치(+0x2c)와 로그인 판정 플래그(+0x50 의 0x22 중 하나)이다. (2026-11-15 WB04-E560)
const PLAYER_STRIDE = 180;
const PLAYER_COUNT = 9;
const SUBSCRIBER_AT = 0x2c;
const FLAGS_AT = 0x50;
const LOGGED_MASK = 0x22;
// 전투 옵션 안의 숨김 BYTE 시작(62)과 간헐천 모드 BYTE 위치(18), 모드 값(1 전체 한 개·2 3×3 구역)이다. (2026-11-15 WB04-E560)
const HIDDEN_BASE = 62;
const GEYSER_MODE_AT = 18;
const MODE_WHOLE = 1;
const MODE_CELLS = 2;
// 스펠 다음 기한 가산(20초)·단독 간헐천 간격(30초)·로그인 직후 난수 초기값(0xbeef)과 구역 수(3×3=9)·분모 상수이다. (2026-11-15 WB04-E560)
const SPELL_INTERVAL = 20.0;
const SINGLE_GEYSER_PERIOD = 30;
const DEFAULT_RANDOM = 0xbeef;
const CELL_COUNT = 9;
const BUDGET_DIVISOR = 3;
// 읽기 전용 현재 전역은 { value } 상자·현재 값을 돌려주는 함수·독립 입력의 단순 값 중 하나로 받아 원문 위치마다 다시 읽는다. (2026-11-15 WB04-E560)
const read = (source) => (typeof source === 'function' ? source() : (source !== null && typeof source === 'object' ? source.value : source));

// 선택된 외부 효과가 함수가 아니면 명시 오류를 낸다(선택되지 않은 경로의 효과는 요구하지 않는다). (2026-11-15 WB04-E560)
function need(effect) {
    if (typeof effect !== 'function') throw new Error('일반 자원 프레임의 실제 장치 미연결');
    return effect;
}

// 감긴 DWORD 를 signed 32비트로 읽는다. (2026-11-15 WB04-E560)
const signed = (value) => value | 0;

// 비정렬 네 BYTE 를 낮은 BYTE 부터 unsigned 로 읽는다. (2026-11-15 WB04-E560)
function readDword(bytes, offset) {
    let value = 0;
    for (let index = 0; index < 4; index += 1) value = (value | (bytes[offset + index] << (index * 8))) >>> 0;
    return value;
}

// 현재 Player 9행을 다시 빌린다. 수신/생성 뒤에도 매번 현재 base 를 재조회하며 물리 크기가 1620 이 아니면 거부한다. (2026-11-15 WB04-E560)
function currentPlayers(globals) {
    const rows = need(globals.players)();
    if (rows.length !== PLAYER_COUNT * PLAYER_STRIDE) throw new RangeError('자원 프레임 Player9 물리 누락');
    return rows;
}

// 0048f4e0: 구독자가 있는 Player 1..8 모두가 flags 0x22 중 하나를 가졌는지 본다. 한 행이 틀려도 8행을 모두 순회하며 행마다 현재 base 를 다시 읽는다. (2026-11-15 WB04-E560)
function allLogged(globals) {
    let result = true;
    for (let owner = 1; owner < PLAYER_COUNT; owner += 1) {
        const rows = currentPlayers(globals);
        const base = owner * PLAYER_STRIDE;
        if (readDword(rows, base + SUBSCRIBER_AT) && !(rows[base + FLAGS_AT] & LOGGED_MASK)) result = false;
    }
    return result;
}

// x87 _ftol 의 signed64 절삭 하위 DWORD 이며 비유한/범위 밖은 indefinite 0 이다(signed 32비트로 돌려준다). (2026-11-15 WB04-E560)
function truncate(value) {
    if (!Number.isFinite(value) || value >= 9223372036854775808 || value < -9223372036854775808) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 구역별 목표 개수이다: 중앙(5)은 center, 모서리(1·3·7·9)는 corner, 변은 center/2(0 방향 절삭)이다. (2026-11-15 WB04-E560)
function target(cell, corner, center) {
    if (cell === 5) return center;
    if (cell === 1 || cell === 3 || cell === 7 || cell === 9) return corner;
    return Math.trunc(center / 2);
}

// 00486bc0 전체이다. 로그인 중이면 아무 것도 하지 않고, 한 만료당 한 번만 소비/생성한다. (2026-11-15 WB04-E560)
export function runBattleResourceFrame(globals, hooks) {
    const options = globals.options;
    if (options.length !== BATTLE_OPTIONS_BYTES) throw new RangeError('일반 자원 프레임 옵션87BYTE 누락');
    if (read(globals.loginEnd) !== 0.0) return;
    const aux = globals.aux;
    // 스펠 만료: 개수가 양수(signed)이고 기한이 있고 시각이 지났으면 보스에서 보충하고, 현재 개수 감산을 새 기한 store 보다 먼저 한다.
    if (signed(aux.spellCount) > 0 && aux.spellDeadline !== 0.0 && read(globals.now) >= aux.spellDeadline) {
        if (read(globals.boss)) need(hooks.replenish)();
        const next = read(globals.now) + SPELL_INTERVAL;
        aux.spellCount = (aux.spellCount - 1) >>> 0;
        aux.spellDeadline = next;
    }
    // SP 주기 만료: 간헐천 프레임 플래그가 없고 기한이 지났을 때만 한다.
    if (!aux.geyserFrameFlag && read(globals.now) >= aux.geyserDeadline) {
        if (!read(globals.battle)) aux.incomePrimed = 0;
        if (read(globals.boss) && read(globals.battle) && allLogged(globals) && battleIncomePeriod(options)) {
            const period = battleIncomePeriod(options);
            const primed = aux.incomePrimed;
            aux.geyserDeadline = period + read(globals.now);
            if (!primed) aux.incomePrimed = 1;
            else {
                // 원문 숨김 BYTE 를 먼저 읽고 현재 구독자가 있는 행만 지급한다.
                for (let owner = 1; owner < PLAYER_COUNT; owner += 1) {
                    if (!options[HIDDEN_BASE + owner] && readDword(currentPlayers(globals), owner * PLAYER_STRIDE + SUBSCRIBER_AT)) {
                        const amount = battleIncomeAmount(options);
                        need(hooks.income)(owner, read(globals.owner), amount);
                    }
                }
            }
        }
    }
    // 일반 간헐천 보충: 일반 간헐천 모드이거나 두번째 기한 전이면 끝난다.
    if (aux.ordinaryGeysers || !(read(globals.now) >= aux.secondaryGeyserDeadline)) return;
    const period = read(globals.single) ? SINGLE_GEYSER_PERIOD : battleGeyserPeriod(options);
    const boss = Boolean(read(globals.boss));
    aux.secondaryGeyserDeadline = period + read(globals.now);
    if (!boss || read(globals.editor) || !battleGeyserPeriod(options) || !allLogged(globals)) return;
    // 지역 난수는 예측 가능 설정이 아니면 벽시각 seed 로 바꾼다.
    const random = new GameRandom(DEFAULT_RANDOM);
    if (!need(hooks.predictable)()) random.setState(RawFortFinishOwner.timeSeed(need(hooks.wallSeconds)()));
    if (!read(globals.single) && read(globals.battle)) {
        if (options[GEYSER_MODE_AT] === MODE_CELLS) {
            // 예산/밀도/간격 getter 는 이 순서로 읽고 두 getter 사이에 첫 정수화를 한다.
            const budget = need(hooks.budget)();
            const density = need(hooks.density)();
            const original = budget;
            const densityAddition = density === 1 ? 0.1 : density === 2 ? 0.2 : 0.0;
            const adjusted = truncate((densityAddition + 1.0) * original);
            const spacing = need(hooks.spacing)();
            const sizeAddition = spacing === 3 ? -0.2 : spacing === 4 ? -0.1 : spacing === 6 ? 0.1 : 0.0;
            const corner = Math.trunc(Math.trunc(truncate(sizeAddition * original + adjusted) / 2) / 4);
            const center = signed(corner + 1);
            // 현재 TYPE 와 실제 flags1 finder 로 아홉 구역을 먼저 센다.
            const counts = new Array(CELL_COUNT + 1).fill(0);
            for (let cell = 1; cell <= CELL_COUNT; cell += 1) {
                const rectangle = need(hooks.partition)(cell);
                counts[cell] = signed(need(hooks.count)(rectangle));
            }
            // 후보는 원문 1..9 순서이며 목표보다 적은 구역이다.
            const choices = [];
            for (let cell = 1; cell <= CELL_COUNT; cell += 1) if (counts[cell] < target(cell, corner, center)) choices.push(cell);
            if (choices.length) {
                // 후보가 하나면 난수를 쓰지 않고, 둘 이상이면 실제 공유 난수를 한 번 소비한다.
                const selected = choices.length === 1 ? 0 : need(hooks.choose)(choices.length) >>> 0;
                if (selected >= choices.length) throw new RangeError('일반 간헐천 실제 난수 후보 범위');
                const cell = choices[selected];
                const missing = signed(target(cell, corner, center) - counts[cell]);
                const quantity = missing <= 1 ? 1 : Math.trunc(missing / 2);
                // 선택한 구역 범위를 다시 읽어 현재 자원으로 실제 배치한다.
                const rectangle = need(hooks.partition)(cell);
                need(hooks.populate)(quantity, random, rectangle, { excludePlayers: 1, allowLand: 1, allowVoid: 1 });
            }
        }
        if (options[GEYSER_MODE_AT] !== MODE_WHOLE) return;
    }
    // 전체 한 개 배치: float 자원보다 TYPE 비용이 작지 않으면 예산/3 목표를 넘는 개수일 때 끝낸다.
    const resource = Math.fround(aux.resourcePool);
    if (!(Math.fround(need(hooks.typeCost)()) < resource)) {
        const goal = Math.trunc(need(hooks.budget)() / BUDGET_DIVISOR);
        if (signed(need(hooks.geyserCount)()) >= goal) return;
    }
    const rectangle = need(hooks.whole)();
    need(hooks.populate)(1, random, rectangle, { excludePlayers: 0, allowLand: 1, allowVoid: 1 });
}
