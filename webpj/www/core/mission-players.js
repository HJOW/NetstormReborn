// 미션 플레이어 목록과 기술·회수·승급·AI 허용 상태를 미션 스크립트 머리 값과 저장 Fort 에서 읽어 만드는 모듈이다(cpppj `o/Player.cpp/.h` 의 `MissionPlayers`, Player.cpp/Totalmade.cpp 의 미션 시작 상태 중 자원·기술·동맹·색만 연결).
// 사용: const players = MissionPlayers.load((key) => script.get(key), fort, types, 6500); players.players[2].stormPower; players.allied(1, 2); players.techAllowed[typeNumber];
//   get(key) = 미션 스크립트 머리 값 조회(없으면 undefined/null, 있으면 문자열; 빈 문자열도 있는 값). fort = `parseFort` 결과(저장 기술/덱을 복사해 둠). types = `createFortTypes` 결과(`records` 188개: name·fromAsset·group; 이름 조회는 모든 레코드의 이름을 ASCII 대소문자 무시로 비교해 첫 일치).
// 규칙: 플레이어 번호는 1~8(0 은 중립)이고 번호가 색 번호의 기본값이다(`none` 은 색으로 쓸 수 없는 이름). 사람(1번)은 이름 `Player` 와 `my…` 머리 값을, AI(2~8번)는 `ai<번호>…` 머리 값을 쓰며 2번만 번호 없는 구식 `ai…` 값을 대신 읽는다.
//   기술 허용표는 `allow`/`deny`/`all`/타입 이름 토큰을 앞에서부터 적용하고, 시작 지식은 `all` 이면 생산 그룹(0~8, `group < 9`)의 자산 타입 전체, 아니면 이름의 타입(0 보다 큰 번호)을 모아 중복 없이 정렬한다.
//   동맹은 자기 자신 비트에 유효 번호(1~8)만 더하고, 한쪽에만 지정된 동맹도 양쪽 비트를 켜 대칭으로 만든다. 숫자 머리 값은 C `strtod` 처럼 앞의 숫자를 읽는다(공백·부호·십진/16진 실수·inf/nan, 숫자가 없으면 0).
// 원본 관측 fixture 가 없는 모듈이며 시험은 손 계산과 실제 타입 표 대조로 한다. 이 모듈은 DOM 을 쓰지 않는다. 기준선 뒤 cpppj 변경이 없다(3.6절 ①). (2026-11-11 WB04-E321: 4-C Player 이식)
import { asciiLower, configParseLong } from '../assets/config.js';
import { FIRST_ASSET_TYPE_NUMBER } from '../assets/type-table.js';
import { configListItem } from './raw-mission-ai-settings.js';

// 원본 소유자 번호는 1~8이며 0은 중립이다. (2026-11-11 WB04-E321)
export const PLAYER_COUNT = 8;

// 00542660 이 가리키는 원본 색 이름의 번호 순서이다(0 번 `none` 은 색으로 고르지 않음). (2026-11-11 WB04-E321)
const COLOR_NAMES = Object.freeze(['none', 'blue', 'red', 'white', 'green', 'purple', 'yellow', 'lightblue', 'orange']);

// 전체 지식에 넣는 생산 그룹의 상한(미만)이다: 004914e0 은 NO_GROUP(10)과 misc(9)를 제외한 그룹 0~8 의 타입만 넣는다. (2026-11-11 WB04-E321)
const PRODUCTION_GROUP_LIMIT = 9;

// C `strtod` 가 읽는 앞부분의 문법이다("C" 로케일): 공백, 부호, 그 뒤 inf/infinity, nan(선택 괄호), 16진 실수(선택 p 지수), 십진 실수(선택 e 지수) 중 하나이다. (2026-11-11 WB04-E321)
const STRTOD_PATTERN = /^[ \t\n\v\f\r]*([+-]?)(?:(inf(?:inity)?)|(nan(?:\([0-9A-Za-z_]*\))?)|0[xX]((?:[0-9A-Fa-f]+(?:\.([0-9A-Fa-f]*))?|\.([0-9A-Fa-f]+)))(?:[pP]([+-]?[0-9]+))?|((?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?))/i;

// 문자열의 앞부분을 C `strtod` 처럼 실수로 읽는다(원본 atof 계열; 읽을 숫자가 없으면 0, 범위를 넘으면 ±Infinity). 16진 실수는 BigInt 가수를 반올림해 2의 거듭제곱을 곱한다. (2026-11-11 WB04-E321)
export function cStringToDouble(text) {
    const match = STRTOD_PATTERN.exec(text);
    if (!match) return 0;
    const sign = match[1] === '-' ? -1 : 1;
    if (match[2] !== undefined) return sign * Infinity;
    if (match[3] !== undefined) return Number.NaN;
    if (match[4] !== undefined) {
        // 16진 실수: 가수 자릿수 전체를 정수로 모으고 소수부 한 자리마다 지수를 4 줄인다.
        const fraction = match[5] ?? match[6] ?? '';
        const digits = match[4].replace('.', '');
        const exponent = (match[7] === undefined ? 0 : Number(match[7])) - 4 * fraction.length;
        const mantissa = Number(BigInt(`0x${digits}`));
        return sign * mantissa * 2 ** exponent;
    }
    return sign * Number(match[8]);
}

// 한 플레이어의 번호·색·SP·지식·기술·동맹 상태이다. (2026-11-11 WB04-E321)
export class Player {
    // number: 번호, color: 그림·글자 출력에 쓰는 팔레트 색 번호이다.
    number = 0;
    color = 0;
    // active: 현재 입력·실행 대상이 활성인지, computer: 컴퓨터 여부이다.
    active = false;
    computer = false;
    // stormPower: 스톰 파워(시작 SP)이다.
    stormPower = 0;
    // name: 이름, startingTech: 시작 기술 목록 문자열, ability: 능력 문자열이다.
    name = '';
    startingTech = '';
    ability = '';
    // allies: 동맹 비트(번호 n 의 비트 = 1 << n)이다.
    allies = 0;
    // knowledge: 시작 지식(타입 번호, 중복 없이 오름차순)이다.
    knowledge = [];
    // storedTechnology: 저장 Fort 의 내용물 목록 전체이다(complete 가 거짓이면 변환한 타입이 0 인 항목에서 읽기가 끊긴 것).
    storedTechnology = { items: [], complete: true };
    // storedDeck: 저장된 덱이다.
    storedDeck = [];
}

// 세미콜론 목록의 타입 이름을 현재 타입 번호로 바꾼다. 원본 목록 순서를 보존하지 않고 중복을 제거해 오름차순으로 정렬한다. all 은 생산 그룹의 자산 타입이다. (2026-11-11 WB04-E321)
function knowledge(text, records) {
    const result = [];
    // 빈 조각을 만나면 목록이 끝난 것이다.
    for (let index = 0; ; index += 1) {
        const token = configListItem(text, index);
        if (token === '') break;
        if (asciiLower(token) === 'all') {
            // 004914e0 은 NO_GROUP 을 제외한 생산 그룹의 타입만 전체 지식에 넣는다.
            for (let type = FIRST_ASSET_TYPE_NUMBER; type < records.length; type += 1) {
                if (records[type].fromAsset && records[type].group < PRODUCTION_GROUP_LIMIT) result.push(type);
            }
        } else {
            const type = findType(records, token);
            if (type > 0) result.push(type);
        }
    }
    return [...new Set(result)].sort((first, second) => first - second);
}

// 타입 이름(ASCII 대소문자 무시)으로 모든 레코드를 0 번부터 비교해 첫 일치 번호를 찾는다. 없으면 -1 이다(이름 없는 타입은 빈 이름을 찾을 때만 맞음). (2026-11-11 WB04-E321)
function findType(records, name) {
    const wanted = asciiLower(name);
    // 첫 일치가 이기므로 앞에서부터 본다.
    for (let type = 0; type < records.length; type += 1) if (asciiLower(records[type].name) === wanted) return type;
    return -1;
}

// 미션 플레이어 목록과 기술·회수·승급·AI 허용 상태이다. (2026-11-11 WB04-E321)
export class MissionPlayers {
    // players: 플레이어(0 번은 중립 자리), techAllowed: 타입 번호별 기술 허용(기본 목록이 비어 있음)이다.
    players = Array.from({ length: PLAYER_COUNT + 1 }, () => new Player());
    techAllowed = [];
    // denySalvage: 회수 금지, denyAscend: 승급 금지, aiOff: AI 끄기이다.
    denySalvage = false;
    denyAscend = false;
    aiOff = false;

    // 사람/AI 머리 값과 요새 저장 상태를 구분하여 초기화한다. 기술 허용표의 deny/allow/all 순서와 시작 SP 덮어쓰기를 적용한다. (2026-11-11 WB04-E321)
    static load(get, fort, types, fallbackMoney) {
        if (typeof get !== 'function' || fort === null || typeof fort !== 'object' || !Array.isArray(types?.records)) throw new TypeError('미션 플레이어 입력 오류');
        // 머리 값 조회는 없으면 undefined 이고 있으면 문자열(빈 문자열 포함)이어야 한다.
        const read = (key) => {
            const value = get(key);
            if (value === undefined || value === null) return undefined;
            if (typeof value !== 'string') throw new TypeError(`미션 머리 값은 문자열이어야 함: ${key}`);
            return value;
        };
        // 숫자 머리 값은 원본 atof 계열처럼 앞의 숫자를 읽고 없으면 대체값을 쓴다.
        const number = (value, fallback) => (value === undefined ? fallback : cStringToDouble(value));
        const records = types.records;
        const result = new MissionPlayers();
        result.techAllowed = new Array(records.length).fill(true);
        result.denySalvage = number(read('denySalvage'), 0) !== 0;
        result.denyAscend = number(read('denyAscend'), 0) !== 0;
        result.aiOff = number(read('aiOff'), 0) !== 0;
        const permissions = read('techAllowed') ?? '';
        let allow = true;
        // 00482eb0 의 상태 변경 토큰을 앞에서부터 적용한다.
        for (let index = 0; ; index += 1) {
            const token = configListItem(permissions, index);
            if (token === '') break;
            const lower = asciiLower(token);
            if (lower === 'allow') allow = true;
            else if (lower === 'deny') allow = false;
            else if (lower === 'all') result.techAllowed.fill(allow);
            else {
                const type = findType(records, token);
                if (type > 0) result.techAllowed[type] = allow;
            }
        }
        // 소유 번호를 색 번호의 기본값으로 사용한다.
        for (let owner = 1; owner <= PLAYER_COUNT; owner += 1) {
            const player = result.players[owner];
            player.number = owner;
            player.color = owner;
            player.computer = owner !== 1;
            player.allies = (1 << owner) & 0xffff;
            const prefix = `ai${owner}`;
            // 번호가 없는 구식 ai 설정은 플레이어 2의 기본 입력이다.
            const aiGet = (suffix) => {
                const value = read(prefix + suffix);
                if (value !== undefined) return value;
                return owner === 2 ? read(`ai${suffix}`) : undefined;
            };
            player.name = owner === 1 ? 'Player' : aiGet('Name') ?? '';
            player.active = owner === 1 || player.name !== '';
            player.stormPower = owner === 1 ? number(read('myStartMoney'), fallbackMoney) : number(aiGet('StartMoney'), 0);
            player.startingTech = owner === 1 ? read('myTech') ?? '' : aiGet('Tech') ?? '';
            player.knowledge = knowledge(player.startingTech, records);
            player.ability = aiGet('Ability') ?? '';
            const color = aiGet('Color');
            if (color !== undefined) {
                const found = COLOR_NAMES.indexOf(asciiLower(color));
                if (found > 0) player.color = found;
            }
            const allyText = owner === 1 ? read('myAllyList') ?? '' : aiGet('AllyList') ?? '';
            // 자기 자신은 항상 동맹이며 유효한 목록 번호만 추가한다.
            for (let index = 0; ; index += 1) {
                const token = configListItem(allyText, index);
                if (token === '') break;
                const ally = configParseLong(token);
                if (ally > 0 && ally <= PLAYER_COUNT) player.allies = (player.allies | (1 << ally)) & 0xffff;
            }
        }
        // 저장된 기술/덱은 시작 지식과 별도로 보존한다. 생산 덱의 재구성은 다음 단계다. 원본 Fort 와 독립된 복사본을 둔다.
        result.players[1].storedTechnology = structuredClone(fort.technology ?? { items: [], complete: true });
        result.players[1].storedDeck = structuredClone(fort.deck ?? []);
        // 경로 조회에서도 동맹을 대칭으로 사용한다.
        for (let first = 1; first <= PLAYER_COUNT; first += 1) {
            // 두 플레이어 중 한쪽의 지정으로 양쪽 비트를 켠다.
            for (let second = first + 1; second <= PLAYER_COUNT; second += 1) {
                if (result.allied(first, second)) {
                    result.players[first].allies = (result.players[first].allies | (1 << second)) & 0xffff;
                    result.players[second].allies = (result.players[second].allies | (1 << first)) & 0xffff;
                }
            }
        }
        return result;
    }

    // 목록 한쪽에만 지정된 동맹도 양방향으로 조회한다. 중립·범위 밖 번호는 어느 플레이어와도 동맹이 아니다. (2026-11-11 WB04-E321)
    allied(first, second) {
        if (!Number.isInteger(first) || !Number.isInteger(second) || first <= 0 || second <= 0 || first > PLAYER_COUNT || second > PLAYER_COUNT) return false;
        return (this.players[first].allies & (1 << second)) !== 0 || (this.players[second].allies & (1 << first)) !== 0;
    }
}
