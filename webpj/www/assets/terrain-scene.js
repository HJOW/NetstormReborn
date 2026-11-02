// 복원한 지형 계산을 출력 목록으로 모은다. SID·게임 규칙 월드가 아닌 정적 표시 자료이다.
// 근거: RiftTypeTable::ZOrder, SquidFactory::PostCreate, RawTerrainStamp::Create,
// RawIslandPostPop::Prefix, RawBridgeConnect::IslandPostPopPrefix/OwnerColorFrame. (2026-11-02 WB03-AA)
import { TypeTable } from './type-table.js';
import { createFortTypes } from './type-flags.js';
import { parseFort } from './fort.js';
import { decodeOriginalText } from './text-encoding.js';
import { ShapeDatabase } from './shapes.js';
import { layoutFort, pieceFrameCodes } from './territory.js';
import { buildIslandMask, WORLD_CELLS } from './terrain-mask.js';
import { buildTerrainFrames, FrameCodeTable } from './terrain-tiles.js';
import { createFringeStamper } from './terrain-fringe.js';
import { collectNoIslandObjects, findSupports } from './island-supports.js';

// 원본00540d10의 깊이 이름/값이다. 표 순서의 첫 부분 문자열 일치가 우선한다.
export const Z_ORDERS = Object.freeze([
    ['zoNONE', -127], ['zoFALLING', 30], ['zoSTALAG', 20], ['zoCHALRING', 20], ['zoBATTLE', 10],
    ['zoEDGEFARM', 0], ['zoISLAND', 0], ['zoBRIDGE', 0], ['zoBRIDGE_CONNECTOR', -10], ['zoARTIFACTS', -15],
    ['zoEMPLACEMENTS', -20], ['zoILLEGAL_DITHER', -21], ['zoFLARES', -22], ['zoMISSILES', -25],
    ['zoFLYER_SHADOWS', -26], ['zoFLYERS', -30], ['zoFENCE', -35], ['zoMANAICON', -36],
    ['zoUBERGUMP', -40], ['zoMENUGUMP', -60], ['zoDIALOGGUMP', -80], ['zoLOOKGUMP', -100], ['zoRISING', -31],
].map(Object.freeze));

// 숫자 또는 원본 zorder 문자열을 signed DWORD로 해석한다. 숫자는 RiftTypeTable::ToInt의 범위를 따른다.
export function parseZOrder(value) {
    if (typeof value === 'number') {
        return value > -2147483649 && value < 2147483648 ? Math.trunc(value) | 0 : 0;
    }
    const text = String(value);
    const word = /([A-Za-z0-9_]+)[ \t]*/.exec(text);
    if (!word) throw new Error('zorder 이름이 없습니다.');
    const item = Z_ORDERS.find(([name]) => name.includes(word[1]));
    if (!item) throw new Error(`알 수 없는 zorder: ${word[1]}`);
    const expression = /[ \t]*([A-Za-z0-9_]+)[ \t]*(\+|-)[ \t]*([0-9]+)/.exec(text);
    if (!expression) return item[1];
    const delta = Number(expression[3]);
    if (delta > 0x7fffffff) throw new RangeError('zorder 오프셋 범위 오류');
    return (item[1] + (expression[2] === '+' ? delta : -delta)) | 0;
}

// 타입 속성의 마지막 zorder를 원본 슬롯 BYTE→signed 깊이로 바꾼다. 미지정은 타입 초기값0이다.
export function typeDrawDepth(definition) {
    let value = 0;
    // 속성은 원본 로더처럼 선언 순서대로 덮어쓴다.
    for (const property of definition.properties) if (property.name.toLowerCase() === 'zorder') value = parseZOrder(property.value);
    return (value << 24) >> 24;
}

// 명시적으로 제공한 소유자 색 번호를 검사한다. 플레이어 번호를 추측해 넣지 않는다.
function colorNumber(color) {
    if (!Number.isInteger(color) || color < 0 || color > 8) throw new RangeError('소유자 색 번호 범위 오류');
    return color;
}

// .fort 배치→마스크→지면/절벽→받침을 공통 WorldSpriteRenderer 입력으로 만든다.
// options: random/crt/stampVariants/themeOf와 excludedFlags, colorOfTerritory(영역→색), colorOfSupport(받침→색).
// 소유자 마감 전 기본은 중립0·지역 원소 도장 없음이다. 저장된 noIsland.owner를 실제 소유자로 사용하지 않는다.
export function buildTerrainScene(fort, table, options = {}) {
    const layout = layoutFort(fort, pieceFrameCodes(table));
    const land = buildIslandMask(layout.map, layout.islands);
    // 필요한 타입별 자산 번호·정의·깊이를 원본 표에서 구한다.
    const type = (name) => {
        const number = table.find(name);
        const definition = table.definitions[number - 70];
        if (!definition) throw new Error(`지형 타입 정의 없음: ${name}`);
        return { number, definition, depth: typeDrawDepth(definition) };
    };
    const isle = type('isle');
    const fringeType = type('fringe');
    const island = type('island');
    const stalag = type('islandStalag');
    const isleFrames = FrameCodeTable.fromDefinition(isle.definition);
    const fringe = createFringeStamper(isleFrames, FrameCodeTable.fromDefinition(fringeType.definition), options);
    const tiles = buildTerrainFrames(land, isleFrames, { ...options, onStamp: fringe.onStamp });
    const supports = findSupports(collectNoIslandObjects(layout));
    const entries = [];
    // 표시 범위는 칸 좌표의 양 끝 포함이다. 초기 카메라를 위한 개발용 자료이며 게임 맵 경계가 아니다.
    const bounds = { left: WORLD_CELLS, top: WORLD_CELLS, right: -1, bottom: -1 };
    const add = (kind, asset, frame, x, y, color) => {
        entries.push(Object.freeze({ kind, type: asset.number, frame, x, y, depth: asset.depth, ownerColor: colorNumber(color) }));
        bounds.left = Math.min(bounds.left, x);
        bounds.top = Math.min(bounds.top, y);
        bounds.right = Math.max(bounds.right, x);
        bounds.bottom = Math.max(bounds.bottom, y);
    };
    // 첫 도장의 지면 뒤 절벽 순서로 출력 자료를 모은다. 절벽의 생성 소유자는 원본처럼0이다.
    for (let index = 0; index < tiles.length; index += 1) {
        if (tiles[index] < 0) continue;
        const x = index % WORLD_CELLS;
        const y = Math.floor(index / WORLD_CELLS);
        add('land', isle, tiles[index], x, y, options.colorOfTerritory?.(land[index]) ?? 0);
        if (fringe.cells[index] >= 0) {
            const point = fringe.positionOf(x, y);
            add('fringe', fringeType, fringe.cells[index], point.x, point.y, 0);
        }
    }
    // F칸에서 island, 그 최초 Pop에서 islandStalag가 같은 anchor에 생긴다. 중립 프레임은8이다.
    for (const support of supports.supports) {
        const color = colorNumber(options.colorOfSupport?.(support) ?? 0);
        const frame = color === 0 ? 8 : color - 1;
        add('support', island, frame, support.anchorX, support.anchorY, color);
        add('stalag', stalag, frame, support.anchorX, support.anchorY, color);
    }
    return { layout, land, tiles, fringe, supports, entries: Object.freeze(entries), bounds: entries.length ? bounds : null };
}

// 개발용 맵 이름을 공개 자산의 파일 이름으로 제한한다. 경로/URL은 받지 않고 외부 파일을 가져오지 않는다.
export function terrainMapName(name) {
    const base = String(name).replace(/\.fort$/i, '');
    if (!/^[A-Za-z0-9_ !()-]{1,100}$/.test(base)) throw new Error('잘못된 맵 이름입니다.');
    return base;
}

// 웹에서 이미 받은 GameFiles만 사용해 정적 장면과 그리기 자산을 만든다. 새 네트워크 요청은 없다.
export function loadTerrainScene(files, name, options = {}) {
    const base = terrainMapName(name);
    const bytes = files.read(`d/${base}.fort`);
    if (!bytes) throw new Error(`맵 파일이 없습니다: ${base}.fort`);
    const table = TypeTable.fromArchive(files, decodeOriginalText, (typeName, error) => { throw new Error(`타입 ${typeName}: ${error.message}`); });
    const shapeBytes = files.read('d/_shapes.shp');
    if (!shapeBytes) throw new Error('셰이프 자산이 없습니다.');
    const fort = parseFort(bytes, createFortTypes(table));
    return { name: base, table, shapes: new ShapeDatabase(shapeBytes), ...buildTerrainScene(fort, table, options) };
}
