// 월드 그림 제출을 타입 표·SHP·카메라·8비트 표면에 연결한다. 게임 규칙이나 SID 사본을 만들지 않는다.
// 근거: cpppj SpriteDraw::Draw(타입 발 기준점), Renderer::DrawSprite(프레임 내부 합성), DrawSort.cpp.
// 호출자가 실제 상태의 깊이·소유자 색 번호·레이어/그림자 색표를 지정한다. 애니메이션/가상 Draw/표시 수집기는 후속이다. (2026-11-02 WB03-Z)
import { sortDrawEntries } from './draw-sort.js';
import { clipRect } from './indexed-surface.js';

// signed32 좌표에 상대 오프셋을 더한다. Renderer::Position처럼 범위 밖을 조용히 감지 않는다.
function position(origin, offset) {
    const result = origin + offset;
    if (!Number.isInteger(result) || result < -0x80000000 || result > 0x7fffffff) throw new RangeError('그림 좌표 범위 오류');
    return result;
}

// 타입의 논리 클러스터 번호와 레이어를 물리 SHP 프레임으로 바꾼다. 없는 레이어는 오류이다.
// frame을 modulo로 감지 않으므로 다른 타입의 프레임/메타데이터를 잘못 그리지 않는다.
export function physicalShapeFrame(table, shapes, type, frame, layer = 0) {
    const view = table.view(type);
    if (!view || view.block < 0 || !Number.isInteger(frame) || frame < 0 || frame >= view.frameCount ||
        !Number.isInteger(layer) || layer < 0) throw new RangeError('타입/논리 프레임/레이어 범위 오류');
    const number = frame + layer * view.frameCount;
    shapes.frame(view.block, number);
    return number;
}

// 전체 제출 목록을 원본 순서로 정렬하고 같은 객체의 후속 레이어는 연속 출력한다.
// 사용: new WorldSpriteRenderer({ table, shapes, ownerColors }).draw(surface, camera, entries, { clip });
// entries: [{ type, frame(논리 클러스터), layer=0, x/y(월드 float), depth(signed WORD), ownerColor=0,
//             shadow=false, colorMap=null, following:[{ frame, layer }] }]. ownerColor는 플레이어 번호가 아닌 색 번호이다.
export class WorldSpriteRenderer {
    // 원본 자산과 등록된 소유자 색표(Map: 타입 → 9×256바이트)를 빌린다. 원본 맵 초기화는 하지 않는다.
    constructor({ table, shapes, ownerColors = new Map() }) {
        // 타입 이름/번호와 클러스터/발 기준점 표이다.
        this.table = table;
        // 타입 로딩 순서와 같은 블록의 VFX 셰이프 데이터베이스이다.
        this.shapes = shapes;
        // 원본 팔레트로 만든 소유자 색 변환표이다. 등록되지 않은 타입은 원색 그대로이다.
        this.ownerColors = ownerColors;
    }

    // 현재 좌표/타입으로 불변 표시 계획을 만든다. 입력 배열·객체·카메라는 바꾸지 않는다.
    // sort=false는 Renderer 정렬 끔 옵션에 대응한다. 화면 밖 항목도 정렬에는 참여한다(수집기와 구별).
    plan(camera, entries, { sort = true } = {}) {
        const plan = entries.map((entry) => {
            const view = this.table.view(entry.type);
            const frame = physicalShapeFrame(this.table, this.shapes, entry.type, entry.frame, entry.layer ?? 0);
            const point = camera.project(entry.x, entry.y);
            // 후속 레이어를 별도 정렬하지 않고 같은 기준점에 이어 그린다.
            const frames = [frame, ...(entry.following ?? []).map((next) => physicalShapeFrame(this.table, this.shapes, entry.type, next.frame, next.layer ?? 0))];
            const owner = entry.ownerColor ?? 0;
            if (!Number.isInteger(owner) || owner < 0 || owner > 8) throw new RangeError('소유자 색 번호 범위 오류');
            const shadow = entry.shadow ?? false;
            // 그림자에는 별도 대상 변환표가 필수이며 소유자 표를 대신 쓰지 않는다.
            const colorMap = entry.colorMap ?? (shadow ? null : this.ownerColors.get(entry.type)?.[owner] ?? null);
            if (shadow && !colorMap) throw new TypeError('그림자에는 배경 색 변환표가 필요합니다.');
            if (colorMap && (!(colorMap instanceof Uint8Array) || colorMap.length !== 256)) throw new TypeError('색 변환표는 256바이트여야 합니다.');
            return {
                // 정렬 키는 float32 월드 좌표이며 화면 위치/타입 발 기준점과 구분한다.
                x: Math.fround(entry.x), y: Math.fround(entry.y), depth: entry.depth,
                // 제출 객체를 추적할 때 쓰는 원래 참조이다. 프레임/색표와 함께 정렬된다.
                source: entry,
                block: view.block, frames,
                screenX: position(point.x, -view.hotspotX), screenY: position(point.y, -view.hotspotY),
                colorMap, shadow,
            };
        });
        // 정렬 끔도 키 계약은 검사하되 제출 순서는 유지한다.
        if (sort) sortDrawEntries(plan);
        else sortDrawEntries(plan.slice());
        return plan;
    }

    // 배경을 지우지 않고 현재 표면에 합성한다. clip은 right/bottom 제외이다.
    // 반환한 계획을 진단에 쓸 수 있다. 부분 갱신/화면 표시(Canvas)는 상위 호출자가 맡는다.
    draw(surface, camera, entries, { clip = surface.bounds, sort = true } = {}) {
        const plan = this.plan(camera, entries, { sort });
        const area = clipRect(clip, surface.bounds);
        // 같은 객체의 모든 레이어를 원래 호출 순서로 합성한다.
        for (const sprite of plan) {
            // 특수 레코드(그림 없음)는 해독하지 않고 건너뛴다.
            for (const number of sprite.frames) {
                const frame = this.shapes.frame(sprite.block, number);
                if (frame.special) continue;
                const left = position(sprite.screenX, frame.rect.left);
                const top = position(sprite.screenY, frame.rect.top);
                const right = position(sprite.screenX, frame.rect.right + 1);
                const bottom = position(sprite.screenY, frame.rect.bottom + 1);
                if (right <= area.left || left >= area.right || bottom <= area.top || top >= area.bottom) continue;
                surface.drawImage(this.shapes.decode(sprite.block, number), left, top, area, sprite.colorMap, sprite.shadow);
            }
        }
        return plan;
    }
}
