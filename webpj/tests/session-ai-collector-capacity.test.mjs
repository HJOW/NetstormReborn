// 실제세션의자원생산여유는완성/건설중을합산하되풀/AI본문을쓰지않는다.
import test from 'node:test';import assert from 'node:assert/strict';
import {SidPool} from '../www/core/sid-pool.js';
import {RawAiBrainMemory,AI_BRAIN_BYTES} from '../www/core/raw-ai-brain.js';
import {RawCampaignSessionAiProduction} from '../www/core/raw-campaign-session-ai-production.js';
import {put} from './raw-support.mjs';
// 현재실제저장소와합성TYPE의읽기전용생산입력을준비한다.
function scene(desired){const pool=new SidPool(),types=Array.from({length:188},()=>({flags2:0})),memory=new RawAiBrainMemory(0x42000000,(bytes)=>bytes.fill(0)),token=memory.allocate(AI_BRAIN_BYTES);memory.bytes(token).fill(0);put(memory.bytes(token),0x260,2);put(memory.bytes(token),0x234,desired);types[137].flags2=0x10000;types[158].flags2=0x210000;let attached=2;const owner={core:{pool,types},aiMemory:memory,aiPreparation:{countAttachedGeysers:()=>attached}},production=new RawCampaignSessionAiProduction(owner);return{pool,memory,token,production,setAttached:(value)=>{attached=value;}};}
// Pop된현재유닛/건설중유닛/다른소유자/무효후보를같은풀에기록하는명시검사입력이다.
function unit(s,{owner=2,type=137,extra=128,state=0}={}){const sid=s.pool.allocate(0),raw=s.pool.physical(sid);raw[10]=type;raw[11]=state;raw[34]=owner;raw[40]=extra;return sid;}
test('수집자생산: 완성5와건설중1이면6목표의추가생산을막고사망뒤한칸을다시허용한다',()=>{const s=scene(6);/* 완성된 다섯 기와 건설 중인 한 기의 합계를 확인한다. */for(let i=0;i<5;i++)unit(s);const pending=unit(s,{extra:129});const before=s.pool.bytes(),body=s.memory.bytes(s.token).slice();assert.equal(s.production.collectorCapacity(s.token),false);assert.deepEqual(s.pool.bytes(),before);assert.deepEqual(s.memory.bytes(s.token),body);s.pool.physical(pending)[11]=2;assert.equal(s.production.collectorCapacity(s.token),true);});
test('수집자생산: 쉬움3/보통6과미연결최소1·다른소유자/사제/유령후보를구분한다',()=>{/* 난이도별 목표와 제외 대상을 각각 대조한다. */for(const desired of [3,6]){const s=scene(desired);/* 마지막 생산 여유 한 칸을 남겨 경계 양쪽을 검사한다. */for(let i=0;i<desired-1;i++)unit(s);unit(s,{owner:1});unit(s,{type:158});unit(s,{state:4});unit(s,{state:8});unit(s,{extra:136});assert.equal(s.production.collectorCapacity(s.token),true);unit(s,{extra:129});assert.equal(s.production.collectorCapacity(s.token),false);}const s=scene(3);s.setAttached(0);assert.equal(s.production.collectorCapacity(s.token),true);unit(s,{extra:129});assert.equal(s.production.collectorCapacity(s.token),false);});
