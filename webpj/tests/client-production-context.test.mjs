// World 없는 생산 UI가 실제 장기 Client 물리와 원본 조회 관문을 쓰는지 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ProductionControlContext, readProductionControlMoney } from '../www/core/production-control-context.js';
import { ClientGameplayGlobals } from '../www/core/client-gameplay-globals.js';
import { ClientPlayerRecords } from '../www/core/client-player-records.js';
import { ClientSpGlobals } from '../www/core/client-sp-globals.js';
import { ClientRandomGlobals } from '../www/core/random.js';
import { RawSurfaceStorage } from '../www/core/raw-surface-storage.js';
import { CampaignSessionAssets } from '../www/core/raw-campaign-session-owners.js';
import { PriestPlacementAssets } from '../www/core/priest-placement-assets.js';
import { RawCanonPixelShape } from '../www/core/raw-canon-pixel-shape.js';
import { ProductionMaintenanceGlobals, ProductionMaintenancePanel } from '../www/core/raw-production-maintenance.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { ClientGumpRoot } from '../www/ui/client-gump-root.js';
import { ClientInventoryGrid } from '../www/ui/client-inventory-grid.js';
import { ClientInventoryItem } from '../www/ui/client-inventory-item.js';
import { ClientImageGump } from '../www/ui/client-image-gump.js';
import { ClientGumpBackground } from '../www/ui/client-gump-background.js';
import { ClientHudDivider } from '../www/ui/client-hud-divider.js';
import { ClientProductionSelection } from '../www/ui/client-production-selection.js';
import { ClientProductionPanel } from '../www/ui/client-production-panel.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put } from './raw-support.mjs';

// 보호된 게임 자산을 읽기만 하며 현재 Runtime을 만들지 않는다.
const data=realCaptureAssets(), game=new GameAssets(data.table,data.database,GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 같은 Client의 실제 Pool·Player·검색/커서·옵션과 자산 측정기를 만든다.
function scene(){
 const gameplay=new ClientGameplayGlobals(),players=new ClientPlayerRecords();players.initialize(()=>{});
 const assets=new CampaignSessionAssets(game,{read(){throw Error('선택하지 않은 파일');}}),geometry=new PriestPlacementAssets(game.typeTable,game.shapes,assets.loadedTypes);
 const storage=new RawSurfaceStorage(assets.types,assets.frames,geometry.shapes,{source:game,authority:gameplay.authority(),gameAux:gameplay.gameAux()});
 const context=new ProductionControlContext(storage,gameplay,players,new Uint8Array(87));
 const measure=new RawCanonPixelShape(storage.pool,storage.types,geometry.frames,geometry.shapes,geometry.hotspots,geometry.geometry,{scaleX:16,scaleY:11});
 return{context,geometry,measure};
}

// 같은 분산 SP DWORD를 float로 해석하며 Runtime 없는 기존 문맥의 조회는 실패해야 한다.
test('생산 Client 문맥: 장기 SP를 직접 읽고 미연결 Runtime을 대체하지 않는다',()=>{
 const {context}=scene();assert.throws(()=>readProductionControlMoney(context,1),/현재 게임 Runtime 미연결/);
 const sp=new ClientSpGlobals(new ClientRandomGlobals(),()=>1234);sp.store().initialize();context.sp=sp;
 const bytes=new DataView(new ArrayBuffer(4));bytes.setFloat32(0,1234.5,true);sp.store().set(1,bytes.getUint32(0,true));
 assert.equal(readProductionControlMoney(context,1),1234.5);bytes.setFloat32(0,-17.25,true);sp.store().set(1,bytes.getUint32(0,true));assert.equal(readProductionControlMoney(context,1),-17.25);
 assert.equal(context.gameplay.borrowed(),false);assert.throws(()=>readProductionControlMoney({},1),/문맥 미연결/);
});

// 빈 HUD 생성은 실제 SID0 finder를 유지하고 선택된 커서 효과에서만 Runtime을 요구한다.
test('생산 Client 문맥: World 없이 실제 격자·항목·선택·패널 생성과 초기 기한 보존',()=>{
 const {context,geometry,measure}=scene(),root=new ClientGumpRoot(1024,768,()=>0,()=>{});let now=9.2,grid;
 const images=new ClientImageGump(root,game,{localOwner:1,ownerMaps:new Uint32Array(188),shadowOffset:{x:5,y:5}},()=>0,()=>0);
 const item=new ClientInventoryItem(root,context,game,geometry,measure,images,{localOwner:1,editor:0,stockEnabled:0,inputDisabled:0,lastType:0,bridgeType:82,pixelsX:16,pixelsY:11,backgroundType:92,backgroundFrame:0},{idleItem:token=>grid.idleItem(token)});
 grid=new ClientInventoryGrid(root,context,{cursorPoint:{x:0,y:0},cursorFlags:0},{now:()=>now,baseHandle:()=>0,allocateItem:size=>item.allocate(size)});
 const selection=new ClientProductionSelection(context,item,{technology:()=>0,templateOwner:()=>1,stock:()=>false,failure(){throw Error('선택하지 않은 실패');},flash(){throw Error('선택하지 않은 점멸');},sound(){throw Error('선택하지 않은 소리');},changed(){}});
 const background=new ClientGumpBackground(root,game,()=>35,()=>null,()=>null,()=>0),divider=new ClientHudDivider(root,{background,backgroundColor:()=>35,dividerColor:()=>55,closePopups(){}});
 // 이번 검사의 글꼴/예산 표시 경계만 기본 Gump로 제공한다. Session 대체 객체는 만들지 않는다.
 const budget={create(parent){const token=root.createBase(0,20,10,1,()=>0);root.attach(token,0,0,parent,2);return token;}};
 const globals=new ProductionMaintenanceGlobals(),panel=new ProductionMaintenancePanel();globals.backgroundType=92;context.options[1]=3;
 const ui=new ClientProductionPanel(root,context,{grid,background,divider,budget,selection,now:()=>now,technology:()=>0,changed(){}},globals,panel);
 try{
  const token=ui.construct(),node=ui.node();assert.ok(token);assert.equal(panel.slots,8);assert.equal(panel.bridgeUntil,9.2);assert.equal(panel.initialized,true);
  assert.equal(root.node(node.bridge).child,0);assert.equal(root.node(node.teleport).child,0);assert.equal(context.gameplay.borrowed(),false);
  now=19;context.options[1]=7;ui.handleHudMode(3);ui.refresh(3);assert.equal(panel.slots,8);assert.equal(panel.bridgeUntil,9.2);
  const zero=context.storage.pool.physical(0);put(zero,6,65535,2);assert.throws(()=>ui.refresh(2),/체인 손상/);put(zero,6,0,2);ui.refresh(2);
  const sid=context.storage.pool.allocate(2),raw=context.storage.pool.allocated(sid);raw[10]=6;put(raw,18,82,2);put(raw,20,0x140,2);raw[23]=2;
  assert.equal(selection.choose(sid,0),1);put(raw,20,0x40,2);assert.throws(()=>selection.choose(sid,0),/현재 게임 Runtime 미연결/);
  put(zero,6,sid,2);assert.throws(()=>selection.shortcut(2),/현재 게임 Runtime 미연결/);put(zero,6,0,2);
 }finally{ui.close();root.dispose();}
});
