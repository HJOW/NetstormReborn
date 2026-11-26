// 보존 Native 건설 출력·입자 수명·인접DWORD와 실제 마나 볼트 그림자를 오프라인 대조한다.
// 사용: node webpj/tools/verify-construction-pixel-observation.mjs. 제품 화면 허용값이나 기본 인접값을 바꾸지 않는다.
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
import {realCaptureAssets} from '../tests/capture-assets-support.mjs';
import {readData} from '../tests/helpers.mjs';
import {GamePalette} from '../www/assets/palette.js';
import {buildSpriteColorMaps} from '../www/core/sprite-color-maps.js';
import {buildLightningPalette} from '../www/core/particle-gradient.js';
import {RawConstructionFrame} from '../www/core/raw-construction-frame.js';
import {ParticlePaletteMemory,OBSERVED_ADJACENT_DWORD} from '../www/render/particle-palette-memory.js';
import {drawTranslatedVfxImage} from '../www/render/vfx-transform.js';
import {readPng} from './image-files.mjs';
// 보존 원문의 위치와 실제 동일성 비교 PNG다.
const ROOT=fileURLToPath(new URL('../../',import.meta.url)),EVIDENCE=path.join(ROOT,'docs/webpj-e1380-evidence');
// 원문 필드 이름으로 TSV를 읽으며 임의의 줄 위치에 의존하지 않는다.
const schema=JSON.parse(fs.readFileSync(path.join(EVIDENCE,'trace-schema.json'),'utf8'));
const records=fs.readFileSync(path.join(EVIDENCE,'frame8000.tsv'),'utf8').trim().split(/\r?\n/).map(line=>{
 const cells=line.split('\t'),fields=schema[cells[0]];assert.ok(fields,cells[0]);assert.equal(cells.length,fields.length);
 return Object.fromEntries(fields.map((name,index)=>[name,index===0||name==='operation'?cells[index]:Number(cells[index])]));
});
// 두 호출에서 같은 실제 캡처 좌표·출력과 입자 경계가 관측되었다.
const watches=records.filter(row=>row.kindName==='watch'),particles=records.filter(row=>row.kindName==='particle'),constructions=records.filter(row=>row.kindName==='construction');
assert.equal(watches.length,2);assert.equal(particles.length,2);assert.equal(constructions.length,2);
// 이진 단정도 수명을 손실 없이 해석한다.
const f32=bits=>{const v=new DataView(new ArrayBuffer(4));v.setUint32(0,bits,true);return v.getFloat32(0,true);};
// 원본 SHP 헤더는 RLE 외곽39×41과 다르며 실제 전체 캡처49×55를 제공한다.
const assets=realCaptureAssets(),metrics=assets.database.squidMetrics(2,0),palette=GamePalette.parse(readData('d/GIFCLOUD.COL')).logical;
assert.deepEqual([metrics.width,metrics.height,metrics.hotspotX,metrics.hotspotY],[49,55,47,59]);
// Native 원문 수명과 색수는 한 칸 뒤DWORD를 실제로 선택한다.
for(let i=0;i<watches.length;i++){
 const w=watches[i],p=particles[i],c=constructions[i];
 assert.equal(w.column+w.originX,947);assert.equal(w.row+w.originY,689);assert.equal(w.row*c.width+w.column,w.linearPixel);
 assert.deepEqual([w.linearPixel,w.output,c.output832,c.paletteCount,c.readableDwords,c.adjacentDword],[1978,0,255,11,12,0]);
 assert.deepEqual([c.width,c.height,c.hotspotX,c.hotspotY],[49,55,47,65]);
 assert.deepEqual([p.particleIndex,p.active,p.originalColor,p.lifeBits,p.maximumBits,p.colorIndex,p.destinationPixel,p.selectedDword],[285,0,23,1074126988,1073741824,11,1978,0]);
 assert.ok(f32(p.lifeBits)>f32(p.maximumBits));
}
// 실제 프레임의 저장 전후가 끊김 없이 이어지고 마지막 작성자가 마나 볼트다.
const writes=records.filter(row=>row.kindName==='write');
assert.deepEqual(writes.map(row=>[row.drawKind,row.token,row.operation,row.before,row.after,row.sourceIndexOrConstructionPixel]),[
 [0,1223800,'indexed-source',240,185,185],[0,782550,'translated',185,232,23],[3,86,'construction-output',232,0,1978],[0,64600,'translated',0,0,93],
]);
// 배경→섬→건설 중 부모→건설 출력→마나 볼트의 실제 연속 저장을 검증한다.
for(let i=1;i<writes.length;i++)assert.equal(writes[i].before,writes[i-1].after);
assert.equal(writes.at(-1).token/50,1292);
// 같은 현재 GIFCLOUD 색표와 SHP로 인접0/1이 서로 다른 최종RGB가 되는지 확인한다.
const shadow=buildSpriteColorMaps(palette).shadow,colors=buildLightningPalette(palette),cases=[];
assert.equal(colors.length,11);assert.equal(OBSERVED_ADJACENT_DWORD,1);assert.deepEqual([shadow[0],shadow[1],shadow[11]],[0,11,11]);
// 명시 외부 메모리 입력만 달리한다. 일반 색11개나 제품 기본값은 수정하지 않는다.
for(const adjacent of [0,1]){
 const memory=new ParticlePaletteMemory(adjacent);memory.assign(colors);
 const frame=new RawConstructionFrame(null,null,null,{count:memory.count,entries:memory.snapshot()},null,{maxHitPoints(){},invalidate(){},draw(){},kill(){}});
 const input=frame.color(f32(particles[0].lifeBits),f32(particles[0].maximumBits));assert.equal(input,adjacent);
 const pixels=new Uint8Array(1024*768);pixels[689*1024+947]=input;
 drawTranslatedVfxImage(pixels,1024,768,assets.database.decode(35,1),assets.database.frame(35,1),{x:954,y:703},0x10000,0x10000,{left:947,top:689,right:948,bottom:690},shadow);
 const index=pixels[689*1024+947],rgb=[palette[index]&255,(palette[index]>>>8)&255,(palette[index]>>>16)&255];cases.push({adjacent,input,index,rgb});
 assert.deepEqual(rgb,adjacent===0?[0,0,0]:[45,25,19]);
}
// 보존 화면의 실제 차이점도 같은 두 RGB인지 확인한다.
const pngs=[path.join(ROOT,'webpj/tests/fixtures/cpppj/product-1370-ai/campaign1-1-ai-l3/ai8000.png'),path.join(ROOT,'docs/webpj-e1373-evidence/ai8000.web.png')];
// 두 관측 PNG를 각 메모리 입력의 결과와 비교한다.
for(let i=0;i<pngs.length;i++){
 const image=readPng(fs.readFileSync(pngs[i])),at=(689*image.width+947)*3;assert.deepEqual(Array.from(image.rgb.subarray(at,at+3)),cases[i].rgb);
}
console.log(JSON.stringify({frame:8000,screen:[947,689],capturePixel:1978,particleLife:f32(particles[0].lifeBits),maximum:2,paletteCount:11,actualAdjacent:0,writes:writes.map(row=>[row.before,row.after]),cases,productChanged:false},null,2));
