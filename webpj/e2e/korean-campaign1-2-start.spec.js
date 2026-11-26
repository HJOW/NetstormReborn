// 한국어1-2의 실제 Start·AI 생성·브리핑·사제 좌클릭 선택이 오류 없이 이어지는지 검사한다.
const {test,expect}=require('@playwright/test');
// 같은 화면의 실제 메뉴 단추를 브라우저 마우스로 누른다.
async function clickLabel(page,label){
 await page.waitForFunction(text=>Boolean(window.netstorm?._mainMenu?.controlPoint(text)),label,{timeout:30000});
 const point=await page.evaluate(text=>{const app=window.netstorm,p=app._mainMenu.controlPoint(text),r=app.canvas.getBoundingClientRect();return{x:r.left+p.x*r.width/app._mainMenu.width,y:r.top+p.y*r.height/app._mainMenu.height};},label);
 await page.mouse.click(point.x,point.y,{delay:60});
}
// 프레임 예외를 삼켜 메뉴만 남는 회귀는 즉시 실패로 기록한다.
async function waitStarted(page,errors){
 // 실제 프레임을 기다리되 상태나 AI를 검사에서 직접 바꾸지 않는다.
 for(let attempt=0;attempt<300;attempt++){
  if(errors.length)throw new Error(errors.join('\n'));
  if(await page.evaluate(()=>window.netstorm?._campaignDisplayActive()&&window.netstorm._clientGuide.current()?.runtime.gate.started===1))return;
  await page.waitForTimeout(100);
 }
 throw new Error('한국어1-2 미션 Start가 완료되지 않았다');
}
// 공격·포획·희생·승리는 수행하지 않고 실제 미션 진입과 아군 사제 선택만 검증한다.
test('한국어 캠페인1-2: AI 생성 뒤 시계가 흐르고 사제를 실제 마우스로 선택한다',async({page})=>{
 test.setTimeout(150000);const errors=[];
 page.on('pageerror',error=>errors.push(error.stack??error.message));page.on('console',message=>{if(message.type()==='error')errors.push(message.text());});
 await page.addInitScript(()=>{
  localStorage.setItem('netstorm.settings',JSON.stringify({version:1,values:{welcomeGiven:1,tellTips:0,sound:0,music:0}}));
  // 메뉴 진입을 위한 기존 완료값만 준비하며 실제 미션의 자본·체력·AI·승패는 수정하지 않는다.
  const done={};for(const name of ['Tutorial1','Tutorial2','Tutorial3','Tutorial4','Tutorial5','Tutorial6','TheWarBegins'])done['Done'+name]='1';
  localStorage.setItem('netstorm.progress',JSON.stringify({version:1,values:done}));
 });
 await page.setViewportSize({width:1024,height:768});await page.goto('/?lang=ko&aspect=4:3&startwait=0');
 await page.waitForFunction(()=>window.netstorm?.state==='clickToStart',null,{timeout:60000});await page.locator('canvas').click();
 await page.waitForFunction(()=>window.netstorm?.state==='mainMenu');await clickLabel(page,'캠페인');await clickLabel(page,'자유를 위한 투쟁');
 const label=await page.evaluate(()=>window.netstorm._mainMenu.dialog.menu.items.find(item=>item.command?.name==='MissionBegin'&&item.command.argument==='MasterOfWhirligigs')?.label);
 expect(label).toContain('훨리기그');await clickLabel(page,label);await waitStarted(page,errors);
 await page.waitForFunction(()=>window.netstorm._mainMenu.dialog?.buttons?.some(button=>button.label==='미션 시작'));
 const mission=await page.evaluate(()=>{const app=window.netstorm,s=app._clientGuide.current().script;return{title:s.get('title'),name:s.get('aiName'),brain:app._campaignWorld.session().participation().brain(2)};});
 expect(mission.title).toBe('훨리기그의 지배자');expect(mission.name).toBe('훨리기그 지배자');expect(mission.brain).toBeGreaterThan(0);
 await clickLabel(page,'미션 시작');await page.waitForFunction(()=>!window.netstorm._mainMenu.dialog&&!window.netstorm._gameClock.paused());
 const before=await page.evaluate(()=>window.netstorm._gameClock.gameSeconds());await page.waitForTimeout(500);expect(await page.evaluate(()=>window.netstorm._gameClock.gameSeconds())).toBeGreaterThan(before);
 const priest=await page.evaluate(()=>{const app=window.netstorm,world=app._campaignWorld,session=world.session(),sid=session.findPriestParticipation(session.localOwner()),r=app.canvas.getBoundingClientRect();let point=null;
  // 실제 월드 적중점만 찾아 사제 슬롯이나 선택 값을 직접 쓰지 않는다.
  for(let y=8;y<760&&!point;y+=2)for(let x=90;x<1020;x+=2)if(world.pick(x,y,0)===sid){point={x:r.left+x*r.width/app._layout.logicalWidth,y:r.top+y*r.height/app._layout.logicalHeight};break;}
  return{sid,point,selected:world.selected()};});
 expect(priest.sid).toBeGreaterThan(0);expect(priest.point).not.toBeNull();expect(priest.selected).toBe(0);
 await page.mouse.move(priest.point.x,priest.point.y);await page.mouse.click(priest.point.x,priest.point.y,{delay:60});
 await expect.poll(()=>page.evaluate(()=>window.netstorm._campaignWorld.selected())).toBe(priest.sid);
 await page.screenshot({path:test.info().outputPath('korean-campaign1-2-selected.png')});
 expect(errors).toEqual([]);
});
