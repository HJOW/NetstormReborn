// 보호 Outpost의 group10과 실제 원문 기술 해석기로 스커미시 시작 기술을 대조한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import {GameAssets} from '../www/assets/game-assets.js';
import {CampaignSessionAssets} from '../www/core/raw-campaign-session-owners.js';
import {RawPlayerTechnology,PlayerTechnologyList,PLAYER_TECHNOLOGY_PARSE_STEP as STEP} from '../www/core/raw-player-technology.js';
import {SidPool} from '../www/core/sid-pool.js';
import {buildSkirmishLaunch,createSkirmishSettings} from '../www/core/skirmish-setup.js';
import {realCaptureAssets} from './capture-assets-support.mjs';
// 실제 자산 분류를 읽고 기술 문자열 해석에만 같은 풀의 세 주소 계약을 제공한다. 생성/게임은 하지 않는다.
const data=realCaptureAssets(),game=new GameAssets(data.table,data.database),assets=new CampaignSessionAssets(game,{read(){throw new Error('기술분류감사 VFS읽기금지');}}),pool=new SidPool();
const tech=new RawPlayerTechnology({pool},{pool},{pool},assets.types,{findType:name=>assets.types.findIndex(t=>t.name===name)});
// 원래 all/명시 목록 해석을 종료까지 실행하고 현재 기술 번호만 돌려준다.
function parse(text){const list=new PlayerTechnologyList();for(let i=0;i<100;i++){const step=tech.parseStep(text,list);if(step===STEP.complete)return Array.from(list.types.slice(0,list.count));assert.equal(step,STEP.progress);}throw new Error('정상기술목록의종료필요');}
// 일반 all의 제외를 유지하면서 로컬과 모든 AI의 명시 Outpost 기술만 추가한다.
test('스커미시 Outpost: 실제 group10은 일반 all에서 제외되고 각 참가자의 시작 목록에만 명시된다',()=>{
 assert.equal(assets.types[126].group,10);assert.equal(parse('all').includes(126),false);
 const settings=createSkirmishSettings();settings.aiCount=3;const launch=buildSkirmishLaunch(settings,1);
 for(const key of ['myTech','ai2Tech','ai3Tech','ai4Tech']){const match=launch.script.match(new RegExp('^'+key+'="([^"\\n]+)"','m'));assert.ok(match,key);const types=parse(match[1]);assert.equal(types.filter(t=>t===126).length,1,key);assert.ok(types.length<=40);}
});
