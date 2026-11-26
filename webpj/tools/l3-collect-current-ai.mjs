// 제품1370 실제 AI 관측을 새 폴더에 재수집한다. 저장소 루트에서 node webpj/tools/l3-collect-current-ai.mjs <새 폴더>로 실행한다.
import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import crypto from 'node:crypto';
import {spawnSync} from 'node:child_process';
import {runCppScenario} from './cpp-oracle.mjs';
import {readBmp,encodeIndexedPng} from './image-files.mjs';
// 확정한 독립 기준과 실제 실행할 제품 경로다. 다른 빌드를1370이라고 기록하지 않는다.
const basis='webpj/tests/fixtures/cpppj/product-1370-ai/campaign1-1-ai-l3',exe=path.resolve('cpppj/build/bin/Release/Netstorm.exe'),directory=process.argv[2];
if(!directory||process.argv.length!==3)throw new Error('저장소 루트에서 사용: node webpj/tools/l3-collect-current-ai.mjs <존재하지 않는 새 폴더>');
if(fs.existsSync(directory))throw new Error('기존 관측 자료를 덮어쓰지 않습니다. 새 폴더를 지정하십시오.');
// 실행 전 실제 바이너리가 기준 EXE와 같은지 확인한다.
const bytes=fs.readFileSync(exe),sha256=crypto.createHash('sha256').update(bytes).digest('hex'),basisMeta=JSON.parse(fs.readFileSync(basis+'/oracle.json','utf8'));
if(sha256!==basisMeta.sha256)throw new Error('현재 EXE는 제품1370 기준 바이트와 다릅니다. 다른 빌드는 별도 출처와 단위로 수집하십시오.');
// 원래 입력과 체크포인트를 그대로 사용한다. 실제 게임 실행 확인은 AGENTS.md의 현재 호스트 규칙을 따른다.
const plan=JSON.parse(fs.readFileSync(basis+'/steps.json','utf8'));
const result=runCppScenario({exe,gameDir:path.resolve('assets/game-data'),shared:plan,scenario:{id:'current-ai-12000',steps:plan.steps,frames:12000},execute:(file,args,options)=>spawnSync(file,[...args,'--test-ally-ai','hard','--test-enemy-ai','easy'],{...options,timeout:900000,windowsHide:true})});
fs.mkdirSync(directory,{recursive:true});fs.writeFileSync(path.join(directory,'steps.json'),JSON.stringify(plan,null,2)+'\n');fs.writeFileSync(path.join(directory,'report.tsv'),result.report);
// 독립 Native가 만든 원문을 웹 계산과 섞지 않는다.
for(const [label,{bmp}] of Object.entries(result.snapshots))fs.writeFileSync(path.join(directory,label+'.png'),encodeIndexedPng(readBmp(bmp)));
for(const [label,{tsv}] of Object.entries(result.dumps))fs.writeFileSync(path.join(directory,label+'.tsv'),tsv);
fs.writeFileSync(path.join(directory,'oracle.json'),JSON.stringify({format:basisMeta.format,version:1,productUnit:1370,host:os.hostname(),completedAt:new Date().toISOString(),exe,exeBytes:bytes.length,sha256,windowsHide:true,args:[...result.args,'--test-ally-ai','hard','--test-enemy-ai','easy']},null,2)+'\n');
console.log('독립 Native AI 수집 완료: '+path.resolve(directory));
