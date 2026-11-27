// 같은 지도/고정 시각의 실제 스커미시에서 난이도별 예약과 채취 목표/실제 수를 읽는다. 아군 조작은 실제 AI만 사용한다.
const fs = require('node:fs'), assert = require('node:assert/strict'), { chromium } = require('@playwright/test');
// 난이도는 원래 메뉴/스커미시 설정을 보존하는 공용 아군 AI 활성화기로 유지한다.
const { installAllyAi } = require('../e2e/ally-ai-test-control');
// 각 난이도의 최대 실제 프레임과 지형 씨앗이다. 생성 대본의 양측 시작 자금만12000으로 지정하며 플레이 중 SP/HP/승패를 쓰지 않는다.
const LIMIT = 8000, SEED = 1;
// 실제 단추 좌표를 논리 화면에서 브라우저 마우스로 누른다.
async function click(page,label) {
 const point=await page.evaluate(text=>{const app=window.netstorm,p=app._mainMenu.controlPoint(text),rect=app.canvas.getBoundingClientRect();if(!p)throw new Error('실제 단추 없음:'+text);return {x:rect.left+p.x*rect.width/app._layout.logicalWidth,y:rect.top+p.y*rect.height/app._layout.logicalHeight};},label);
 await page.mouse.click(point.x,point.y);await page.evaluate(()=>window.netstorm.stepFixedFrames(2));
}
// 실제 원래 생산/다리/수집자 몸체를 감싸 전후 값을 읽고 반환/예외/게임 효과를 그대로 전달한다.
async function observe(page) {
 await page.evaluate(async()=>{
  const [{RawAiDeployment},{RegularProcess},{RawAiBridgePath},{RawAiStrategySetup},{RawAiCollectors}]=await Promise.all([import('./core/raw-ai-deployment.js'),import('./core/squid-process.js'),import('./core/raw-ai-bridge-path.js'),import('./core/raw-ai-strategy-setup.js'),import('./core/raw-ai-collectors.js')]);
  // 브라우저 관측 저장소와 동기 생산 호출의 현재 AI 문맥이다.
  window.aiTelemetry={production:[],bridges:[],collectorMoves:[],samples:[]};let deployment=null;
  // 현재 토큰의 실제 공유 본체와 게임 시각을 읽는다.
  const state=(token)=>{const app=window.netstorm,session=app._campaignWorld.session(),raw=session.aiMemory().bytes(token),view=new DataView(raw.buffer,raw.byteOffset,raw.byteLength);return {view,time:app._gameClock.gameSeconds(),owner:view.getUint32(0x260,true)};};
  const place=RawAiDeployment.prototype.place,initialize=RegularProcess.prototype.initializeDetached;
  RawAiDeployment.prototype.place=function(token,...args){const previous=deployment;deployment={...state(token),type:args[0]};try{return place.call(this,token,...args);}finally{deployment=previous;}};
  RegularProcess.prototype.initializeDetached=function(...args){const result=initialize.apply(this,args);if(deployment?.owner===2)window.aiTelemetry.production.push({time:deployment.time,duration:args[1],type:deployment.type,wallet:deployment.view.getInt32(0x2d8,true),desired:deployment.view.getUint32(0x234,true),collectors:deployment.view.getUint32(0x2e4,true)});return result;};
  // 다리 재고 추가/실제 소비 뒤 원래 QWORD 예약 기한과 현재 시각의 차이를 읽는다.
  for(const [prototype,method] of [[RawAiBridgePath.prototype,'consume'],[RawAiStrategySetup.prototype,'seedBridges']]){
   const original=prototype[method];prototype[method]=function(token,...args){const before=state(token),deadline=before.view.getFloat64(0x630,true),result=original.call(this,token,...args),after=state(token),next=after.view.getFloat64(0x630,true);if(after.owner===2&&next!==deadline&&next>after.time)window.aiTelemetry.bridges.push({method,time:after.time,next,duration:next-after.time});return result;};
  }
  // 실제 채취 유닛 배치에서 원래 이동 제어 기한의 변화만 관측한다.
  const home=RawAiCollectors.prototype.placeInHome;
  RawAiCollectors.prototype.placeInHome=function(token,...args){const before=state(token),deadline=before.view.getFloat64(0x5a8,true),result=home.call(this,token,...args),after=state(token),next=after.view.getFloat64(0x5a8,true);if(after.owner===2&&next!==deadline&&next>after.time)window.aiTelemetry.collectorMoves.push({time:after.time,next,duration:next-after.time});return result;};
 });
}
// 같은 씨앗과 원래 설정의 세 실제 게임을 순차 실행하고 원문을 각 경계에 보존한다.
async function run() {
 const browser=await chromium.launch({headless:true}),results=[];
 try {
  // 원래 난이도 값1/2/3만 바꾸며 아군은 동일한 보통 설정을 사용한다.
  for(const level of [1,2,3]){
   const context=await browser.newContext({viewport:{width:1024,height:768},locale:'en-US'}),page=await context.newPage(),errors=[];
   page.on('pageerror',error=>errors.push(error.stack??error.message));page.on('console',message=>{if(message.type()==='error')errors.push(message.text());});
   await page.addInitScript(value=>localStorage.setItem('netstorm.settings',JSON.stringify({version:1,values:{welcomeGiven:1,tellTips:0,autoDemo:0,sound:0,music:0,difficulty:2,skirmishAiCount:1,skirmishAi2Diff:value,skirmishAi2Ally:0}})),level);
   await page.goto('http://localhost:9803/debug.html?fixed=25&aspect=4:3&title=1&lang=en');await page.waitForFunction(()=>window.netstorm?.state==='clickToStart',null,{timeout:120000});await page.locator('canvas').click();await page.evaluate(()=>window.netstorm._prepareCampaignAudio());
   while(await page.evaluate(()=>window.netstorm.state!=='mainMenu'))await page.evaluate(()=>window.netstorm.stepFixedFrames(1));
   await page.evaluate(()=>window.netstorm.stepFixedFrames(1));await installAllyAi(page,'skirmish',{preserveDifficulty:true});await observe(page);
   // 지형 씨앗과 자금 충분 조건(양측12000)을 생성 대본 입력으로 지정하고 원래 스커미시 적재를 사용한다.
   const launch=await page.evaluate(async seed=>{const {buildSkirmishLaunch}=await import('./core/skirmish-setup.js'),app=window.netstorm,settings=app._loadSkirmishSettings(),launch=buildSkirmishLaunch(settings,seed);if((launch.script.match(/StartMoney=5000/gu)??[]).length!==2)throw new Error('자금 충분 조건의 원래 두 시작 자금 키 필요');launch.script=launch.script.replaceAll('StartMoney=5000','StartMoney=12000');await app._beginSkirmish(launch);return {settings,islands:launch.islands,central:Array.from(launch.central),mapSize:launch.mapSize,script:launch.script};},SEED);
   for(let frame=0;frame<1500;frame++){if(await page.evaluate(()=>Boolean(window.netstorm._mainMenu.dialog)&&Boolean(window.netstorm._campaignWorld?.session().localConnectionReady())))break;await page.evaluate(()=>window.netstorm.stepFixedFrames(1));}
   await click(page,'Go!');
   const activation=await page.evaluate(()=>window.netstormAllyTestState);assert.ok(activation?.brain);assert.equal(activation.money,12000);for(const key of ['money','allies','name','technology'])assert.deepEqual(activation[key],activation.before[key]);
   fs.writeFileSync('extracted/skirmish-ai-telemetry-level-'+level+'-initial.json',JSON.stringify({level,seed:SEED,launch,activation},null,2));
   await page.evaluate(()=>{window.netstorm._presenter.present=()=>{};});
   let frame=0;
   // 전체 프레임을 실행하며100프레임마다 실제 AI 본문과 오류/승패 창을 읽는다.
   for(;frame<=LIMIT;frame+=100){
    if(frame)await page.evaluate(()=>window.netstorm.stepFixedFrames(100));
    const sample=await page.evaluate(()=>{const app=window.netstorm,session=app._campaignWorld.session(),token=session.participation().brain(2),raw=session.aiMemory().bytes(token),view=new DataView(raw.buffer,raw.byteOffset,raw.byteLength);return {time:app._gameClock.gameSeconds(),sp:session.sp(2),wallet:view.getInt32(0x2d8,true),rates:[0x220,0x224,0x228].map(offset=>view.getFloat32(offset,true)),desired:view.getInt32(0x234,true),count:view.getInt32(0x2e4,true),initialized:view.getUint32(0x258,true),scan:view.getUint32(0x25c,true),stop:view.getUint32(0x254,true),enemy:view.getUint32(0x264,true),priest:view.getUint32(0x29c,true),temple:view.getUint32(0x294,true),buttons:app._mainMenu.dialog?.buttons?.map(button=>button.label)??[]};});
    await page.evaluate(row=>window.aiTelemetry.samples.push(row),{frame,...sample});
    if(frame%2000===0||errors.length){console.log(JSON.stringify({level,frame,...sample,errors}));fs.writeFileSync('extracted/skirmish-ai-telemetry-current.json',JSON.stringify({level,seed:SEED,frame,launch,activation,telemetry:await page.evaluate(()=>window.aiTelemetry),errors},null,2));}
    assert.deepEqual(errors,[]);if(sample.buttons.length)break;
   }
   fs.writeFileSync('extracted/skirmish-ai-telemetry-level-'+level+'-dump.tsv',await page.evaluate(()=>window.netstorm.sessionDump()));// 마지막 실제 실행 표본을 기록하며 종료 뒤100이 증가한 반복변수는 프레임 수로 쓰지 않는다.
   const telemetry=await page.evaluate(()=>window.aiTelemetry),result={level,seed:SEED,limit:LIMIT,frame:telemetry.samples.at(-1).frame,launch,activation,telemetry,errors};results.push(result);fs.writeFileSync('extracted/skirmish-ai-telemetry-telemetry.json',JSON.stringify(results,null,2));
   const rate=level===1?4:level===3?.5:1;
   assert.ok(telemetry.production.length>0,'실제 생산 예약 관측 필요');assert.ok(telemetry.bridges.length>0,'실제 다리 예약 관측 필요');assert.ok(telemetry.collectorMoves.length>0,'실제 채취 제어 관측 필요');
   for(const row of telemetry.production)assert.equal(row.duration,rate);for(const row of telemetry.bridges)assert.ok(Math.abs(row.duration-rate)<1e-6);for(const row of telemetry.collectorMoves)assert.ok(Math.abs(row.duration-2*rate)<1e-6);
   assert.ok(telemetry.samples.every(row=>row.desired===(level===1?3:6)));await context.close();
  }
 }finally{await browser.close();}
}
run().catch(error=>{console.error(error.stack??error);process.exitCode=1;});
