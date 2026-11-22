// 전용 도움말의 실제 요구 에너지 그림과 TYPE 정보 HTML을 만든다.
import { formatPlayerMenuText } from '../core/raw-player-menu.js';
import { bindAiTypeRequiredEnergy } from '../core/raw-type-required-energy.js';
import { cameraFtolLow } from '../render/camera-view.js';
import { typeNumber, typeString } from '../assets/type-table.js';

// 원본 00540ad8 ASCII 문자 순서이며 점은 구분자로 소비한다.
const MANA_LETTERS = 'wrtsWRTS.?>';

// 004735c0: 비점 문자별 원본 그림 프레임과 앞쪽 강조를 순서대로 출력한다.
export function manaPictureText(mana, highlighted = -1, type = 103, suppliedFrames = null) {
    if (typeof mana !== 'string' || !Number.isInteger(highlighted) || !Number.isInteger(type) || (suppliedFrames !== null && (!(suppliedFrames instanceof Int32Array) || suppliedFrames.length !== 128))) throw new TypeError('도움말 마나 그림의 실제 입력 오류');
    const end = mana.indexOf('\0'); if (end >= 0) mana = mana.slice(0, end);
    if (mana.length >= 256) throw new RangeError('도움말 마나 원본 256바이트 초과');
    const frames = suppliedFrames ?? new Int32Array(128);
    // 기본 프레임은 실제 PE 문자표에서 한 번 배정하며 호출자가 제공한 현재 표는 보존한다.
    if (!suppliedFrames) for (let index = 0; index < MANA_LETTERS.length; index += 1) frames[MANA_LETTERS.charCodeAt(index)] = index;
    let output = '', count = 0;
    // 점은 아이콘으로 세지 않고 각 실제 비점 문자만 소비한다.
    for (let index = 0; index < mana.length; index += 1) {
        const code = mana.charCodeAt(index); if (code === 46) continue;
        if (code >= 128) throw new RangeError('도움말 마나 비ASCII 전역 조회 미복원');
        const frame = (frames[code] + (count < highlighted ? 11 : 0)) | 0; output += `~[I${type >>> 0}.${frame}]`; count += 1;
    }
    if (output.length >= 250) throw new RangeError('도움말 마나 원본 그림 250바이트 초과');
    return output;
}

// 현재 TYPE·번역·Config를 원본 조회 순서대로 읽어 별도 정보 패널의 HTML을 만든다.
export function buildQuickHelpInfoHtml(hooks, manaType = 103, manaFrames = null) {
    if (!hooks || ['type', 'description', 'hitPoints', 'translate', 'substitute', 'config'].some((name) => typeof hooks[name] !== 'function')) throw new TypeError('도움말 TYPE 정보의 실제 장치 누락');
    const current = () => hooks.type(), translate = (value) => hooks.translate(value), print = (pattern, values) => formatPlayerMenuText(pattern, values);
    const ctext = (value) => String(value).split('\0', 1)[0];
    const heading = print('<h2>%s</h2>', [hooks.description()]); let alignment;
    if (current().theme < 5) {
        const names = ['Wind', 'Rain', 'Thunder', 'Sun', 'Energy'], pattern = translate('Alignment: <c>%s</c>'), theme = current().theme;
        if (theme < 0 || theme >= names.length) throw new RangeError('도움말 원소 이름표 범위');
        alignment = print(pattern, [translate(names[theme])]);
    } else alignment = print(translate('Alignment: <c>None</c>'), []);
    let classification = '';
    if (ctext(current().classification)) { const pattern = translate('Class: <c>%s</c>'), label = translate(ctext(current().classification)), value = ctext(hooks.substitute(label)); if (value.length >= 1024) throw new RangeError('도움말 클래스 치환 범위'); classification = print(pattern, [value]); }
    let hits;
    if ((current().flags1 & 0x10) && hooks.hitPoints()) hits = print(translate('Hits: <c>%d</c>'), [hooks.hitPoints()]);
    else hits = print(translate('Hits: <c>Invulnerable</c>'), []);
    // 원본은 실수 0 판정과 값 변환 시 현재 TYPE을 각각 다시 읽는다.
    const number = (numeric, empty, read) => read() === 0 ? print(translate(empty), []) : print(translate(numeric), [cameraFtolLow(read())]);
    const range = number('Range: <c>%d</c>', 'Range: <c>n/a</c>', () => current().range); let damage;
    if (current().damage === 0) damage = print(translate((current().flags2 & 0x100) ? 'Damage: <c>special</c>' : 'Damage: <c>n/a</c>'), []);
    else damage = print(translate('Damage: <c>%d</c>'), [cameraFtolLow(current().damage)]);
    const cost = number('Cost in Storm Power: <c>%d</c>~[IfortGump.3]', 'Cost in Storm Power: <c>n/a</c>', () => current().cost); let mana;
    if (!ctext(current().mana)) mana = print(translate((current().flags2 & 0x100) ? '%s to Build: <c>n/a</c>' : '%s to Build: <c>None</c>'), [hooks.config('influence')]);
    else {
        const letters = ctext(current().mana); if (letters.length >= 256) throw new RangeError('도움말 요구 에너지 원본 256바이트 초과');
        const upper = letters.replace(/[a-z]/g, (letter) => String.fromCharCode(letter.charCodeAt(0) - 32));
        const pictures = manaPictureText(upper, -1, manaType, manaFrames), pattern = translate('%s to Build: <c>%s</c>');
        mana = print(pattern, [hooks.config('influence'), pictures]);
    }
    const html = `${heading}${alignment}<br>${classification}<br>${hits}<br>${range}<br>${damage}<br>${cost}<br>${mana}<br>`;
    if (html.length >= 1024) throw new RangeError('도움말 정보 원본 1024바이트 초과'); return html;
}

// TYPE의 실제 Help 프레임과 SHP 기준점을 읽는다. 그림 제외 TYPE과 프레임 -1은 null이다.
export function createQuickHelpPicture(game, type) {
    if (!game || !Number.isInteger(type) || type < 0 || type >= 188) throw new RangeError('도움말 그림 TYPE 범위');
    if (type < 70 || type - 70 >= game.types().length || type === 109 || type === 161) return null;
    const view = game.typeTable.view(type), frameNumber = game.types()[type - 70].definition.helpFrame;
    if (frameNumber === -1) return null;
    const block = type - 70, frame = game.shapes.blocks[block].frames[frameNumber];
    if (!frame) throw new RangeError('도움말 그림의 실제 SHP 프레임 누락');
    const metrics = game.shapes.squidMetrics(block, frameNumber);
    return { type, frameNumber, frame, image: frame.special ? null : game.shapes.decode(block, frameNumber),
        hotspotX: Math.fround(metrics.hotspotX - view.hotspotX), hotspotY: Math.fround(metrics.hotspotY - view.hotspotY) };
}

// 미션 월드가 없을 때(메인 메뉴 Help) 전용 도움말의 TYPE 정보를 게임 자산의 장기 TYPE 표에서 읽는다(원본 QuickHelpHtml.cpp ReadAssetQuickHelpType).
// 월드가 있으면 Session.helpTypeInfo 가 같은 항목을 현재 표에서 읽는다. 자산 TYPE(70~185)만 range·damage(hpPerSec)·class 를 정의에서 읽고 float32 로 저장하며 그 밖은 0·빈 문자열이다.
// 인자: game(게임 자산: typeTable.definitions 116개·types()), types(현재 TYPE 188칸 레코드 배열), type(번호). 반환: { theme, flags1, flags2, cost, range, damage, classification, mana }.
// 범위 밖 번호나 23바이트를 넘는 class 는 원본처럼 조용히 넘기지 않고 RangeError 로 알린다. (2026-11-23 WB04-E1240: 월드 없는 Help 의 TYPE 정보)
export function readAssetQuickHelpType(game, types, type) {
    if (!game?.typeTable || !Array.isArray(types) || !Number.isInteger(type) || type < 0 || type >= types.length) throw new RangeError('월드 설치 전 Help TYPE 범위');
    const record = types[type];
    const result = { theme: record.theme, flags1: record.flags1, flags2: record.flags2, cost: record.cost, range: 0, damage: 0, classification: '',
        mana: bindAiTypeRequiredEnergy(game.typeTable.definitions, types)(type) };
    // 자산 TYPE 은 번호 70 부터 정의 순서대로다(원본 kFirstAssetTypeNumber).
    if (type >= 70 && type - 70 < game.types().length) {
        const definition = game.types()[type - 70].definition;
        result.range = Math.fround(typeNumber(definition, 'range') ?? 0); result.damage = Math.fround(typeNumber(definition, 'hpPerSec') ?? 0);
        const classification = typeString(definition, 'class');
        if (classification !== null) result.classification = classification.split('\0', 1)[0];
        if (result.classification.length > 23) throw new RangeError('월드 설치 전 Help class 23바이트 초과');
    }
    return result;
}
