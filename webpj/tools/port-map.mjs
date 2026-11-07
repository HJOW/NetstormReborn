// webpj 이식 대응표(webpj/PORT_MAP.tsv)를 만들고 검사하며, 기준선 뒤 cpppj 변경을 보고하는 도구이다.
// 대응표의 한 행은 cpppj 기준선의 소스 묶음 하나(같은 폴더·같은 이름의 .cpp/.h/.inc, 이하 "단위")이다.
//  - cpp    : 단위의 대표 파일(저장소 기준). .cpp 가 있으면 .cpp, 없으면 .h, 그것도 없으면 .inc 이다.
//  - class  : 분류. 이 도구의 RULES 가 정하며 대응표에서 손으로 바꾸지 않는다(바꿀 때는 규칙과 사유를 고친다).
//             port(이식 대상) / adapt(창·입력·소리·파일·시계처럼 웹 대체가 필요한 플랫폼 경계) /
//             exclude-mp(LAN·멀티플레이) / exclude-edit(Edit·커스텀 맵) / exclude-host(DirectDraw·D3D11/FSR·CD·Win32·개발자 파일 출력)
//  - web    : 대응 웹 모듈(webpj 기준 경로, 여럿이면 `;`). 사람이 적는다.
//  - base   : 그 단위를 웹과 대조·판정한 cpppj 커밋. 사람이 적는다(판정한 단위에서 기준선 또는 그 뒤 커밋으로 올린다).
//  - status : todo(미이식) / partial(웹 모듈이 일부만 가짐) / port(이식·L1/cpppj 검사 통과) / session(앱 수명 연결) / oracle(cpppj 오라클 비교 통과) / -(제외 분류)
//  - note   : `[규칙ID] ` 접두(도구가 씀) + 사람이 쓰는 메모(남은 범위·3.6절 판정·경계).
// 사용(저장소 루트 또는 webpj 에서):
//   node webpj/tools/port-map.mjs            → 대응표 검사 + 분류/상태 개수 + 기준선(또는 행 base) 뒤 cpppj 변경 보고. 오류가 있으면 종료 코드 1.
//   node webpj/tools/port-map.mjs --write    → 기준선 단위 목록과 규칙으로 행·분류·규칙 접두를 다시 만든다. web·base·status·사람 메모는 보존한다.
//   node webpj/tools/port-map.mjs --why 이름 → 단위 이름(예: SidPool)의 분류 규칙과 사유를 출력한다.
// 이 도구는 로컬 파일과 git 기록만 읽고 네트워크를 쓰지 않는다.
// (2026-11-06 WB02-G: LEFT_JOBS.webpj.md 3.0·3.2절 순서1의 대응표·기준선 재반영 요구로 추가)
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

// 저장소 루트의 절대 경로이다. (2026-11-06 WB02-G)
export const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');

// 대응표 파일의 저장소 상대 경로이다. (2026-11-06 WB02-G)
export const MAP_PATH = 'webpj/PORT_MAP.tsv';

// 이식 기준선 cpppj 커밋(제품985·표시11.01)이다. LEFT_JOBS.webpj.md 3.6절의 결정이며 바꿀 때는 그 절과 함께 고친다. (2026-11-06 WB02-G)
export const BASELINE = '1f00a25a';

// 대응표가 다루는 cpppj 소스 폴더이다. platform/·app/ 은 원본에 없는 새 코드(콘솔·BMP·진입점)라 대응표에 넣지 않는다. (2026-11-06 WB02-G)
export const SCOPES = Object.freeze(['cpppj/src/o', 'cpppj/src/client', 'cpppj/src/zacket']);

// 단위를 이루는 소스 확장자와 대표 파일 선택 순서이다. (2026-11-06 WB02-G)
export const EXTENSIONS = Object.freeze(['.cpp', '.h', '.inc']);

// 대응표 열 이름과 순서이다. (2026-11-06 WB02-G)
export const COLUMNS = Object.freeze(['cpp', 'class', 'web', 'base', 'status', 'note']);

// 분류 이름이다. 3.0절의 다섯 분류와 같다. (2026-11-06 WB02-G)
export const CLASSES = Object.freeze(['port', 'adapt', 'exclude-mp', 'exclude-edit', 'exclude-host']);

// 상태 이름이다. todo→partial→port→session→oracle 순서로 올리며 제외 분류는 '-' 이다. (2026-11-06 WB02-G)
export const STATUSES = Object.freeze(['todo', 'partial', 'port', 'session', 'oracle', '-']);

// 이름 묶음으로 분류하는 규칙이다. 위에서부터 처음 맞는 규칙을 쓴다. names 는 단위 이름(확장자 없는 파일 이름) 목록, pattern 은 이름 정규식이다.
// 각 규칙의 reason 은 근거(cpppj 머리 주석·CMake 주석·호출 위치)를 줄여 쓴 것이며 상세는 docs/webpj-port-map-20261106.md 3절에 있다.
// 단독 미션이 실제로 지나는 코드는 이름이 멀티플레이 같아도 port 다(LEFT_JOBS.webpj.md 1절 경계). (2026-11-06 WB02-G)
export const RULES = Object.freeze([
    { id: 'mp-lan-tcp', class: 'exclude-mp', names: ['LocalTcpTransport'], reason: '로컬 LAN TCP 프레임 전송(982). 단독은 zlocal 내부 양끝만 쓴다' },
    { id: 'mp-saved-battle', class: 'exclude-mp', names: ['RawSavedStateRead', 'SavedStateFiles', 'RawStartupStateCheck'], reason: '일반 전투 저장상태(!state.dat) 읽기·정리와 재접속 질문. 웹은 저장상태 파일이 없다' },
    { id: 'mp-outpost', class: 'exclude-mp', names: ['RawOutpostLifecycle'], reason: 'Outpost(멀티플레이 전용) 배치/거리 목록. RawRegionOwnership 의 outpost 분기에서만 부른다' },
    { id: 'mp-zsend', class: 'exclude-mp', names: ['HtmlZsend'], reason: 'HTML $Zsend 의 87h 패킷을 11번 연결로 송신. WB02-E 에서 미연결로 결정' },
    { id: 'edit-editor', class: 'exclude-edit', pattern: /Editor/, reason: '이름에 Editor 가 든 편집기 전용 장치(UberEditor·섬 추가 창·작업본 파일·Test Battle 등)' },
    { id: 'edit-island-add', class: 'exclude-edit', names: ['RawIslandAdd', 'RawIslandPlacement'], reason: '편집기의 섬 한 개 추가와 임시 청크 지도 배치 검사' },
    {
        id: 'edit-fort-save', class: 'exclude-edit',
        names: ['RawFortObjectSave', 'RawFortObjectsSave', 'RawFortRegionSave', 'RawFortContentSave', 'RawFortTerritorySave', 'RawFortSnapshotBasics', 'RawFortSnapshotParent', 'RawFortMoneySave', 'RawFortMissionSave', 'RawFortBadgesSave', 'RawFortCoreWrite', 'RawFortSpWrite', 'RawFortSubscriberWrite', 'RawDataManagerFileSave', 'RawMenuSaveFort', 'SaveFortNameDialog'],
        reason: 'Fort 저장(Save as)·snapshot writer. Battle snapshot·CoreData 기록은 일반 전투(MP)와 공유한다',
    },
    {
        id: 'edit-existing-fort', class: 'exclude-edit',
        names: ['RawFortTypeNamesLoad', 'RawFortContentsLoad', 'RawFortDeckLoad', 'RawDeckRestore', 'RawFortTerritoryRecover', 'FortTypeConversionStorage', 'RawFortSubscriberLoad', 'RawSubscriberIdGenerate', 'RawFortSubscriberChange'],
        reason: '기존 Fort Editor 설치의 타입 변환·Deck·Subscriber·지형 회복(RawCampaignSession 의 "기존Editor" 경로). 미션 적재는 RawCampaignWorldLoad/RawFortWorldLoad 를 쓴다',
    },
    { id: 'edit-new-fort', class: 'exclude-edit', names: ['RawNewFort', 'RawCampaignNewFort', 'NewFortManagerDevices'], reason: '새 맵 Template manager·보호 작업본 파일. cpppj 987 뒤로 원격 Fort(MP)도 같은 생성을 쓴다' },
    { id: 'edit-fort-name', class: 'exclude-edit', names: ['RawFortCurrentName', 'FortCurrentNameDevices'], reason: 'Editor 플레이어 초기화·현재 Fort 이름과 원격 Fort 경로(987)' },
    { id: 'edit-map-list', class: 'exclude-edit', names: ['FortSelection', 'SoloMapCatalog', 'RawSoloMapList', 'TellMapMenus'], reason: '편집/단독(커스텀) 맵 선택·목록. WB03-I 에서 웹 제외(User Made Campaigns 포함)' },
    { id: 'host-cd', class: 'exclude-host', names: ['CdDirectory', 'CdPresence', 'ClientCdGlobals', 'RecheckCdInput'], reason: 'CD-ROM 루트 열거·표시 파일 탐색·RecheckCD' },
    { id: 'host-gpu', class: 'exclude-host', names: ['GpuPresenter', 'FsrConstants'], reason: '7차 Direct3D 11 AA/FSR 1.0 출력기' },
    { id: 'host-display-mode', class: 'exclude-host', names: ['ScreenModeTransition', 'ScreenResolutions', 'ScreenMouseKey'], reason: '해상도/전체화면 모드 전환·F12 전체화면 마우스 모드. 웹은 화면비 정책만 쓴다' },
    { id: 'host-tech-help', class: 'exclude-host', names: ['TechnicalHelp', 'TechnicalHelpClient'], reason: '별도 기술 도움말 프로그램 실행(WB02-E 미연결)' },
    { id: 'host-memory', class: 'exclude-host', names: ['RawAllocationFailure', 'ClientAllocationFailure'], reason: '메모리 할당 실패 new_handler·종료 수명(979)' },
    { id: 'host-win32', class: 'exclude-host', names: ['Win32FileError', 'ReadOnlyFileMap', 'FontFaceEncoding'], reason: 'Win32 오류→errno·파일 매핑·Windows ANSI 글꼴 이름 변환' },
    { id: 'host-dev-files', class: 'exclude-host', names: ['ObjectDumpDisk', 'RawObjectDump', 'RawProcessSummary', 'RawDataManagerDump', 'HelpListDisk', 'QuickHelpList'], reason: '개발자 진단 파일 출력(F7 객체 덤프·procSumm.txt·DataManager 덤프·CreateHelpList)' },
    { id: 'host-capture', class: 'exclude-host', names: ['RawCaptureSequence', 'ClientCaptureSequence', 'CaptureInput', 'RawIndexedBitmap', 'IndexedBitmapFile'], reason: '연속 화면 8비트 BMP 파일 저장(Capture 명령)' },
    { id: 'adapt-screen', class: 'adapt', names: ['Screen', 'ScreenPresentation', 'Cursor', 'ParticlePaletteMemory', 'GumpPointerBoundary', 'WindowTitle', 'TextClipboard'], reason: '화면 장치·표시 확대·하드웨어 커서·OS 입력 폴링·창 제목·클립보드. 웹은 Canvas/DOM 으로 대체' },
    { id: 'adapt-sound', class: 'adapt', names: ['ClientAudio', 'SoundDevice', 'SoundMusicFile', 'SoundMusicRuntime', 'MoviePcmOutput', 'MovieTimer', 'SmackerDecoder', 'MovieLifecycle'], reason: 'DirectSound·WAVE 파일·음악 스레드·Smacker 디코더/PCM 출력. 웹은 Web Audio·영상 요소로 대체' },
    {
        id: 'adapt-file', class: 'adapt',
        names: ['BaseFile', 'RawBaseFileIO', 'RawBaseFileOpenPolicy', 'RawBaseFileReadPolicy', 'RawBaseFileWholeRead', 'StartupArchiveFiles', 'StartupFiles', 'RawConfigFileLoad', 'FileSpecQuery', 'ConfigFileRead', 'DataManagerFileRead', 'GifFileRead', 'PaletteFileRead', 'QuickHelpFileRead', 'TypeFileRead', 'XlatFileRead', 'NativeShapeFile', 'FontCacheFile', 'NativeFontCacheFile', 'ClientColorMapFile', 'RawColorMapCache', 'KoreanResources'],
        reason: 'FILE·VFS·FileSpec 검색·파일 읽기 경계와 캐시 파일. 웹은 준비 자료(www/data)와 game-files 조회로 대체',
    },
    { id: 'adapt-options', class: 'adapt', names: ['ClientOptionsDisk', 'ClientOptionsFile', 'ClientOptionsSaveIO'], reason: 'options.cfg 저장 장치. 웹은 버전 있는 브라우저 저장소로 대체' },
    { id: 'adapt-main', class: 'adapt', names: ['ClientMain'], reason: 'WinMain·창 프로시저·메인 루프. 웹은 netstorm.js 수명과 core/main-loop.js 순서 실행기로 대체' },
    { id: 'adapt-external', class: 'adapt', names: ['ExternalHelpClient'], reason: 'Windows URL 실행. 웹은 새 탭/창 열기(2026-11-05 사용자 결정)' },
    { id: 'adapt-screenshot', class: 'adapt', names: ['GifScreenshot'], reason: '화면 저장 GIF 파일 출력. 웹은 브라우저 내려받기 등으로 대체' },
    { id: 'adapt-log', class: 'adapt', names: ['RawTraceLog'], reason: 'trace 로그 스트림. 웹은 콘솔로 대체' },
    { id: 'zacket-default', class: 'port', pattern: /^zacket\//, reason: '단독 미션 내부 전송(zlocal 양끝)·Zone/로그인 바이트' },
    { id: 'o-default', class: 'port', pattern: /^o\//, reason: '원본 \\Ns\\O\\ 공용 규칙·자료 모듈' },
    { id: 'client-default', class: 'port', pattern: /^client\//, reason: '원본 클라이언트 모듈(화면·UI·입력·미션·세션)' },
]);

// cpppj 단위에 대응하지 않는 웹 전용 모듈(webpj 기준 경로)과 사유이다. www 의 모든 .js 모듈은 대응표의 web 칸이나 이 목록 중 한 곳에 있어야 한다. (2026-11-06 WB02-G)
export const WEB_ONLY = Object.freeze({
    'www/assets/menu-assets.js': '메인 메뉴 자산 경로 묶음(웹 자산 준비). 원본 setup.cfg 값과 실행 파일 전역을 근거로 적은 웹 구성',
    'www/assets/terrain-scene.js': '개발용 지형 미리보기의 정적 출력 목록(게임 규칙 없음, WB03-AA)',
    'www/ui/terrain-preview.js': '개발용 지형 미리보기 화면(?map=, WB03-AA)',
    'www/ui/external-link.js': '바깥 링크(http/https/mailto)의 새 탭/창 열기 가능 판정. 원본은 셸 실행(4a00c0 autoOpen)이라 웹 대체 정책(2026-11-05 사용자 결정, WB03-AD)',
    'www/platform/external-opener.js': '판정을 마친 바깥 주소를 숨은 앵커 click 으로 새 탭/창에 여는 브라우저 경계(WB03-AD)',
});

// git 명령을 저장소 루트에서 실행해 출력을 돌려준다. 실패하면 null 이다. (2026-11-06 WB02-G)
function git(...args) {
    try {
        return execFileSync('git', args, { cwd: REPO_ROOT, encoding: 'utf8', stdio: ['ignore', 'pipe', 'ignore'], maxBuffer: 1 << 28 });
    } catch {
        return null;
    }
}

// 저장소 상대 경로 → 단위 열쇠(`o/SidPool` 처럼 SCOPES 접두 'cpppj/src/' 를 뗀, 확장자 없는 경로). 범위 밖이거나 소스가 아니면 null 이다. (2026-11-06 WB02-G)
export function unitKey(file) {
    const normalized = file.replaceAll('\\', '/');
    if (!SCOPES.some((scope) => normalized.startsWith(`${scope}/`))) return null;
    const extension = path.posix.extname(normalized);
    if (!EXTENSIONS.includes(extension)) return null;
    return normalized.slice('cpppj/src/'.length, -extension.length);
}

// 단위 열쇠의 분류 규칙을 고른다. 반환: RULES 의 항목. 맞는 규칙이 없으면 null(미분류)이다. (2026-11-06 WB02-G)
export function classify(key) {
    const name = path.posix.basename(key);
    // 위에서부터 처음 맞는 규칙을 고른다.
    for (const rule of RULES) {
        if (rule.names?.includes(name)) return rule;
        if (rule.pattern?.test(rule.pattern.source.startsWith('^') ? key : name)) return rule;
    }
    return null;
}

// revision 시점의 SCOPES 소스 파일 목록으로 단위 표를 만든다. 반환: Map(열쇠 → { key, files[], primary }). git 이 없으면 null 이다. (2026-11-06 WB02-G)
export function listUnits(revision = BASELINE) {
    const output = git('ls-tree', '-r', '--name-only', revision, '--', ...SCOPES);
    if (output === null) return null;
    const units = new Map();
    // 파일마다 같은 열쇠의 단위에 모은다.
    for (const file of output.split('\n').filter(Boolean)) {
        const key = unitKey(file);
        if (!key) continue;
        if (!units.has(key)) units.set(key, { key, files: [] });
        units.get(key).files.push(file);
    }
    // 단위마다 대표 파일을 .cpp→.h→.inc 순서로 고른다.
    for (const unit of units.values()) {
        unit.files.sort();
        unit.primary = EXTENSIONS.map((extension) => `cpppj/src/${unit.key}${extension}`).find((file) => unit.files.includes(file));
    }
    return new Map([...units.entries()].sort(([a], [b]) => a.localeCompare(b, 'en')));
}

// 대응표 글을 읽는다. `#` 줄은 머리말, 첫 비주석 줄은 열 이름이다. 반환: { rows[] } (행은 COLUMNS 를 열쇠로 하는 객체). (2026-11-06 WB02-G)
export function parseMap(text) {
    const lines = text.split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#'));
    if (lines.length === 0) return { header: null, rows: [] };
    const header = lines[0].split('\t');
    // 둘째 줄부터 행 객체로 바꾼다.
    const rows = lines.slice(1).map((line, index) => {
        const cells = line.split('\t');
        const row = Object.fromEntries(COLUMNS.map((column, at) => [column, cells[at] ?? '']));
        row.line = index + 2;
        row.cellCount = cells.length;
        return row;
    });
    return { header, rows };
}

// 대응표 파일을 읽는다. 없으면 빈 표이다. (2026-11-06 WB02-G)
export function readMap() {
    const full = path.join(REPO_ROOT, MAP_PATH);
    return fs.existsSync(full) ? parseMap(fs.readFileSync(full, 'utf8')) : { header: null, rows: [] };
}

// 메모 칸에서 `[규칙ID] ` 접두와 사람 메모를 나눈다. 반환: { rule, text }. (2026-11-06 WB02-G)
export function splitNote(note) {
    const match = /^\[([a-z0-9-]+)\]\s?(.*)$/.exec(note ?? '');
    return match ? { rule: match[1], text: match[2] } : { rule: null, text: note ?? '' };
}

// 대응표 머리말이다. 열 설명과 갱신 방법을 적는다. (2026-11-06 WB02-G)
const HEADER_COMMENT = [
    `# webpj 이식 대응표(WB02-G, 2026-11-06). 기준선 cpppj ${BASELINE}(제품985)의 ${SCOPES.join('·')} 소스 단위(.cpp/.h/.inc 묶음)마다 한 행이다.`,
    '# 열: cpp(대표 파일)	class(분류, tools/port-map.mjs 규칙이 정함)	web(웹 모듈, `;` 구분)	base(웹과 대조·판정한 cpppj 커밋)	status(todo/partial/port/session/oracle, 제외는 -)	note([규칙ID] 사람 메모)',
    '# 분류를 바꿀 때는 이 파일이 아니라 tools/port-map.mjs 의 RULES 와 사유를 고치고 --write 로 다시 만든다. web·base·status·사람 메모는 --write 가 보존한다.',
    '# 상세 규칙과 판정 기록: docs/webpj-port-map-20261106.md, LEFT_JOBS.webpj.md 3.0·3.6절.',
];

// 행 배열을 대응표 글로 만든다. 탭·줄바꿈은 칸 안에 넣을 수 없으므로 공백으로 바꾼다. (2026-11-06 WB02-G)
export function formatMap(rows) {
    const clean = (value) => String(value ?? '').replace(/[\t\r\n]+/g, ' ');
    return `${[...HEADER_COMMENT, COLUMNS.join('\t'), ...rows.map((row) => COLUMNS.map((column) => clean(row[column])).join('\t'))].join('\n')}\n`;
}

// 기준선 단위 표와 기존 행으로 새 행 배열을 만든다. 분류·규칙 접두는 규칙으로 다시 쓰고 web·base·status·사람 메모는 보존한다.
// 새 단위는 status todo(제외 분류는 '-')로 넣는다. 기준선에 없는 기존 행은 버린다(반환의 dropped). (2026-11-06 WB02-G)
export function rebuildRows(units, existing = []) {
    const byCpp = new Map(existing.map((row) => [row.cpp, row]));
    const rows = [];
    // 기준선 단위마다 행을 만든다.
    for (const unit of units.values()) {
        const rule = classify(unit.key);
        if (!rule) throw new Error(`분류 규칙 없음: ${unit.key}`);
        const old = byCpp.get(unit.primary);
        byCpp.delete(unit.primary);
        const excluded = rule.class.startsWith('exclude-');
        const status = old?.status && (excluded ? old.status === '-' : old.status !== '-') ? old.status : excluded ? '-' : 'todo';
        const human = splitNote(old?.note).text;
        rows.push({ cpp: unit.primary, class: rule.class, web: old?.web ?? '', base: old?.base ?? '', status, note: `[${rule.id}]${human ? ` ${human}` : ''}` });
    }
    return { rows, dropped: [...byCpp.keys()] };
}

// 웹 모듈 경로(webpj 기준)를 저장소 상대 경로로 바꾼다. (2026-11-06 WB02-G)
function webPath(module) {
    return path.posix.join('webpj', module);
}

// 대응표를 검사한다. units 를 주면 기준선 단위와 행이 일대일인지도 본다(git 이 없으면 null 을 넘겨 건너뛴다).
// 반환: { errors[], counts: { class: {…}, status: {…}, byClassStatus: {…} } }. (2026-11-06 WB02-G)
export function checkMap(map = readMap(), units = listUnits()) {
    const errors = [];
    if (!map.header || map.header.join('\t') !== COLUMNS.join('\t')) errors.push(`열 이름이 ${COLUMNS.join(',')} 이 아님`);
    const seen = new Set();
    const counts = { class: {}, status: {}, byClassStatus: {} };
    const commits = new Map();
    // 행마다 칸과 규칙·상태 조건을 확인한다.
    for (const row of map.rows) {
        const where = `${row.line}행 ${row.cpp}`;
        if (row.cellCount !== COLUMNS.length) errors.push(`${where}: 칸 수 ${row.cellCount}`);
        if (seen.has(row.cpp)) errors.push(`${where}: 중복 행`);
        seen.add(row.cpp);
        const key = unitKey(row.cpp);
        const rule = key ? classify(key) : null;
        if (!rule) errors.push(`${where}: 범위 밖 경로이거나 분류 규칙 없음`);
        else if (rule.class !== row.class) errors.push(`${where}: 분류 ${row.class} 가 규칙 ${rule.id}(${rule.class})와 다름`);
        if (!CLASSES.includes(row.class)) errors.push(`${where}: 모르는 분류 ${row.class}`);
        if (!STATUSES.includes(row.status)) errors.push(`${where}: 모르는 상태 ${row.status}`);
        const excluded = row.class.startsWith('exclude-');
        if (excluded !== (row.status === '-')) errors.push(`${where}: 제외 분류와 상태 '-' 가 맞지 않음`);
        const note = splitNote(row.note);
        if (rule && note.rule !== rule.id) errors.push(`${where}: 메모 접두 [${note.rule}] 가 규칙 [${rule.id}] 와 다름(--write 로 갱신)`);
        const modules = row.web ? row.web.split(';').map((item) => item.trim()) : [];
        // 웹 모듈은 webpj 안의 실제 파일이어야 한다.
        for (const module of modules) {
            if (!module || module.startsWith('/') || module.includes('..') || !fs.existsSync(path.join(REPO_ROOT, webPath(module)))) errors.push(`${where}: 없는 웹 모듈 ${module}`);
        }
        const tracked = !['todo', '-'].includes(row.status);
        if (tracked && (modules.length === 0 || !row.base)) errors.push(`${where}: 상태 ${row.status} 는 web 과 base 가 필요함`);
        if (row.status === 'todo' && modules.length > 0) errors.push(`${where}: web 이 있으면 todo 가 아님(partial 이상)`);
        if (row.base) {
            if (!commits.has(row.base)) commits.set(row.base, git('cat-file', '-e', `${row.base}^{commit}`) !== null);
            if (!commits.get(row.base)) errors.push(`${where}: base 커밋 ${row.base} 없음`);
        }
        counts.class[row.class] = (counts.class[row.class] ?? 0) + 1;
        counts.status[row.status] = (counts.status[row.status] ?? 0) + 1;
        const pair = `${row.class}/${row.status}`;
        counts.byClassStatus[pair] = (counts.byClassStatus[pair] ?? 0) + 1;
    }
    // 기준선 단위와 행이 일대일인지, 규칙의 이름이 실제 단위인지(오타 방지) 본다.
    if (units) {
        const expected = new Set([...units.values()].map((unit) => unit.primary));
        for (const cpp of expected) if (!seen.has(cpp)) errors.push(`기준선 단위가 대응표에 없음: ${cpp}`);
        for (const cpp of seen) if (!expected.has(cpp)) errors.push(`기준선에 없는 행: ${cpp}`);
        const names = new Set([...units.keys()].map((key) => path.posix.basename(key)));
        for (const rule of RULES) for (const name of rule.names ?? []) if (!names.has(name)) errors.push(`규칙 ${rule.id} 의 이름 ${name} 이 기준선 단위에 없음`);
    }
    // 웹 모듈마다 대응표 web 칸이나 WEB_ONLY 에 있어야 한다.
    const mapped = new Set(map.rows.flatMap((row) => (row.web ? row.web.split(';').map((item) => item.trim()) : [])));
    for (const module of listWebModules()) if (!mapped.has(module) && !Object.hasOwn(WEB_ONLY, module)) errors.push(`대응도 웹 전용 표시도 없는 웹 모듈: ${module}`);
    for (const module of Object.keys(WEB_ONLY)) {
        if (mapped.has(module)) errors.push(`웹 전용 모듈이 대응표에도 있음: ${module}`);
        if (!fs.existsSync(path.join(REPO_ROOT, webPath(module)))) errors.push(`없는 웹 전용 모듈: ${module}`);
    }
    return { errors, counts };
}

// webpj/www 아래 JavaScript 모듈(webpj 기준 경로)을 모은다. 자산 복사본(www/data)과 의존 패키지는 뺀다. (2026-11-06 WB02-G)
export function listWebModules() {
    const root = path.join(REPO_ROOT, 'webpj', 'www');
    const found = [];
    // 폴더를 깊이 우선으로 훑는다.
    const walk = (directory) => {
        for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
            const full = path.join(directory, entry.name);
            const relative = path.relative(path.join(REPO_ROOT, 'webpj'), full).replaceAll('\\', '/');
            if (entry.isDirectory()) { if (relative !== 'www/data' && entry.name !== 'node_modules') walk(full); }
            else if (entry.name.endsWith('.js')) found.push(relative);
        }
    };
    walk(root);
    return found.sort();
}

// 저장소 상대 cpppj 소스 경로의 대응표 행 base 를 돌려준다(판정을 마친 커밋). 행이 없거나 base 가 비면 null 이다.
// tools/fixture-manifest.mjs 가 cpppj 근거 소스의 재검토 기준으로 쓴다. (2026-11-06 WB02-G)
export function reviewedBase(file, map = readMap()) {
    const key = unitKey(file);
    if (!key) return null;
    const row = map.rows.find((item) => unitKey(item.cpp) === key);
    return row?.base || null;
}

// 기준선(또는 행 base) 뒤 cpppj 변경을 모은다. 반환: { changed[], added[], removed[] }.
//  - changed: 행 단위의 변경 { cpp, class, status, base, commits[] , verdict } — verdict 는 판정 안내(제외 분류는 ① 자동, 미이식은 이식 때 판정, 이식분은 ②/③ 판정 필요)
//  - added: 기준선 뒤 새로 생긴 범위 안 소스(대응표에 없음), removed: HEAD 에서 사라진 기준선 단위 파일. (2026-11-06 WB02-G)
export function collectChanges(map = readMap(), units = listUnits()) {
    if (!units) return null;
    const rowByKey = new Map(map.rows.map((row) => [unitKey(row.cpp), row]));
    const fileToKey = new Map();
    // 기준선 단위 파일 → 열쇠 대응을 만든다.
    for (const unit of units.values()) for (const file of unit.files) fileToKey.set(file, unit.key);
    const head = listUnits('HEAD') ?? new Map();
    const headFiles = new Set([...head.values()].flatMap((unit) => unit.files));
    const added = [...headFiles].filter((file) => !fileToKey.has(file)).sort();
    const removed = [...fileToKey.keys()].filter((file) => !headFiles.has(file)).sort();
    // 단위별 기준 커밋(행 base, 없으면 기준선)마다 한 번씩 git log 를 모아 부른다.
    const groups = new Map();
    for (const unit of units.values()) {
        const base = rowByKey.get(unit.key)?.base || BASELINE;
        if (!groups.has(base)) groups.set(base, []);
        groups.get(base).push(unit);
    }
    const changed = [];
    // 기준 커밋 묶음마다 변경 파일을 커밋별로 읽는다.
    for (const [base, members] of groups) {
        const memberKeys = new Set(members.map((unit) => unit.key));
        const log = git('log', '--format=@%h %s', '--name-only', `${base}..HEAD`, '--', ...SCOPES) ?? '';
        const perKey = new Map();
        let current = null;
        // 로그 줄을 커밋 머리(@해시 제목)와 파일 이름으로 나눈다.
        for (const line of log.split('\n')) {
            if (line.startsWith('@')) { current = line.slice(1); continue; }
            const key = fileToKey.get(line.trim());
            if (!current || !key || !memberKeys.has(key)) continue;
            if (!perKey.has(key)) perKey.set(key, new Set());
            perKey.get(key).add(current);
        }
        // 변경이 있는 단위마다 판정 안내를 붙인다.
        for (const [key, set] of perKey) {
            const row = rowByKey.get(key);
            const excluded = row?.class.startsWith('exclude-');
            const verdict = excluded ? '① 자동(제외 분류, 기록만)' : row?.status === 'todo' ? '이식 때 판정(아직 웹에 없음)' : '②/③ 판정 필요(웹에 이식분 있음)';
            changed.push({ cpp: row?.cpp ?? key, class: row?.class ?? '?', status: row?.status ?? '?', base, commits: [...set], verdict });
        }
    }
    changed.sort((a, b) => a.cpp.localeCompare(b.cpp, 'en'));
    return { changed, added, removed };
}

// --why: 단위 이름의 분류 규칙과 사유를 출력한다. (2026-11-06 WB02-G)
function explain(name, units) {
    const keys = [...(units?.keys() ?? [])].filter((key) => path.posix.basename(key) === name || key === name);
    if (keys.length === 0) { console.log(`기준선 단위에 ${name} 없음`); return 1; }
    // 같은 이름이 여러 폴더에 있으면 모두 보여 준다.
    for (const key of keys) {
        const rule = classify(key);
        console.log(`${key}\t${rule.class}\t[${rule.id}] ${rule.reason}`);
    }
    return 0;
}

// 명령줄 진입점이다. (2026-11-06 WB02-G)
function main(argv) {
    const units = listUnits();
    if (!units) { console.error(`git 기록에서 기준선 ${BASELINE} 을 읽지 못했습니다.`); return 1; }
    const whyAt = argv.indexOf('--why');
    if (whyAt >= 0) return explain(argv[whyAt + 1] ?? '', units);
    if (argv.includes('--write')) {
        const { rows, dropped } = rebuildRows(units, readMap().rows);
        fs.writeFileSync(path.join(REPO_ROOT, MAP_PATH), formatMap(rows), 'utf8');
        console.log(`대응표 갱신: ${rows.length}행${dropped.length ? `, 기준선에 없어 뺀 행 ${dropped.length}개: ${dropped.join(' ')}` : ''}`);
        return 0;
    }
    const map = readMap();
    const { errors, counts } = checkMap(map, units);
    console.log(`대응표 ${map.rows.length}행 (기준선 ${BASELINE} 단위 ${units.size}개), 오류 ${errors.length}개`);
    console.log(`분류: ${CLASSES.map((name) => `${name} ${counts.class[name] ?? 0}`).join(', ')}`);
    console.log(`상태: ${STATUSES.map((name) => `${name} ${counts.status[name] ?? 0}`).join(', ')}`);
    for (const error of errors) console.log(`오류\t${error}`);
    const changes = collectChanges(map, units);
    // 기준선(또는 행 base) 뒤 변경을 출력한다.
    for (const item of changes.changed) console.log(`cpppj 변경\t${item.cpp}\t${item.class}/${item.status}\t${item.base} 이후 ${item.commits.length}커밋\t${item.verdict}\t${item.commits.join(' | ')}`);
    for (const file of changes.added) console.log(`새 파일\t${file}\t기준선 뒤 추가(대응표 밖, 재기준선 때 등록)`);
    for (const file of changes.removed) console.log(`삭제\t${file}\tHEAD 에 없음`);
    if (changes.changed.length + changes.added.length + changes.removed.length === 0) console.log('기준선(행 base) 뒤 cpppj 변경 없음');
    return errors.length > 0 ? 1 : 0;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) process.exitCode = main(process.argv.slice(2));
