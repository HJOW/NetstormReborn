// 웹 판본의 가상 파일 조회 계층이다. 원본의 "느슨한 파일 → 아카이브(TAFF) 순서" 조회 규칙(docs/formats/vfs.md)을 웹 자산에 맞춰 옮겼다.
//  - 느슨한 파일: 웹 공개 데이터 폴더(www/data)에서 받은 파일(경로 → 바이트)이다. 원본의 "기본 경로 디스크" 자리다.
//  - 아카이브: 등록 순서대로 조회하는 TAFF 아카이브다. 원본 설치본에는 netstorm.tarc 하나뿐이다.
// 느슨한 파일이 아카이브보다 우선하며(원본 FUN_0041b4c0 의 열기 순서), 이름 비교는 대소문자를 무시한다.
// 경로는 원본 설정이 만드는 `\D\tell.english` 같은 역슬래시 형식과 웹 자산 경로 `d/tell.english` 형식을 모두 받아 같은 이름으로 취급한다.
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다.
// (2026-10-28 WB02-D: 메뉴 대화상자 본문·설정 파일을 경로로 찾기 위해 추가)
import { TaffArchive, archiveNameMatches, normalizeArchiveName } from './taff.js';

// 경로를 비교용 열쇠로 바꾼다. '\' 를 '/' 로 바꾸고 소문자로 만들며 앞의 "./" 와 "/" 를 뗀다. (2026-10-28 WB02-D)
export function normalizeFilePath(path) {
    let key = String(path).replace(/\\/g, '/').toLowerCase();
    if (key.startsWith('./')) key = key.slice(2);
    while (key.startsWith('/')) key = key.slice(1);
    return key;
}

// 파일 조회 계층이다. 느슨한 파일 표와 TAFF 아카이브 목록을 가진다.
// 사용: const files = new GameFiles({ loose: Map(경로 → Uint8Array), archives: [TaffArchive] });
//       files.read('\\D\\tell.english'); files.find('\\D\\offical*.english'). (2026-10-28 WB02-D)
export class GameFiles {
    // 느슨한 파일 Map 과 아카이브 배열을 받는다. 느슨한 파일의 이름은 원래 대소문자를 보존해 목록에 쓴다.
    constructor({ loose = new Map(), archives = [] } = {}) {
        // 비교용 열쇠 → { name, bytes } 표이다. 같은 열쇠가 또 나오면 먼저 나온 것을 둔다.
        this._loose = new Map();
        // 느슨한 파일의 원래 이름 순서 목록이다(find 결과 순서).
        this._looseNames = [];
        // 이름이 같으면 먼저 등록된 파일을 쓴다(원본의 첫 일치).
        for (const [name, bytes] of loose) {
            const key = normalizeFilePath(name);
            if (this._loose.has(key)) continue;
            this._loose.set(key, { name, bytes });
            this._looseNames.push(name);
        }
        // 조회 순서대로의 TAFF 아카이브 목록이다.
        this.archives = archives.slice();
    }

    // 아카이브 바이트로 TAFF 를 열어 목록 끝에 등록한다. 만든 TaffArchive 를 돌려준다.
    addArchive(bytes) {
        const archive = new TaffArchive(bytes);
        this.archives.push(archive);
        return archive;
    }

    // 경로를 아카이브 안 비교용 이름 형식(소문자 `\d\이름`)으로 바꾼다. (2026-10-28 WB02-D)
    static toArchiveName(path) {
        return `\\${normalizeFilePath(path).replace(/\//g, '\\')}`;
    }

    // 경로를 대소문자를 유지한 채 아카이브 안 이름 형식(`\d\Name`)으로 바꾼다. 목록(find) 결과의 표기에 쓴다. (2026-10-28 WB02-D)
    static toDisplayName(path) {
        let name = String(path).replace(/\//g, '\\');
        if (name.startsWith('.\\')) name = name.slice(2);
        while (name.startsWith('\\')) name = name.slice(1);
        return `\\${name}`;
    }

    // 경로의 파일이 있는지 확인한다.
    has(path) {
        return this.read(path) !== null;
    }

    // 파일 바이트를 읽는다. 느슨한 파일 → 아카이브(등록 순서) 순이며 없으면 null 이다. 반환 배열은 호출자가 바꿔도 되는 복사본이 아니다(느슨한 파일은 원본 배열).
    read(path) {
        const loose = this._loose.get(normalizeFilePath(path));
        if (loose) return loose.bytes;
        const archiveName = GameFiles.toArchiveName(path);
        // 등록 순서대로 아카이브를 조회한다.
        for (const archive of this.archives) {
            const bytes = archive.read(archiveName);
            if (bytes) return bytes;
        }
        return null;
    }

    // 폴더 + 파일 이름 와일드카드('*'·'?')와 일치하는 파일 이름을 돌려준다. 느슨한 파일을 먼저, 아카이브를 등록 순서로 모으되
    // 대소문자 무시로 같은 이름은 첫 번째만 남긴다(원본 FileFinder 의 첫 중복 제거). 이름 형식은 아카이브 안 이름(`\d\offical1.english`)으로 통일한다.
    // 정렬은 하지 않으며(원본은 상위 Tell 이 정렬) 느슨한 파일은 등록 순서, 아카이브는 이름 인덱스 순서다.
    find(pattern) {
        const archivePattern = GameFiles.toArchiveName(pattern);
        const names = [];
        const seen = new Set();
        // 같은 이름을 대소문자 무시로 한 번만 담는 보조 함수다.
        const add = (name) => {
            const key = normalizeArchiveName(name);
            if (seen.has(key)) return;
            seen.add(key);
            names.push(name);
        };
        // 느슨한 파일에서 패턴과 맞는 것을 아카이브 이름 형식(원래 대소문자 유지)으로 바꿔 담는다.
        for (const name of this._looseNames) {
            const displayName = GameFiles.toDisplayName(name);
            if (archiveNameMatches(archivePattern, displayName)) add(displayName);
        }
        // 아카이브마다 일치하는 이름을 등록 순서로 담는다.
        for (const archive of this.archives) for (const name of archive.find(archivePattern)) add(name);
        return names;
    }
}
