// AI 초기 갱신(패치 00414710)을 옮긴 모듈이다(cpppj `o/RawAiBrainInitialUpdate.cpp/.h`, 10.78): 600바이트 설정 재복사 → 선택된 이름 → 실제 기술 부모 보장/목록 추가 → 기술 내용물을 순회한 건설 분류와 최대 비용.
//  - new RawAiBrainInitialUpdate(memory, player, technology, finder, types, globals, hooks): memory 는 E164 `RawAiBrainMemory`, player 는 같은 PATCH 풀의 `RawPlayerWorld`(실제 9 Player 와 기술 부모 생성),
//    technology 는 같은 풀의 `RawPlayerTechnology`(문자열 → 40개 목록 → ensure/contained 생성 사슬), finder 는 현재 타입 6/권한/죽음 진단 전역(`ContainedFinderState`), types 는 현재 TYPE 표의 같은 배열({ flags2, cost, theme }, 복사하지 않는다),
//    globals = { missionMode()(00594fa4 의 현재 비제로 표식), excludedType()(0054126c 의 현재 분류 제외 TYPE, 133), workshops(theme 0/1/2/3 의 현재 b0/b4/b8/ac 워크샾 TYPE 배열 4개) },
//    hooks = { defaultName()(004410f0 "defaultAiName" 실제 언어 조회), requiredEnergy(type)(TYPE+a0 의 현재 문자열, 최대 길이 분기에서는 다시 조회) } 이며 도달한 위치에서만 요구한다.
//  - update(token, settings): 600바이트 설정을 본체 처음에 다시 복사하고(이름 owner 는 복사 전에 캡처한다), 현재 mission 표식이 켜져 있으면 설정 이름(없으면 기본 언어 이름)을 Player 9기록의 이름(+0xc)에 쓴다. 이후 현재 owner 를 매 원본 위치에서 다시 읽어
//    기술 부모가 없으면 Player 를 생성하고 그래도 없으면 끝낸다. 설정의 기술 문자열(+0x1e)이 있으면 목록에 추가한다. 다섯 분류 플래그(+0x3b4..)와 최대 비용(+0x290)만 0 으로 하고 빌더 개수/원소 목록 개수/+0x394/+0x390/+0xad8 은 유지한다.
//    기술 부모의 내용물을 기존 커서로 순회하며(다음 SID 를 먼저 저장, 방문 시 현재 flags 4/TYPE 를 읽음) 빌더 이동 계열(flags2 0x30000)은 빌더 목록에 쌓고(용량 미만일 때), TYPE 요구 에너지 문자열 길이로 +0xad8 을 올리고, 최대 비용을 ftol 로 올린다.
//    분류 제외 TYPE 이 아니면 theme(0..5)별 개수를 세며(theme 5 는 실제 stack 칸이 sun battery 계수와 같은 칸을 공유) 0x800 battery 는 짝 계수를 올리고 첫 원소가 3 이면 theme 로 바꾼다. theme 별 첫 개수(5 로 나눈 나머지 1)마다 원소 목록에 추가하고
//    후보 워크샾의 ftol 비용이 기존 후보보다 크면 후보를 교체한다. 마지막에 단독 battery 원소를 중립 3 으로 치환하거나 한 번 제거하며 목록 개수를 압축한다. 지갑(+0x2d8)/SP 는 바꾸지 않는다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E168: AI 초기 갱신 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';
import { RawContainedFinder } from './raw-contained-finder.js';
import { MISSION_AI_SETTINGS_SIZE } from './raw-mission-ai-settings.js';

// Player 기록 폭/개수와 이름 위치, 설정의 기술 문자열/빌더/분류 위치와 TYPE 번호 0 이외의 상한이다. (2026-11-09 WB04-E168)
const PLAYER_BYTES = 180;
const PLAYERS = 9;
const NAME_AT = 12;
const TECHNOLOGY_TEXT_AT = 0x1e;
const MAX_SID = 65535;
const TWO_63 = 9223372036854775808;

// 비정렬 실제 DWORD 를 읽는다(범위 밖은 거부). (2026-11-09 WB04-E168)
function read(raw, offset) {
    if (offset < 0 || offset + 4 > raw.length) throw new RangeError('AI초기갱신실제필드범위오류');
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 원본 DWORD 의 바이트만 쓴다(주변 원문은 변경하지 않는다). (2026-11-09 WB04-E168)
function put(raw, offset, value) {
    if (offset < 0 || offset + 4 > raw.length) throw new RangeError('AI초기갱신실제필드범위오류');
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 원본 ftol 의 signed64 절삭 하위 DWORD 이며 FISTP 무효 결과의 하위 DWORD 는 0 이다. (2026-11-09 WB04-E168)
function truncate(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(value))));
}

// 실제 NUL 문자열을 소유 메모리 안에서 읽는다(NUL 없이 끝나면 명시 거부). 바이트마다 한 글자(latin1)로 읽는다. (2026-11-09 WB04-E168)
function text(raw, offset) {
    let end = offset;
    // NUL 바이트나 소유 메모리 끝을 만날 때까지 한 바이트씩 전진한다.
    while (end < raw.length && raw[end] !== 0) end += 1;
    if (end === raw.length) throw new RangeError('AI초기갱신NUL문자열소유메모리범위오류');
    let result = '';
    // 시작부터 NUL 앞까지 한 바이트씩 문자로 바꾼다.
    for (let index = offset; index < end; index += 1) result += String.fromCharCode(raw[index]);
    return result;
}

// 원본 TYPE 에너지 C 문자열 길이이며 NUL 이후는 포함하지 않는다. (2026-11-09 WB04-E168)
function length(value) {
    const nul = String(value).indexOf('\0');
    return nul < 0 ? String(value).length : nul;
}

// 초기 갱신 처리기이다. 큰 초기 update 를 빈 성공으로 대체하지 않고 원본 원문/목록과 실제 기술 사슬을 재사용한다. (2026-11-09 WB04-E168)
export class RawAiBrainInitialUpdate {
    #memory;
    #player;
    #technology;
    #finder;
    #types;
    #globals;
    #hooks;

    // 같은 메모리/Player9/실제 기술/contained 와 현재 TYPE 의 수명은 객체보다 길어야 한다. (2026-11-09 WB04-E168)
    constructor(memory, player, technology, finder, types, globals, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI초기갱신 본체 소유자 누락');
        if (player?.pool !== technology?.pool || player?.pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length === 0
            || !globals || globals.workshops?.length !== 4 || typeof globals.missionMode !== 'function' || typeof globals.excludedType !== 'function' || !finder) {
            throw new TypeError('AI초기갱신같은PATCH세계/TYPE/워크샾4개누락');
        }
        this.#memory = memory;
        this.#player = player;
        this.#technology = technology;
        this.#finder = finder;
        this.#types = types;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 타입 0 도 원본처럼 조회 가능하며 표 밖은 host 메모리 계약 오류이다. (2026-11-09 WB04-E168)
    #type(number) {
        if (number >= this.#types.length) throw new RangeError('AI초기갱신실제TYPE범위오류');
        return this.#types[number];
    }

    // 004709d0 의 0..3 반환/4..5 의 0 반환이며 원본 범위 밖 값은 명시 거부한다. (2026-11-09 WB04-E168)
    #workshop(theme) {
        if (theme < 0 || theme > 5) throw new RangeError('AI초기갱신실제원소범위오류');
        return theme < 4 ? this.#globals.workshops[theme] >>> 0 : 0;
    }

    // 원본은 owner 를 600바이트 복사 전에 캡처하며 이후 기술 처리는 현재 owner 를 다시 읽는다. (2026-11-09 WB04-E168)
    update(token, settings) {
        const raw = this.#memory.bytes(token);
        const player = this.#player;
        const pool = player.pool;
        const hooks = this.#hooks;
        if (raw.length !== AI_BRAIN_BYTES || settings?.length !== MISSION_AI_SETTINGS_SIZE) throw new TypeError('AI초기갱신실제본체/600byte설정누락');
        const nameOwner = read(raw, 0x260);
        if (nameOwner >= PLAYERS) throw new RangeError('AI초기갱신실제이름owner범위오류');
        raw.set(settings, 0);
        if (this.#globals.missionMode()) {
            let name;
            if (raw[0]) name = text(raw, 0);
            else {
                if (typeof hooks.defaultName !== 'function') throw new Error('AI실제기본이름언어조회누락');
                name = String(hooks.defaultName());
                name = name.slice(0, length(name));
            }
            const records = player.records;
            const offset = nameOwner * PLAYER_BYTES + NAME_AT;
            if (offset + name.length + 1 > records.length) throw new RangeError('AI초기갱신실제이름저장소범위오류');
            // 이름의 각 글자를 한 바이트로 쓰고 NUL 로 끝낸다.
            for (let index = 0; index < name.length; index += 1) records[offset + index] = name.charCodeAt(index) & 0xff;
            records[offset + name.length] = 0;
        }
        const owner = () => read(raw, 0x260) | 0;
        if (!player.technology(owner())) player.create(owner());
        if (!player.technology(owner())) return;
        if (raw[TECHNOLOGY_TEXT_AT]) this.#technology.addList(owner(), text(raw, TECHNOLOGY_TEXT_AT));
        const counts = new Int32Array(10);
        // 원본은 이 5개 분류 플래그와 최대 비용만 초기화한다. 빌더 count/원소 목록 count/394/390/ad8 은 유지한다.
        for (let index = 0; index < 5; index += 1) put(raw, 0x3b4 + index * 4, 0);
        put(raw, 0x290, 0);
        const parent = player.technology(owner());
        if (parent > MAX_SID || parent >= pool.capacity) throw new RangeError('AI초기갱신실제기술부모범위오류');
        const cursor = new RawContainedFinder(pool, this.#finder);
        // 기존 커서는 다음 SID 를 먼저 저장하고 현재 flags 4/TYPE 를 방문 시 읽는다.
        for (let sid = cursor.begin(parent); sid; sid = cursor.next()) {
            const piece = pool.slot(sid);
            const type = piece[18] | (piece[19] << 8);
            const entry = this.#type(type);
            if (!(piece[20] & 4)) continue;
            const builderCount = read(raw, 0x28c) | 0;
            const builderCapacity = read(raw, 0x288) | 0;
            if ((entry.flags2 & 0x30000) && builderCount < builderCapacity) {
                if (builderCount < 0) throw new RangeError('AI초기갱신실제빌더count범위오류');
                const builders = this.#memory.bytes(read(raw, 0x284));
                put(builders, builderCount * 4, type);
                put(raw, 0x28c, builderCount + 1);
            }
            if (typeof hooks.requiredEnergy !== 'function') throw new Error('AI초기갱신실제TYPE요구에너지누락');
            const firstLength = length(hooks.requiredEnergy(type));
            if (read(raw, 0xad8) <= firstLength) put(raw, 0xad8, length(hooks.requiredEnergy(type)));
            const maximum = read(raw, 0x290) | 0;
            const cost = Math.fround(entry.cost);
            put(raw, 0x290, truncate(maximum > cost ? maximum : cost));
            if (type === (this.#globals.excludedType() >>> 0)) continue;
            // 원본의 theme 5 는 실제 counts[5]를 사용하여 sun battery 계수와 같은 칸을 공유한다. 별도 6번째 원소로 분리하지 않는다.
            const theme = entry.theme | 0;
            if (theme < 0 || theme > 5) throw new RangeError('AI초기갱신원본분류stack배열범위오류');
            counts[theme] += 1;
            if (entry.flags2 & 0x800) {
                if (theme + 5 >= counts.length) throw new RangeError('AI초기갱신battery분류stack배열범위오류');
                counts[theme + 5] += 1;
                if (read(raw, 0x390) === 3) put(raw, 0x390, theme);
            }
            if (counts[theme] % 5 === 1) {
                const count = read(raw, 0x3b0) | 0;
                if (count < 0) throw new RangeError('AI초기갱신실제원소count범위오류');
                put(raw, 0x398 + count * 4, theme);
                put(raw, 0x3b0, (read(raw, 0x3b0) + 1) >>> 0);
                const candidate = this.#workshop(theme);
                const previous = read(raw, 0x394);
                if (!previous || (truncate(Math.fround(this.#type(candidate).cost)) | 0) > (truncate(Math.fround(this.#type(previous).cost)) | 0)) put(raw, 0x394, candidate);
            }
        }
        // 원본은 단독 battery 원소를 중립 3 으로 치환하거나 한 번 제거하며 기존 목록 count 전체를 압축한다.
        for (let theme = 0; theme < 5; theme += 1) {
            if (counts[theme + 5] !== 1 || counts[theme] !== 1) continue;
            let removed = 0;
            // 제거 뒤에도 source 는 원래 index 로 진행하며 destination 만 한 칸 앞으로 쓴다.
            for (let index = 0; index < (read(raw, 0x3b0) | 0); index += 1) {
                const value = read(raw, 0x398 + index * 4);
                put(raw, 0x398 + (index - removed) * 4, value);
                if (value === theme) {
                    if (counts[3] % 5 === 0) {
                        put(raw, 0x398 + index * 4, 3);
                        put(raw, 0x3b4 + theme * 4, 2);
                        counts[3] += 1;
                        break;
                    }
                    if (!removed) {
                        removed = 1;
                        put(raw, 0x3b4 + theme * 4, 1);
                        counts[3] += 1;
                    }
                }
            }
            put(raw, 0x3b0, (read(raw, 0x3b0) - removed) >>> 0);
        }
    }
}
