// WB04-E1373의 보존된 Native/Web 화면 차이를 실제 자산의 겹침 색·좌표로 독립 검증한다.
// 사용: node webpj/tools/verify-ai-overlap-observation.mjs [보존된 웹 PNG 폴더]. 제품 표시 검사의 차이 허용값이 아니다.
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
// 실행 위치와 무관한 저장소 루트와 검사할 실제 관측 PNG 폴더다.
const PROJECT_ROOT=fileURLToPath(new URL('../../',import.meta.url));
const OBSERVATION_DIR=process.argv[2]?path.resolve(process.argv[2]):path.join(PROJECT_ROOT,'docs/webpj-e1373-evidence');
import { realCaptureAssets } from '../tests/capture-assets-support.mjs';
import { buildStandardOwnerColorMaps } from '../www/assets/owner-colors.js';
import { GamePalette } from '../www/assets/palette.js';
import { readData } from '../tests/helpers.mjs';
import { readPng } from './image-files.mjs';
// 실제 TYPE155의 셰이프85, 소유자1 치환표와 원본 논리 팔레트다.
const assets=realCaptureAssets(),owner=buildStandardOwnerColorMaps(155,82)[1],palette=GamePalette.parse(readData('d/GIFCLOUD.COL')).logical;
// 각 시점·위치에서 Native와 웹의 최상위 실제 프레임 번호다.
const cases=[{frame:7000,pairs:[[1008,494,0,3]]},{frame:8000,pairs:[]},{frame:12000,pairs:[[384,307,1,2],[1008,494,0,3],[816,769,1,0]]}];
// 팔레트 RGBX DWORD의 실제 RGB를 읽는다.
const rgb=index=>[palette[index]&255,(palette[index]>>>8)&255,(palette[index]>>>16)&255];
// PNG의 세 색 채널을 작은 값 사본으로 읽는다.
const at=(image,x,y)=>Array.from(image.rgb.subarray((y*image.width+x)*3,(y*image.width+x+1)*3));
const summary=[];
// 단계별 전체 화면 차이를 구한 뒤 실제 프레임의 공통 불투명 색과 대조한다.
for(const item of cases){
 const native=readPng(fs.readFileSync(path.join(PROJECT_ROOT,'webpj/tests/fixtures/cpppj/product-1370-ai/campaign1-1-ai-l3/ai'+item.frame+'.png')));
 const web=readPng(fs.readFileSync(path.join(OBSERVATION_DIR,'ai'+item.frame+'.web.png')));
 assert.equal(web.width,native.width);assert.equal(web.height,native.height);
 const differences=new Set(),modeled=new Set(),details=[];
 // 전체 화면의 실제 RGB 차이를 독립적으로 센다.
 for(let i=0;i<native.width*native.height;i++)if(native.rgb[i*3]!==web.rgb[i*3]||native.rgb[i*3+1]!==web.rgb[i*3+1]||native.rgb[i*3+2]!==web.rgb[i*3+2])differences.add(i);
 // 각 겹침 쌍의 공통 불투명 영역에서 예상되는 최상위 색만 계산한다.
 for(const [x,y,nf,wf] of item.pairs){
  const nr=assets.database.frame(85,nf).rect,wr=assets.database.frame(85,wf).rect,nd=assets.database.decode(85,nf),wd=assets.database.decode(85,wf);
  let count=0,nativeMatches=0,webMatches=0;
  // 실제 VFX 사각형은 양 끝 포함이며 화면 밖 픽셀은 비교에서 제외한다.
  for(let v=Math.max(nr.top,wr.top);v<=Math.min(nr.bottom,wr.bottom);v++)for(let u=Math.max(nr.left,wr.left);u<=Math.min(nr.right,wr.right);u++){
   const px=x+u,py=y+v,ni=(v-nr.top)*nd.width+u-nr.left,wi=(v-wr.top)*wd.width+u-wr.left;
   if(px<0||py<0||px>=native.width||py>=native.height||!nd.opacity[ni]||!wd.opacity[wi])continue;
   const nc=rgb(owner[nd.indices[ni]]),wc=rgb(owner[wd.indices[wi]]);
   if(JSON.stringify(nc)===JSON.stringify(wc))continue;
   count++;modeled.add(py*native.width+px);
   if(JSON.stringify(at(native,px,py))===JSON.stringify(nc))nativeMatches++;
   if(JSON.stringify(at(web,px,py))===JSON.stringify(wc))webMatches++;
  }
  details.push({x,y,nativeFrame:nf,webFrame:wf,pixels:count,nativeMatches,webMatches});
 }
 const unexplained=[...differences].filter(i=>!modeled.has(i)).map(i=>({x:i%native.width,y:Math.floor(i/native.width),native:at(native,i%native.width,Math.floor(i/native.width)),web:at(web,i%web.width,Math.floor(i/web.width))}));
 summary.push({frame:item.frame,actual:differences.size,modeled:modeled.size,sameCoordinateSet:differences.size===modeled.size&&[...differences].every(i=>modeled.has(i)),pairs:details,unexplained});
}
// 보존된 관측 자체가 달라지면 성공으로 출력하지 않는다. 제품의 새 화면을 이 값으로 허용하지 않는다.
for(const observation of summary){
 assert.equal(observation.actual,observation.frame===7000?31:observation.frame===8000?1:99);
 if(observation.frame!==8000){
  assert.equal(observation.sameCoordinateSet,true);
  // 각 쌍은 실제 Native와 웹 PNG에서 계산한 최상위 색까지 모두 일치해야 한다.
  for(const pair of observation.pairs){assert.equal(pair.nativeMatches,pair.pixels);assert.equal(pair.webMatches,pair.pixels);}
 }else assert.deepEqual(observation.unexplained,[{x:947,y:689,native:[0,0,0],web:[45,25,19]}]);
}
console.log(JSON.stringify(summary,null,2));
