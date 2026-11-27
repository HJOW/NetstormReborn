// 실제 팔레트/표시기를 사용해 부분 변경 밖 픽셀과 팔레트·크기 변경의 전체 복원을 대조한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import {IndexedPresenter} from '../../www/render/presenter.js';
import {IndexedSurface} from '../../www/render/indexed-surface.js';
import {Netstorm} from '../../www/netstorm.js';
import {Screen,ScreenMode} from '../../www/render/screen.js';
import {campaignOwners} from '../../tests/campaign-session-support.mjs';
// Canvas 호출 인자와 ImageData만 빌리는 외부 장치다. 색 변환/영역 선택은 실제 표시기가 한다.
function scene(width=8,height=6,scale=1,mode='sharp'){
 const uploads=[],draws=[],context={setTransform(){},drawImage(...args){draws.push(args);}},doc={createElement(){const canvas={width:0,height:0,getContext(){return{createImageData(w,h){return{data:new Uint8ClampedArray(w*h*4)};},putImageData(...args){uploads.push(args);},drawImage(){}};}};return canvas;}};
 const presenter=new IndexedPresenter(doc,mode),surface=new IndexedSurface(width,height),layout={canvasWidth:width*scale,canvasHeight:height*scale},rgb=new Uint8Array(768);
 for(let i=0;i<256;i++)rgb[i*3]=i;presenter.setPalette(rgb);surface.fill(3);presenter.present(context,surface,layout,true);uploads.length=0;draws.length=0;return{presenter,surface,layout,context,uploads,draws};
}
// 변경하지 않았다고 보고한 바깥 값은 이전 RGBA를 보존하고 실제 변경 영역만 두 Canvas에 보낸다.
test('부분 표시: 바깥 픽셀 보존·정수2배의 정확한 원본/대상 사각형',()=>{
 const s=scene(8,6,2);s.surface.fill(91);const rect={left:2,top:1,right:6,bottom:4};s.presenter.present(s.context,s.surface,s.layout,true,rect);
 for(let y=0;y<6;y++)for(let x=0;x<8;x++)assert.equal(s.presenter.imageWords[y*8+x],s.presenter.lookup[x>=2&&x<6&&y>=1&&y<4?91:3]);
 assert.deepEqual(s.uploads.map(x=>x.slice(1)),[[0,0,2,1,4,3]]);assert.deepEqual(s.draws[0].slice(1),[2,1,4,3,4,2,8,6]);
});
// 팔레트와 새 크기는 이전 캐시를 전체 복원하고, 비정수 최근접 배율은 전체 표시의 샘플링을 유지한다.
test('부분 표시: 팔레트/크기 변경 전체 복원과 비정수 배율 표시',()=>{
 const s=scene(8,6,1.5,'nearest');s.surface.fill(91);s.presenter.present(s.context,s.surface,s.layout,true,{left:2,top:1,right:6,bottom:4});assert.equal(s.uploads[0].length,7);assert.deepEqual(s.draws[0].slice(1),[0,0,12,9]);
 const rgb=new Uint8Array(768);rgb[91*3+1]=225;s.presenter.setPalette(rgb);s.uploads.length=0;s.draws.length=0;s.presenter.present(s.context,s.surface,s.layout,true,{left:2,top:1,right:6,bottom:4});assert.equal(s.uploads[0].length,3);assert.deepEqual(Array.from(s.presenter.imageWords),new Array(48).fill(s.presenter.lookup[91]));assert.equal(s.draws[0].length,5);
 const larger=new IndexedSurface(10,8);larger.fill(91);s.uploads.length=0;s.presenter.present(s.context,larger,{canvasWidth:10,canvasHeight:8},true,{left:2,top:1,right:3,bottom:2});assert.equal(s.uploads[0].length,3);assert.equal(s.presenter.imageWords.length,80);assert.ok(s.presenter.imageWords.every(x=>x===s.presenter.lookup[91]));
});
// 실제 Screen의 행 간격과 변경 경계가 표시기까지 그대로 전달돼야 한다.
test('Client 부분 표시: DIB 패딩 제외 복사와 같은 변경 사각형 전달',()=>{
 const app=new Netstorm(),surface=new IndexedSurface(7,4),calls=[],rect={left:1,top:1,right:4,bottom:3},pixels=new Uint8Array(32);surface.fill(5);pixels.fill(71);app._surface=surface;app._layout={logicalWidth:7,logicalHeight:4,visible:true};app._ctx={};app._presenter={present(...args){calls.push(args);}};app._presentClientScreen(rect,{width:7,height:4,pitch:8,pixels});
 for(let y=0;y<4;y++)for(let x=0;x<7;x++)assert.equal(surface.pixels[y*7+x],x>=1&&x<4&&y>=1&&y<3?71:5);assert.equal(calls.length,1);assert.equal(calls[0][4],rect);assert.equal(calls[0][3],true);
});
// 여러 Screen 복사는 같은 동기 프레임 끝에 한 번만 전송하고 다음 직접 호출은 즉시 표시한다.
test('Client 프레임 표시: 두 변경 범위 합산·한 전송·배치 관문 반납',()=>{
 const app=new Netstorm(),calls=[],surface=new IndexedSurface(7,4),pixels=new Uint8Array(32);app._surface=surface;app._layout={visible:true,logicalWidth:7,logicalHeight:4,drawScaleX:1,drawScaleY:1};app._ctx={};app._presenter={present(...args){calls.push(args);}};app._mainMenu={dirty:false,dialog:null};app._displayDirty=false;app._clientRenderer={invalidateAll(){throw new Error('불필요한 전체무효화');}};app._campaignDisplayActive=()=>true;app._campaignCameraDevice=()=>({applyCameraFrame(){}});
 app._drawCampaignRendererFrame=()=>{for(const rect of [{left:1,top:1,right:3,bottom:2},{left:4,top:2,right:6,bottom:3}]){app._presentClientScreen(rect,{width:7,height:4,pitch:8,pixels});assert.equal(calls.length,0);}};
 app._render(0);assert.equal(calls.length,1);assert.deepEqual(calls[0][4],{left:1,top:1,right:6,bottom:3});assert.equal(app._clientScreenPresentBatch,false);assert.equal(app._clientScreenPresentBounds,null);assert.equal(app._displayDirty,false);
 app._presentClientScreen({left:0,top:0,right:1,bottom:1},{width:7,height:4,pitch:8,pixels});assert.equal(calls.length,2);
});
// 오류 전에 실행한 Screen 복사는 화면에도 남아야 하고 다음 프레임을 가로막지 않는다.
test('Client 프레임 표시: 렌더 예외의 앞선 부분 출력과 finally 반납',()=>{
 const app=new Netstorm(),calls=[],rect={left:1,top:1,right:3,bottom:2},surface=new IndexedSurface(7,4),pixels=new Uint8Array(32);app._surface=surface;app._layout={visible:true,logicalWidth:7,logicalHeight:4,drawScaleX:1,drawScaleY:1};app._ctx={};app._presenter={present(...args){calls.push(args);}};app._mainMenu={dirty:false,dialog:null};app._displayDirty=false;app._campaignDisplayActive=()=>true;app._drawCampaignRendererFrame=()=>{app._presentClientScreen(rect,{width:7,height:4,pitch:8,pixels});throw new Error('원래렌더실패');};
 assert.throws(()=>app._render(0),/원래렌더실패/);assert.equal(calls.length,1);assert.deepEqual(calls[0][4],rect);assert.equal(app._clientScreenPresentBatch,false);assert.equal(app._clientScreenPresentBounds,null);assert.equal(app._displayDirty,true);
});
// 네이티브 행 복사와 원래 DWORD/단일BYTE 경로의 전체 결과·범위 실패 접두를 같은 입력으로 대조한다.
test('Screen 행 복사: 독립128입력에서 원래 경로의 픽셀/패딩/예외 접두와 일치',()=>{
 // 화면의 같은 초기 픽셀과 잠금/clip을 만든다. Array 입력은 원래 DWORD 경로를 선택한다.
 function run(source,number){const screen=new Screen(13,9);screen.init();screen.initDibSection();screen.setMode(ScreenMode.fallbackWindowed);const bytes=screen.lock();for(let i=0;i<bytes.length;i++)bytes[i]=(i*11+number)&255;screen.setClip(1,1,12,8);let error=null;try{screen.blitBuffer(number%9-3,(number*3)%9-3,source,number%17,{left:number%5-2,top:(number*7)%5-2,right:8+number%8,bottom:4+number%6});}catch(e){error=e.name+':'+e.message;}const output=Array.from(bytes);screen.unlock();return{error,output};}
 // 서로 다른 줄 간격과 잘린/부족한 원본을 모두 보존해 실패 뒤 이미 쓴 바이트도 비교한다.
 for(let number=0;number<128;number++){const source=Uint8Array.from({length:20+number},(_,i)=>(i*17+number)&255);assert.deepEqual(run(source,number),run(Array.from(source),number),String(number));}
});
// 객체 목록은 현재 슬롯을 읽되, 반환한 숫자 스냅샷이 뒤 raw 변경에 따라 바뀌면 안 된다.
test('World 객체 목록: SID순서·상태/TYPE 관문·signed프레임/float·반환 사본 보존',()=>{
 const o=campaignOwners(),pool=o.core.pool,slots=[100,101,102,103,104,105],saved=slots.map(sid=>pool.slot(sid));try{
  // 물리 입력만 준비한다. 유닛 생성/게임 프레임/승패는 실행하지 않는다.
  for(const sid of slots){const raw=pool.physical(sid);raw[10]=70;raw[11]=0;raw[34]=sid-99;const view=new DataView(raw.buffer,raw.byteOffset,raw.length);view.setUint32(36,0xffffffff,true);view.setFloat32(14,-0,true);view.setFloat32(18,Infinity,true);}
  pool.physical(101)[11]=1;pool.physical(102)[10]=69;pool.physical(104)[11]=8;pool.physical(103)[10]=158;pool.physical(105)[10]=187;
  const before=o.world.objects().filter(x=>slots.includes(x.sid));assert.deepEqual(before,[100,103,105].map(sid=>({sid,object:{type:sid===100?70:sid===103?158:187,owner:sid-99,frame:-1,x:-0,y:Infinity}})));
  const raw=pool.physical(100);raw[10]=71;raw[34]=8;new DataView(raw.buffer,raw.byteOffset,raw.length).setFloat32(14,12.5,true);const current=o.world.objects().find(x=>x.sid===100);assert.equal(current.object.type,71);assert.equal(current.object.owner,8);assert.equal(current.object.x,12.5);assert.equal(before[0].object.type,70);assert.equal(before[0].object.owner,1);assert.ok(Object.is(before[0].object.x,-0));
 }finally{for(let i=0;i<slots.length;i++)pool.physical(slots[i]).set(saved[i]);o.dispose();}
});
