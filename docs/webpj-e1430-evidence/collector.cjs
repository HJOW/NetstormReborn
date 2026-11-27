// 실제 운영 번들에서 한국어31미션의 첫 안내 창과 줄폭을 확인한다. 미션 시작/공격/승리 조작은 하지 않는다.
'use strict';
// 실제 브라우저와 원문 파일, 검증 단언이다.
const {chromium}=require('@playwright/test'),fs=require('node:fs'),assert=require('node:assert/strict');
const MISSIONS = ['Tutorial1','Tutorial2','Tutorial3','Tutorial4','Tutorial5','Tutorial6','TheWarBegins','MasterOfWhirligigs','SaveTheIsland','FragileFortune','ThunderingPower','DissolvedAlliance','TheNoose','RainVsRain','RunForIt','GuardMyBack','Surrounded','BreakingThrough','ToTheRescue','Vicious','EnemyTerritory','FinalConfrontation','bc1menu76','bc2bridging76','bc3acid_bar76','bc4bow_rush76','bc5walkers76','bc6mows76','bc7sun_bars76','bc8fury76','bc9ssun_bars76'];
// 실제글자배치검토용대표창만스크린샷을남긴다.31개전체상태는JSON으로보존한다.
const CAPTURES = new Set(['Tutorial1','TheWarBegins','TheNoose','FinalConfrontation','bc6mows76']);


// 같은 앱의 원래 미션 진입/반납 경로만 사용해 한국어 첫 안내를 수집한다.
async function run(){
 const browser=await chromium.launch({headless:true}),context=await browser.newContext({viewport:{width:1024,height:768},locale:'ko-KR'}),page=await context.newPage(),errors=[],rows=[],geometryIssues=[];
 page.on('pageerror',error=>errors.push(error.stack??error.message));page.on('console',message=>{if(message.type()==='error')errors.push(message.text());});
 const out='extracted/e1430-korean-initial';fs.mkdirSync(out,{recursive:true});
 try{
  await page.addInitScript(()=>localStorage.setItem('netstorm.settings',JSON.stringify({version:1,values:{welcomeGiven:1,tellTips:0,autoDemo:0,sound:0,music:0}})));
  await page.goto('http://localhost:9803/?aspect=4:3&lang=ko');await page.waitForFunction(()=>window.netstorm?.state==='clickToStart',null,{timeout:120000});
  assert.equal(await page.evaluate(()=>window.netstorm.options.language),'korean');await page.locator('canvas').click();await page.waitForFunction(()=>window.netstorm.state==='mainMenu',null,{timeout:120000});
  // 실제 첫 안내가 뜰 때까지만 기다리고 시작/다음 버튼은 누르지 않는다.
  for(const mission of MISSIONS){
   await page.evaluate(()=>window.netstorm._leaveCampaignMission());await page.waitForFunction(()=>!window.netstorm._campaignWorld&&!window.netstorm._clientGuide.current());
   await page.evaluate(name=>window.netstorm._beginCampaignFromDialog(name),mission);
   await page.waitForFunction(name=>{const app=window.netstorm,current=app._clientGuide.current();return current?.script.name.toLowerCase()===name.toLowerCase()&&Boolean(app._campaignWorld?.session().localConnectionReady())&&Boolean(app._mainMenu.dialog);},mission,{timeout:90000});
   const row=await page.evaluate(()=>{const app=window.netstorm,current=app._clientGuide.current(),d=app._mainMenu.dialog;/* 원본 줄폭은 뒤 공백을 포함한다. 실제 마지막 글자까지의 폭을 별도로 재며 원시 폭/문자열도 모두 보존한다. */ const lines=d.body.softLines.map(line=>{let advance=0,visible=0;const fragments=[];for(const f of line.fragments){const full=f.picture?{width:f.picture.width??0}:d.body.fonts.measure(f.bytes,f.context);let end=f.bytes?.length??0;while(end>0&&f.bytes[end-1]===32)end--;const content=f.picture?full:d.body.fonts.measure(f.bytes.subarray(0,end),f.context);if(f.picture||end>0)visible=Math.max(visible,advance+content.width);fragments.push({text:f.bytes?String.fromCharCode(...f.bytes):null,fullWidth:full.width,contentWidth:content.width,advance});advance+=full.width;}return {logical:line.width,visible,fragments};});return {mission:current.script.name,title:current.script.get('title'),language:app._gameData.language,x:d.x,width:d.width,logicalWidth:app._layout.logicalWidth,maxLine:d.body.maximumWidth(),visibleMaxLine:Math.max(0,...lines.map(l=>l.visible)),lines,bodyWidth:d.body.baseWidth,buttons:d.buttons.map(b=>b.label),time:app._gameClock.gameSeconds(),started:current.runtime.gate.started};});
   rows.push(row);fs.writeFileSync(out+'/initial-missions.json',JSON.stringify({scope:'운영번들 한국어31미션진입/첫안내만',rows,errors},null,2));console.log(JSON.stringify(row));
   assert.equal(row.language,'korean');if(!(row.x>=0&&row.x+row.width<=row.logicalWidth)||row.visibleMaxLine>row.bodyWidth)geometryIssues.push({...row,overflow:row.visibleMaxLine-row.bodyWidth});assert.deepEqual(errors,[]);
   if(CAPTURES.has(mission)||row.maxLine>row.bodyWidth)await page.screenshot({path:out+'/'+mission+'.png'});
  }
  assert.equal(rows.length,31);fs.writeFileSync(out+'/geometry-issues.json',JSON.stringify(geometryIssues,null,2));assert.deepEqual(geometryIssues,[],'한국어 첫 안내 폭 초과');console.log('PASS: 한국어31미션첫안내/창범위/줄폭;미션시작없음');
 }finally{fs.writeFileSync(out+'/initial-missions.json',JSON.stringify({scope:'운영번들 한국어31미션진입/첫안내만',rows,errors},null,2));await browser.close();}
}
run().catch(error=>{console.error(error.stack??error);process.exitCode=1;});
