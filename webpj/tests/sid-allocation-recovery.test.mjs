// 원본 PATCH 할당 회복47행의 전체 풀/삭제 기록/진단 순서를 그대로 대조한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import {SidPool} from '../www/core/sid-pool.js';
import {readRows,adler32,put} from './raw-support.mjs';
// 원문 기대값은 C++ fixture를 그대로 읽으며 후보가 계산한 값을 기대값으로 만들지 않는다.
const rows=readRows(new URL('../../cpppj/tests/fixtures/sid-allocation-recovery-x86.tsv',import.meta.url));
// 두 삭제 배열의20쌍을 원문 client/other 순서의320바이트로 직렬화한다.
function history(pool){const b=new Uint8Array(320);let at=0;for(const client of [true,false])for(const row of pool.deletions(client)){put(b,at,row.sid);put(b,at+4,row.type);at+=8;}return b;}
test('SID PATCH: 원본47행의 진단 복귀/최대50회수/부분 상태와 전체 해시',()=>{
 assert.equal(rows.length,47);
 // 각 Native 입력을 새 실제 풀에 적용하고 동일한 한 번의 할당만 수행한다.
 for(const row of rows){
  const [mode,flags,initialServer,boss]=row.slice(0,4).map(Number),owner={value:Boolean(initialServer)},pool=new SidPool(23005,{serverOwner:owner});
  let bridge=82,disallow=0,kills=0;const events=[];
  // 입력 후보의 원시 VT/TYPE/void 비트만 쓴다. 삭제는 실제 제품 release를 사용한다.
  const candidate=(sid,type)=>{const raw=pool.physical(sid);put(raw,0,0x10001000);raw[10]=type;raw[11]=4;};
  if(mode===15){pool.allocate(1);pool.allocate(1);}
  const client=Boolean(flags&2),head=pool.firstFree(client),tail=pool.tail(client);
  if(mode===1)disallow=1;if(mode===3)pool.physical(head)[11]=0;
  if(mode===4||mode>=5&&mode<=9||mode===17)put(pool.physical(0),client?4:18,tail,2);
  if(mode===6){for(let sid=10;sid<70;sid++)candidate(sid,82);}else if(mode>=5&&mode<=9){candidate(10,82);candidate(11,mode===7?70:82);candidate(13,82);}
  if(mode===17)bridge=0x10052;if(mode===11)pool.physical(23001)[11]=0;if(mode===12)pool.physical(23002)[11]=0;
  if(mode===13||mode===14)put(pool.physical(23002),4,mode===13?23004:23001,2);
  pool.bindAllocation({disallow:()=>disallow,boss:()=>Boolean(boss),bridgeType:()=>bridge,
   assertion:(expression,file,line)=>{events.push('A:'+line+':'+Buffer.from(expression).toString('hex')+':'+Buffer.from(file).toString('hex'));if(mode===1&&line===0xe8)disallow=0;if(mode===11&&line===0xf0)pool.physical(23001)[11]=1;},
   reclaim:(sid)=>{kills++;events.push('K:'+sid+':0');pool.release(sid);if(kills===1){if(mode===7)bridge=70;else if(mode===8)candidate(12,82);else if(mode===9)throw new Error('명시 삭제 장치 실패');}}
  });
  let result=0,stopped=0;try{result=pool.allocate(flags);}catch{stopped=1;}
  const actual=[stopped,result,pool.freeCount,pool.predictableCursor,pool.firstFree(true),pool.tail(true),pool.firstFree(false),pool.tail(false),adler32(pool.bytes()),adler32(history(pool)),bridge,disallow];
  assert.deepEqual(actual,row.slice(4,16).map(Number),'Native 입력 '+row.slice(0,4));assert.equal(events.join(';')||'-',row[16]);
 }
});

// 실제 장기 Storage와 현재 World의 Content 가상 삭제/다음 대여를 확인한다. 전투/승리 시험은 아니다.
import {RawSurfaceStorage} from '../www/core/raw-surface-storage.js';
import {RawSurfaceWorld} from '../www/core/raw-surface-world.js';
import {BridgeConnectState} from '../www/core/raw-bridge-connect.js';
import {FrameCodeTable} from '../www/assets/terrain-tiles.js';
import {typeConstructorAddress} from '../www/core/squid-constructors.js';
import {ContentFormHost} from '../www/core/content-form-host.js';
import {ContentFormLifetime,ContentLifetimeState} from '../www/core/content-form-lifetime.js';
// 원본 C++ 회복 검사와 같은 최소 TYPE/빈 프레임 입력이다.
function worldInputs(){const types=Array.from({length:188},(_,number)=>({cost:0,flags1:0,flags2:0,footX:0,footY:0,...([1,2,3,6].includes(number)?{constructorAddress:typeConstructorAddress(number)}:{})})),frames=types.map(()=>new FrameCodeTable([])),shapes=types.map(()=>({frameCount:0})),links=new BridgeConnectState();Object.assign(links,{bridgeType:82,islandType:94,stalagType:95,connectorType:155,noIslandType:157});return {types,frames,shapes,links};}
// Session과 같이 현재 Content 클래스 호스트만 후설치한다.
function bindContents(core){const host=new ContentFormHost(core.pool,core.types,()=>{}),lifetime=new ContentFormLifetime(core.pool,core.types,host,core.destroy,new ContentLifetimeState(),{},core.forms.hooks());core.forms.bind(lifetime);return lifetime;}
test('SID 회복: 같은 장기 Pool이 현재 World의 가상 삭제만 선택한다',()=>{
 const input=worldInputs(),storage=new RawSurfaceStorage(input.types,input.frames,input.shapes);let assertions=0;storage.bindDisplayDiagnostics({value:''},()=>{assertions++;});
 const world=new RawSurfaceWorld(input.types,input.frames,input.shapes,input.links,83,{storage}),core=world.core();bindContents(core);
 const first=core.factory.createContained(2),before=core.pool.freeCount;assert.equal(first,5);put(core.pool.physical(0),4,core.pool.tail(true),2);
 storage.links.bridgeType=0x10006;storage.bindDisplayDiagnostics({value:''},(expression,file,line)=>{assert.equal(line,0x14a);throw new Error('전체 DWORD 예약 꼬리');});assert.throws(()=>core.pool.allocate(2),/전체 DWORD/);assert.equal(core.pool.freeCount,before);assert.equal(core.pool.slot(first)[11]&1,0);
 storage.links.bridgeType=6;storage.bindDisplayDiagnostics({value:''},()=>{assertions++;});const replacement=core.factory.createContained(2);assert.equal(replacement,14999);assert.ok(core.pool.slot(first)[11]&1);assert.equal(core.pool.freeCount,before);assert.equal(assertions,0);assert.equal(core.pool.firstFree(true),first);
 world.destroy(replacement,0x10);const residual=core.factory.createContained(2);assert.equal(residual,first);world.dispose();
 storage.links.bridgeType=6;put(core.pool.physical(0),4,core.pool.tail(true),2);assert.throws(()=>core.pool.allocate(2),/현재 World/);
 const next=new RawSurfaceWorld(input.types,input.frames,input.shapes,input.links,83,{storage,prepared:true}),current=next.core();bindContents(current);storage.links.bridgeType=6;const value=current.factory.createContained(2);assert.ok(value);assert.equal(assertions,0);next.destroy(value,0x10);next.dispose();
});
