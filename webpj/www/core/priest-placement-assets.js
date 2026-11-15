// 원본 자산에서 사제 배치·건설이 쓰는 프레임 코드·기준점·SHP 자료를 같은 TYPE 번호로 묶어 소유하는 자료 묶음이다(cpppj `client/PriestPlacementAssets.cpp/.h`, 10.78 전용).
// 사용: const metadata = new PriestPlacementAssets(table, database, records);
//   - metadata.frames[type] = { frames(프레임 코드 목록), defaultFrame } — 자산 번호 70 부터 `.type` 정의의 프레임 순서와 특수 기본 프레임을 복사한다. 내장 타입은 빈 표와 0 이다.
//   - metadata.hotspots[type] = { x, y } — 타입 로더가 한 번 변환한 타입 기준점(조회 때 발자국/화면 배율을 다시 곱하지 않는다).
//   - metadata.shapes — 실제 SHP 추가 헤더를 같은 번호로 연결한 물리 프레임 표(`squidRendererShapes`).
//   - metadata.geometry = { patternTypes[8], decoderDiagnostics } — CanonTypeDecoder 가 패턴을 고르는 원본 전역 순서(puzzlePiece·bridge·island·noIsland·thunderCannon·rainCannon·windArcher·windBlocker)의 TYPE 번호이다.
// table 은 `TypeTable`, database 는 `ShapeDatabase`, records 는 `buildTypeRecords(table.definitions)` 의 188칸 TYPE 레코드(hotspotX·hotspotY)이다. 내부 배열을 소유하므로 배치 모듈이 임시 자료를 참조하지 않는다.
// 패턴 이름은 typename 이 아니라 실제 로딩 파일 이름(TYPE_LOAD_ORDER)으로 해석한다. 이 모듈은 DOM 을 쓰지 않는다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인).
// (2026-11-15 WB04-E562: 4-D C 구획 RawCampaignConstructionBindings 의 시험 장면과 Session 조립이 쓰는 자료 묶음으로 이식)
import { squidRendererShapes } from '../render/squid-renderer.js';
import { FIRST_ASSET_TYPE_NUMBER, PATCH_TYPE_COUNT, TYPE_LOAD_ORDER } from '../assets/type-table.js';

// CanonTypeDecoder 가 패턴을 고르는 원본 전역의 순서이다. 사제 전용 배치의 종류 계약은 별도로 유지한다. (2026-11-15 WB04-E562)
const PATTERN_NAMES = Object.freeze(['puzzlePiece', 'bridge', 'island', 'noIsland', 'thunderCannon', 'rainCannon', 'windArcher', 'windBlocker']);

// 사제 배치 검사와 건설이 쓰는 프레임·핫스폿·셰이프·기하 자료이다. (2026-11-15 WB04-E562)
export class PriestPlacementAssets {
    // 내장 타입도 포함하여 모든 배치 조회가 원본 타입 번호를 그대로 쓰게 한다. 자산 정의/레코드 크기가 맞지 않으면 거부한다.
    constructor(table, database, records) {
        if (!Array.isArray(records) || records.length !== PATCH_TYPE_COUNT || table?.definitions?.length !== TYPE_LOAD_ORDER.length) {
            throw new RangeError('사제 배치 자산의 TYPE 표/정의 판본 불일치');
        }
        // 타입별 프레임 자료이다(내장 타입은 빈 표).
        this.frames = records.map(() => ({ frames: [], defaultFrame: 0 }));
        // 프레임 기준점을 찾는 타입별 핫스폿 자료이다.
        this.hotspots = records.map((record) => ({ x: record.hotspotX | 0, y: record.hotspotY | 0 }));
        // 프레임의 크기·그림 자료를 조회하는 셰이프 자료이다.
        this.shapes = squidRendererShapes(table, database);
        // 자산 번호 70 부터 프레임 순서와 특수 기본 프레임을 복사한다.
        for (let block = 0; block < table.definitions.length; block += 1) {
            const definition = table.definitions[block];
            if (!definition) throw new Error(`사제 배치 자산 타입 정의 없음: ${TYPE_LOAD_ORDER[block]}`);
            this.frames[FIRST_ASSET_TYPE_NUMBER + block] = { frames: definition.clusters.map((cluster) => cluster.code), defaultFrame: definition.defaultFrame | 0 };
        }
        // 프레임 발자국·사각형·칸 순회를 계산하는 기하 상태이다(패턴 여덟 TYPE). 이름은 실제 로딩 파일 이름으로 해석한다.
        this.geometry = {
            patternTypes: PATTERN_NAMES.map((name) => {
                const block = TYPE_LOAD_ORDER.indexOf(name);
                if (block === -1) throw new Error(`사제 배치 패턴 타입 없음: ${name}`);
                return FIRST_ASSET_TYPE_NUMBER + block;
            }),
            decoderDiagnostics: null,
        };
    }
}
