// 원본 타입 표(오브젝트 정의 `.type` 파일 116개의 로딩 순서·프레임 코드·특수 프레임·기준점)를 읽는 모듈이다.
// 본문 속 그림 명령 `~[I이름.프레임]` 이 "타입 이름 → 타입 번호 → 셰이프 블록 → 프레임 번호" 로 가려면 이 표가 필요하다.
// 복원 기준: cpppj `RiftType.h/.cpp`(프레임 코드·글자 구간), `TypeParser.cpp`(.type 문법), `RiftTypeTable.cpp`(번호·이름·기준점 비율), `TypeLoadOrder.cpp`(로딩 순서)의
// 10.78 패치판 경로이다. 원본 `Rifttype.cpp` 로더(0049c3b0)·글자 구간 표 생성(0049b060)·이름 조회(0049a860)에 대응한다.
// 규칙 요약:
//  - 자산 타입의 번호 = 70 + 로딩 순서(0~115), 셰이프 블록 번호 = 로딩 순서. 타입 표 전체 크기는 188 이다(내장 타입 70개와 뒤쪽 여분 포함).
//  - 클러스터(프레임 한 칸)는 `이름 : 플래그 : "그림" #번호 ;` 한 줄이며 선언 순서가 논리 프레임 번호이다(레이어는 셰이프에서 뒤쪽에 붙는다).
//  - 클러스터 이름은 첫 글자(방향 글자 A~Z), 둘째 글자(변형, 한 글자 이름이면 'P'), 숫자(하위 8비트)로 쪼갠다.
//  - `default` 가 마지막으로 나온 클러스터가 기본 프레임, `gumpframe` 가 붙은 클러스터가 그림(gump) 프레임(없으면 -1 이라 기본 프레임으로 돌아간다).
//  - 방향 글자 'A'~'P' 마다 그 글자를 가진 첫 클러스터의 번호와, 글자가 하나라도 있는 글자의 수(letterKinds)를 구한다.
//  - `hotFootRatioX/Y` 속성은 비율 × (가로 16, 세로 11) 을 0쪽으로 버린 값이 타입 기준점이다.
// (2026-11-01 WB03-T: 도움말 그림(아이콘) 지원을 위해 추가)

// 자산 타입의 첫 번호이다(원본 `(읽은 파일 수 + 0x45) * 500` 의 0x45 = 69 에 1 을 더한 70, 두 판본 공통). (2026-11-01 WB03-T)
export const FIRST_ASSET_TYPE_NUMBER = 70;

// 패치판 타입 표의 전체 크기이다(cpppj kPatchTypeCount). 이 번호 이상은 타입이 없다. (2026-11-01 WB03-T)
export const PATCH_TYPE_COUNT = 188;

// 방향 글자(A~P)의 개수이다(원본 글자 구간 표 크기 16). (2026-11-01 WB03-T)
export const LETTER_COUNT = 16;

// 패치판의 자산 타입 로딩 순서 116개이다(Netstorm.exe 0x540dd0 의 이름 포인터 배열). 블록 i = 로딩 순서 i 번째 타입이다.
// 이름은 파일 이름(`\d\이름.type`)이며 대소문자를 구분하지 않고 찾는다. (2026-11-01 WB03-T)
export const TYPE_LOAD_ORDER = Object.freeze([
    'dude', 'sunArcher', 'sunAviary', 'sunFlyer', 'banner',
    'windBattery', 'rainBattery', 'thunderBattery', 'sunBlocker', 'thunderBlocker',
    'blankMissile', 'bolt', 'bridge', 'sunDisc', 'emptyGeyser',
    'sunFactory', 'windFactory', 'rainFactory', 'thunderFactory', 'residence',
    'fencemark', 'flag', 'fortGump', 'icon', 'island',
    'islandStalag', 'playerBanner', 'isle', 'isleBig', 'geyserBrightener',
    'treeTwo', 'treeThree', 'fringe', 'mana', 'range',
    'manabolt', 'flare', 'puzzlePiece', 'playerIsland', 'battleIsland',
    'particlePlaceHolder', 'sunBalloon', 'buried', 'bombExplodeSmall', 'bombExplodeMedium',
    'bombExplodeLarge', 'bombHeal', 'bombInvisible', 'bombParalyze', 'bombHardener',
    'bombTreason', 'edgeFarm', 'geyser', 'windVortex', 'rainVortex',
    'thunderVortex', 'outpost', 'mcloud', 'sunCannon', 'rainCannon',
    'rainCannonMissile', 'thunderCannon', 'thunderCannonMissile', 'sunWalker', 'teleportEffect',
    'bulf', 'sunFence', 'windWalker', 'windFlyer', 'windAviary',
    'windArcher', 'windBalloon', 'windBlocker', 'rainAviary', 'rainFlyer',
    'rainFence', 'rainBalloon', 'rainBlocker', 'thunderArcher', 'thunderFence',
    'growingRainBlocker', 'platform', 'player', 'mog', 'nugget',
    'bridgeConnector', 'rainWalker', 'noIsland', 'priest', 'lightning',
    'anim', 'challengeIsland', 'fakeThreeByThreeSurface', 'sunFlyerBomb', 'altar',
    'dais', 'rune', 'forceField', 'bombSpecialOne', 'daisExtraFrames',
    'fenceShield', 'bombTwister', 'bombIITwister', 'bombIIITwister', 'GravitationEffect',
    'bombGraviton', 'HealEffect', 'MeteorEffect', 'BOMBmeteor', 'bombIMano',
    'bombIIMano', 'bombIIIMano', 'bombLightingZap', 'bombLightingwave', 'RUIN',
    'Monument',
]);

// 클러스터 줄의 플래그 단어와 그 비트이다(원본 로더가 프레임 코드의 4번째 바이트에 반영하는 7종, 대소문자 무시). 그 밖의 단어는 원본이 무시한다. (2026-11-01 WB03-T)
const CLUSTER_FLAG_BITS = Object.freeze({ fringe: 0x01, suck: 0x02, rim: 0x04, lit: 0x08, unlit: 0x10, cracked: 0x20, hard: 0x40 });

// 기준점 비율의 고정 배율이다(가로 16, 세로 11, 원본 ROM 상수 00503358/0050334c). (2026-11-01 WB03-T)
const HOT_FOOT_SCALE_X = 16;
const HOT_FOOT_SCALE_Y = 11;

// 글자(영문자·밑줄) 여부이다(원본 자산 문법의 식별자 글자). (2026-11-01 WB03-T)
function isLetter(char) {
    return (char >= 'A' && char <= 'Z') || (char >= 'a' && char <= 'z') || char === '_';
}

// 숫자 글자 여부이다. (2026-11-01 WB03-T)
function isDigit(char) {
    return char >= '0' && char <= '9';
}

// 숫자 토큰의 모양이다(부호·정수부·소수부·지수). 원본 from_chars(general)처럼 `00`·`08` 도 10진수로 읽는다. (2026-11-01 WB03-T)
const NUMBER_PATTERN = /^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?/;

// `.type` 문법의 토큰 분석기이다. 단어(식별자)·문자열·숫자·한 글자 문장부호·끝을 구분하고 `//` 주석과 공백을 건너뛴다.
// 사용: const lexer = new TypeLexer(text); lexer.current 로 현재 토큰을 보고 take·expect 로 소비한다. (2026-11-01 WB03-T)
class TypeLexer {
    // 원문(첫 NUL 앞까지)을 받아 첫 토큰을 읽어 둔다.
    constructor(text) {
        // 읽을 원문이다(NUL 뒤는 읽지 않는다).
        const nul = text.indexOf('\0');
        this.text = nul >= 0 ? text.slice(0, nul) : text;
        // 현재 읽는 위치이다.
        this.position = 0;
        // 현재 토큰 { kind: 'word'|'string'|'number'|'punct'|'end', text, number } 이다.
        this.current = { kind: 'end', text: '', number: 0 };
        this.next();
    }

    // 공백과 `//` 주석(줄 끝까지)을 건너뛴다.
    _skipSpace() {
        // 주석을 건너뛴 뒤에도 공백·주석이 이어질 수 있어 반복한다.
        while (this.position < this.text.length) {
            const char = this.text[this.position];
            if (char === ' ' || char === '\t' || char === '\r' || char === '\n' || char === '\f' || char === '\v') {
                this.position += 1;
                continue;
            }
            if (char === '/' && this.text[this.position + 1] === '/') {
                // 줄 끝(LF)까지 주석이다. CR 만 있는 줄바꿈 파일도 같은 줄의 CR 뒤에서 LF 를 만날 때까지 건너뛴다.
                while (this.position < this.text.length && this.text[this.position] !== '\n') this.position += 1;
                continue;
            }
            break;
        }
    }

    // 오류를 던진다(손상된 정의 파일을 찾기 쉽게 위치를 붙인다).
    fail() {
        throw new Error(`.type 문법 오류: 위치 ${this.position}`);
    }

    // 다음 토큰을 읽는다. 닫히지 않은 문자열·잘못된 숫자는 오류이다.
    next() {
        this._skipSpace();
        if (this.position >= this.text.length) {
            this.current = { kind: 'end', text: '', number: 0 };
            return;
        }
        const char = this.text[this.position];
        if (char === '"') {
            // 문자열: 닫는 따옴표까지이며 이스케이프는 해석하지 않는다(원본 자산에 Windows 역슬래시 경로가 있다).
            const start = this.position + 1;
            const end = this.text.indexOf('"', start);
            if (end < 0) this.fail();
            this.current = { kind: 'string', text: this.text.slice(start, end), number: 0 };
            this.position = end + 1;
        } else if (isLetter(char)) {
            // 단어: 영문자·밑줄로 시작하고 영문자·숫자·밑줄이 이어진다(`A00`·`sunCannon`·`hotFootRatioX`).
            const start = this.position;
            this.position += 1;
            // 단어의 나머지 글자를 읽는다.
            while (this.position < this.text.length && (isLetter(this.text[this.position]) || isDigit(this.text[this.position]))) this.position += 1;
            this.current = { kind: 'word', text: this.text.slice(start, this.position), number: 0 };
        } else if (isDigit(char) || char === '-' || char === '+' || char === '.') {
            const matched = NUMBER_PATTERN.exec(this.text.slice(this.position, this.position + 64));
            if (!matched) this.fail();
            const number = Number(matched[0]);
            if (!Number.isFinite(number)) this.fail();
            this.current = { kind: 'number', text: matched[0], number };
            this.position += matched[0].length;
        } else {
            // 그 밖의 글자는 한 글자 문장부호이다(`:` `;` `{` `}` `=` `#`).
            this.current = { kind: 'punct', text: char, number: 0 };
            this.position += 1;
        }
    }

    // 예상한 문장부호를 소비한다. 다르면 오류이다.
    expect(punctuation) {
        if (this.current.kind !== 'punct' || this.current.text !== punctuation) this.fail();
        this.next();
    }

    // 단어 또는 문자열 토큰의 문자열을 소비해 돌려준다. 종류가 다르면 오류이다.
    take(kind) {
        if (this.current.kind !== kind) this.fail();
        const { text } = this.current;
        this.next();
        return text;
    }

    // 숫자 토큰의 값을 소비해 돌려준다. 숫자가 아니면 오류이다.
    takeNumber() {
        if (this.current.kind !== 'number') this.fail();
        const { number } = this.current;
        this.next();
        return number;
    }
}

// 프레임 번호를 원본 `_ftol`(0쪽으로 버림)과 char 저장(하위 8비트)으로 바꾼다. 32비트 정수 범위 밖이면 오류이다. (2026-11-01 WB03-T)
function frameNumber(number) {
    const truncated = Math.trunc(number);
    if (truncated < -0x80000000 || truncated > 0x7fffffff) throw new Error('프레임 번호가 정수 범위를 벗어났습니다.');
    return truncated & 0xff;
}

// 클러스터 이름을 방향 글자(side)·변형(variant)·번호(number)로 쪼갠다. 이름은 영문자로 시작해야 한다.
// 영문자 부분이 이름 전체이면 숫자는 다음 토큰에서 읽는다. 사용: readFrameCode('A01', lexer) → { side: 65, variant: 80, number: 1, flags: 0 }. (2026-11-01 WB03-T)
function readFrameCode(label, lexer) {
    // 이름 앞쪽의 영문자 글자 수이다.
    let letters = 0;
    while (letters < label.length && isLetter(label[letters])) letters += 1;
    if (letters === 0 || label[0] < 'A' || label[0] > 'Z') throw new Error('잘못된 프레임 방향 글자입니다.');
    let number;
    if (letters === label.length) number = lexer.takeNumber();
    else {
        const rest = label.slice(letters);
        // 나머지는 숫자 전체여야 한다(`01`·`10`, 원본 from_chars 가 끝까지 소비해야 함).
        const matched = NUMBER_PATTERN.exec(rest);
        if (!matched || matched[0].length !== rest.length) throw new Error('잘못된 클러스터 프레임 번호입니다.');
        number = Number(matched[0]);
    }
    return { side: label.charCodeAt(0), variant: letters === 1 ? 0x50 : label.charCodeAt(1), number: frameNumber(number), flags: 0 };
}

// `.type` 문법 하나를 읽어 정의로 만든다(원본 0049c3b0 의 문법·프레임 코드·특수 프레임 부분).
// 반환: { name, constructorWords, flags[], properties[{ name, value }], clusters[{ name, code, imageCount }], defaultFrame, gumpFrame, helpFrame, baseFrame }.
// 읽는 순서: `typename 이름 [단어…] [typeflags 단어… ;] { 키 = 값 ; … }` 다음에 클러스터 줄들이 파일 끝까지 이어진다.
// 사용: parseTypeDefinition(decodeOriginalText(bytes)). (2026-11-01 WB03-T)
export function parseTypeDefinition(text) {
    const lexer = new TypeLexer(text);
    // 정의 결과이다. 그림 프레임 번호의 -1 초기화(gumpFrame)는 원본 로딩이 먼저 쓰는 값이다(미지정이면 기본 프레임으로 돌아가는 근거).
    const type = { name: '', constructorWords: [], flags: [], properties: [], clusters: [], defaultFrame: 0, gumpFrame: -1, helpFrame: -1, baseFrame: -1 };
    if (lexer.take('word').toLowerCase() !== 'typename') throw new Error('typename 이 없습니다.');
    type.name = lexer.take('word');
    // constructor 수식어와 선택적인 typeflags 를 읽는다.
    while (lexer.current.kind === 'word') {
        const word = lexer.take('word');
        if (word.toLowerCase() === 'typeflags') {
            // 플래그 단어들은 `;` 까지이다.
            while (lexer.current.kind === 'word') type.flags.push(lexer.take('word'));
            lexer.expect(';');
        } else {
            type.constructorWords.push(word);
        }
    }
    lexer.expect('{');
    // 속성은 선언 순서대로 보존한다(`키 = 문자열 또는 숫자 ;`).
    while (lexer.current.kind === 'word') {
        const name = lexer.take('word');
        lexer.expect('=');
        const value = lexer.current.kind === 'string' ? lexer.take('string') : lexer.takeNumber();
        lexer.expect(';');
        type.properties.push({ name, value });
    }
    lexer.expect('}');
    // 도움말 프레임을 이미 명시했는지 여부이다(`help` 가 나오면 이후 `default` 가 도움말 프레임을 덮지 않는다).
    let hasHelp = false;
    // 클러스터 줄을 파일 끝까지 읽는다. 선언 순서가 논리 프레임 번호이다.
    while (lexer.current.kind !== 'end') {
        const label = lexer.take('word');
        const code = readFrameCode(label, lexer);
        lexer.expect(':');
        const index = type.clusters.length;
        // 클러스터 플래그 단어들이다. 알려진 7종은 프레임 코드 비트가 되고 특수 프레임 표시는 번호를 기록한다.
        while (lexer.current.kind === 'word') {
            const flag = lexer.take('word').toLowerCase();
            if (Object.hasOwn(CLUSTER_FLAG_BITS, flag)) code.flags |= CLUSTER_FLAG_BITS[flag];
            if (flag === 'baseframe') type.baseFrame = index;
            if (flag === 'gumpframe') type.gumpFrame = index;
            if (flag === 'default') {
                type.defaultFrame = index;
                if (!hasHelp) type.helpFrame = index;
            }
            if (flag === 'help') {
                type.helpFrame = index;
                hasHelp = true;
            }
        }
        // 그림 참조(`: "파일.gif" #번호`)가 하나 이상 이어진다. 이름은 빌드용 참조일 뿐이라 개수만 센다(레이어 수).
        let imageCount = 0;
        do {
            lexer.expect(':');
            lexer.take('string');
            lexer.expect('#');
            lexer.takeNumber();
            imageCount += 1;
        } while (lexer.current.kind === 'punct' && lexer.current.text === ':');
        lexer.expect(';');
        type.clusters.push({ name: label, code, imageCount });
    }
    return type;
}

// 속성 이름(대소문자 무시)의 마지막 숫자 값을 돌려준다. 없거나 문자열이면 null 이다. (2026-11-01 WB03-T)
function numberProperty(definition, propertyName) {
    const lower = propertyName.toLowerCase();
    // 같은 속성이 여러 번 나오면 마지막 값이 이긴다(원본이 순서대로 덮어씀).
    for (let i = definition.properties.length - 1; i >= 0; i -= 1) {
        const property = definition.properties[i];
        if (property.name.toLowerCase() === lower) return typeof property.value === 'number' ? property.value : null;
    }
    return null;
}

// 기준점 비율을 칸 크기 배율로 바꾼다(float32 로 읽어 곱한 뒤 0쪽으로 버림, 원본 `TypeHotFootOffset`). (2026-11-01 WB03-T)
function hotFootOffset(ratio, scale) {
    return Math.trunc(Math.fround(ratio) * scale);
}

// 타입 하나의 읽기 전용 보기이다. 글자 구간 표와 특수 프레임·기준점을 미리 계산해 둔다. (2026-11-01 WB03-T)
function buildView(definition, block) {
    // 방향 글자 'A'~'P' 마다 첫 클러스터 번호(없으면 -1)이다.
    const first = new Array(LETTER_COUNT).fill(-1);
    // 모든 클러스터를 훑어 글자별 첫 번호를 정한다.
    definition.clusters.forEach((cluster, index) => {
        const letter = cluster.code.side - 0x41;
        if (letter >= 0 && letter < LETTER_COUNT && first[letter] === -1) first[letter] = index;
    });
    // 구간이 하나라도 있는 글자의 수이다(원본 타입 +0x12c, 글자 식의 유효 범위 `A ≤ 글자 < A + letterKinds`).
    const letterKinds = first.filter((value) => value !== -1).length;
    const ratioX = numberProperty(definition, 'hotFootRatioX');
    const ratioY = numberProperty(definition, 'hotFootRatioY');
    return Object.freeze({
        block,
        frameCount: definition.clusters.length,
        defaultFrame: definition.defaultFrame,
        gumpFrame: definition.gumpFrame,
        letterKinds,
        first: Object.freeze(first),
        hotspotX: ratioX === null ? 0 : hotFootOffset(ratioX, HOT_FOOT_SCALE_X),
        hotspotY: ratioY === null ? 0 : hotFootOffset(ratioY, HOT_FOOT_SCALE_Y),
    });
}

// 자산이 없는 내장 타입의 빈 보기이다(원본은 모든 선택 필드가 0 이고 프레임이 없다). (2026-11-01 WB03-T)
const EMPTY_VIEW = Object.freeze({ block: -1, frameCount: 0, defaultFrame: 0, gumpFrame: 0, letterKinds: 0, first: Object.freeze(new Array(LETTER_COUNT).fill(-1)), hotspotX: 0, hotspotY: 0 });

// 원본 타입 표이다. 로딩 순서 116개의 정의로 타입 번호(70~185)의 보기를 만들고 이름으로 번호를 찾는다.
// 사용: const table = TypeTable.fromArchive(archive, decodeOriginalText); table.find('mana'); table.view(103). (2026-11-01 WB03-T)
export class TypeTable {
    // definitions 는 TYPE_LOAD_ORDER 와 같은 순서의 정의 배열이다. 읽지 못한 타입은 null 이며 프레임이 없는 타입이 된다.
    constructor(definitions) {
        if (definitions.length !== TYPE_LOAD_ORDER.length) throw new Error('타입 정의 수가 로딩 순서와 다릅니다.');
        // 번호(70~)별 보기이다. 정의가 없으면 빈 보기이다.
        this.views = definitions.map((definition, index) => (definition ? buildView(definition, index) : EMPTY_VIEW));
        // 이름(소문자) → 타입 번호 표이다. 원본은 타입 표를 0번부터 비교해 첫 일치를 고르므로 같은 이름이면 앞의 것이 이긴다.
        this.byName = new Map();
        // 로딩 순서대로 이름을 등록한다.
        TYPE_LOAD_ORDER.forEach((name, index) => {
            const key = name.toLowerCase();
            if (!this.byName.has(key)) this.byName.set(key, FIRST_ASSET_TYPE_NUMBER + index);
        });
    }

    // 아카이브의 `\d\이름.type` 116개를 읽어 표를 만든다. decode 는 바이트 → 문자열 함수이다.
    // 파일이 없거나 문법이 틀린 타입은 onError(이름, 오류) 로 알리고 프레임 없는 타입으로 둔다(그림이 안 나올 뿐 다른 타입은 정상). (2026-11-01 WB03-T)
    static fromArchive(archive, decode, onError = null) {
        // 로딩 순서대로 정의를 읽는다.
        const definitions = TYPE_LOAD_ORDER.map((name) => {
            try {
                const bytes = archive.read(`\\d\\${name}.type`);
                if (!bytes) throw new Error('아카이브에 파일이 없습니다.');
                return parseTypeDefinition(decode(bytes));
            } catch (error) {
                onError?.(name, error);
                return null;
            }
        });
        return new TypeTable(definitions);
    }

    // 이름(대소문자 무시)으로 타입 번호를 찾는다. 없으면 -1 이다. 자산 타입만 찾는다(내장 프로세스 타입 이름은 그림에 쓰이지 않는다).
    find(name) {
        return this.byName.get(String(name).toLowerCase()) ?? -1;
    }

    // 타입 번호로 읽기 전용 보기를 돌려준다. 내장 타입(70 미만)과 여분(186~187)은 빈 보기이고, 표 밖 번호는 null 이다.
    view(type) {
        if (!Number.isInteger(type) || type < 0 || type >= PATCH_TYPE_COUNT) return null;
        const index = type - FIRST_ASSET_TYPE_NUMBER;
        if (index < 0 || index >= this.views.length) return EMPTY_VIEW;
        return this.views[index];
    }
}
