// 실제 배포 아카이브의 31캠페인/3데모 대본·시작 fort에서 Outpost 제외를 읽기 전용 감사한다.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import {readData} from '../tests/helpers.mjs';
import {GameData,ARCHIVE_PATH,SETUP_PATH} from '../www/assets/game-data.js';
import {MissionScript} from '../www/core/mission-script.js';
import {parseFort} from '../www/assets/fort.js';
import {createFortTypes} from '../www/assets/type-flags.js';
import {realCaptureAssets} from '../tests/capture-assets-support.mjs';
import {GameAssets} from '../www/assets/game-assets.js';
import {CampaignSessionAssets} from '../www/core/raw-campaign-session-owners.js';
import {RawPlayerTechnology,PlayerTechnologyList,PLAYER_TECHNOLOGY_PARSE_STEP as STEP} from '../www/core/raw-player-technology.js';
import {SidPool} from '../www/core/sid-pool.js';
// 기존 첫 안내 감사와 같은 목록이다. 데모는 일반 같은 이름과 별도 행으로 보존한다.
const names=['Tutorial1','Tutorial2','Tutorial3','Tutorial4','Tutorial5','Tutorial6','TheWarBegins','MasterOfWhirligigs','SaveTheIsland','FragileFortune','ThunderingPower','DissolvedAlliance','TheNoose','RainVsRain','RunForIt','GuardMyBack','Surrounded','BreakingThrough','ToTheRescue','Vicious','EnemyTerritory','FinalConfrontation','bc1menu76','bc2bridging76','bc3acid_bar76','bc4bow_rush76','bc5walkers76','bc6mows76','bc7sun_bars76','bc8fury76','bc9ssun_bars76'];
const specs=[...names.map(name=>({name,demo:false})),...['DemoStormRages','GuardMyBack','DissolvedAlliance'].map(name=>({name,demo:true}))];
const data=GameData.build({files:new Map([[ARCHIVE_PATH,readData(ARCHIVE_PATH)],[SETUP_PATH,readData(SETUP_PATH)]]),language:'english'}),capture=realCaptureAssets(),types=createFortTypes(capture.table),assets=new CampaignSessionAssets(new GameAssets(capture.table,capture.database),{read(){throw new Error('추가게임읽기금지');}}),pool=new SidPool();
const tech=new RawPlayerTechnology({pool},{pool},{pool},assets.types,{findType:name=>assets.types.findIndex(t=>t.name===name)}),rows=[];
// 원래 목록 해석기를 완전히 끝내며 예상 밖 무진행/상한 초과를 통과시키지 않는다.
function parsed(text){const list=new PlayerTechnologyList();for(let i=0;i<100;i++){const step=tech.parseStep(text??'',list);if(step===STEP.complete)return Array.from(list.types.slice(0,list.count));assert.equal(step,STEP.progress);}throw new Error('기술목록해석종료필요');}
// 시작 배치/내용물의 모든 중첩을 보되 type-conversion 표로 이미 현재 번호가 된 자료만 사용한다.
function walk(value,found=[]){if(!value||typeof value!=='object')return found;if(value.type===126)found.push({type:value.type,owner:value.owner??null});for(const [key,next]of Object.entries(value))if(key!=='typeConversion'&&key!=='sections'&&key!=='typeHashes'){if(Array.isArray(next))for(const child of next)walk(child,found);else if(next&&typeof next==='object'&&!(next instanceof Uint8Array))walk(next,found);}return found;}
// 플레이를 시작하지 않고 최초 실행 대본과 선택한 시작 이미지의 실제 내용을 읽는다.
for(const spec of specs){const mission=new MissionScript(data,spec.name);try{assert.equal(mission.loaded,true);const fort=parseFort(mission.readFortImage(mission.fortName()),types),technology=[];for(const key of ['myTech',...Array.from({length:8},(_,i)=>`ai${i+1}Tech`)]){const text=mission.get(key),numbers=parsed(text);technology.push({key,text,outpost:numbers.includes(126)});}rows.push({...spec,fort:mission.fortName(),objects:walk({chaff:fort.chaff,territories:fort.territories,technology:fort.technology,deck:fort.deck}),technology});}finally{mission.dispose();}}
fs.writeFileSync('extracted/e1448-mission-exclusion.json',JSON.stringify({scope:'대본/시작fort정적감사, 게임실행/후반승리없음',rows},null,2));
assert.equal(rows.length,34);for(const row of rows){assert.deepEqual(row.objects,[],row.name);assert.equal(row.technology.some(x=>x.outpost),false,row.name);}console.log('PASS: 31캠페인/3데모 실제 실행대본 시작기술·시작fort Outpost 없음');
