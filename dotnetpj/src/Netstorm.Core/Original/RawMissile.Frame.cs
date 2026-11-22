// 실제미사일의현재raw주소/수명/충돌/피해와원문효과순서를보존한다.
namespace Netstorm.Core.Original;

/// <summary>RawMissile.cpp의두진단관문과판본별전체Frame본문이다.</summary>
public sealed partial class RawMissile
{
    /// <summary>진단이실제객체를지우면현재raw식별자를다시읽지않는다.</summary>
    private bool CheckBase(MissileFields fields,PathLifeWatch? watch)
    {
        // 호출중삭제를객체바깥의관찰상태로확인한다.
        bool Alive()=>watch?.Alive??true;
        if(!Alive())return false;if(!_state.Checking.IsEnabled)return true;RawSidPool pool=Pool;var effects=new PathBaseRobustHooks{Form=()=>Alive()?Read(fields.Bytes,4):0,Parent=()=>Alive()?Read(fields.Bytes,8):0,Capacity=()=>(uint)pool.Capacity,Assertion=_hooks.Assertion};_=RawPathArrivalParent.Robust(effects);return Alive();
    }
    /// <summary>최초표시SID/raw주소와두번째진단뒤현재발사자를구별해실제프레임을진행한다.</summary>
    public void Frame(MissileFields fields,PathLifeWatch? watch=null)
    {
        bool patch=Pool.Edition==OriginalEdition.Patch1078;if(patch&&!CheckBase(fields,watch))return;
        // 실제Host에서지워진프로세스의원문필드를후속조회하지않는다.
        bool Alive()=>watch?.Alive??true;
        bool reflected=patch&&Read(fields.Bytes,12)!=0;Layout layout=patch?PatchLayout:CdLayout;ushort assetSid=fields.Parent();ReadOnlySpan<byte> firing=Slot(fields.Firing()),target=Slot(fields.Target(),reflected),asset=Slot(assetSid);bool archer=firing[10]==_state.ReadSunArcherType(),groupZero=archer||(patch&&firing[10]==_state.ReadCrossbowType()),cannon=firing[10]==_state.ReadSunCannonType()||(patch&&(firing[10]==_state.ReadThunderCannonType()||firing[10]==_state.ReadRainCannonType()));if(!reflected&&((!groupZero&&!cannon)||Read(fields.Bytes,12)!=0))throw new InvalidOperationException("미사일 미복원Frame파생/반사 경계");
        if(patch&&Read(fields.Bytes,40)!=0){Put(fields.Bytes,40,0);Put(fields.Bytes,44,1);}float margin=patch&&asset[10]==_state.ThunderMissileType?_state.ThunderRangeMargin:reflected?0:_state.RangeMargin;
        Action<ushort,uint> unpop=Need(_hooks.Unpop);Action<ushort,float,float,uint> pop=Need(_hooks.Pop);Action<ushort,uint> destroy=Need(_hooks.Destroy);if(!reflected)_=Need(_hooks.Center);
        float[] origin=Point(fields.Bytes,layout.Origin),endpoint=Point(fields.Bytes,layout.Endpoint);double end=D(fields.Bytes,layout.End),start=D(fields.Bytes,layout.Start),remaining=patch?end-_state.Now:(float)(end-_state.Now),duration=patch?end-start:(float)(end-start);if(!patch&&!(duration>0))throw new InvalidOperationException("CD 미사일0거리Frame 미복원경계");float dx=(float)((double)endpoint[0]-origin[0]),dy=(float)((double)endpoint[1]-origin[1]);double factor=patch?1.0/duration:(double)dx/duration;
        float[] current=[(float)((double)endpoint[0]-(patch?(double)dx*factor:factor)*remaining),(float)((double)endpoint[1]-(patch?factor*(double)dy:(double)dy/duration)*remaining)];
        if(!Valid(current)){if(patch&&Read(fields.Bytes,44)!=0){Need(_hooks.ImpactSound)(current[0],current[1],_missileTypes[(int)_state.CornerImpactType].ImpactSound);if(!Alive())return;}destroy(assetSid,0);return;}
        unpop(assetSid,0);if(!Alive())return;pop(assetSid,current[0],current[1],0);if(!Alive())return;bool corner=Read(fields.Bytes,28)!=0;
        if(fields.Target()!=0&&(target[11]&3)==0)
        {
            if(corner){float[] point=Point(fields.Bytes,layout.Endpoint);F(fields.Bytes,layout.Box,point[0]-1);F(fields.Bytes,layout.Box+4,point[1]-1);F(fields.Bytes,layout.Box+8,point[0]);F(fields.Bytes,layout.Box+12,point[1]);}
            else{float[] bounds=Need(_hooks.TargetBounds)(fields.Target(),Point(fields.Bytes,layout.Position)).ToArray();float allowance=F(fields.Bytes,layout.Allowance);F(fields.Bytes,layout.Box,bounds[0]-allowance);F(fields.Bytes,layout.Box+4,bounds[1]-allowance);F(fields.Bytes,layout.Box+8,bounds[2]+allowance);F(fields.Bytes,layout.Box+12,bounds[3]+allowance);}
        }
        float[] previous=Point(fields.Bytes,layout.Previous);bool hit=false;
        if(Valid(previous)&&Read(fields.Bytes,16)==0&&(target[patch?40:35]&0x10)==0)
        {
            float[] low=[Math.Min(previous[0],current[0]),Math.Min(previous[1],current[1])],high=[Math.Max(previous[0],current[0]),Math.Max(previous[1],current[1])],boxLow=Point(fields.Bytes,layout.Box),boxHigh=Point(fields.Bytes,layout.Box+8);hit=low[0]<=boxHigh[0]&&boxLow[0]<=high[0]&&low[1]<=boxHigh[1]&&boxLow[1]<=high[1];
        }
        float[] currentCenter=reflected?origin:Need(_hooks.Center)(fields.Firing()).ToArray();float currentDistance=(float)Distance(current,currentCenter);bool exhausted;
        if(reflected)exhausted=_missileTypes[asset[10]].Range+margin<currentDistance;
        else if(corner){float[] endCenter=Need(_hooks.Center)(fields.Firing()).ToArray();exhausted=patch&&!groupZero?Distance(endpoint,endCenter)<currentDistance:(float)Distance(endpoint,currentCenter)+margin<Distance(current,endCenter);}
        else{float range=_missileTypes[firing[10]].Range;exhausted=patch?range+margin<currentDistance:range+margin<Distance(current,currentCenter);}
        float[] last=current;
        if(hit&&patch&&target[10]==_state.FloaterType){double roll=Need(_hooks.RandomFloat)(0,1000);if(roll<_state.AvoidanceCutoff){hit=false;Put(fields.Bytes,16,1);}}
        if(hit)
        {
            last=[(float)(((double)current[0]+previous[0])*.5),(float)(((double)current[1]+previous[1])*.5)];
            if(fields.Target()!=0&&(target[11]&1)==0)
            {
                if(corner){Need(_hooks.Impact)(last[0],last[1],2,.02f);uint sound=_missileTypes[(int)_state.CornerImpactType].ImpactSound;Need(_hooks.ImpactSound)(last[0],last[1],sound);}
                else if(patch&&asset[10]==_state.IceMissileType){Need(_hooks.Impact)(last[0],last[1],2,.02f);if(_missileTypes[asset[10]].ImpactSound!=0){Func<int,int,int> random=Need(_hooks.RandomInteger);Action<float,float,string> sound=Need(_hooks.ImpactName);int number=random(1000,5000);sound(last[0],last[1],"iceCannonImpact"+(number/1000).ToString(System.Globalization.CultureInfo.InvariantCulture)+".wav");}}
                else{Need(_hooks.HitEffect)(last[0],last[1]);if(_missileTypes[asset[10]].ImpactSound!=0&&(!patch||!exhausted))Need(_hooks.ImpactSound)(last[0],last[1],_missileTypes[asset[10]].ImpactSound);}
            }
            if(patch&&asset[10]==_state.IceMissileType&&!corner)Need(_hooks.Ricochet)(fields);
            if(patch&&asset[10]==_state.ThunderMissileType&&target[10]==_state.StoneTowerType)Need(_hooks.QueueCommand)(_state.HitCommand,_state.ThunderMissileType,_state.StoneTowerType);
        }
        if(patch&&!CheckBase(fields,watch))return;Point(fields.Bytes,layout.Previous,last);if(!hit&&!exhausted)return;ReadOnlySpan<byte> finishingFiring=patch?Slot(fields.Firing(),true):firing;bool liveFiring=(finishingFiring[11]&1)==0;int firingGroup=_types[finishingFiring[10]].Group;
        if(liveFiring&&(firingGroup==0||firingGroup==1)){if(hit&&!corner)Need(_hooks.Hit)(fields.Firing(),fields.Target());else Need(_hooks.Refresh)(fields.Firing());}
        else if(patch&&(liveFiring||(hit&&!corner)))
        {
            MissileTypeData damageType=_missileTypes[liveFiring?finishingFiring[10]:(int)_state.ReadRainCannonType()];float damage=(float)((double)damageType.DamagePerSecond*(double).6f);if(_missileTypes[finishingFiring[10]].SpecialFlyerDamage&&(_types[target[10]].Flags2&0x120000)!=0)damage=_missileTypes[finishingFiring[10]].FlyerDamagePerSecond;bool eligible=fields.Target()!=0&&(liveFiring?(target[11]&2)!=0&&(target[11]&1)==0:(target[11]&3)==0);if(eligible)Need(_hooks.Damage)(fields.Target(),DamageInteger(damage),asset[34],0);
        }
        if(patch&&!hit&&asset[10]==_state.IceMissileType)Need(_hooks.Impact)(last[0],last[1],2,.02f);if(patch&&Read(fields.Bytes,44)!=0)Need(_hooks.ImpactSound)(last[0],last[1],_missileTypes[(int)_state.CornerImpactType].ImpactSound);destroy(assetSid,0);
    }
}
