// 실제 아군 AI 스커미시에서 일반 사제 메뉴/마우스로 Outpost 건설을 관찰한다. 공격/목표 명령이나 SP/HP 쓰기는 하지 않는다.
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
// 적용한 본 소스의 허용/금지 설정을 각각 새 브라우저 문맥에서 시작한다.
async function run(){const browser=await chromium.launch({headless:true});const results=[];try{
 for(const scenario of [{name:'construct-main',candidate:false,ban:false}]){
  const context=await browser.newContext({viewport:{width:1024,height:768},locale:'en-US'}),page=await context.newPage(),errors=[];
  try{
   page.on('pageerror',error=>errors.push(error.stack??error.message));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
   await page.addInitScript(()=>localStorage.setItem('netstorm.settings',JSON.stringify({version:1,values:{welcomeGiven:1,tellTips:0,autoDemo:0,sound:0,music:0,difficulty:2}})));
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
   const result={scenario,launch,activation,spot,actual,errors};results.push(result);fs.writeFileSync('extracted/e1447-outpost/browser-construction.json',JSON.stringify(results,null,2));await page.screenshot({path:'extracted/e1447-outpost/'+scenario.name+'-menu.png'});fs.writeFileSync('extracted/e1447-outpost/'+scenario.name+'-dump.tsv',await page.evaluate(()=>window.netstorm.sessionDump()));
   assert.deepEqual(errors,[]);assert.equal(actual.activeAI,true);assert.deepEqual(actual.technology,[true,true]);assert.equal(actual.entries.length,!scenario.ban?1:0);if(actual.entries.length)assert.equal(actual.entries[0].enabled,true);
   // 일반 사제 메뉴의 실제 Outpost 행을 선택해 미리보기를 만든다. SP/HP/슬롯/목표에는 쓰지 않는다.
   try {
    const menuPath=await page.evaluate(()=>{const root=window.netstorm._clientGump;
     // 실제 열린 부모의 자식 메뉴 ID를 따라 Outpost가 들어 있는 경로를 찾는다.
     function find(token,path,seen){if(!token||seen.has(token))return null;const next=new Set(seen);next.add(token);const node=root.node(token);if(!node?.items?.length)return null;for(let row=0;row<node.items.length;row++){const item=node.items[row];if((item.command>>>0)===0x02000060&&(item.parameter>>>16)===126)return[...path,{token,row}];if(item.childId){const child=root.createState().entries[item.childId],found=find(child,[...path,{token,row}],next);if(found)return found;}}return null;}
     for(let token=root.root().child;token;token=root.node(token).next){const path=find(token,[],new Set());if(path)return path;}throw new Error('실제 Construct 하위 Outpost 메뉴 경로 필요');});
    result.construction={menuPath};
    // 각 부모를 실제로 연 뒤 현재 배치로 다음 메뉴 행을 누른다.
    for(const step of menuPath){const point=await page.evaluate(({token,row})=>{const app=window.netstorm,root=app._clientGump,node=root.node(token),b=root.collection().bounds(token),r=app.canvas.getBoundingClientRect(),horizontal=Boolean(node.menuFlags&0x40000),x=horizontal?node.itemWidth*(row+0.5):20,y=horizontal?(b.bottom-b.top)/2:node.offsets[row]+node.heights[row]/2;return{x:r.left+(b.left+x)*r.width/app._layout.logicalWidth,y:r.top+(b.top+y)*r.height/app._layout.logicalHeight};},step);await page.mouse.click(point.x,point.y);await page.evaluate(()=>window.netstorm.stepFixedFrames(12));}
    result.construction.selected=await page.evaluate(()=>{const a=window.netstorm,c=a._clientGameplay.cursor;return{type:c.type,preview:c.preview,lockDepth:c.lockDepth,sp:a._campaignWorld.session().sp(1)};});
    assert.equal(result.construction.selected.type,126);assert.ok(result.construction.selected.preview);assert.equal(result.construction.selected.lockDepth,0);
    // 원래 MayPlace로 유효한 화면 위치를 읽기 탐색한다. 강제 허용 플래그를 사용하지 않는다.
    const probe=await page.evaluate(async anchor=>{const {RawCursorPosition}=await import('./core/raw-cursor-position.js'),app=window.netstorm,s=app._campaignWorld.session(),cursor=app._clientGameplay.cursor,pl=s.cursor().placement,view=app._clientGameplay.camera().snapshot().data,cam=[view.getUint32(0,true),view.getUint32(4,true)],before=s.sp(1),snap=app._clientGameplay.cursorGlobals().snapToGrid().value,candidates=[];let probes=0;
     // 현재 첫 그림의 hotspot/크기와 원래 스냅을 그대로 적용해 실제 커서와 같은 좌표를 얻는다.
     for(let y=60;y<700;y+=11)for(let x=110;x<950;x+=16){const sx=(x+pl.hotspotX-Math.trunc(pl.imageWidth/2)-16)|0,sy=(y+pl.hotspotY-Math.trunc(pl.imageHeight/2)-11)|0,p=RawCursorPosition.screenToWorld(sx,sy,cam),v=snap?RawCursorPosition.truncatePoint(p):p,wx=Math.floor(Math.fround(v[0])+0.5),wy=Math.floor(Math.fround(v[1])+0.5);probes++;if(s.mayPlaceConstruction({type:126,argument:cursor.argument,x:wx,y:wy,flags:cursor.direction,owner:1,mode:1})){const r=app.canvas.getBoundingClientRect();candidates.push({logicalX:x,logicalY:y,x:r.left+x*r.width/app._layout.logicalWidth,y:r.top+y*r.height/app._layout.logicalHeight,worldX:wx,worldY:wy,distance:(x-anchor.x)**2+(y-anchor.y)**2});}}
     candidates.sort((a,b)=>a.distance-b.distance);return{probes,before,after:s.sp(1),placement:{hotspotX:pl.hotspotX,hotspotY:pl.hotspotY,imageWidth:pl.imageWidth,imageHeight:pl.imageHeight},snap,candidates:candidates.slice(0,100)};},spot);
    result.construction.probe=probe;fs.writeFileSync('extracted/e1447-outpost/browser-construction.json',JSON.stringify(results,null,2));assert.equal(probe.after,probe.before);assert.ok(probe.candidates.length,'현재 지도에서 원래 MayPlace를 통과하는 지점 필요');
    // 실제 마우스로 후보를 확인하고 원래 커서의 최종 공간/출처 판정까지 통과한 위치만 누른다.
    let candidate=null;result.construction.hovers=[];
    for(const point of probe.candidates){await page.mouse.move(point.x,point.y);await page.evaluate(()=>window.netstorm.stepFixedFrames(3));const hover=await page.evaluate(()=>{const app=window.netstorm,c=app._clientGameplay.cursor,p=app._campaignWorld.session().cursor().placement;return{type:c.type,preview:c.preview,legal:c.legal,legalGump:c.legalGump,x:c.x,y:c.y,argument:c.argument,direction:c.direction,placementAllowed:p.placementAllowed,spaceAllowed:p.spaceAllowed,sourceAllowed:p.sourceAllowed};});result.construction.hovers.push({point,hover});if(hover.legal){candidate=point;result.construction.hover=hover;break;}}
    result.construction.candidate=candidate;await page.screenshot({path:'extracted/e1447-outpost/construct-preview.png'});fs.writeFileSync('extracted/e1447-outpost/browser-construction.json',JSON.stringify(results,null,2));assert.ok(candidate,'실제 커서의 모든 배치 관문을 통과하는 지점 필요');
    await page.mouse.click(candidate.x,candidate.y);await page.evaluate(()=>window.netstorm.stepFixedFrames(4));
    // 실제 건설 프로세스가 완료 플래그를 내릴 때까지 아군 AI와 원래 프레임을 계속 실행한다.
    let built=null;
    for(let frame=0;frame<=4000;frame+=50){if(frame)await page.evaluate(()=>window.netstorm.stepFixedFrames(50));const state=await page.evaluate(()=>{const app=window.netstorm,s=app._campaignWorld.session(),units=[];for(const {sid,object}of s.world().objects())if(object.type===126&&object.owner===1){const raw=s.core().pool.physical(sid);if(!(raw[11]&15))units.push({sid,extra:raw[40],state:raw[11]});}return{time:app._gameClock.gameSeconds(),units,outposts:app._surfaceStorage.sidLists.outposts.items(),residences:app._surfaceStorage.sidLists.residences.items(),sp:s.sp(1),ai:Boolean(window.netstormAllyTestState?.token)};});built={frame,...state};if(errors.length||state.units.some(u=>!(u.extra&1)))break;}
    result.construction.completed=built;fs.writeFileSync('extracted/e1447-outpost/browser-construction.json',JSON.stringify(results,null,2));await page.screenshot({path:'extracted/e1447-outpost/construct-completed.png'});fs.writeFileSync('extracted/e1447-outpost/construct-completed.tsv',await page.evaluate(()=>window.netstorm.sessionDump()));
    assert.deepEqual(errors,[]);const complete=built.units.find(u=>!(u.extra&1));assert.ok(complete,'원래 건설 프로세스의 실제 완공 필요');assert.ok(built.outposts.includes(complete.sid));assert.ok(built.residences.includes(complete.sid));assert.equal(built.ai,true);
   } catch(error) {
    result.constructionFailure=String(error.stack??error);fs.writeFileSync('extracted/e1447-outpost/browser-construction.json',JSON.stringify(results,null,2));await page.screenshot({path:'extracted/e1447-outpost/construct-failure.png'});fs.writeFileSync('extracted/e1447-outpost/construct-failure.tsv',await page.evaluate(()=>window.netstorm.sessionDump()));throw error;
   }
   console.log(JSON.stringify({name:scenario.name,technology:actual.technology,entries:actual.entries,errors}));
  }finally{await context.close();}
 }
}finally{await browser.close();}}
run().catch(error=>{console.error(error.stack??error);process.exitCode=1;});
