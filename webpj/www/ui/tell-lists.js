// Tell 대화상자의 파일 목록 메뉴(`$Menu=@패턴,명령,인자` · `$Menu=#패턴,명령,인자`)를 복원하는 모듈이다.
// 패턴과 맞는 파일(예: `\D\offical*.english` = 캠페인 묶음 파일 8개)마다 파일 이름(첫 점 앞)으로 설정 객체를 등록하고(본문 키 `title`·섹션 `[Overview]` 조회용),
// 객체 순서를 이름순으로 정렬해 목록 행을 만든다. 제목은 `{<이름>.title}` 로 읽으며 `--` 로 시작하는 제목은 구분선이다.
// 원본 주소: `@` 목록 004ce1b0, `#` 목록 004ce580. 복원 기준: cpppj TellFileMenus.cpp(10.78 경로), docs/exe/cpp-tell-file-menus-client-20261025.md.
// `%` 인라인 목록(`%{userSpec}` 등 사용자 캠페인 목록)은 웹 판본의 구현 대상이 아니다(사용자 맵·캠페인 파일을 읽지 않음). `*SOLOLIST*` 같은 특수 목록은 아직 지원하지 않는다.
// 웹 판본에서 보이지 않게 할 목록 객체(User Made Campaigns = `offical6`)는 아래 WEB_EXCLUDED_LIST_OBJECTS 로 목록 층에서만 뺀다.
// (2026-10-28 WB03-D: Campaign 대화상자의 캠페인 묶음 목록을 만들기 위해 추가. 2026-10-30 WB03-I: 웹 제외 목록 객체 추가)
import { Config } from '../assets/config.js';
import { decodeOriginalText } from '../assets/text-encoding.js';

// 웹 판본에서 파일 목록에 올리지 않는 목록 객체 이름(대소문자 무시)이다. `offical6` 은 Campaign 대화상자의 마지막 행 'User Made Campaigns' 이며,
// 웹 판본은 사용자 캠페인을 제공하지 않으므로 그 행도, 설정 객체 등록도 하지 않는다(Tell 섹션 `offical6.overview` 도 열리지 않는다).
// 사용: expandFileList(..., { excluded: [] }) 로 호출하면 원본과 같은 8행이 나온다(원본 스크린샷 대조용). (2026-10-30 WB03-I: 사용자 결정 반영)
export const WEB_EXCLUDED_LIST_OBJECTS = Object.freeze(['offical6']);

// 경로에서 목록 객체 이름(마지막 경로 구분자 뒤 파일 이름의 첫 점 앞)을 구한다. 예: `\d\offical2sep.english` → `offical2sep`. (2026-10-28 WB03-D)
function listObjectName(path) {
    const base = path.slice(Math.max(path.lastIndexOf('\\'), path.lastIndexOf('/')) + 1);
    const dot = base.indexOf('.');
    return { fileName: base, name: dot < 0 ? base : base.slice(0, dot) };
}

// 파일 목록 메뉴 항목을 만든다. label 은 `@패턴` 또는 `#패턴`, command·argument 는 지시문의 명령 이름·인자(`{cur.file}` 포함 가능)이다.
// 파일마다 설정 객체를 등록부에 올려 두고(이미 같은 이름의 객체가 있으면 그대로 쓴다) 나중에 Tell 이 `이름.overview` 같은 섹션을 찾을 수 있게 한다.
// 반환: [{ kind: 'text'|'separator', label, command }] 이며 표시 순서는 이름(대소문자 무시) 오름차순이다. `@` 는 제목이 비면 파일 이름을, `#` 는 빈 제목을 그대로 쓴다.
// 사용: expandFileList(gameData, '@\\D\\offical*.english', 'Tell', '{cur.file}.overview'). (2026-10-28 WB03-D)
// 다섯째 인자 `{ excluded }` 는 목록에서 뺄 객체 이름(대소문자 무시) 배열이며 기본값은 WEB_EXCLUDED_LIST_OBJECTS 이다. 뺀 객체는 행도 설정 객체 등록도 하지 않는다.
// 무언가를 뺀 목록은 맨 앞·맨 뒤의 구분선과 연달아 놓인 구분선을 지운다. 남은 객체의 `index` 는 뺀 객체까지 센 원래 순번 그대로 둔다. (2026-10-30 WB03-I)
export function expandFileList(gameData, label, command, argument, { excluded = WEB_EXCLUDED_LIST_OBJECTS } = {}) {
    const emptyTitleUsesName = label[0] === '@';
    const pattern = label.slice(1);
    const found = [];
    const seen = new Set();
    const excludedNames = new Set(excluded.map((name) => name.toLowerCase()));
    let skipped = false;
    // 패턴과 맞는 파일에서 같은 객체 이름(대소문자 무시)을 한 번씩만 모은다. 등록 index 는 모은 순서(뺀 객체 포함)이다.
    // 현재 언어의 파일이 없으면 영어 파일로 대체해 찾는다(GameData.findFiles, 2026-10-30 WB03-H).
    for (const path of gameData.findFiles(pattern)) {
        const { fileName, name } = listObjectName(path);
        const key = name.toLowerCase();
        if (seen.has(key)) continue;
        const index = seen.size;
        seen.add(key);
        // 웹 판본에서 뺀 객체는 건너뛴다(2026-10-30 WB03-I).
        if (excludedNames.has(key)) {
            skipped = true;
            continue;
        }
        found.push({ path, fileName, name, index });
    }
    // 이름(대소문자 무시) 오름차순으로 정렬한다.
    found.sort((a, b) => (a.name.toLowerCase() < b.name.toLowerCase() ? -1 : a.name.toLowerCase() > b.name.toLowerCase() ? 1 : 0));
    const items = [];
    // 정렬된 순서대로 객체를 등록하고 행을 만든다.
    for (const entry of found) {
        if (!gameData.registry.findByName(entry.name)) {
            const object = new Config(gameData.registry, entry.name);
            object.append(`path="${entry.path}"`);
            object.append(`fileName="${entry.fileName}"`);
            object.append(`index=${entry.index}`);
            const bytes = gameData.files.read(entry.path);
            if (bytes) object.loadBytes(bytes, decodeOriginalText);
        }
        let title = gameData.get(`${entry.name}.title`) ?? '';
        if (title === '' && emptyTitleUsesName) title = entry.name;
        // `--` 로 시작하는 제목은 구분선이다(인자 치환을 하지 않는다).
        if (title.startsWith('--')) {
            items.push({ kind: 'separator', label: '', command: null });
            continue;
        }
        // 인자의 `{cur.file}` 을 현재 객체 이름으로 치환한다(임시 `cur` 객체를 올렸다 내린다).
        const cur = new Config(gameData.registry, 'cur');
        let resolved;
        try {
            cur.append(`file="${entry.name}"`);
            resolved = gameData.substitute(argument);
        } finally {
            cur.dispose();
        }
        items.push({ kind: 'text', label: title, command: Object.freeze({ name: command, argument: resolved }) });
    }
    return skipped ? trimSeparators(items) : items;
}

// 행 목록의 맨 앞·맨 뒤 구분선과 연달아 놓인 구분선(둘째부터)을 지운 새 배열을 돌려준다. 객체를 빼서 구분선만 남은 자리를 정리하는 데 쓴다.
// 예: [구분선, A, 구분선, 구분선, B, 구분선] → [A, 구분선, B]. (2026-10-30 WB03-I)
function trimSeparators(items) {
    const kept = [];
    // 앞 행이 없거나 구분선이면 구분선을 건너뛴다.
    for (const item of items) {
        if (item.kind === 'separator' && (kept.length === 0 || kept[kept.length - 1].kind === 'separator')) continue;
        kept.push(item);
    }
    // 끝에 남은 구분선을 지운다.
    while (kept.length > 0 && kept[kept.length - 1].kind === 'separator') kept.pop();
    return kept;
}
