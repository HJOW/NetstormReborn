// 현행 Document WebMCP에 실제 게임 입력·조회·캡처·녹화를 등록한다.
// 호스트는 내부 Raw 객체 대신 직렬화 가능한 상태와 실제 UI 사건만 제공한다.
const TOOL_SPECS = Object.freeze([
 { name:'netstorm_get_state', description:'현재 게임 단계·미션·선택·화면 크기·게임 시각을 조회합니다.', host:'getState', readOnly:true, properties:{} },
 { name:'netstorm_input', description:'실제 게임 창에 마우스 이동/버튼·휠·키·확정 문자(한 번에16자 이하)를 보냅니다. 좌표는 논리 화면 좌표입니다.', host:'input', properties:{type:{enum:['pointer','wheel','key','text']},action:{enum:['move','down','up','click','press']},x:{type:'number'},y:{type:'number'},button:{enum:['left','middle','right']},deltaY:{type:'number'},key:{type:'string'},code:{type:'string'},text:{type:'string',maxLength:16},shift:{type:'boolean'},control:{type:'boolean'},alt:{type:'boolean'}},required:['type'] },
 { name:'netstorm_capture', description:'현재 표시된 게임 Canvas를 PNG로 캡처합니다.', host:'capture', readOnly:true, properties:{} },
 { name:'netstorm_recording_start', description:'현재 게임 화면과 게임 효과음·음악의 WebM 녹화를 시작합니다. 마이크는 사용하지 않습니다.', host:'startRecording', properties:{framesPerSecond:{type:'integer',minimum:5,maximum:60},maximumSeconds:{type:'integer',minimum:1,maximum:1800}} },
 { name:'netstorm_recording_stop', description:'게임 녹화를 종료하고 다운로드 주소·파일명·크기를 반환합니다. download=true면 실제 파일 내려받기도 시작합니다.', host:'stopRecording', properties:{download:{type:'boolean'}} },
]);
// 취소 요청을 일반 DOM AbortError로 전달한다.
function checkAbort(signal) { if(signal?.aborted)throw signal.reason??new DOMException('요청이 취소됐습니다.','AbortError'); }
// 호스트가 모든 공개 기능을 연결했는지 등록 전에 검사한다.
function validateHost(host) { for(const spec of TOOL_SPECS)if(typeof host?.[spec.host]!=='function')throw new TypeError('WebMCP 호스트 기능 누락: '+spec.host); }
// 표준 API가 없는 브라우저는 일반 게임을 그대로 실행한다. 생성한 등록만 Abort로 반납한다.
export async function installWebMcp(document,host,{signal}={}) {
 const context=document?.modelContext;
 if(typeof context?.registerTool!=='function')return Object.freeze({supported:false,dispose(){}});
 validateHost(host);
 const Controller=document.defaultView?.AbortController??AbortController, controller=new Controller();
 // 앱 반납 중 등록이 진행 중이어도 같은 문서 수명에서 모두 취소한다.
 const abort=()=>controller.abort(signal?.reason);
 signal?.addEventListener('abort',abort,{once:true});if(signal?.aborted)abort();
 try {
  // 각 등록은 같은 호스트를 호출하되 사용자 입력·캡처를 내부 게임 객체로 우회하지 않는다.
  for(const spec of TOOL_SPECS)await context.registerTool({
   name:spec.name,description:spec.description,inputSchema:{type:'object',properties:spec.properties,required:spec.required??[],additionalProperties:false},
   annotations:{readOnlyHint:Boolean(spec.readOnly)},
   async execute(input,options={}) {
    checkAbort(options.signal);
    try { const result=await host[spec.host](input??{},options);checkAbort(options.signal);return result; }
    catch(error){
     // 브라우저의 일반 UnknownError 대신 조작에 필요한 원래 메시지를 전달하되 요청 취소는 표준 취소로 유지한다.
     checkAbort(options.signal);const name=error?.name??'Error',message=error?.message??String(error);
     return{isError:true,error:{name,message},content:[{type:'text',text:message}]};
    }
   },
  },{signal:controller.signal});
  checkAbort(signal);
 }catch(error){signal?.removeEventListener('abort',abort);controller.abort();throw error;}
 return Object.freeze({supported:true,dispose(){signal?.removeEventListener('abort',abort);controller.abort();}});
}
