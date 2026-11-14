// 원본 초기 카메라 선택(resetCamera)과 world float 중심 이동(433ba0/4340d0)을 같은 raw SID 풀의 현재 슬롯과 화면에 연결한다(cpppj client/CameraHome.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// Reset 은 free 가 아닌 슬롯을 SID 2부터 번호 순으로 훑어 로컬 사제(요새/전투 모드)나 Challenge 타입의 마지막 후보를 고르고, 선택이 있으면 그 슬롯으로, 사제가 둘 이상이면 단정도 평균으로, 아니면 (128,128) 로 중심을 옮긴다.
// 중심 이동은 433aa0 전체 부모(center 콜백)에 world float 를 그대로 넘기며 미니맵의 497f20 직접 요청과 달리 현재 카메라도 즉시 적용한다. 억제 깊이(005c89b8)는 Reset 동안 1 올렸다 내리며 예외에도 복구한다.
// 사용: const home = new CameraHome(pool, types, createCameraHomeState(), (x, y) => camera.centerImmediately(...), hooks); home.reset(); home.centerOn(sid). (2026-11-15 WB04-E464)
import { configParseLong } from '../assets/config.js';
import { readRawTextToken } from '../ui/html-text.js';

// resetCamera 가 선택하는 사제 genus(flags2)와 4ac200 이 Carrier 로 보는 두 genus 비트, free/dead 공통 상태 비트다.
const PRIEST_GENUS = 0x200000; const CARRIER_GENUS = 0x30000; const FREE = 1; const DEAD = 2;
// 슬롯 안의 위치: TYPE +10, 상태 +11, 좌표 DWORD +14/+18, 10.78 소유자 BYTE +34(CD 는 +32).
const TYPE_AT = 10; const STATE_AT = 11; const X_AT = 14; const Y_AT = 18; const OWNER_AT = 34;
// 대본 `<$ViewSpot,(XxY)>` 명령 번호(보호 00542a58, 이름 표 00543250 의 ViewSpot)이며 RunForIt [Init]이 입장 카메라에 쓴다.
export const VIEW_SPOT_COMMAND = 0x020000d9;
// 호출 선택이 없을 때 쓰는 CameraHomeState 기본값이다(원본 PE 의 초기 모드/화면/월드 크기).
export function createCameraHomeState() {
    return { localPlayer: 1, fort: false, battle: true, challenge: false, challengeType: 0, width: 640, height: 480, worldWidth: 4096, worldHeight: 2816, suppressionDepth: 0, requests: 0, revision: 0 };
}

// 슬롯의 정렬되지 않은 좌표 DWORD 를 직접 읽는다(주변 payload 는 읽지 않는다).
function coordinate(raw, offset) { return new DataView(raw.buffer, raw.byteOffset + offset, 4).getFloat32(0, true); }

// 원문 433ad0 의 인자 해석이다: 맨 앞 '(' 한 글자만 건너뛰고 'x' 와 ')' 로 나눈 두 토큰을 atol 한다. 숫자가 아닌 필드는 atol 처럼 0 이다. 두 번째 토큰은 첫 토큰이 멈춘 위치에서 이어 읽는다.
// 반환은 CenterPoint 와 같은 칸 단위 float 두 값이다(원문 atol→fild→fstp). 사용: parseViewSpot('(139x88)') → { x: 139, y: 88 }.
export function parseViewSpot(text) {
    const first = readRawTextToken(text, text.length > 0 && text[0] === '(' ? 1 : 0, 'x');
    const second = readRawTextToken(text, first.next, ')');
    return { x: Math.fround(configParseLong(first.text)), y: Math.fround(configParseLong(second.text)) };
}

// 원본 PATCH 4340d0 의 초기 대상과 같은 중심 이동 몸체다. 풀/타입/상태는 호출 동안 살아 있어야 한다.
// center(x, y)는 world float 를 받는 433aa0 전체 부모이며 픽셀 setter 로 대체하지 않는다. hooks = { contentPreview(sid), suppressionDepth()→DWORD, setSuppressionDepth(DWORD) }(깊이 두 장치는 쌍으로만 제공).
export class CameraHome {
    // 번호 순 조회와 free/dead/좌표를 공유하는 실제 풀, 같은 TYPE 의 genus/발자국, 매 조회의 모드/소유자/화면 크기·카운터, 433aa0 부모, 외부 경계다.
    #pool; #types; #state; #center; #hooks;
    // 실제 현재 상태와 화면 소비자를 첫 풀 접근 전에 확인한다(C++ invalid_argument → TypeError).
    constructor(pool, types, state, center, hooks = {}) {
        if (!types?.length || typeof center !== 'function' || !(state.width > 0) || !(state.height > 0) || pool.capacity > 65536 || Boolean(hooks.suppressionDepth) !== Boolean(hooks.setSuppressionDepth)) throw new TypeError('초기 카메라의 실제 풀/타입/화면 소비자 범위 오류');
        this.#pool = pool; this.#types = types; this.#state = state; this.#center = center; this.#hooks = { ...hooks };
    }

    // 외부 화면 효과가 로딩 깊이를 바꿨으면 예전 사본이 아니라 현재 실제 값을 읽는다.
    #depth() {
        if (this.#hooks.suppressionDepth) this.#state.suppressionDepth = this.#hooks.suppressionDepth() >>> 0;
        return this.#state.suppressionDepth;
    }

    // 실제 소유자 갱신을 먼저 끝내고 현재 state 에도 원문의 깊이를 반영한다.
    #setDepth(depth) {
        if (this.#hooks.setSuppressionDepth) this.#hooks.setSuppressionDepth(depth >>> 0);
        this.#state.suppressionDepth = depth >>> 0;
    }

    // 원문 433ba0/4340d0 처럼 float 저장 뒤의 좌표만 부모에 넘기며 카운터/변환을 여기서 먼저 실행하지 않는다.
    centerPoint(x, y) { this.#center(x, y); }

    // PATCH 433ba0: 실제 SID 의 현재 좌표에서 TYPE 발자국 전체(절반이 아님)를 뺀 float 로 중심을 옮긴 뒤 현재 Carrier genus 의 preview 를 요청한다. dead 를 고른 경우 다른 후보를 다시 찾지 않는다(free/dead 는 자연 반환).
    centerOn(sid) {
        if (!sid) return;
        const pool = this.#pool; const raw = pool.physical(sid); // 선택된 같은 슬롯을 중심 이동 직전에 다시 읽는다.
        if (raw[STATE_AT] & (FREE | DEAD)) return;
        if (raw[TYPE_AT] >= this.#types.length) throw new RangeError('초기 카메라의 현재 TYPE 누락');
        const type = this.#types[raw[TYPE_AT]];
        const x = Math.fround(coordinate(raw, X_AT) - type.footX); const y = Math.fround(coordinate(raw, Y_AT) - type.footY); // 계산 뒤 float 저장을 거친다.
        this.centerPoint(x, y);
        const currentType = pool.physical(sid)[TYPE_AT]; // 화면 적용 뒤 원본 4ac200 이 읽는 현재 타입을 다시 조회한다.
        if (currentType >= this.#types.length) throw new RangeError('중심 이동 뒤 현재 TYPE 누락');
        if (this.#types[currentType].flags2 & CARRIER_GENUS) {
            if (this.#hooks.contentPreview) this.#hooks.contentPreview(sid);
            else if (!this.#depth()) throw new Error('카메라 Carrier의 실제 내용물 표시 미연결');
        }
    }

    // 원본 반복자는 2부터 시작하며 free 비트만 보고 다음 현재 번호로 간다. 단정도 누적(합/개수)과 마지막 후보, 억제 깊이 증감, 예외 복구를 원문 순서대로 유지한다.
    reset() {
        const pool = this.#pool; const state = this.#state; let selected = 0; let sumX = 0; let sumY = 0; let count = 0; let challengeX = 0; let challengeY = 0; let foundChallenge = false;
        // free/dead/void 를 합쳐 걸러내지 않고 원본과 같은 SID 순서를 유지한다.
        for (let number = 2; number < pool.capacity; number += 1) {
            const raw = pool.physical(number); // 반복 중의 현재 원본 슬롯이다.
            if (raw[STATE_AT] & FREE) continue;
            if (raw[TYPE_AT] >= this.#types.length) throw new RangeError('초기 카메라 순회의 TYPE 범위 오류');
            if ((state.fort || state.battle) && raw[OWNER_AT] === state.localPlayer && (this.#types[raw[TYPE_AT]].flags2 & PRIEST_GENUS)) {
                selected = number; sumX = Math.fround(sumX + coordinate(raw, X_AT)); sumY = Math.fround(sumY + coordinate(raw, Y_AT)); count = Math.fround(count + 1);
            }
            if (state.challenge && raw[TYPE_AT] === state.challengeType) {
                selected = number;
                if (raw[OWNER_AT] === state.localPlayer) { challengeX = coordinate(raw, X_AT); challengeY = coordinate(raw, Y_AT); foundChallenge = true; break; }
            }
        }
        // 이 원본 분기는 좌표를 쓰지 않은 스택을 읽는다. 정의된 미션 초기화 경로와 구별해 실행을 거부한다.
        if (state.challenge && selected && !foundChallenge) throw new Error('원본 Challenge 초기 카메라의 미초기화 좌표 경로');
        const depth = this.#depth(); // 실제 object loadingDepth 와 같은 현재 전역을 증가 직전에 읽는다.
        if (depth === 0xffffffff) throw new RangeError('카메라 억제 깊이 감김의 외부 표시 생성 경로 미연결');
        this.#setDepth(depth + 1);
        try {
            if (state.challenge) {
                if (!selected) {
                    challengeX = Math.fround(state.worldWidth * 0.5 * 0.0625); challengeY = Math.fround(state.worldHeight * 0.5 * Math.fround(0.09090909361839294));
                }
                this.centerPoint(challengeX, challengeY);
            } else if (selected) this.centerOn(selected);
            else if (count > 1) { const inverse = Math.fround(1 / count); this.centerPoint(Math.fround(sumX * inverse), Math.fround(sumY * inverse)); }
            else this.centerPoint(128, 128);
        } catch (error) { this.#setDepth(this.#depth() - 1); throw error; }
        this.#setDepth(this.#depth() - 1);
    }
}
