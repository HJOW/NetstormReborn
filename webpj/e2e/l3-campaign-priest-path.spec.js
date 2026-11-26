// 캠페인1-2 저장 사제의 실제 선택·이동·경로 진행·도착 정리를 기준선과 대조한다.
const {test,expect}=require('@playwright/test');
const fs=require('node:fs'),path=require('node:path');
const {oracleFolder,loadOracle,runL3}=require('./l3-common');
// 같은 실제 미션·SID15000의 네 체크포인트 자료다.
const ORACLE=oracleFolder('campaign-priest-path-l3');
// 보고서·전체 Session을 포함해 소비하는 원시 자료를 명시한다.
const FILES=['steps.json','oracle.json','report.tsv','before.png','before.tsv','start.png','start.tsv','walking.png','walking.tsv','arrived.png','arrived.tsv'];
// C++가 기록한 실제 위치와 +36의 부착 프로세스 머리다. HP는 모든 지점에서100이다.
const PRIEST={290:{x:53,y:56,processHead:0,hp:100},301:{x:53,y:56,processHead:18,hp:100},350:{x:51.209293365478516,y:55,processHead:55,hp:100},800:{x:50,y:56,processHead:33,hp:100}};
// 경로를 합성하지 않고 P·F5·실제 땅 클릭으로 원래 명령·Path Form을 실행한다.
test('L3 저장 사제: 1-2의 실제 이동과 부착 과정이 cpppj와 같다',async({browser})=>{
 test.setTimeout(600000);
 const {plan}=loadOracle(ORACLE);
 // 누락된 자료를 건너뛰지 않고 원시 관측의 존재를 모두 검사한다.
 for(const file of FILES)expect(fs.existsSync(path.join(ORACLE,file)),file).toBe(true);
 expect(fs.readFileSync(path.join(ORACLE,'report.tsv'),'utf8')).toContain('raw_camera_frame\t320\t221');
 await runL3({browser,plan,oracleDir:ORACLE,inspect:async(page,frame)=>{
  // 같은 SID15000의 실제 슬롯만 읽고 경로·위치·체력을 직접 쓰지 않는다.
  const state=await page.evaluate(()=>{const session=window.netstorm._campaignWorld.session(),raw=session.core().pool.slot(15000),view=new DataView(raw.buffer,raw.byteOffset,raw.byteLength);return{x:view.getFloat32(14,true),y:view.getFloat32(18,true),processHead:view.getUint32(36,true),hp:view.getInt32(26,true)};});
  expect(state).toEqual(PRIEST[frame]);
 }});
});
