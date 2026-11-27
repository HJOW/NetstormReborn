# 실제 V8 heap의 약한 간선은 제외하고 GC root에서 남은 World/Session까지의 최단 강한 참조를 찾는다.
import json,sys,array,collections
from pathlib import Path
source=Path(sys.argv[1]);d=json.loads(source.read_bytes());m=d['snapshot']['meta'];n=d['nodes'];e=d['edges'];s=d['strings'];nf=m['node_fields'];ef=m['edge_fields'];k=len(nf);z=len(ef);N=len(n)//k;nt=m['node_types'][0];et=m['edge_types'][0]
ni=nf.index('name');ti=nf.index('type');ci=nf.index('edge_count');iti=ef.index('type');ini=ef.index('name_or_index');toi=ef.index('to_node')
# 노드별 간선 시작과 BFS 이전 노드를 압축 정수 배열에 보관한다.
start=array.array('I',[0]);a=0
for i in range(N):a+=n[i*k+ci]*z;start.append(a)
targets={i for i in range(N) if nt[n[i*k+ti]]=='object' and s[n[i*k+ni]] in ['RawSurfaceWorld','RawCampaignSession','RawCampaignSessionOwners','CampaignWorld']}
print('targets',[(i,s[n[i*k+ni]]) for i in targets])
parents=array.array('i',[-1])*N;via=array.array('i',[-1])*N;parents[0]=0;q=collections.deque([0]);found=set()
while q and found!=targets:
 u=q.popleft()
 for j in range(start[u],start[u+1],z):
  if et[e[j+iti]]=='weak':continue
  edgeLabel=s[e[j+ini]] if et[e[j+iti]] not in ['element','hidden'] else ''
  if 'part of key' in edgeLabel:continue
  v=e[j+toi]//k
  if parents[v]>=0:continue
  parents[v]=u;via[v]=j;q.append(v)
  if v in targets:found.add(v)
# 경로마다 간선 이름/종류와 실제 heap 노드 번호를 출력한다.
for target in sorted(targets):
 print('TARGET',target,s[n[target*k+ni]],'reachable',parents[target]>=0)
 if parents[target]<0:continue
 chain=[];v=target
 while v:
  j=via[v];name=e[j+ini];kind=et[e[j+iti]];label=str(name) if kind in ['element','hidden'] else s[name];chain.append((v,nt[n[v*k+ti]],s[n[v*k+ni]],kind,label));v=parents[v]
 for row in reversed(chain):print(row)
