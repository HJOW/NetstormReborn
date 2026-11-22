using System.Buffers.Binary;

// 현재 raw 필드/자산 상태/권한과 원문 Gun 단계 전환을 같은 실제 Pool에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawGunProcess.cpp의 생성·표적·프레임 본체다. 미선택 큰 장치는 미리 요구하지 않는다.</summary>
public sealed partial class RawGunProcess
{
    /// <summary>PATCH outer 기록용 가상 표다.</summary>
    private const uint PatchOuter = 0x50b668;
    /// <summary>CD outer 기록용 가상 표다.</summary>
    private const uint CdOuter = 0x501118;
    /// <summary>PATCH inner Baseprocess 기록용 가상 표다.</summary>
    private const uint PatchInner = 0x50b648;
    /// <summary>CD inner Baseprocess 기록용 가상 표다.</summary>
    private const uint CdInner = 0x5010f8;
    /// <summary>원문 float 검색 지연의 바이어스다.</summary>
    private const float PollBias = 0.4f;
    /// <summary>원문 float 검색 난수 구간의 상한이다.</summary>
    private const float PollRandom = 0.2f;
    /// <summary>부모/표적의 같은 실제 슬롯 물리다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>원문 vector 값 복사와 같은 불변 TYPE 사본이다.</summary>
    private readonly OriginalTypeRecord[] _types;
    /// <summary>현재 시각/두 권한/역할/방향별 동맹이다.</summary>
    private readonly GunProcessState _state;
    /// <summary>큰 효과의 함수 값 사본이다.</summary>
    private readonly GunProcessHooks _hooks;
    /// <summary>같은 Pool과 판본별 TYPE 크기를 확인하고 값형 표/장치만 복사한다.</summary>
    public RawGunProcess(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, GunProcessState state, GunProcessHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171)) throw new ArgumentException("GunProcess 타입 판본/크기 오류");
        _pool = pool; _types = types.Select(type => type.Clone()).ToArray(); _state = state; _hooks = hooks.Copy();
    }
    /// <summary>외부 Host/Kernel 결합에서 실제 물리 정체성을 확인한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>선택된 실제 효과만 요구하며 미복원을 noop으로 바꾸지 않는다.</summary>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("GunProcess 선택된 실제 효과 경계 미연결");
    /// <summary>정렬되지 않은 실제 DWORD를 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(bytes[offset..]);
    /// <summary>padding을 보존하며 지정 DWORD만 쓴다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[offset..], value);
    /// <summary>dirty를 추가하지 않고 실제64비트 시각만 쓴다.</summary>
    private static void PutTime(GunProcessFields fields, double value) => BinaryPrimitives.WriteUInt64LittleEndian(fields.Bytes.AsSpan(16), BitConverter.DoubleToUInt64Bits(value));
    /// <summary>시간 변경은 bit2를 OR하고 같은64비트 시각에 쓴다.</summary>
    private static void SetTime(GunProcessFields fields, double value) { fields.Bytes[8] |= 2; PutTime(fields, value); }
    /// <summary>단계 변경은 bit8을 OR하고 같은 DWORD에 쓴다.</summary>
    private static void SetStage(GunProcessFields fields, uint value) { fields.Bytes[8] |= 8; Put(fields.Bytes, 28, value); }
    /// <summary>표적 변경은 bit4를 OR하고 같은 WORD에만 쓴다.</summary>
    private static void SetTargetWord(GunProcessFields fields, ushort value) { fields.Bytes[8] |= 4; BinaryPrimitives.WriteUInt16LittleEndian(fields.Bytes.AsSpan(24), value); }
    /// <summary>현재 부모와 TYPE 물리 범위만 검사한다. 표적의 free/dead를 여기서 거부하지 않는다.</summary>
    private ReadOnlySpan<byte> Parent(GunProcessFields fields)
    {
        ushort parent = fields.Parent();
        if (parent == 0 || parent >= _pool.Capacity) throw new ArgumentOutOfRangeException(nameof(fields), "GunProcess 부모 SID 오류");
        ReadOnlySpan<byte> raw = _pool.Slot(parent);
        if (raw[10] >= _types.Length) throw new ArgumentOutOfRangeException(nameof(fields), "GunProcess 부모 TYPE 오류");
        return raw;
    }
    /// <summary>0046a1a0은 실제 초기화 뒤 flags0 부착 부모를 호출한다.</summary>
    public void Construct(GunProcessFields fields, ushort parent)
    {
        Action<GunProcessFields, ushort> attach = Need(_hooks.Attach); Initialize(fields, parent); attach(fields, parent);
    }
    /// <summary>실제 outer/inner VT·zero·현재 시각/group을 원문 순서로 쓰며 부착은 factory가 맡는다.</summary>
    public void Initialize(GunProcessFields fields, ushort parent)
    {
        if (parent == 0 || parent >= _pool.Capacity) throw new ArgumentException("GunProcess 생성 부모 SID 오류");
        int group = _types[_pool.Slot(parent)[10]].Group;
        if (group is not (0 or 1)) throw new InvalidOperationException("GunProcess 생성 group 원본 assert");
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        Put(fields.Bytes, 0, patch ? PatchOuter : CdOuter); Put(fields.Bytes, 40, patch ? PatchInner : CdInner);
        fields.Bytes[8] = 0xff; Put(fields.Bytes, 44, 0); Put(fields.Bytes, 48, 0);
        SetTargetWord(fields, 0); SetTime(fields, _state.Now); SetStage(fields, 0); fields.Bytes[8] |= 0x10; Put(fields.Bytes, 32, unchecked((uint)group));
    }
    /// <summary>현재 부착 통지는 대기3→2/6→5만 되돌리고 다른 상태는 보존한다.</summary>
    public static void Attached(GunProcessFields fields)
    {
        if (fields.Stage() == 3) SetStage(fields, 2);
        if (fields.Stage() == 6) SetStage(fields, 5);
    }
    /// <summary>현재 boss/server 관문 뒤 표적 감시를 제거하고 선택한 재시작/전송만 수행한다.</summary>
    public void ClearTarget(GunProcessFields fields, bool restart)
    {
        if (!_state.IsBoss()) return;
        if (!_state.IsServer()) throw new InvalidOperationException("GunProcess clear 서버 원본 assert");
        ushort old = fields.Target(); bool transmit = restart ? old != 0 || fields.Stage() != 1 : old != 0;
        if ((old != 0 && _hooks.RemoveDependency is null) || (transmit && _hooks.RepopForm is null)) throw new InvalidOperationException("GunProcess clear 실제 효과 경계 누락");
        if (old != 0) _hooks.RemoveDependency!(fields.Form(), old);
        SetTargetWord(fields, 0); if (restart) SetStage(fields, 1);
        if (transmit) _hooks.RepopForm!(fields);
    }
    /// <summary>현재 표적 감시 교체/단계2/실제 애니메이션 반복 조회·Kill/사거리/전송 순서다.</summary>
    public void SetTarget(GunProcessFields fields, ushort target)
    {
        if (!_state.IsServer() || _hooks.RemoveDependency is null || _hooks.AddDependency is null || _hooks.AnimationForm is null
            || _hooks.KillAnimation is null || _hooks.RefreshRange is null || _hooks.RepopForm is null) throw new InvalidOperationException("GunProcess 표적 지정 효과 누락");
        if (fields.Target() != 0) _hooks.RemoveDependency(fields.Form(), fields.Target());
        SetTargetWord(fields, target); _hooks.AddDependency(fields.Target(), fields); SetStage(fields, 2);
        if (_hooks.AnimationForm(fields.Parent()) != 0)
        {
            // 두 판본 모두 stop helper가 현재 애니메이션을 재검사하고 Kill 인자에서도 다시 읽는다.
            if (_hooks.AnimationForm(fields.Parent()) != 0) _hooks.KillAnimation(_hooks.AnimationForm(fields.Parent()), 0x40);
        }
        _hooks.RefreshRange(fields.Parent()); _hooks.RepopForm(fields);
    }
    /// <summary>PATCH checking의 현재 애니메이션 부재 진단만 수행한다.</summary>
    private void CheckNoAnimation(GunProcessFields fields)
    {
        if (_pool.Edition == OriginalEdition.Patch1078 && _state.Checking.IsEnabled)
        {
            if (_hooks.AnimationForm is null || _hooks.AnimationForm(fields.Parent()) != 0) throw new InvalidOperationException("GunProcess getAnimFormSid==0 원본 assert");
        }
    }
    /// <summary>0046b830의 현재 권한/표적/시간 관문 뒤 상태0~10의 한 단계만 진행한다.</summary>
    public void Frame(GunProcessFields fields)
    {
        ReadOnlySpan<byte> parent = Parent(fields); bool patch = _pool.Edition == OriginalEdition.Patch1078;
        int extra = patch ? 40 : 35, owner = patch ? 34 : 32;
        if (fields.Group() != _types[parent[10]].Group || (parent[extra] & 9) != 0) throw new InvalidOperationException("GunProcess Frame group/abstract/astral 원본 assert");
        if ((parent[extra] & 0x20) != 0 || _state.FixedMode || _state.Disabled) return;
        if (_state.IsBoss())
        {
            ushort target = fields.Target(); bool valid = target != 0 && target < _pool.Capacity; bool clear = !valid;
            if (valid)
            {
                ReadOnlySpan<byte> raw = _pool.Slot(target); byte first = parent[owner], second = raw[owner];
                if (first > 8 || second > 8) throw new ArgumentOutOfRangeException(nameof(fields), "GunProcess 동맹 소유자 범위 오류");
                clear = (raw[11] & 3) != 0 || (_state.IsAlly(first, second) && !_state.FriendlyFire.IsEnabled);
            }
            if (clear) ClearTarget(fields, false);
        }
        parent = Parent(fields);
        // PATCH는 unordered 시각도 반환하고 CD는 unordered에서 원문처럼 다음 상태로 진행한다.
        if (patch ? !(fields.Time() <= _state.Now) : _state.Now < fields.Time()) return;
        if ((parent[extra] & 1) != 0) return;
        byte type = parent[10]; uint stage = fields.Stage();
        switch (stage)
        {
            case 0:
                CheckNoAnimation(fields);
                if (patch && _state.Checking.IsEnabled && fields.Target() != 0) throw new InvalidOperationException("GunProcess idle 표적 원본 assert");
                break;
            case 1:
                if (patch && _state.Checking.IsEnabled && fields.Target() != 0) throw new InvalidOperationException("GunProcess 검색 표적 원본 assert");
                if (_state.IsServer() && (!_state.FortMode || _state.FriendlyFire.IsEnabled))
                {
                    if (_hooks.TargetingMode is null || _hooks.FindTarget is null || _hooks.RandomBetween is null) throw new InvalidOperationException("GunProcess 검색 경계 누락");
                    uint mode = _hooks.TargetingMode(fields.Parent(), parent[owner]);
                    ushort found = _hooks.FindTarget(BitConverter.UInt32BitsToSingle(Read(parent, 14)), BitConverter.UInt32BitsToSingle(Read(parent, 18)), type, mode, parent[owner]);
                    if (found != 0) SetTarget(fields, found);
                    else { SetTime(fields, _state.Now); double random = _hooks.RandomBetween(0, PollRandom); PutTime(fields, (random + (double)PollBias) + _state.Now); }
                }
                if (patch && _state.Checking.IsEnabled) Need(_hooks.ValidateIslandOwners)(fields.Parent());
                return;
            case 2:
                CheckNoAnimation(fields);
                if (fields.Target() != 0)
                {
                    int group = _types[Parent(fields)[10]].Group;
                    if ((group != 0 || type == _state.ReadWindArcherType()) && type != _state.ReadThunderCannonType() && type != _state.ReadRainCannonType())
                    { Need(_hooks.StartAnimation)(stage, fields.Form(), fields.Target()); SetStage(fields, 3); }
                    else SetStage(fields, 4);
                    return;
                }
                break;
            case 3: case 6: case 9: return;
            case 4:
                CheckNoAnimation(fields); fields.Bytes[8] |= 8;
                Put(fields.Bytes, 28, fields.Target() != 0 ? 5u : 1u); return;
            case 5:
                if (fields.Target() != 0) { CheckNoAnimation(fields); Need(_hooks.StartAnimation)(stage, fields.Form(), fields.Target()); SetStage(fields, 6); return; }
                break;
            case 7: CheckNoAnimation(fields); Fire(fields); return;
            case 8: Need(_hooks.StartAnimation)(8, fields.Form(), 0); SetStage(fields, 9); return;
            case 10: break;
            default: throw new InvalidOperationException("GunProcess 단계 원본 assert");
        }
        SetStage(fields, 1);
    }
}
