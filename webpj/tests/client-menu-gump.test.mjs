// 공유 메뉴의 생성은 World 없이 수행하고 실제 객체 Form 효과에서만 현재 호스트를 조회한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import {ClientGumpRoot} from '../www/ui/client-gump-root.js';
import {ClientMenuGump} from '../www/ui/client-menu-gump.js';

// 실제 Gump·메뉴·레이아웃 본체에 그리기/글꼴 경계와 선택 가능한 실제 Form 장치를 제공한다.
function scene(){
 const root=new ClientGumpRoot(640,480,()=>0,()=>{}),calls=[],forms=new Set();let current=null,reads=0;
 // 월드 효과의 실제 호출 순서·소유자·대상을 관찰하는 Form 장치다.
 const host={pool:{edition:'patch1078',capacity:32768},create(mode){calls.push(['create',mode]);forms.add(1001);return 1001;},setOwner(form,owner){calls.push(['owner',form,owner]);},attach(form,object,flags){calls.push(['attach',form,object,flags]);},isGumpForm(form){return forms.has(form);},destroy(form,flags){calls.push(['destroy',form,flags]);forms.delete(form);}};
 // 스타일·그리기는 이번 입력/수명 검사에서 요청되지 않는 경계다. Session 대체 객체는 제공하지 않는다.
 const parts={assets:{},fonts:{measureFont:()=>null},text:{styleEngine:()=>({clear(){}})},background:{construct(token,width,height,flags){root.factory().construct(token,0,width,height,flags);}},decoration:{},title:{},config:{},backgroundType:0,foreground:7,disabled:8,shadow:0,sound(){},command(){},invalidate(){},formHost(){reads++;if(!current)throw new Error('현재 World 없음');return current;}};
 const menu=new ClientMenuGump(root,parts);
 return{root,menu,calls,reads:()=>reads,ready(){current=host;root.setFormHost(host);}};
}

// State7의 공유 Chat/메뉴 공장은 생성·객체0 부착에서 아직 없는 World를 요구하지 않아야 한다.
test('Client 메뉴 지연 Form: Session 없는 생성과 객체0 부착은 호스트를 읽지 않는다',()=>{
 const s=scene();assert.equal(s.reads(),0);
 const token=s.menu.beginVertical();s.menu.attachObject(0,20,30);
 assert.ok(s.root.node(token));assert.equal(s.reads(),0);assert.deepEqual(s.calls,[]);
 s.menu.close();s.root.dispose();
});

// Ready 뒤 같은 생성물의 객체 연결이 현재 실제 장치의 생성·등록·부착과 공통 반납을 사용한다.
test('Client 메뉴 지연 Form: 생성 뒤 Ready가 된 현재 호스트로 실제 객체를 연결한다',()=>{
 const s=scene();const token=s.menu.beginVertical();s.ready();s.menu.attachObject(52,20,30);
 const node=s.root.node(token);assert.ok(s.reads()>0);assert.equal(node.object,52);assert.equal(node.form,1001);
 assert.deepEqual(s.calls,[['create',2],['owner',1001,node.id],['attach',1001,52,0]]);
 s.menu.close();s.root.lifetime().cleanup();assert.equal(s.root.node(token),null);assert.ok(s.calls.some(c=>c[0]==='destroy'&&c[1]===1001));s.root.dispose();
});
