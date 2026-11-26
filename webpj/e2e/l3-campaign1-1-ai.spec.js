// 실제 아군/적 AI 결정·게임 상태와 동일 화면을 현행 제품1370 독립 오라클에 대조한다.
const {test,expect}=require('@playwright/test');
const fs=require('node:fs'),path=require('node:path');
const {runL3}=require('./l3-common'),{installAllyAi}=require('./ally-ai-test-control');
// 기존225553df의 일반 미션 오라클과 다른 실제 테스트 AI 실행 자료다.
const ORACLE=path.join(__dirname,'..','tests','fixtures','cpppj','product-1370-ai','campaign1-1-ai-l3');
// 모든 실제 수집물을 명시해 누락과 자료 추적 목록의 소비 경계를 함께 확인한다.
const FILES=['steps.json','oracle.json','report.tsv','ai1000.png','ai1000.tsv','ai6000.png','ai6000.tsv','ai7000.png','ai7000.tsv','ai8000.png','ai8000.tsv','ai9000.png','ai9000.tsv','ai10000.png','ai10000.tsv','ai12000.png','ai12000.tsv'];
// 화면 차이0까지 검증한 네 시점이다. 다른 세 시점의 차이는 아래 진단에 보존한다.
const MATCHED_SCREENS=new Set(['ai1000','ai6000','ai9000','ai10000']);
test('AI L3:7시점 전체 게임·AI 덤프와4시점 화면 일치,3시점 표시 차이 진단',async({browser})=>{
 test.setTimeout(900000);
 const plan=JSON.parse(fs.readFileSync(path.join(ORACLE,'steps.json'),'utf8')),meta=JSON.parse(fs.readFileSync(path.join(ORACLE,'oracle.json'),'utf8'));
 // 출처 파일의 누락과 외부 입력으로 달라진 Native 카메라 자료를 거부한다.
 for(const file of FILES)expect(fs.existsSync(path.join(ORACLE,file)),file).toBe(true);
 const cameras=fs.readFileSync(path.join(ORACLE,'report.tsv'),'utf8').split(/\r?\n/).filter(line=>line.startsWith('camera\t')).map(line=>line.split('\t').slice(1));
 expect(cameras).toHaveLength(7);expect(cameras.every(value=>value[0]==='976'&&value[1]==='793')).toBe(true);
 expect(meta.productUnit).toBe(1370);expect(meta.sha256).toBe('c3daa5bfbebfbcb91ab1b95965f9cecb745e17ec1110e9b574080229227249b7');expect(meta.windowsHide).toBe(true);
 const {results,errors}=await runL3({browser,plan,oracleDir:ORACLE,compare:false,snapshotPhase:'frameEnd',sessionDumpOptions:{includeAi:true},batchIdleFrames:true,prepareMission:async(page)=>{
  await installAllyAi(page,'thewarbegins');
  // 물리 Canvas 출력만 미루고 Kernel·Screen·Renderer는 원래 모든 프레임을 실행한다.
  await page.evaluate(()=>{window.netstorm._presenter.present=()=>{};});
 }});
 expect(errors).toEqual([]);
 const {compareSessionDumps}=await import('../tools/session-dump-compare.mjs'),{readPng,indexedToRgb}=await import('../tools/image-files.mjs'),{compareImages}=await import('../tools/screen-compare.mjs');
 const diagnostic=[];
 // 소유자0 SP와 주소 잔재 Player만 제외하며 실제1~8 SP·난수·프로세스·객체·AI 행은 모두 비교한다.
 for(const [key,value] of Object.entries(results))if(key.startsWith('session-dump:')){
  const name=key.slice(13),expected=fs.readFileSync(path.join(ORACLE,name+'.tsv'),'utf8'),difference=compareSessionDumps(expected,value,{ignoreSpOwnerZero:true});
  expect(value.split('\n').filter(line=>line.startsWith('ai\t')),name+' 실제 두 AI').toHaveLength(2);
  expect(difference.differences.filter(item=>!item.key.startsWith('player:')).map(item=>item.key),name+' 실제 본문·AI·SP').toEqual([]);
 }
 // 미확정 차이를 허용 영역으로 마스킹하지 않고 별도 결과로 남긴다. 해당 표시 문제는 인계의 열린 작업이다.
 for(const [key,shot] of Object.entries(results))if(key.startsWith('snapshot:')){
  const name=key.slice(9),palette=new Uint8Array(768);
  // 실제 BGRX 팔레트를 비교 도구의 RGB 순서로 바꾼다.
  for(let index=0;index<256;index++){palette[index*3]=shot.palette[index*4+2];palette[index*3+1]=shot.palette[index*4+1];palette[index*3+2]=shot.palette[index*4];}
  const actual=indexedToRgb({width:shot.width,height:shot.height,pixels:Uint8Array.from(shot.pixels)},palette),difference=compareImages(readPng(fs.readFileSync(path.join(ORACLE,name+'.png'))),actual,[]);
  if(MATCHED_SCREENS.has(name))expect(difference.outside,name+' 화면').toBe(0);
  else diagnostic.push({name,pixels:difference.outside,box:difference.outsideBox});
 }
 await test.info().attach('원인 미확정 표시 차이',{body:JSON.stringify(diagnostic,null,2),contentType:'application/json'});
 console.log('AI 표시 미해결 진단 '+JSON.stringify(diagnostic));
});
