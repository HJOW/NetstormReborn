// 원본 데이터 아카이브(netstorm.tarc, "TAFF v0.2")를 읽는 모듈이다. 형식과 복호화 규칙은 docs/formats/taff.md 를 따른다.
//  - 머리: 매직 "TAFF v" + 엔트리 개수(0x14) + 이름 인덱스 표 위치(0x1C) + 디렉터리 위치(0x20)·크기(0x24) + 데이터 영역 위치(0x28), 모두 리틀엔디언.
//  - 디렉터리: 엔트리 레코드 N개(데이터 영역 기준 오프셋 u32, 크기 u32, NUL 로 끝나는 이름 `\d\altar.type`).
//  - 이름 인덱스 표: 이름순(대소문자 무시)으로 정렬된 레코드의 디렉터리 기준 오프셋 N개(u32).
//  - 엔트리 데이터는 키 "mydoghasfleas"(13바이트)와 XOR 되어 있으며 키 위치는 엔트리 시작부터 0 으로 순환한다.
// 이름 비교는 대소문자를 무시한 완전 일치이고, 요청 이름이 ".\" 로 시작하면 "." 을 떼고 비교한다(원본 FUN_0041a240).
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다.
// (2026-10-28 WB02-D: 메뉴 대화상자 본문(tell.english 등)과 설정 용어표를 아카이브에서 읽기 위해 추가)

// 아카이브 머리의 매직 문자열 앞부분이다("TAFF v0.2" + 0x1A 중 버전 앞까지만 확인한다). (2026-10-28 WB02-D)
const TAFF_MAGIC = 'TAFF v';

// 머리 필드의 오프셋이다. (2026-10-28 WB02-D)
const OFFSET_ENTRY_COUNT = 0x14;
const OFFSET_INDEX_TABLE = 0x1c;
const OFFSET_DIRECTORY = 0x20;
const OFFSET_DIRECTORY_SIZE = 0x24;
const OFFSET_DATA = 0x28;

// 머리의 최소 길이이다(마지막 필드 끝). 이보다 짧으면 TAFF 가 아니다. (2026-10-28 WB02-D)
const HEADER_SIZE = 0x2c;

// 엔트리 데이터의 XOR 키이다. 원본 실행 파일에서 "TAFF v%d.%d" 문자열 바로 뒤에 저장되어 있으며 options.cfg·setup.cfg 와 같다. (2026-10-28 WB02-D)
const XOR_KEY = Uint8Array.from('mydoghasfleas', (character) => character.charCodeAt(0));

// 설정 파일(options.cfg·setup.cfg)과 아카이브가 함께 쓰는 XOR 복호화이다. 키 위치는 배열 시작부터 0 으로 순환하며 새 배열을 돌려준다.
// 사용: xorDecode(암호문 조각) — 아카이브 엔트리나 인코딩된 설정 파일 전체에 쓴다. (2026-10-28 WB02-D)
export function xorDecode(bytes) {
    const plain = new Uint8Array(bytes.length);
    // 모든 바이트를 키 순환 위치와 XOR 한다.
    for (let i = 0; i < bytes.length; i += 1) plain[i] = bytes[i] ^ XOR_KEY[i % XOR_KEY.length];
    return plain;
}

// 아카이브 안 이름을 비교용으로 바꾼다. 소문자로 만들고 '/' 를 '\' 로 바꾸며, ".\" 로 시작하면 "." 을 뗀다. (2026-10-28 WB02-D)
export function normalizeArchiveName(name) {
    let normalized = String(name).replace(/\//g, '\\').toLowerCase();
    if (normalized.startsWith('.\\')) normalized = normalized.slice(1);
    return normalized;
}

// '*'(0개 이상)와 '?'(1개) 만 쓰는 와일드카드 패턴을 정규식으로 바꾼다. 대소문자는 무시한다. (2026-10-28 WB02-D)
function wildcardToRegExp(pattern) {
    const escaped = pattern.replace(/[.+^${}()|[\]\\]/g, '\\$&').replace(/\*/g, '.*').replace(/\?/g, '.');
    return new RegExp(`^${escaped}$`, 'i');
}

// 아카이브 안 이름이 폴더 + 파일 이름 와일드카드 패턴과 맞는지 확인한다. 폴더 부분(마지막 '\' 앞)은 대소문자 무시 완전 일치이고,
// 파일 이름 부분은 '*'·'?' 와일드카드를 쓴다. 사용: archiveNameMatches('\\d\\offical*.english', '\\d\\offical1.english') → true. (2026-10-28 WB02-D)
export function archiveNameMatches(pattern, name) {
    const normalizedPattern = normalizeArchiveName(pattern);
    const patternSlash = normalizedPattern.lastIndexOf('\\');
    const candidate = normalizeArchiveName(name);
    const candidateSlash = candidate.lastIndexOf('\\');
    return normalizedPattern.slice(0, patternSlash + 1) === candidate.slice(0, candidateSlash + 1)
        && wildcardToRegExp(normalizedPattern.slice(patternSlash + 1)).test(candidate.slice(candidateSlash + 1));
}

// TAFF 아카이브 한 개이다. 바이트 전체를 받아 디렉터리만 해석하고, 엔트리는 요청할 때 복호화한다.
// 사용: const archive = new TaffArchive(bytes); archive.read('\\d\\tell.english'); archive.find('\\d\\offical*.english').
// 형식이 맞지 않거나 범위를 벗어나면 생성자가 오류를 던진다. (2026-10-28 WB02-D)
export class TaffArchive {
    // 아카이브 바이트(Uint8Array)를 받아 머리와 디렉터리를 검증하며 읽는다.
    constructor(bytes) {
        if (!(bytes instanceof Uint8Array) || bytes.length < HEADER_SIZE) throw new Error('TAFF: 파일이 너무 짧습니다.');
        if (String.fromCharCode(...bytes.subarray(0, TAFF_MAGIC.length)) !== TAFF_MAGIC) throw new Error('TAFF: 매직 문자열이 맞지 않습니다.');
        const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
        const count = view.getUint32(OFFSET_ENTRY_COUNT, true);
        const indexOffset = view.getUint32(OFFSET_INDEX_TABLE, true);
        const directoryOffset = view.getUint32(OFFSET_DIRECTORY, true);
        const directorySize = view.getUint32(OFFSET_DIRECTORY_SIZE, true);
        const dataOffset = view.getUint32(OFFSET_DATA, true);
        if (indexOffset + count * 4 > bytes.length) throw new Error('TAFF: 이름 인덱스 표가 파일 밖입니다.');
        if (directoryOffset + directorySize > bytes.length) throw new Error('TAFF: 디렉터리가 파일 밖입니다.');
        // 원본 아카이브 바이트다. 엔트리 복호화에서 조각을 잘라 쓴다.
        this.bytes = bytes;
        // 데이터 영역의 파일 내 시작 위치다.
        this.dataOffset = dataOffset;
        // 정규화한 이름 → 엔트리({ name, offset, size }) 표이다. 같은 이름이 또 나오면 먼저 나온 것을 둔다(원본의 첫 일치).
        this._entries = new Map();
        // 이름 인덱스 표 순서(이름순)의 엔트리 목록이다.
        this._ordered = [];
        // 인덱스 표의 각 항목이 가리키는 디렉터리 레코드를 읽는다.
        for (let i = 0; i < count; i += 1) {
            const record = directoryOffset + view.getUint32(indexOffset + i * 4, true);
            if (record + 8 > directoryOffset + directorySize) throw new Error(`TAFF: ${i}번 레코드가 디렉터리 밖입니다.`);
            const offset = view.getUint32(record, true);
            const size = view.getUint32(record + 4, true);
            let end = record + 8;
            while (end < bytes.length && bytes[end] !== 0) end += 1;
            if (end >= bytes.length) throw new Error(`TAFF: ${i}번 이름이 끝나지 않습니다.`);
            const name = String.fromCharCode(...bytes.subarray(record + 8, end));
            if (dataOffset + offset + size > bytes.length) throw new Error(`TAFF: ${name} 의 데이터가 파일 밖입니다.`);
            const entry = Object.freeze({ name, offset, size });
            this._ordered.push(entry);
            const key = normalizeArchiveName(name);
            if (!this._entries.has(key)) this._entries.set(key, entry);
        }
    }

    // 엔트리 개수이다(중복 이름도 센다).
    get count() {
        return this._ordered.length;
    }

    // 모든 엔트리 이름을 이름 인덱스 표 순서(원본 대소문자 그대로)로 돌려준다.
    names() {
        return this._ordered.map((entry) => entry.name);
    }

    // 이름의 엔트리 정보({ name, offset, size })를 돌려준다. 없으면 null 이다. 이름 비교는 대소문자 무시·".\" 접두 허용이다.
    stat(name) {
        return this._entries.get(normalizeArchiveName(name)) ?? null;
    }

    // 이름의 엔트리가 있는지 확인한다.
    has(name) {
        return this.stat(name) !== null;
    }

    // 이름의 엔트리를 복호화해 새 Uint8Array 로 돌려준다. 없으면 null 이다.
    read(name) {
        const entry = this.stat(name);
        if (!entry) return null;
        const start = this.dataOffset + entry.offset;
        return xorDecode(this.bytes.subarray(start, start + entry.size));
    }

    // 와일드카드(파일 이름 부분의 '*'·'?')와 일치하는 엔트리 이름을 인덱스 표 순서로 돌려준다.
    // 폴더 부분(마지막 '\' 앞)은 대소문자 무시 완전 일치여야 한다. 원본 guideSpec = "{DataDir}\offical*.{currentLanguage}" 같은 목록 조회에 쓴다.
    find(pattern) {
        // 폴더가 같고 파일 이름이 패턴과 맞는 엔트리만 모은다.
        return this._ordered.filter((entry) => archiveNameMatches(pattern, entry.name)).map((entry) => entry.name);
    }
}
