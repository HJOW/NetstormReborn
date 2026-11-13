// 순수 배경 계획의 원문 IndexedImage/프레임 상대점 출력 어댑터다(cpppj client/GumpBackground.cpp/.h). 기존 UI도 같은 계획기를 쓴다. (2026-11-14 WB04-E415)
import { drawIndexedImage } from './indexed-surface.js';

// source opacity와 pass순서/명암 표를 유지하며 현재 canvas.indices에만 쓴다.
export function drawRawGumpBackground(canvas, texture, frameOffset, passes, maps) {
    if (canvas.width <= 0 || canvas.height <= 0 || canvas.indices.length < canvas.width * canvas.height || texture.indices.length < texture.width * texture.height || texture.opacity.length < texture.width * texture.height) throw new RangeError('배경 IndexedImage 용량 오류');
    // 원문 바탕/테두리의 겹침 순서를 유지한다.
    for (const pass of passes) { if (!pass.validTile) continue; const colors = pass.shade === 1 ? maps.bright : pass.shade === 2 ? maps.dark : null;
        // 화면 격자 기준점에 실제 SHP 상대점을 더한다.
        for (const point of pass.origins) drawIndexedImage(canvas.indices, canvas.width, canvas.height, texture, (point.x + frameOffset.x) | 0, (point.y + frameOffset.y) | 0, pass.clip, colors);
    }
}
