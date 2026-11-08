// 현재 HP 와 캡처 마스크에서 고정 연소점 행을 선택하는 원본 Burning 몸체이다(cpppj `o/RawBurningSources.cpp/.h`, 10.78: 패치 00424110 연소점 선택. Step/캡처/팔레트/표시는 별도 연결).
//  - new RawBurningSources(pool, types, visual, random, state): pool 은 현재 victim raw HP/타입을 읽는 같은 PATCH SidPool, types 는 현재 HasHP/genus/최대 HP 를 읽는 실제 188 TYPE 표(`{ flags1, flags2, maxHitPoints }`), visual 은 같은 풀의 `RawBurningVisual`(고정 source 점의 실제 150슬롯 Append 몸체),
//    random 은 다른 게임 기능과 공유하는 실제 32비트 게임 난수(`next(limit)`, CRT rand 가 아님), state 는 실제 최대 HP 조회의 현재 옵션/타입을 매 호출 다시 읽는 `{ weakenedMana, manaType: 158, daisType: 165, sharedWeakenedMana?(), sharedManaType?(), sharedDaisType?(), victim: { checking(), robust?, assertion? } }` 이다.
//    shared* getter 가 있으면 독립 값보다 우선한다(실제 GameAux 의 사제 1/4 HP·사제/다이스 TYPE 주소를 빌려 Session 생성/반납으로 바꾸지 않음).
//  - select(fields): 현재 HP(최대 HP getter/진단보다 먼저 같은 물리 슬롯에서 저장)와 이미지 크기로 목표 행을 구한다. 피해 부모(`runBurningVictimParent`, Sources 입구)가 조기 반환하면 아무것도 하지 않고, 최대 HP 진단 복귀 뒤 같은 선택 슬롯의 현재 TYPE 의 flags2 0x400 이면 목표를 1 로 강제한다.
//    목표가 선택 개수와 같으면 반환, 이미지 높이가 3 미만이면 선택 개수만 갱신, 선택 개수가 작으면 최대 300번의 시도로 난수(x → y 순, 아래 픽셀이 투명하면 rowTaken 에 도달하지 않음)로 행을 골라 6..9개의 대각선 점을 Append 하며(성공한 점마다 rowTaken 표식, 4개 이상 성공이면 선택 +1),
//    300번째 시도 뒤 남은 횟수가 0 이면 마지막 행을 제외한 y/x 순서의 최초 7개 불투명 점 폴백을 실행한다. 선택 개수가 크면 첫 source 점의 행 표식(1 이어야 함)을 지우고 같은 행의 source 표식/life 를 0 으로 바꾸되 total/selected 는 보존한다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E183: Burning 연소점 선택 이식으로 추가)
import { BurningVictimEntry, makeBurningVictimHooks, runBurningVictimParent } from './raw-burning-victim.js';
import { burningSourceCount } from './raw-burning-source-count.js';
import { currentRiftTypeNumber, readRiftHitPoints } from './squid-factory.js';

// 원본의 연소점 행 폭·시도 상한·폴백 점 수·투명색과 고정 source 수명이다. (2026-11-09 WB04-E183)
const ROW_WIDTH = 5;
const ATTEMPTS = 300;
const FALLBACK = 7;
const TRANSPARENT = 0xff;
const SOURCE_LIFE = 9999;
const INT32_MAX = 2147483647;
const TWO_31 = 2147483648;

// 연소점 선택기이다. 기존 Append/난수와 현재 타입 저장소를 사용하며 다른 풀의 시각 효과를 합치지 않는다. (2026-11-09 WB04-E183)
export class RawBurningSources {
    #pool;
    #types;
    #visual;
    #random;
    #state;

    // 풀 동일성과 필수 협력자를 생성 때 확인한다. (2026-11-09 WB04-E183)
    constructor(pool, types, visual, random, state) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Burning sources 는 10.78 SidPool 만 받습니다.');
        if (visual?.pool !== pool) throw new TypeError('Burning sources/Append SID 풀 불일치');
        if (!Array.isArray(types) || typeof random?.next !== 'function' || !state?.victim) throw new TypeError('Burning sources 타입 표/난수/상태 누락');
        this.#pool = pool;
        this.#types = types;
        this.#visual = visual;
        this.#random = random;
        this.#state = state;
    }

    // 후속 실제 Step/현재 피해 월드 연결의 풀 동일성 검사용 풀이다. (2026-11-09 WB04-E183)
    get pool() { return this.#pool; }

    // 게임 정수 난수의 실제 상한은 제외이며 minimum 포함/maximum 제외, maximum ≤ minimum 의 0 나눗셈은 거부한다. (2026-11-09 WB04-E183)
    #random32(minimum, maximum) {
        if (maximum <= minimum) throw new RangeError('Burning sources 원본 난수 나눗셈 범위 오류');
        return (minimum + this.#random.next((maximum - minimum) >>> 0)) | 0;
    }

    // 원본 char 픽셀 0xff 투명 검사와 정상 캡처 범위를 확인한다. (2026-11-09 WB04-E183)
    #opaque(fields, x, y) {
        const { width, height, pixels } = fields.image;
        if (x < 0 || y < 0 || x >= width || y >= height) throw new RangeError('Burning sources 캡처 픽셀 범위 오류');
        const index = y * width + x;
        if (index >= pixels.length) throw new RangeError('Burning sources 캡처 버퍼 미연결');
        return pixels[index] !== TRANSPARENT;
    }

    // 원본 float 좌표를 0 방향 정수화해 rowTaken 의 대각선 행을 찾는다. 비유한/범위 밖 좌표는 원본 ftol 의 미정의 후속 접근 전에 거부한다. (2026-11-09 WB04-E183)
    #row(particle) {
        const { x, y } = particle;
        if (!Number.isFinite(x) || !Number.isFinite(y) || x < -TWO_31 || x >= TWO_31 || y < -TWO_31 || y >= TWO_31) throw new RangeError('Burning sources 연소점 좌표 오류');
        const sum = Math.trunc(x) + Math.trunc(y);
        if (sum < -TWO_31 || sum > INT32_MAX) throw new RangeError('Burning sources 행 덧셈 범위 오류');
        return Math.trunc(sum / ROW_WIDTH) | 0;
    }

    // 원본의 선택 개수는 실제 추가 성공 수와 다르며 제거 후에도 selected 를 감소시키지 않는다. (2026-11-09 WB04-E183)
    select(fields) {
        const pool = this.#pool;
        const state = this.#state;
        const types = this.#types;
        const victim = runBurningVictimParent(() => fields.victim, BurningVictimEntry.Sources, state.victim.checking ?? (() => 0), makeBurningVictimHooks(pool, types, state.victim));
        if (victim === null) return;
        const selected = victim;
        // 원문은 최대 HP getter/진단보다 먼저 같은 물리 슬롯의 HP(DWORD)를 저장한다.
        const raw = pool.slot(selected);
        const hitPoints = (raw[26] | (raw[27] << 8) | (raw[28] << 16) | (raw[29] << 24)) | 0;
        const type = raw[10];
        if (type >= types.length) throw new RangeError('Burning sources 현재 타입 범위 오류');
        const record = types[type];
        const checking = state.victim.checking ?? (() => 0);
        const maximum = readRiftHitPoints(record, {
            checking: () => checking() >>> 0,
            quarter: () => (state.sharedWeakenedMana ? state.sharedWeakenedMana() : state.weakenedMana),
            priest: () => (state.sharedManaType ? state.sharedManaType() : state.manaType),
            dais: () => (state.sharedDaisType ? state.sharedDaisType() : state.daisType)
        }, {
            number: () => currentRiftTypeNumber(record, types, types),
            assertion: (expression, file, line) => {
                if (typeof state.victim.assertion !== 'function') throw new Error('연소점 TYPE HP의 실제 진단 장치 미연결');
                state.victim.assertion(expression, file, line);
            }
        });
        let wanted = burningSourceCount(hitPoints, maximum, fields.image.width, fields.image.height);
        // 원문 4ac200 은 진단 뒤 같은 선택 슬롯의 현재 TYPE 를 다시 읽는다.
        const currentType = pool.slot(selected)[10];
        if (currentType >= types.length) throw new RangeError('연소점 진단 복귀 뒤 현재 TYPE 범위');
        if (types[currentType].flags2 & 0x400) wanted = 1;
        if (fields.selected === wanted) return;
        if (fields.image.height < 3) {
            fields.selected = wanted;
            return;
        }
        const { width, height } = fields.image;
        // 실제 픽셀/행 주소가 필요한 경계만 호스트 범위를 확인한다.
        if (width < 0 || height < 0 || width + height > INT32_MAX) throw new RangeError('Burning sources 이미지 크기 오류');
        if (fields.total > fields.capacity || fields.particles.length !== fields.capacity) throw new RangeError('Burning sources 현재 입자 배열 오류');
        const rowCount = Math.trunc((width + height) / ROW_WIDTH);
        if (fields.rows.length < rowCount) throw new RangeError('Burning sources 행 표식 미연결');
        if (fields.selected < wanted) {
            const xMaximum = Math.min(width - 1, Math.max(8, width - 7));
            wanted = Math.min(wanted, Math.trunc((xMaximum + height - 3) / ROW_WIDTH));
            let attempts = ATTEMPTS;
            // 원본은 300번째 시도 뒤 목표에 닿아도 남은 횟수가 0 이면 폴백을 실행한다.
            while (fields.selected < wanted) {
                const before = attempts;
                attempts -= 1;
                if (!before) break;
                const x = this.#random32(0, xMaximum);
                const y = this.#random32(2, height - 1);
                const row = Math.trunc((x + y) / ROW_WIDTH);
                // 10.78 은 아래 픽셀이 투명하면 rowTaken 에 도달하지 않는다(작은 캡처의 끝 후보도 이 순서로 단락).
                if (!this.#opaque(fields, x, y + 1)) continue;
                if (row < 0 || row >= fields.rows.length) throw new RangeError('Burning sources 새 행 인덱스 오류');
                if (fields.rows[row]) continue;
                const count = this.#random32(6, 10);
                let accepted = 0;
                // 같은 대각선의 각 점은 Append 실패와 무관하게 rowTaken 을 표식한다.
                for (let index = 0; index < count; index += 1) {
                    const px = x + Math.trunc(index / 2);
                    const py = y - Math.trunc(index / 2);
                    if (px < width - 1 && py >= 0 && this.#opaque(fields, px, py)) {
                        this.#visual.append(fields, { x: px, y: py, velocityX: 0, velocityY: 0, life: SOURCE_LIFE, source: true });
                        accepted += 1;
                        fields.rows[row] = 1;
                    }
                }
                if (accepted > 3) fields.selected += 1;
            }
            if (attempts < 1) {
                let left = FALLBACK;
                // 폴백은 마지막 행을 제외한 y/x 순서에서 최초 7개 불투명 점을 사용한다.
                for (let y = 0; y < height - 1 && left; y += 1) {
                    // 원본은 Append 가 실패해도 불투명 점 하나를 사용한 것으로 센다.
                    for (let x = 0; x < width && left; x += 1) {
                        if (this.#opaque(fields, x, y)) {
                            this.#visual.append(fields, { x, y, velocityX: 0, velocityY: 0, life: SOURCE_LIFE, source: true });
                            left -= 1;
                        }
                    }
                }
            }
            return;
        }
        let first = fields.total;
        // 선택 개수가 줄면 실제 life 와 무관하게 첫 source 포인터 표식을 찾는다.
        for (let index = 0; index < fields.total; index += 1) {
            if (fields.particles[index].source) {
                first = index;
                break;
            }
        }
        if (first === fields.total) return;
        const row = this.#row(fields.particles[first]);
        if (row < 0 || row >= fields.rows.length) throw new RangeError('Burning sources 제거 행 인덱스 오류');
        if (fields.rows[row] !== 1) throw new Error('Burning sources rowTaken 원본 assert');
        fields.rows[row] = 0;
        // 같은 source 행만 포인터 표식/life 를 0 으로 바꾸고 total/selected 는 보존한다.
        for (let index = 0; index < fields.total; index += 1) {
            const particle = fields.particles[index];
            if (particle.source && this.#row(particle) === row) {
                particle.source = false;
                particle.life = 0;
            }
        }
    }
}
