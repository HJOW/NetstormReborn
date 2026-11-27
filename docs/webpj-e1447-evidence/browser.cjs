// 실제 아군 AI를 활성화한 시작 직후 사제 메뉴의 Outpost 기술/금지를 관찰한다. 건설/공격 명령은 선택하지 않는다.
const fs=require('node:fs'),assert=require('node:assert/strict'),{chromium}=require('@playwright/test');
const {installAllyAi}=require('../../e2e/ally-ai-test-control');
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
// 본 소스/허용 후보/금지 후보를 각각 새 브라우저 문맥에서 시작한다.
async function run(){const browser=await chromium.launch({headless:true});const results=[];try{
 for(const scenario of [{name:'before',candidate:false,ban:false},{name:'allowed',candidate:true,ban:false},{name:'banned',candidate:true,ban:true}]){
  const context=await browser.newContext({viewport:{width:1024,height:768},locale:'en-US'}),page=await context.newPage(),errors=[];
  try{
   page.on('pageerror',error=>errors.push(error.stack??error.message));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
   await page.addInitScript(()=>localStorage.setItem('netstorm.settings',JSON.stringify({version:1,values:{welcomeGiven:1,tellTips:0,autoDemo:0,sound:0,music:0,difficulty:2}})));
   if(scenario.candidate)await page.route('**/core/skirmish-setup.js',route=>route.fulfill({status:200,contentType:'application/javascript',body:fs.readFileSync('extracted/e1447-outpost/skirmish-setup.js','utf8')}));
   await page.goto('http://localhost:9803/debug.html?fixed=25&aspect=4:3&title=1&lang=en');await ready(page);await installAllyAi(page,'skirmish',{preserveDifficulty:true});
   // 실제 메뉴 작성기를 호출 그대로 통과시키고 선택된 건설 항목만 읽는다.
   await page.evaluate(async()=>{const {ClientMenuGump}=await import('./ui/client-menu-gump.js'),original=ClientMenuGump.prototype.entry;window.outpostEntries=[];ClientMenuGump.prototype.entry=function(...args){if((args[1]>>>0)===0x02000060&&(args[2]>>>16)===126)window.outpostEntries.push({text:args[0],command:args[1],parameter:args[2],include:args[3],enabled:args[4]});return original.apply(this,args);};});
   const launch=await page.evaluate(async ban=>{const {createSkirmishSettings,buildSkirmishLaunch}=await import('./core/skirmish-setup.js'),app=window.netstorm,settings=createSkirmishSettings();settings.aiCount=1;settings.bans=ban?['outpost']:[];const launch=buildSkirmishLaunch(settings,4);await app._beginSkirmish(launch);return{script:launch.script,settings};},scenario.ban);
   // 실제 적재와 Go! 뒤 같은 원래 프레임만 실행한다.
   for(let i=0;i<1500;i++){if(await page.evaluate(()=>Boolean(window.netstorm._mainMenu.controlPoint('Go!'))&&Boolean(window.netstorm._campaignWorld?.session().localConnectionReady())))break;await page.evaluate(()=>window.netstorm.stepFixedFrames(1));}
   await click(page,'Go!');const activation=await page.evaluate(()=>window.netstormAllyTestState);assert.equal(activation.money,5000);for(const key of ['money','allies','name','technology'])assert.deepEqual(activation[key],activation.before[key]);
   // 원래 View Your Priest 키로 카메라만 이동한 뒤 현재 적중점을 읽는다.
   await page.screenshot({path:'extracted/e1447-outpost/'+scenario.name+'-before-f5.png'});
   await page.keyboard.press('F5');await page.evaluate(()=>window.netstorm.stepFixedFrames(3));
   const spot=await page.evaluate(()=>{const app=window.netstorm,w=app._campaignWorld,s=w.session();let priest=0;for(const {sid,object}of s.world().objects())if(object.owner===1&&object.type===158){priest=sid;break;}let hit=null;for(let y=6;y<762&&!hit;y+=3)for(let x=90;x<1020;x+=3)if(w.pick(x,y,0)===priest){hit=[x,y];break;}if(!priest||!hit)return{priest,hit,layout:{...app._layout}};const r=app.canvas.getBoundingClientRect();return{priest,x:r.left+hit[0]*r.width/app._layout.logicalWidth,y:r.top+hit[1]*r.height/app._layout.logicalHeight};});
   fs.writeFileSync('extracted/e1447-outpost/'+scenario.name+'-spot.json',JSON.stringify(spot,null,2));await page.screenshot({path:'extracted/e1447-outpost/'+scenario.name+'-after-f5.png'});assert.ok(Number.isFinite(spot.x),'현재사제의실제화면적중점필요');
   await page.mouse.click(spot.x,spot.y,{button:'right'});await page.evaluate(()=>window.netstorm.stepFixedFrames(3));
   const actual=await page.evaluate(()=>{const s=window.netstorm._campaignWorld.session();return{time:window.netstorm._gameClock.gameSeconds(),technology:[Boolean(s.findTechnology(1,126)),Boolean(s.findTechnology(2,126))],entries:window.outpostEntries,activeAI:Boolean(window.netstormAllyTestState?.token)};});
   const result={scenario,launch,activation,spot,actual,errors};results.push(result);fs.writeFileSync('extracted/e1447-outpost/browser.json',JSON.stringify(results,null,2));await page.screenshot({path:'extracted/e1447-outpost/'+scenario.name+'-menu.png'});fs.writeFileSync('extracted/e1447-outpost/'+scenario.name+'-dump.tsv',await page.evaluate(()=>window.netstorm.sessionDump()));
   assert.deepEqual(errors,[]);assert.equal(actual.activeAI,true);assert.deepEqual(actual.technology,scenario.candidate?[true,true]:[false,false]);assert.equal(actual.entries.length,scenario.candidate&&!scenario.ban?1:0);if(actual.entries.length)assert.equal(actual.entries[0].enabled,true);
   console.log(JSON.stringify({name:scenario.name,technology:actual.technology,entries:actual.entries,errors}));
  }finally{await context.close();}
 }
}finally{await browser.close();}}
run().catch(error=>{console.error(error.stack??error);process.exitCode=1;});
