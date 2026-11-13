// 게임 실행에 필요한 타입·셰이프 자산 저장소의 이름 조회 층을 옮긴 모듈이다(cpppj `client/GameAssets.cpp/.h`, 10.78 전용).
// cpppj 의 GameAssets 는 `.type` 116개와 `_shapes.shp` 를 읽어 연결하는 집계인데, 웹은 이미 `TypeTable.fromArchive`(타입 정의 읽기·번호 70부터의 표)·`ShapeDatabase`(SHP 블록)·`buildTypeRecords`(타입 레코드)가 같은 일을 하므로
// 이 모듈은 남은 세 가지만 더한다 — E272 이월분:
//  - 블록 수 검사: SHP 블록 수가 로딩 순서(116)와 다르면 거부한다(CD 자료를 10.78 로 읽는 실수를 막는 원문 검사).
//  - find(name): 파일 이름(assetName)을 먼저, 없으면 `.type` 안의 typename 으로 찾는다. ASCII 대소문자는 구분하지 않으며 같은 이름이면 앞선 로딩 순서가 이긴다.
//    예: 'dude' 파일의 typename 은 'Man', 'edgeFarm'→'efarm', 'anim'→'Flare'(typename 'flare' 는 두 타입이 쓰지만 파일 이름 'flare' 가 먼저 선택됨). 없으면 Error.
//  - frameIndex(type, cluster, layer): 클러스터 이름과 레이어를 SHP 의 원래 프레임 번호 `layer × 클러스터 수 + 클러스터 순번`으로 바꾼다(레이어별 연속 프레임 유지, GIF 번호로 다시 정렬하지 않음).
//    중복 클러스터는 첫 일치이며 SHP 블록이나 프레임 수를 벗어나면 RangeError, 없는 클러스터는 Error 이다.
// 사용: const assets = new GameAssets(table, shapes); const bridge = assets.find('bridge'); assets.frameIndex(bridge, 'A11', 0); assets.types()[i].block + 70 === 타입 번호.
// 타입 정의를 읽지 못한 타입(definition null)은 typename 검색에서 건너뛰고 frameIndex 는 거부한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E385: 5-A GameAssets 이월분으로 추가, 기준선 뒤 cpppj 변경 없음)
import { FIRST_ASSET_TYPE_NUMBER, TYPE_LOAD_ORDER } from './type-table.js';

// 사용자 입력 이름을 원본 ASCII 식별자 규칙으로 비교한다(비 ASCII 는 그대로). (2026-11-13 WB04-E385)
const asciiLower = (text) => String(text).replace(/[A-Z]/g, (letter) => String.fromCharCode(letter.charCodeAt(0) + 32));
const sameName = (left, right) => asciiLower(left) === asciiLower(right);

// 자산 저장소이다. types() 는 원본 로딩 순서의 { assetName, definition, block } 목록이다(block + 70 이 타입 번호). (2026-11-13 WB04-E385)
export class GameAssets {
    // 읽은 타입 표, SHP 데이터베이스, 로딩 순서의 자산 목록이다.
    #table;
    #shapes;
    #types;

    // 타입 표와 SHP 를 연결한다. SHP 블록 수가 로딩 순서와 다르면 거부한다. 사용: new GameAssets(TypeTable.fromArchive(archive, decode), new ShapeDatabase(bytes)). (2026-11-13 WB04-E385)
    constructor(table, shapes) {
        if (shapes.blocks.length !== TYPE_LOAD_ORDER.length) throw new Error('Shape block count does not match selected edition (use --cd for CD data)');
        if (table.definitions.length !== TYPE_LOAD_ORDER.length) throw new Error('타입 정의 수가 로딩 순서와 다릅니다.');
        this.#table = table;
        this.#shapes = shapes;
        // 다른 판본의 추가 타입이나 디렉터리 정렬 순서를 섞지 않는다.
        this.#types = Object.freeze(TYPE_LOAD_ORDER.map((assetName, block) => Object.freeze({ assetName, definition: table.definitions[block], block })));
    }

    // 원본 로딩 순서를 보존한 자산 목록이다. (2026-11-13 WB04-E385)
    types() { return this.#types; }

    // 파일 이름 또는 typename 으로 타입을 찾는다. 파일 이름이 먼저, typename 은 다음 단계이다. (2026-11-13 WB04-E385)
    find(name) {
        for (const type of this.#types) if (sameName(type.assetName, name)) return type;
        // 원본 typename 은 파일 별명과 별도로 보존한다.
        for (const type of this.#types) if (type.definition && sameName(type.definition.name, name)) return type;
        throw new Error(`Unknown game asset type: ${name}`);
    }

    // 클러스터·레이어를 SHP 의 원래 프레임 번호로 바꾼다. (2026-11-13 WB04-E385)
    frameIndex(type, clusterName, layer) {
        const clusters = type.definition?.clusters ?? [];
        const stride = clusters.length;
        if (type.block >= this.#shapes.blocks.length) throw new RangeError('Shape type block index');
        if (stride === 0 || !Number.isSafeInteger(layer) || layer < 0 || layer * stride + stride > Number.MAX_SAFE_INTEGER) throw new RangeError('Shape layer index');
        // 중복 클러스터도 원본 프레임 검색처럼 첫 일치를 선택한다.
        for (let index = 0; index < stride; index += 1) {
            if (sameName(clusters[index].name, clusterName)) {
                const frame = layer * stride + index;
                if (frame >= this.#shapes.blocks[type.block].frames.length) throw new RangeError('Shape frame index');
                return frame;
            }
        }
        throw new Error(`Unknown shape cluster: ${clusterName}`);
    }

    // 타입 번호(70부터)이다. 편의 접근자로 cpppj 호출자의 `kFirstAssetTypeNumber + type.block` 을 한 곳에 둔다. (2026-11-13 WB04-E385)
    typeNumber(type) { return FIRST_ASSET_TYPE_NUMBER + type.block; }

    // 연결한 SHP 데이터베이스와 타입 표이다. (2026-11-13 WB04-E385)
    get shapes() { return this.#shapes; }
    get typeTable() { return this.#table; }
}
