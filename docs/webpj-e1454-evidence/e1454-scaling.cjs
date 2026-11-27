// 원래 전체 표시기와 부분 표시기의 실제 Canvas 픽셀을 정수/비정수 확대·팔레트/크기 변경에서 대조한다.
const fs=require('node:fs'),assert=require('node:assert/strict'),{chromium}=require('@playwright/test');
// 별도 게임 플레이 없이 Canvas 표시 모듈만 같은 브라우저 장치에서 실행한다.
async function run(){const browser=await chromium.launch({headless:true});try{const page=await browser.newPage(),errors=[];page.on('pageerror',e=>errors.push(String(e)));await page.route('**/render/presenter.js?candidate=E1454',route=>route.fulfill({status:200,contentType:'text/javascript; charset=utf-8',body:fs.readFileSync('extracted/e1454-perf/presenter.js','utf8')}));await page.goto('http://localhost:9803/debug.html?fixed=25');
 const result=await page.evaluate(async()=>{const {IndexedPresenter:Original}=await import('./render/presenter.js?original=E1454'),{IndexedPresenter:Candidate}=await import('./render/presenter.js?candidate=E1454'),{IndexedSurface}=await import('./render/indexed-surface.js'),rows=[];
  // 각각 독립 Canvas/표시기를 만들되 같은 실제 논리 표면과 팔레트를 준다.
  for(const mode of ['sharp','nearest','smooth'])for(const scale of [0.75,1,1.25,2]){const oldCanvas=document.createElement('canvas'),newCanvas=document.createElement('canvas'),old=new Original(document,mode),candidate=new Candidate(document,mode),oldCtx=oldCanvas.getContext('2d'),newCtx=newCanvas.getContext('2d');let surface=new IndexedSurface(8,8);const rgb=new Uint8Array(768);for(let i=0;i<256;i++)rgb.set([i,255-i,(i*7)&255],i*3);old.setPalette(rgb);candidate.setPalette(rgb);
   // 각 초기/부분 변경/팔레트 교체/크기 교체가 이미 그려진 바깥 픽셀에도 같은 영향을 줘야 한다.
   const steps=[null,{left:0,top:0,right:2,bottom:2},{left:3,top:4,right:8,bottom:7},{left:-2,top:-2,right:3,bottom:3},{left:1,top:1,right:1,bottom:3},{left:3,top:2,right:5,bottom:4}];
   for(let step=0;step<steps.length;step++){if(step===5){surface=new IndexedSurface(12,8);rgb[25]=73;old.setPalette(rgb);candidate.setPalette(rgb);}const rect=steps[step],layout={canvasWidth:surface.width*scale,canvasHeight:surface.height*scale};if(step===0||step===5){oldCanvas.width=newCanvas.width=layout.canvasWidth;oldCanvas.height=newCanvas.height=layout.canvasHeight;}
    // 실제로 바뀐 사각형 안의 색 번호만 쓴다. 빈 변경은 원래 픽셀을 유지한다.
    for(let y=0;y<surface.height;y++)for(let x=0;x<surface.width;x++)if(!rect||(x>=rect.left&&x<rect.right&&y>=rect.top&&y<rect.bottom))surface.pixels[y*surface.width+x]=(x*11+y*7+step*31)&255;
    old.present(oldCtx,surface,layout,true);candidate.present(newCtx,surface,layout,true,rect);const a=oldCtx.getImageData(0,0,oldCanvas.width,oldCanvas.height).data,b=newCtx.getImageData(0,0,newCanvas.width,newCanvas.height).data;let differences=0;for(let i=0;i<a.length;i+=4)if(a[i]!==b[i]||a[i+1]!==b[i+1]||a[i+2]!==b[i+2]||a[i+3]!==b[i+3])differences++;rows.push({mode,scale,step,width:oldCanvas.width,height:oldCanvas.height,differences});
   }
  }return rows;});fs.writeFileSync('extracted/e1454-scaling.json',JSON.stringify({rows:result,errors},null,2));assert.deepEqual(errors,[]);assert.equal(result.length,72);assert.ok(result.every(x=>x.differences===0));console.log('PASS: 실제Chromium Canvas 3확대방식×4배율×6변경=72시점 모두0픽셀');
 }finally{await browser.close();}}
run().catch(e=>{console.error(e.stack??e);process.exitCode=1;});
