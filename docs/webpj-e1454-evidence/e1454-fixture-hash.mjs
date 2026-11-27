// 기존 도구의 같은 정규화/해시로 실제 fixture 파일만 확인한다. 느린 Git 기준선 재검토와 분리한 결과다.
import fs from 'node:fs';
import path from 'node:path';
import {measure,UNHASHED_KINDS} from '../tools/fixture-manifest.mjs';
// 저장소 루트와 기존 추적 목록이며 문서/기준선 계산 칸은 쓰지 않는다.
const root=path.resolve('..'),manifest=JSON.parse(fs.readFileSync('tests/fixtures/manifest.json','utf8')),missing=[],mismatches=[];let checked=0,unhashed=0;
// 원래 추적 목록의 파일 존재와 해시 대상만 같은 규칙으로 확인한다.
for(const entry of manifest.entries){const file=path.join(root,entry.path);if(!fs.existsSync(file)){missing.push(entry.path);continue;}if(UNHASHED_KINDS.includes(entry.kind)){unhashed++;continue;}const actual=measure(fs.readFileSync(file));checked++;if(actual.sha256!==entry.sha256)mismatches.push({id:entry.id,path:entry.path,expected:entry.sha256,actual:actual.sha256});}
const result={entries:manifest.entries.length,checked,unhashed,missing,mismatches,scope:'원래measure의정규화해시/존재만; Git기준선drift별도기존CLI실행중'};fs.writeFileSync('extracted/e1454-fixture-hash.json',JSON.stringify(result,null,2));console.log(JSON.stringify(result));process.exitCode=missing.length||mismatches.length?1:0;
