const fs=require('node:fs'), {chromium,expect}=require('@playwright/test');
const {installAllyAi}=require('../e2e/ally-ai-test-control');
const {assertAllyAiMission,allyAiCondition}=require('../e2e/ally-ai-scope');
// 실제 다음 미션 연결은 현재 자동 승리 검사 범위와 별도로 보존한다.
const NEXT={thewarbegins:'masterofwhirligigs',masterofwhirligigs:'savetheisland',savetheisland:'fragilefortune',fragilefortune:'thunderingpower',thunderingpower:'dissolvedalliance'};
// 최신 사용자 지시로1-3/1-4도 승리 수집을 허용한다. 다음 미션의 브리핑 연결은 그대로 유지한다.
const PLAYABLE=new Set(Object.keys(NEXT));
// 다음 미션의 진입/첫 안내는 승리 검사와 별개다.1-5 다음1-6도 시작 단추를 누르지 않고 확인할 수 있다.
const BRIEFING=new Set([...Object.keys(NEXT),...Object.values(NEXT)]);
// 실행할 미션과 실제 논리 프레임 상한이다.
const mission=(process.argv[2]??'masterofwhirligigs').toLowerCase(),limit=Number(process.argv[3]??60000);
assertAllyAiMission(mission);
if(!PLAYABLE.has(mission)||!Number.isInteger(limit)||limit<100||limit>120000)throw new Error('아군 AI 캠페인 관찰 인자 오류');
// 선택적 세 번째/네 번째 인자는 메뉴 대기 프레임과 원래 AI 능력 설정의 전략이다.
const condition=allyAiCondition(process.argv[4]??0,process.argv[5]??'default');
// 다른 조건의 증빙은 별도 이름으로 보존하고 기존 기본 조건의 파일 이름도 유지한다.
const OUTPUT_BASE='extracted/ally-flow-'+mission+(condition.menuFrames||condition.profile!=='default'?'-wait'+condition.menuFrames+'-'+condition.profile:'');
// 실제 시작 그림·메뉴의 모든 프레임을 통과한다.
async function ready(page){
 await page.waitForFunction(()=>window.netstorm?.state==='clickToStart',null,{timeout:120000});await page.locator('canvas').click();await page.evaluate(()=>window.netstorm._prepareCampaignAudio());
 while(await page.evaluate(()=>window.netstorm.state!=='mainMenu'))await page.evaluate(()=>window.netstorm.stepFixedFrames(1));
 await page.evaluate(()=>window.netstorm.stepFixedFrames(1));
}
// 실제 UI 단추만 누르고 입력이 소비될 두 프레임을 진행한다.
async function clickLabel(page,label){
 const point=await page.evaluate(text=>{const app=window.netstorm,p=app._mainMenu.controlPoint(text),rect=app.canvas.getBoundingClientRect();if(!p)throw new Error('현재 UI 단추 없음: '+text);return{x:rect.left+p.x*rect.width/app._layout.logicalWidth,y:rect.top+p.y*rect.height/app._layout.logicalHeight};},label);
 await page.mouse.click(point.x,point.y);await page.evaluate(()=>window.netstorm.stepFixedFrames(2));
}
// 다음 미션 요청 수락과 실제 World/브리핑 표시를 구분한다. 고정 시각에서도 비동기 적재와 원래 State 프레임을 모두 진행한다.
async function settledBriefing(page,name){
 if(!BRIEFING.has(name.toLowerCase()))throw new Error('실제 다음 미션 첫 안내 허용 범위 밖: '+name);
 // 읽기 전용 준비 상태를 매 프레임 확인하며 목표·SP·완료값을 쓰지 않는다.
 for(let frame=0;frame<1000;frame++){
  const state=await page.evaluate(expected=>{const app=window.netstorm;return{ready:app._clientGuide.current()?.script.name.toLowerCase()===expected&&Boolean(app._campaignWorld?.session().localConnectionReady())&&app._campaignDisplayActive()&&Boolean(app._mainMenu.dialog),mission:app._clientGuide.current()?.script.name??null,buttons:app._mainMenu.dialog?.buttons?.map(button=>button.label)??[]};},name);
  if(state.ready)return state;
  await page.evaluate(()=>window.netstorm.stepFixedFrames(1));
 }
 throw new Error('다음 미션의 실제 World/브리핑 준비 시간 초과: '+name);
}
// 실제 사제 판정과 희생 종료 호출을 감싸 관측하며 반환·효과·인자는 바꾸지 않는다.
async function observe(page){
 await page.evaluate(async()=>{
  const {RawMissionOutcome}=await import('./core/raw-mission-outcome.js'),{RawDaisRegular}=await import('./core/raw-dais-regular.js');
  window.allyObservation={events:[],captured:{},dead:{}};
  // 현재 값이 바뀔 때만 상태 전이를 기록한다.
  for(const [method,key] of [['priestCaptured','captured'],['priestDead','dead']]){
   const original=RawMissionOutcome.prototype[method];
   RawMissionOutcome.prototype[method]=function(owner){const result=original.call(this,owner),state=window.allyObservation;if(state[key][owner]!==result){state[key][owner]=result;state.events.push({method,owner,result,time:window.netstorm._gameClock.gameSeconds()});}return result;};
  }
  const finish=RawDaisRegular.prototype.finishSacrifice;
  RawDaisRegular.prototype.finishSacrifice=function(...args){window.allyObservation.events.push({method:'finishSacrifice',args,time:window.netstorm._gameClock.gameSeconds()});return finish.apply(this,args);};
 });
}
// 긴실제AI진행중오류가나도단언/브라우저종료전에당시상태·전체오류·덤프·화면을보존한다.
async function preserveFrameFailure(page, result) {
 // 프레임상태는읽기만하고같은결과파일을각경계에남긴다. 덤프실패도원래오류를가리지않는다.
 const base=OUTPUT_BASE+'-failure';
 try{result.dump=await page.evaluate(()=>window.netstorm.sessionDump());}catch(error){result.dumpError=String(error);}
 fs.writeFileSync(base+'.json',JSON.stringify(result,null,2));
 console.error(JSON.stringify({mission:result.mission,frame:result.frame,errorCount:result.errors.length,firstErrors:result.errors.slice(0,3)}));
 try{
  // 생략하던최종Canvas출력만복구하며게임프레임/명령은추가하지않는다.
  await page.evaluate(()=>{const app=window.netstorm;app._presenter.present=window.originalFlowPresent;app._presenter.present(app._ctx,app._surface,app._layout,true);});
  await page.screenshot({path:base+'.png'});
 }catch(error){console.error('실제AI오류화면보존실패',String(error));}
}
// AI 자연 승리·새 완료값 저장·새로고침 뒤 다음 미션 해제를 같은 브라우저 저장소에서 확인한다.
async function run(){
 const browser=await chromium.launch({headless:true});
 try{
  const page=await browser.newPage({viewport:{width:1024,height:768},locale:'en-US'}),errors=[];
  page.on('pageerror',error=>errors.push(error.stack??error.message));page.on('console',message=>{if(message.type()==='error')errors.push(message.text());});
  await page.addInitScript(()=>localStorage.setItem('netstorm.settings',JSON.stringify({version:1,values:{welcomeGiven:1,tellTips:0,autoDemo:0,sound:0,music:0}})));
  await page.goto('http://localhost:9803/debug.html?fixed=25&aspect=4:3&title=1&lang=english');await ready(page);
  // 실제 메뉴 프레임으로만 기다려 원래 난수 소비와 시각 시드를 다르게 한다.
  if(condition.menuFrames)await page.evaluate(frames=>window.netstorm.stepFixedFrames(frames),condition.menuFrames);
  const startingCondition=await page.evaluate(()=>({game:window.netstorm._gameClock.gameSeconds(),unix:window.netstorm._gameClock.unixSeconds(),random:window.netstorm._clientRandom.game.state}));
  const before=await page.evaluate(()=>localStorage.getItem('netstorm.progress'));
  await installAllyAi(page,mission,{profile:condition.profile});await observe(page);await page.evaluate(name=>window.netstorm._beginCampaignFromDialog(name),mission);await page.evaluate(()=>window.netstorm.stepFixedFrames(120));
  // 1-5는 원래 두 장 브리핑이다. 실제 A1. 단추를 눌러 둘째 안내로 이동한다.
  if(mission==='thunderingpower'){const label=await page.evaluate(()=>window.netstorm._mainMenu.dialog?.buttons?.find(button=>button.command?.name==='Tell'&&button.command.argument==='A1.')?.label);if(label)await clickLabel(page,label);}
  await clickLabel(page,'Play Mission');
  const activation=await page.evaluate(()=>window.netstormAllyTestState);
  expect(activation.money).toBe(activation.before.money);expect(activation.allies).toBe(activation.before.allies);expect(activation.name).toEqual(activation.before.name);expect(activation.technology).toEqual(activation.before.technology);
  if(condition.profile==='guard-priest')expect(activation.abilities&0x800,'사제 채취 능력 해제').toBe(0);
  // 실제 시작 시각·난수와 생성 직후 능력 비트를 관찰값으로 남긴다.
  console.log(JSON.stringify({mission,condition,startingCondition,abilities:activation.abilities}));
  // 최종 Canvas 출력만 미루며 Screen·Renderer·Kernel은 모두 매 프레임 실행한다.
  await page.evaluate(()=>{const app=window.netstorm;window.originalFlowPresent=app._presenter.present;app._presenter.present=()=>{};});
  let terminal=null;
  //100프레임을 묶어도 내부 프레임은 생략하지 않는다. 승리/패배는 실제 UI와 사제 상태로만 판정한다.
  for(let frame=0;frame<=limit;frame+=100){
   if(frame)await page.evaluate(()=>window.netstorm.stepFixedFrames(100));
   const current=await page.evaluate(()=>{const app=window.netstorm;return{time:app._gameClock.gameSeconds(),buttons:app._mainMenu.dialog?.buttons?.map(button=>button.label)??[],commands:app._mainMenu.dialog?.buttons?.map(button=>({label:button.label,command:button.command}))??[],observation:window.allyObservation,sp:app._campaignWorld.session().sp(1)};});
   if(frame%2000===0)console.log(JSON.stringify({mission,condition,frame,...current,errors}));
   if(errors.length){await preserveFrameFailure(page,{mission,limit,condition,startingCondition,frame,before,activation,current,errors}).catch(error=>console.error('실제AI오류증빙보존실패',error));throw new Error('실제 게임 프레임 오류: '+errors[0]);}
   // 실제1-5 시간 안내는 항복하지 않는 DoNothing 단추, 승리 뒤 편지는 VictoryNote 단추만 누른다.
   if(mission==='thunderingpower'&&current.buttons.length&&!current.buttons.includes('Next Mission')){const safe=current.commands.findLast(button=>button.command?.name==='DoNothing')??current.commands.find(button=>button.command?.name==='Tell'&&button.command.argument==='VictoryNote');if(safe){await clickLabel(page,safe.label);continue;}}
   if(frame===limit)terminal={frame,...current,won:false,reason:'프레임 상한'};
   if(current.buttons.includes('Next Mission')){terminal={frame,...current,won:true};break;}
   if(current.buttons.length&&current.observation.dead[1]){terminal={frame,...current,won:false};break;}
  }
  // 종료 화면은 현재 표면을 실제 Canvas에 한 번 출력해 보존한다.
  await page.evaluate(()=>{const app=window.netstorm;app._presenter.present=window.originalFlowPresent;app._presenter.present(app._ctx,app._surface,app._layout,true);});
  await page.screenshot({path:OUTPUT_BASE+'-terminal.png'});
  // 마지막 본문과 AI 결정 상태도 관측해 상한 종료를 승리로 오인하지 않는다.
  const diagnostics=await page.evaluate(()=>{const app=window.netstorm,session=app._campaignWorld.session(),ais=[];for(let owner=1;owner<9;owner++){const token=session.participation().brain(owner);if(!token)continue;const raw=session.aiMemory().bytes(token),view=new DataView(raw.buffer,raw.byteOffset,raw.byteLength);ais.push({owner,token,initialized:view.getUint32(0x258,true),scan:view.getUint32(0x25c,true),stop:view.getUint32(0x254,true),enemy:view.getUint32(0x264,true),flags:view.getUint32(0x248,true),wallet:view.getInt32(0x2d8,true),priests:[view.getUint32(0x29c,true),view.getUint32(0x2a0,true)],body:Array.from(raw)});}return{ais,dump:app.sessionDump()};});
  fs.writeFileSync(OUTPUT_BASE+'-terminal.tsv',diagnostics.dump);delete diagnostics.dump;
  const result={mission,limit,condition,startingCondition,before,activation,terminal,diagnostics,errors};
  fs.writeFileSync(OUTPUT_BASE+'.json',JSON.stringify(result,null,2));
  expect(terminal?.won,'실제 AI 자연 승리').toBe(true);
  const saved=await page.evaluate(()=>localStorage.getItem('netstorm.progress'));
  const key='done'+mission,lookup=text=>Object.entries(JSON.parse(text??'{}').values??{}).find(([name])=>name.toLowerCase()===key)?.[1];
  expect(lookup(before)).not.toBe('1');expect(String(lookup(saved))).toBe('1');result.saved=saved;
  // 긴 실제 플레이 뒤의 후속 검사 실패에도 이미 확인한 저장/이동 증빙을 잃지 않게 각 경계에서 보존한다.
  const preserve=()=>fs.writeFileSync(OUTPUT_BASE+'.json',JSON.stringify(result,null,2));preserve();
  // 실제 완료 단추가 다음 미션 브리핑으로 이어지는지 확인한다. 목표 수행 입력은 넣지 않는다.
  await clickLabel(page,'Next Mission');result.directNextBriefing=await settledBriefing(page,NEXT[mission]);result.directNext=result.directNextBriefing.mission;preserve();
  await page.screenshot({path:OUTPUT_BASE+'-direct-next.png'});
  await page.reload({waitUntil:'domcontentloaded',timeout:120000});await ready(page);const reloaded=await page.evaluate(()=>localStorage.getItem('netstorm.progress'));expect(String(lookup(reloaded))).toBe('1');result.reloaded=reloaded;preserve();
  await clickLabel(page,'Campaign');await clickLabel(page,'Struggle For Freedom');
  const label=await page.evaluate(name=>window.netstorm._mainMenu.dialog.menu.items.find(item=>item.command?.name==='MissionBegin'&&item.command.argument.toLowerCase()===name)?.label,NEXT[mission]);
  expect(label).toBeTruthy();await clickLabel(page,label);result.menuNextBriefing=await settledBriefing(page,NEXT[mission]);
  result.menuNext=result.menuNextBriefing.mission;expect(errors).toEqual([]);
  await page.screenshot({path:OUTPUT_BASE+'-unlocked-next.png'});fs.writeFileSync(OUTPUT_BASE+'.json',JSON.stringify(result,null,2));console.log(JSON.stringify({mission,result:'자연 승리·새 완료값 저장·다음 미션·새로고침 뒤 해제 확인',terminal:terminal.frame}));
 }finally{await browser.close();}
}
run().catch(error=>{console.error(error.stack??error);process.exitCode=1;});
