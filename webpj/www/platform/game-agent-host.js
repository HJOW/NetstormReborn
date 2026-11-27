// 게임 내부 상태를 공개하지 않고 값 사본·실제 DOM 입력·현재 Canvas·녹화만 제공하는 어댑터다.
import { GameRecorder } from './game-recorder.js';
// DOM 버튼 번호와 buttons 비트는 서로 다른 순서이므로 한 표로 관리한다.
const BUTTONS=Object.freeze({left:{button:0,mask:1},middle:{button:1,mask:4},right:{button:2,mask:2}});
// key 생략 코드가 같은 실제 게임 키 경로를 타도록 특수 키의 물리 이름을 정한다.
const KEY_CODES=Object.freeze({Shift:'ShiftLeft',Control:'ControlLeft',Alt:'AltLeft',' ':'Space'});
// 선택 보조 키는 모두 불리언으로 검증하고 DOM 이름으로 바꾼다.
function modifiers(input){
 for(const key of ['shift','control','alt'])if(input[key]!==undefined&&typeof input[key]!=='boolean')throw new TypeError('보조 키 값은 불리언이어야 합니다.');
 return{shiftKey:Boolean(input.shift),ctrlKey:Boolean(input.control),altKey:Boolean(input.alt)};
}
// 해당 문서의 Canvas에만 사건을 보내 브라우저 단축키나 OS 입력으로 새지 않게 한다.
export class GameAgentHost {
 // 현재 앱과 Canvas를 소유한 문서·녹화기다. 내부 게임 객체는 결과로 반환하지 않는다.
 #app;#canvas;#view;#recorder;
 // 연속 누름/끌기에서 유지할 실제 DOM buttons와 마지막 논리 좌표다.
 #buttons=0;#disposed=false;
 // 같은 앱 수명의 실제 Canvas와 현재 오디오 조회기를 연결한다.
 constructor(app){
  if(!app?.canvas?.ownerDocument?.defaultView)throw new TypeError('게임 Canvas가 준비돼야 합니다.');
  this.#app=app;this.#canvas=app.canvas;this.#view=this.#canvas.ownerDocument.defaultView;
  this.#recorder=new GameRecorder({canvas:this.#canvas,audio:()=>this.#app._audio,view:this.#view});
 }
 // 반납된 어댑터는 이전 Canvas나 새 게임 수명에 입력하지 않는다.
 #live(){if(this.#disposed||this.#app.canvas!==this.#canvas)throw new Error('게임 도구 수명이 끝났습니다.');}
 // 논리 좌표를 실제 화면 좌표로 바꾸며 화면 밖·숨겨진 Canvas를 거부한다.
 #coordinates(input){
  const layout=this.#app._layout,rect=this.#canvas.getBoundingClientRect(),{x,y}=input;
  if(!Number.isFinite(x)||!Number.isFinite(y)||x<0||y<0||x>=layout.logicalWidth||y>=layout.logicalHeight||rect.width<=0||rect.height<=0)throw new RangeError('입력 좌표는 현재 논리 화면 안이어야 합니다.');
  return{clientX:rect.left+x*rect.width/layout.logicalWidth,clientY:rect.top+y*rect.height/layout.logicalHeight};
 }
 // 원본 큐를 직접 쓰지 않고 실제 사건을 넣기 전에 남은 공간만 읽는다.
 #space(count){if(this.#app._clientInputQueue&&this.#app._clientInputQueue.freeSlots<count)throw new Error('게임 입력 큐가 찼습니다. 다음 프레임 뒤 다시 시도하십시오.');}
 // 상태·메뉴·선택은 직렬화 가능한 값만 복사한다. 조회로 게임 시계나 난수를 소비하지 않는다.
 getState(){
  this.#live();const app=this.#app,session=app._campaignWorld?.session(),current=app._clientGuide?.current(),menu=app._mainMenu,owner=session?.localOwner()??0,pointer=app._input.pointer;
  const labels=[...(menu?.dialog?.buttons??[]).map(button=>button.label),...(menu?.dialog?.menu?.items??[]).map(item=>item.label)].filter(label=>typeof label==='string');
  // 현재 보이는 메뉴의 실제 조작점만 값으로 제공한다.
  const controls=[...new Set(labels)].map(label=>{const point=menu.controlPoint(label);return point?{label,x:point.x,y:point.y}:null;}).filter(Boolean);
  return{state:app.state,mission:current?.script.name??null,gameSeconds:app._gameClock?.gameSeconds()??null,logicalSize:{width:app._layout.logicalWidth,height:app._layout.logicalHeight},selected:app._campaignWorld?.selected()??0,localOwner:owner||null,stormPower:owner>=1&&owner<=8&&session.spStorage().initialized?session.sp(owner):null,pointer:{x:pointer.x,y:pointer.y,inside:pointer.inside,buttons:pointer.buttons},keys:[...app._input.keys],controls,recording:this.#recorder.status()};
 }
 // 포인터·키·휠·IME 확정문을 실제 Canvas 리스너에 전달한다.
 input(input={}){
  this.#live();if(!input||typeof input!=='object'||Array.isArray(input))throw new TypeError('게임 입력 객체가 필요합니다.');
  const canvas=this.#canvas,view=this.#view,mods=modifiers(input);canvas.focus();
  if(input.type==='pointer'){
   // 탭 전환/blur가 실제 게임의 누름을 풀었으면 어댑터에도 반영해 버튼이 붙잡혀 남지 않게 한다.
   this.#buttons=this.#app._input.pointer.buttons&7;
   const action=input.action??'click',button=BUTTONS[input.button??'left'],point=this.#coordinates(input);
   if(!button||!['move','down','up','click'].includes(action))throw new TypeError('마우스 동작 또는 버튼 오류');
   this.#space(action==='click'?2:action==='move'?0:1);
   // 이동·누름·뗌은 같은 버튼 상태를 이어 쓴다.
   const send=(type,buttons)=>canvas.dispatchEvent(new view.PointerEvent(type,{...point,...mods,bubbles:true,cancelable:true,pointerId:1,pointerType:'mouse',isPrimary:true,button:type==='pointermove'?-1:button.button,buttons}));
   send('pointermove',this.#buttons);
   if(action==='down'||action==='click'){if(this.#buttons&button.mask)throw new Error('이미 누른 버튼입니다.');this.#buttons|=button.mask;send('pointerdown',this.#buttons);}
   if(action==='up'||action==='click'){this.#buttons&=~button.mask;send('pointerup',this.#buttons);}
   if(action==='click'&&button.button===0)canvas.dispatchEvent(new view.MouseEvent('click',{...point,...mods,bubbles:true,cancelable:true,button:0,buttons:this.#buttons}));
  }else if(input.type==='wheel'){
   const point=this.#coordinates(input);if(!Number.isFinite(input.deltaY)||Math.abs(input.deltaY)>1000)throw new RangeError('휠 이동량 범위는 -1000~1000입니다.');
   canvas.dispatchEvent(new view.WheelEvent('wheel',{...point,...mods,bubbles:true,cancelable:true,deltaY:input.deltaY,deltaMode:0}));
  }else if(input.type==='key'){
   const key=input.key,action=input.action??'press';if(typeof key!=='string'||!key||key.length>32||!['down','up','press'].includes(action))throw new TypeError('키 이름 또는 동작 오류');
   const code=input.code??KEY_CODES[key]??(/^[a-z]$/iu.test(key)?'Key'+key.toUpperCase():/^\d$/u.test(key)?'Digit'+key:key);
   if(typeof code!=='string'||!code||code.length>32)throw new TypeError('물리 키 코드 오류');
   this.#space(action==='press'?2:1);
   // 원래 keydown·keyup 리스너가 문자와 가상 키를 구분한다.
   for(const type of action==='press'?['keydown','keyup']:[action==='down'?'keydown':'keyup'])canvas.dispatchEvent(new view.KeyboardEvent(type,{...mods,bubbles:true,cancelable:true,key,code,repeat:false}));
  }else if(input.type==='text'){
   // eslint-disable-next-line no-control-regex -- NUL/제어문자 검출 자체가 이 검사의 목적이다.
   const text=input.text;if(typeof text!=='string'||!text||text.length>16||/[\u0000-\u001f\u007f]/u.test(text))throw new TypeError('확정 문자는 제어문자 없이 한 번에16자 이하로 입력하십시오.');
   this.#space([...text].length*(this.#app._clientNativeInputEnabled?1:2));
   if(this.#app._clientNativeInputEnabled){canvas.dispatchEvent(new view.CompositionEvent('compositionstart',{bubbles:true}));canvas.dispatchEvent(new view.CompositionEvent('compositionend',{bubbles:true,data:text}));}
   else{
    // 구형 입력창도 같은 문자 keydown 경계를 사용하며 문자열 버퍼를 직접 고치지 않는다.
    for(const key of text)for(const type of ['keydown','keyup'])canvas.dispatchEvent(new view.KeyboardEvent(type,{bubbles:true,cancelable:true,key,code:/^[a-z]$/iu.test(key)?'Key'+key.toUpperCase():key}));
   }
  }else throw new TypeError('지원하지 않는 게임 입력 종류입니다.');
  return{accepted:true,state:this.#app.state};
 }
 // 표시 Canvas의 PNG를 MCP 이미지 콘텐츠와 크기로 반환한다.
 capture(){this.#live();return{width:this.#canvas.width,height:this.#canvas.height,content:[{type:'image',mimeType:'image/png',data:this.#canvas.toDataURL('image/png').split(',')[1]}]};}
 // 게임 소리 버스만 녹화하며 사용자 마이크 권한을 요청하지 않는다.
 startRecording(options){this.#live();return this.#recorder.start(options);}
 // 마지막 인코딩 조각까지 받은 실제 다운로드 결과를 반환한다.
 async stopRecording({download=false}={},{signal}={}){
  this.#live();if(typeof download!=='boolean')throw new TypeError('녹화 내려받기 선택은 불리언이어야 합니다.');
  const result=await this.#recorder.stop();
  if(signal?.aborted)throw signal.reason??new this.#view.DOMException('요청이 취소됐습니다.','AbortError');
  const downloadStarted=download?this.#recorder.download():false;
  return{...result,downloadStarted};
 }
 // 등록 반납 뒤 녹화 장치·Blob 주소도 같은 수명에서 정리한다.
 async dispose(){if(this.#disposed)return;this.#disposed=true;await this.#recorder.dispose();}
}
