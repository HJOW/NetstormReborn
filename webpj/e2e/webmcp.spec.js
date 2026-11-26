// 실제 Document WebMCP 등록과 게임 UI·PNG·효과음 포함 녹화·다운로드·반납을 검증한다.
const {test,expect,chromium}=require('@playwright/test');
const fs=require('node:fs');
// 브라우저가 등록한 실제 Tool 객체로 실행하며 callback을 직접 호출하지 않는다.
async function call(page,name,input={}){
 const result=await page.evaluate(async({name,input})=>{const tools=await document.modelContext.getTools(),tool=tools.find(item=>item.name===name);if(!tool)throw new Error('등록 도구 없음: '+name);return JSON.parse(await document.modelContext.executeTool(tool,input));},{name,input});
 if(result?.isError)throw new Error('MCP 도구 오류: '+result.error.message);return result;
}
// 사용자 시작 클릭으로 오디오 정책을 통과하고 원래 모든 시작 프레임을 실행한다.
async function ready(page){
 await page.waitForFunction(()=>window.netstorm?.state==='clickToStart',null,{timeout:120000});await page.locator('canvas').click();await page.evaluate(()=>window.netstorm._prepareCampaignAudio());
 while(await page.evaluate(()=>window.netstorm.state!=='mainMenu'))await page.evaluate(()=>window.netstorm.stepFixedFrames(1));await page.evaluate(()=>window.netstorm.stepFixedFrames(1));
}
// 실제 UI의 조작점을 읽되 클릭 자체는 공개 WebMCP 입력 도구에만 맡긴다.
async function clickLabel(page,label){
 const point=await page.evaluate(label=>{const point=window.netstorm._mainMenu.controlPoint(label);if(!point)throw new Error('현재 UI 항목 없음: '+label);return{x:point.x,y:point.y};},label);
 await call(page,'netstorm_input',{type:'pointer',action:'click',...point});await page.evaluate(()=>window.netstorm.stepFixedFrames(2));
}
test('WebMCP가 없는 기본 브라우저에서도 일반 게임 시작이 유지된다',async({page})=>{
 test.setTimeout(150000);await page.addInitScript(()=>localStorage.setItem('netstorm.settings',JSON.stringify({version:1,values:{welcomeGiven:1,tellTips:0,autoDemo:0,music:0,sound:0}})));
 await page.goto('/debug.html?fixed=25&aspect=4:3&title=1&lang=english');await ready(page);
 const state=await page.evaluate(()=>({state:window.netstorm.state,host:window.netstorm._agentHost,bridge:window.netstorm._agentBridge}));expect(state).toEqual({state:'mainMenu',host:null,bridge:null});
});
test('WebMCP 실제 게임: 등록·메뉴·입력·PNG·효과음 WebM·다운로드와 앱 반납',async()=>{
 test.setTimeout(300000);
 const browser=await chromium.launch({headless:true,args:['--enable-features=WebMCP']});
 try{
  const page=await browser.newPage({viewport:{width:1024,height:768},locale:'en-US',acceptDownloads:true}),errors=[];
  page.on('pageerror',error=>errors.push(error.message));page.on('console',message=>{if(message.type()==='error')errors.push(message.text());});
  await page.addInitScript(()=>localStorage.setItem('netstorm.settings',JSON.stringify({version:1,values:{welcomeGiven:1,tellTips:0,autoDemo:0,music:0,sound:1}})));
  await page.goto((test.info().project.use.baseURL??'http://localhost:9803')+'/debug.html?fixed=25&aspect=4:3&title=1&lang=english');await ready(page);
  await page.waitForFunction(()=>window.netstorm._agentBridge?.supported===true);
  expect(await page.evaluate(async()=>(await document.modelContext.getTools()).map(tool=>tool.name).sort())).toEqual(['netstorm_capture','netstorm_get_state','netstorm_input','netstorm_recording_start','netstorm_recording_stop']);
  expect((await call(page,'netstorm_get_state')).logicalSize).toEqual({width:1024,height:768});
  await clickLabel(page,'Campaign');await clickLabel(page,'Struggle For Freedom');
  const label=await page.evaluate(()=>window.netstorm._mainMenu.dialog.menu.items.find(item=>item.command?.name==='MissionBegin'&&item.command.argument.toLowerCase()==='thewarbegins')?.label);expect(label).toBeTruthy();await clickLabel(page,label);
  // 실제 적재가 끝나 첫 브리핑 단추가 생길 때까지 허용된 진입 프레임만 진행한다.
  for(let frame=0;frame<300&&!await page.evaluate(()=>Boolean(window.netstorm._mainMenu.controlPoint('Play Mission')));frame++)await page.evaluate(()=>window.netstorm.stepFixedFrames(1));
  await clickLabel(page,'Play Mission');await page.waitForTimeout(500);
  const initial=await call(page,'netstorm_get_state');expect(initial.mission.toLowerCase()).toBe('thewarbegins');expect(initial.stormPower).toBe(3000);
  const shot=await call(page,'netstorm_capture');expect(shot.content[0].mimeType).toBe('image/png');expect(shot.width).toBeGreaterThan(0);
  fs.writeFileSync(test.info().outputPath('webmcp-game-canvas.png'),Buffer.from(shot.content[0].data,'base64'));
  await test.info().attach('실제 MCP PNG',{body:Buffer.from(shot.content[0].data,'base64'),contentType:'image/png'});
  const started=await call(page,'netstorm_recording_start',{framesPerSecond:10,maximumSeconds:10});expect(started.includesGameAudio).toBe(true);
  await call(page,'netstorm_input',{type:'key',action:'press',key:'p',code:'KeyP'});await page.evaluate(()=>window.netstorm.stepFixedFrames(40));await page.waitForTimeout(400);
  const downloadPromise=page.waitForEvent('download'),recorded=await call(page,'netstorm_recording_stop',{download:true}),download=await downloadPromise;
  expect(recorded.downloadStarted).toBe(true);expect(recorded.bytes).toBeGreaterThan(1000);expect(download.suggestedFilename()).toBe(recorded.fileName);
  const destination=test.info().outputPath('native-effects.webm');await download.saveAs(destination);expect(fs.statSync(destination).size).toBe(recorded.bytes);
  // 실제 녹화 파일을 브라우저 오디오 디코더로 풀어 무음 트랙만 저장한 경우도 거부한다.
  const pcm=await page.evaluate(async url=>{const context=new AudioContext({sampleRate:48000});try{const bytes=await(await fetch(url)).arrayBuffer(),buffer=await context.decodeAudioData(bytes);let sum=0,maximum=0;const channel=buffer.getChannelData(0);for(const value of channel){sum+=value*value;maximum=Math.max(maximum,Math.abs(value));}return{channels:buffer.numberOfChannels,samples:buffer.length,rms:Math.sqrt(sum/channel.length),maximum};}finally{await context.close();}},recorded.url);
  expect(pcm.channels).toBe(2);expect(pcm.samples).toBeGreaterThan(1000);expect(pcm.rms).toBeGreaterThan(0.00001);expect(pcm.maximum).toBeGreaterThan(0.001);
  const final=await call(page,'netstorm_get_state');expect(final.selected).toBe(15000);expect(final.recording.recording).toBe(false);expect(errors).toEqual([]);
  // 실제 Enter·IME 확정·Backspace·제출을 기존 채팅 큐로 전달한다.
  await call(page,'netstorm_input',{type:'key',key:'Enter'});await page.evaluate(()=>window.netstorm.stepFixedFrames(2));
  await call(page,'netstorm_input',{type:'text',text:'MCP test'});await page.evaluate(()=>window.netstorm.stepFixedFrames(2));
  expect(await page.evaluate(()=>window.netstorm._campaignProduction.ui.chat().inputText())).toBe('MCP test');
  await call(page,'netstorm_input',{type:'key',key:'Backspace'});await page.evaluate(()=>window.netstorm.stepFixedFrames(2));expect(await page.evaluate(()=>window.netstorm._campaignProduction.ui.chat().inputText())).toBe('MCP tes');
  const beforeLines=await page.evaluate(()=>window.netstorm._clientChatWindows.log.lineCount);
  await call(page,'netstorm_input',{type:'key',key:'Enter'});await page.evaluate(()=>window.netstorm.stepFixedFrames(3));expect(await page.evaluate(()=>window.netstorm._clientChatWindows.log.lineCount)).toBe(beforeLines+1);
  await call(page,'netstorm_input',{type:'key',key:'Escape'});await page.evaluate(()=>window.netstorm.stepFixedFrames(2));
  // 실제 포커스 해제로 누름을 푼 뒤에도 새 마우스 입력이 붙잡혀 거부되지 않아야 한다.
  await call(page,'netstorm_input',{type:'pointer',action:'down',button:'middle',x:600,y:300});await page.evaluate(()=>window.netstorm.canvas.dispatchEvent(new FocusEvent('blur')));
  await call(page,'netstorm_input',{type:'pointer',action:'down',button:'middle',x:600,y:300});await call(page,'netstorm_input',{type:'pointer',action:'up',button:'middle',x:600,y:300});await page.evaluate(()=>window.netstorm.stepFixedFrames(2));
  expect((await call(page,'netstorm_get_state')).pointer.buttons).toBe(0);expect(errors).toEqual([]);
  // line 보고서에서도 소리 검증 수치와 원문 결과가 사라지지 않게 파일로 보존한다.
  fs.writeFileSync(test.info().outputPath('webmcp-game-result.json'),JSON.stringify({initial,started,recorded,pcm,final},null,2));console.log('MCP 실제 녹화 PCM '+JSON.stringify(pcm));
  await test.info().attach('실제 녹화 상태와 PCM',{body:JSON.stringify({initial,started,recorded,pcm,final},null,2),contentType:'application/json'});
  await page.evaluate(()=>window.netstorm.destroy());expect(await page.evaluate(async()=>(await document.modelContext.getTools()).length)).toBe(0);
  expect(await page.evaluate(async url=>{try{await fetch(url);return true;}catch{return false;}},recorded.url)).toBe(false);
 }finally{await browser.close();}
});
