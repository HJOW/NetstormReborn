// 기본자금5000/지도씨앗1/아군AI와적4명모두Hard의실제다리정체비교자료다. 목표/HP/SP/승패상태를쓰지않는다.
// 실제 아군 AI의 스커미시 적 전멸/승리 안내와 나가기/재적재를 관찰한다. 캠페인은 실행하지 않는다.
const fs=require('node:fs'),assert=require('node:assert/strict'),{chromium}=require('@playwright/test');
// 현재 사용자 허용 범위의 공용 아군 AI 활성화기다.
const {installAllyAi}=require('../e2e/ally-ai-test-control');
// 원래 생성 대본에서 로컬 시작 자금만20000으로 지정하며 적은 기본5000을 유지한다.
const LIMIT=26000,SEED=1,BASE='extracted/e1437-web-same-plan-long';
// C++계획생성기의실제출력과동일한초기fort/영역/중앙섬을외부입력으로사용한다.
const CPP_PLAN=JSON.parse(fs.readFileSync('extracted/e1437-native-plan.json','utf8'));
// 실제 화면의 원래 Tell/메뉴 단추를 누른다.
async function click(page,label){
 const p=await page.evaluate(text=>{const app=window.netstorm,p=app._mainMenu.controlPoint(text),r=app.canvas.getBoundingClientRect();if(!p)throw new Error('실제 단추 없음:'+text);return{x:r.left+p.x*r.width/app._layout.logicalWidth,y:r.top+p.y*r.height/app._layout.logicalHeight};},label);
 await page.mouse.click(p.x,p.y);await page.evaluate(()=>window.netstorm.stepFixedFrames(2));
}
// 원래 타이틀을 모두 실행하고 메인 메뉴에 들어간다.
async function ready(page){
 await page.waitForFunction(()=>window.netstorm?.state==='clickToStart',null,{timeout:120000});await page.locator('canvas').click();await page.evaluate(()=>window.netstorm._prepareCampaignAudio());
 while(await page.evaluate(()=>window.netstorm.state!=='mainMenu'))await page.evaluate(()=>window.netstorm.stepFixedFrames(1));
 await page.evaluate(()=>window.netstorm.stepFixedFrames(1));
}
// 실제 사제/팀 판정과 희생의 반환/효과는 보존하고 그 호출에서 읽은 값만 저장한다.
async function observe(page){
 await page.evaluate(async()=>{
  const {RawMissionOutcome}=await import('./core/raw-mission-outcome.js'),{RawDaisRegular}=await import('./core/raw-dais-regular.js');
  window.skirmishObservation={events:[],dead:{},captured:{},teams:{}};
  // 실제 프레임이 조회한 사제 상태의 전이만 보존한다.
  for(const [method,key] of [['priestDead','dead'],['priestCaptured','captured']]){
   const original=RawMissionOutcome.prototype[method];RawMissionOutcome.prototype[method]=function(owner){const result=original.call(this,owner),o=window.skirmishObservation;if(o[key][owner]!==result){o[key][owner]=result;o.events.push({method,owner,result,time:window.netstorm._gameClock.gameSeconds()});}return result;};
  }
  const event=RawMissionOutcome.prototype.event;
  RawMissionOutcome.prototype.event=function(...args){const result=event.apply(this,args),[state,key,condition]=args;if(key==='goodTeamDead'||key==='badTeamDead'){const o=window.skirmishObservation,entry={condition:Boolean(condition),latched:state[key],time:window.netstorm._gameClock.gameSeconds()};if(o.teams[key]?.condition!==entry.condition)o.events.push({method:'teamOutcome',key,...entry});o.teams[key]=entry;}return result;};
  const finish=RawDaisRegular.prototype.finishSacrifice;
  RawDaisRegular.prototype.finishSacrifice=function(...args){window.skirmishObservation.events.push({method:'finishSacrifice',args,time:window.netstorm._gameClock.gameSeconds()});return finish.apply(this,args);};
 });
}
// 최종 Canvas 제출만 복원해 같은 프레임의 화면을 저장한다. 게임 프레임은 추가하지 않는다.
async function screen(page,file){
 await page.evaluate(()=>{const app=window.netstorm;if(window.skirmishOriginalPresent)app._presenter.present=window.skirmishOriginalPresent;app._presenter.present(app._ctx,app._surface,app._layout,true);});await page.screenshot({path:file});
}
// 오류/승패/상한 어느 경우든 단언 전에 원문을 남긴다.
async function preserve(page,result){
 try{fs.writeFileSync(BASE+'-terminal.tsv',await page.evaluate(()=>window.netstorm.sessionDump()));}catch(error){result.dumpError=String(error);}
 fs.writeFileSync(BASE+'.json',JSON.stringify(result,null,2));await screen(page,BASE+'-terminal.png');
}
// 생성 입력을 고정한 실제 단일 적 스커미시를 진행하고 승리 이후 원래 UI로만 나간다.
async function run(){
 const browser=await chromium.launch({headless:true});
 try{
  const page=await browser.newPage({viewport:{width:1024,height:768},locale:'en-US'}),errors=[];
  page.on('pageerror',error=>errors.push(error.stack??error.message));page.on('console',message=>{if(message.type()==='error')errors.push(message.text());});
  await page.addInitScript(()=>localStorage.setItem('netstorm.settings',JSON.stringify({version:1,values:{welcomeGiven:1,tellTips:0,autoDemo:0,sound:0,music:0,difficulty:3,skirmishAiCount:4,skirmishAi2Diff:3,skirmishAi2Ally:0,skirmishAi3Diff:3,skirmishAi3Ally:0,skirmishAi4Diff:3,skirmishAi4Ally:0,skirmishAi5Diff:3,skirmishAi5Ally:0}})));
  // 원래 생성 경계 직전에 C++ 오라클과 같은 외부 초기 난수 입력만 지정한다.
  const original=fs.readFileSync('www/core/raw-campaign-session-skirmish.js','utf8'),point='        if (!plan) return;';assert.equal(original.split(point).length,2);
  const input=original.replace(point,point+'\n        o.random.setState(0x12345678); o.randomGlobals.crt.setState(0x23456789); window.skirmishInput={game:o.random.state,crt:o.randomGlobals.crt.state};');
  await page.route('**/core/raw-campaign-session-skirmish.js',route=>route.fulfill({status:200,contentType:'application/javascript',body:input}));
  await page.goto('http://localhost:9803/debug.html?fixed=25&aspect=4:3&title=1&lang=en');await ready(page);
  const beforeProgress=await page.evaluate(()=>localStorage.getItem('netstorm.progress'));await installAllyAi(page,'skirmish',{preserveDifficulty:true});await observe(page);
  const launch=await page.evaluate(async ({seed,plan})=>{const app=window.netstorm,{buildSkirmishLaunch}=await import('./core/skirmish-setup.js'),settings=app._loadSkirmishSettings(),launch=buildSkirmishLaunch(settings,seed);launch.islands=plan.islands;launch.central=new Uint8Array(plan.central);launch.mapSize=plan.mapSize;if((launch.script.match(/myStartMoney=5000/gu)??[]).length!==1)throw new Error('기본 로컬 시작 자금 키 필요');await app._beginSkirmish(launch);return{settings,islands:launch.islands,central:Array.from(launch.central),mapSize:launch.mapSize,script:launch.script};},{seed:SEED,plan:CPP_PLAN});
  // 실제 비동기 적재와 첫 안내를 원래 프레임으로 기다린다.
  for(let frame=0;frame<1500;frame++){if(await page.evaluate(()=>Boolean(window.netstorm._mainMenu.controlPoint('Go!'))&&Boolean(window.netstorm._campaignWorld?.session().localConnectionReady())))break;await page.evaluate(()=>window.netstorm.stepFixedFrames(1));}
  launch.worldInput=await page.evaluate(()=>window.skirmishInput);assert.deepEqual(launch.worldInput,{game:0x12345678,crt:0x23456789});await click(page,'Go!');const activation=await page.evaluate(()=>window.netstormAllyTestState);assert.equal(activation.money,5000);for(const key of ['money','allies','name','technology'])assert.deepEqual(activation[key],activation.before[key]);
  const result={limit:LIMIT,seed:SEED,controller:'실제아군Hard/적4명Hard',localStartSp:5000,enemyStartSp:5000,beforeProgress,launch,activation,errors};fs.writeFileSync(BASE+'-initial.json',JSON.stringify(result,null,2));
  await page.evaluate(()=>{window.skirmishOriginalPresent=window.netstorm._presenter.present;window.netstorm._presenter.present=()=>{};});
  let executed=0,current=null;
  // 내부 프레임을 생략하지 않고100프레임마다 실제 팀 판정/사제 상태를 읽는다.
  for(let frame=0;frame<=LIMIT;frame+=100){
   if(frame)await page.evaluate(()=>window.netstorm.stepFixedFrames(100));executed=frame;
   current=await page.evaluate(()=>{const app=window.netstorm;return{time:app._gameClock.gameSeconds(),buttons:app._mainMenu.dialog?.buttons?.map(button=>button.label)??[],commands:app._mainMenu.dialog?.buttons?.map(button=>({label:button.label,command:button.command}))??[],sp:app._campaignWorld.session().sp(1),observation:window.skirmishObservation};});
   if(frame%2000===0){const dump=await page.evaluate(()=>window.netstorm.sessionDump());fs.writeFileSync(BASE+'-frame-'+frame+'.tsv',dump);console.log(JSON.stringify({frame,...current,errors}));}
   if(errors.length||current.buttons.length)break;
  }
  result.terminal={frame:executed,...current,won:current.observation.teams.badTeamDead?.condition===true&&current.observation.teams.goodTeamDead?.condition===false&&current.buttons.includes('Leave')};await preserve(page,result);assert.deepEqual(errors,[]);console.log(JSON.stringify({result:'아군AI/적4명Hard 관찰 완료;승리여부는실제team판정값만기록',terminal:result.terminal}));
 }finally{await browser.close();}
}
run().catch(error=>{console.error(error.stack??error);process.exitCode=1;});
