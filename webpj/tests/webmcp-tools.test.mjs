// 표준 등록의 선택 지원·읽기 전용 표기·취소·부분 실패 정리를 검사한다. 실제 게임 조작/녹화는 e2e가 검증한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import {installWebMcp} from '../www/platform/webmcp-tools.js';
// 외부 등록을 포함한 문서 대역이다. signal 반납과 등록 실패가 다른 도구를 지우지 않는지 확인한다.
function documentFixture(failAt=0){
 const tools=new Map([['external',{name:'external'}]]),calls=[];
 return{tools,calls,modelContext:{async registerTool(tool,{signal}){
  if(signal.aborted)throw signal.reason;
  if(calls.length+1===failAt||tools.has(tool.name))throw new Error('검사 등록 실패');
  calls.push(tool.name);tools.set(tool.name,tool);
  signal.addEventListener('abort',()=>{if(tools.get(tool.name)===tool)tools.delete(tool.name);},{once:true});
 }}};
}
// 기능 이름과 입력 전달만 기록하는 호스트 대역이다. 게임 상태를 만들어 내는 대역으로 쓰지 않는다.
function hostFixture(){return Object.fromEntries(['getState','input','capture','startRecording','stopRecording'].map(name=>[name,async(input)=>({name,input})]));}
test('WebMCP: 미지원 문서는 호스트 없이도 일반 게임을 유지한다',async()=>{
 const bridge=await installWebMcp({},{});assert.equal(bridge.supported,false);bridge.dispose();
});
test('WebMCP:5도구 등록·실행 취소·앱 수명 종료는 자기 등록만 반납한다',async()=>{
 const document=documentFixture(),lifetime=new AbortController(),host=hostFixture(),bridge=await installWebMcp(document,host,{signal:lifetime.signal});
 assert.equal(bridge.supported,true);assert.equal(document.calls.length,5);
 assert.equal(document.tools.get('netstorm_capture').annotations.readOnlyHint,true);assert.equal(document.tools.get('netstorm_input').annotations.readOnlyHint,false);
 const input=document.tools.get('netstorm_input');assert.deepEqual(await input.execute({type:'key',key:'p'}),{name:'input',input:{type:'key',key:'p'}});
 host.input=async()=>{throw new RangeError('입력 좌표 범위 오류');};assert.deepEqual(await input.execute({}),{isError:true,error:{name:'RangeError',message:'입력 좌표 범위 오류'},content:[{type:'text',text:'입력 좌표 범위 오류'}]});
 const canceled=new AbortController();canceled.abort();await assert.rejects(input.execute({},{signal:canceled.signal}),{name:'AbortError'});
 lifetime.abort();assert.deepEqual([...document.tools.keys()],['external']);bridge.dispose();
});
test('WebMCP: 부분 등록 실패와 사전 취소는 도구를 남기지 않는다',async()=>{
 const document=documentFixture(3);await assert.rejects(installWebMcp(document,hostFixture()),/등록 실패/);assert.deepEqual([...document.tools.keys()],['external']);
 const canceled=new AbortController();canceled.abort();const another=documentFixture();await assert.rejects(installWebMcp(another,hostFixture(),{signal:canceled.signal}),{name:'AbortError'});assert.deepEqual([...another.tools.keys()],['external']);
});
