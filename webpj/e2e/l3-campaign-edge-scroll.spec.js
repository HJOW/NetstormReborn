// 실제 포인터 폴링·가장자리 이동·정지의 프레임 끝 화면과 상태를 C++ 기준선과 대조한다.
const { test, expect } = require('@playwright/test');
const fs = require('node:fs'), path = require('node:path');
const { oracleFolder, loadOracle, runL3 } = require('./l3-common');
// C++는 격리 전체화면, 웹은 같은1024×768 논리 화면이다.
const ORACLE = oracleFolder('campaign-edge-scroll-l3');
// 실제 수집·소비하는 모든 원시 파일이다.
const FILES = ['steps.json','oracle.json','report.tsv','before.png','before.tsv','during.png','during.tsv','after.png','after.tsv','stop.png','stop.tsv'];
// 독립 C++ 관측의 카메라 X/Y다. 경계에서 최대3072로 제한된다.
const CAMERAS = {290:[976,793],350:[1993,793],400:[3072,793],450:[3072,793]};
// 표시 장치로 복사하기 전의 DIB와 비교해 카메라 Scroll 직후의 동일 시점을 확인한다.
test('L3 가장자리: 포인터 이동만으로 오른쪽 스크롤·중앙 정지가 cpppj와 같다',async({browser})=>{
 test.setTimeout(600000);
 const {plan,meta}=loadOracle(ORACLE);
 // 원본 실행 조건을 바꾼 창 강제 인자가 출처에 남지 않아야 한다.
 expect(plan.settings.startInFullScreen).toBe(1);expect(meta.args).not.toContain('--window');
 // 화면·상태·보고서 중 하나라도 빠지면 비교를 생략하지 않고 실패한다.
 for(const file of FILES)expect(fs.existsSync(path.join(ORACLE,file)),file).toBe(true);
 const native={};let frame=0;
 // Session 밖 카메라 전역도 별도 원시 보고서에서 읽어 웹 관측과 연결한다.
 for(const line of fs.readFileSync(path.join(ORACLE,'report.tsv'),'utf8').split(/\r?\n/)) {
  const fields=line.split('\t');if(fields[0]==='frame')frame=Number(fields[1]);
  if(fields[0]==='raw_camera_frame')native[frame]=fields.slice(1,3).map(Number);
 }
 expect(native).toEqual(CAMERAS);
 await runL3({browser,plan,oracleDir:ORACLE,snapshotPhase:'frameEnd',inspect:async(page,at)=>{
  // 현재 카메라·로컬 AI·자금을 읽기만 하여 실제 이동과 아군 AI 미사용을 확인한다.
  const state=await page.evaluate(()=>{const app=window.netstorm,view=app._clientGameplay.camera().snapshot().pixels,session=app._campaignWorld.session();return{camera:[view.x,view.y],brain:session.participation().brain(1),sp:session.sp(1)};});
  expect(state).toEqual({camera:CAMERAS[at],brain:0,sp:3000});
 }});
});
