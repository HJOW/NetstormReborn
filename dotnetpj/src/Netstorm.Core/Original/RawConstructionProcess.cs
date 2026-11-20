using System.Buffers.Binary;

// 건설 프로세스의 원본 시작·HP 진행·완료·Form 삭제 수명을 같은 월드에서 실행한다.
namespace Netstorm.Core.Original;

/// <summary>원본0x38바이트 객체의 진행·전송·음향 의미 필드다.</summary>
public sealed class ConstructionProcessFields
{
    /// <summary>부모1·시작2·끝4·추가8의 전송 변경 비트다.</summary>
    public byte Dirty { get; set; } = 0xff;
    /// <summary>건설을 시작한 게임 시각이다.</summary>
    public double Start { get; set; }
    /// <summary>최대HP에 이르는 게임 시각이다.</summary>
    public double End { get; set; }
    /// <summary>현재 jimBuild.wav 소리 항목이며1초 이하 건설은0이다.</summary>
    public uint Sound { get; set; }
    /// <summary>전송 비트8이 선택하는 추가 바이트를 변환 없이 보존한다.</summary>
    public byte Additional { get; set; }
    /// <summary>부착한 프로세스가 별도로 소유할 필드 사본이다.</summary>
    public ConstructionProcessFields Copy() => (ConstructionProcessFields)MemberwiseClone();
}

/// <summary>하위 효과 뒤에도 다시 읽는 현재 게임 전역과 독립 검사값이다.</summary>
public sealed class ConstructionProcessState
{
    /// <summary>독립 실행의 현재 시각이다.</summary>
    public double Now { get; set; }
    /// <summary>실제 세션 시계를 소비 위치마다 읽는 장치다.</summary>
    public Func<double>? CurrentTime { get; set; }
    /// <summary>독립 건설 프로세스 타입이다.</summary>
    public uint ProcessType { get; set; } = 30;
    /// <summary>장기 소유자의 현재 건설 프로세스 타입이다.</summary>
    public Func<uint>? CurrentProcessType { get; set; }
    /// <summary>완료 때 삭제할 진행음 프로세스 타입이다.</summary>
    public uint SoundProcessType { get; set; } = 47;
    /// <summary>현재 로컬 소유자 번호의 독립값이다.</summary>
    public uint LocalPlayer { get; set; } = 1;
    /// <summary>실제 세션 로컬 소유자를 읽는다.</summary>
    public Func<uint>? CurrentLocalPlayer { get; set; }
    /// <summary>독립 서버 권한이다.</summary>
    public bool Boss { get; set; } = true;
    /// <summary>HP·표시·음향 뒤 현재 권한을 다시 읽는다.</summary>
    public Func<bool>? SharedBoss { get; set; }
    /// <summary>완성한 발자국의 유닛을 밀어낼 genus 마스크다.</summary>
    public uint PushGenus { get; set; } = 0x10444200;
    /// <summary>다른 배치·충돌 모듈과 같은 현재 마스크를 읽는다.</summary>
    public Func<uint>? SharedPushGenus { get; set; }
    /// <summary>Form 삭제 때HP를 채우지 않는 받침TYPE다.</summary>
    public uint DaisType { get; set; } = 165;
    /// <summary>모든 건설을 한 프레임에 마치는 현재 비영 뷰다.</summary>
    public RawCheckingFlag QuickBuild { get; set; } = new();
    /// <summary>신전genus0x200만 즉시 완료하는 현재 비영 뷰다.</summary>
    public RawCheckingFlag QuickVortex { get; set; } = new();
    /// <summary>PATCH Run 입구의 현재 Form 연결 진단이다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();
    /// <summary>장기 시계가 연결되면 독립값 대신 현재값을 읽는다.</summary>
    public double Time() => CurrentTime?.Invoke() ?? Now;
    /// <summary>부착 시점의 현재 프로세스 타입이다.</summary>
    public uint Type() => CurrentProcessType?.Invoke() ?? ProcessType;
    /// <summary>각 완료 관문의 현재 권한이다.</summary>
    public bool IsBoss() => SharedBoss?.Invoke() ?? Boss;
    /// <summary>완료 효과 뒤의 현재 밀어내기 마스크다.</summary>
    public uint PushMask() => SharedPushGenus?.Invoke() ?? PushGenus;
    /// <summary>갱신 효과 뒤의 현재 로컬 소유자다.</summary>
    public uint Local() => CurrentLocalPlayer?.Invoke() ?? LocalPlayer;
}

/// <summary>건설 밖의 실제 표시·예약·음향·공간 효과다. 실제 Host가 있으면 Attach는 생략할 수 있다.</summary>
public sealed class ConstructionProcessHooks
{
    /// <summary>원본0x38바이트 프로세스 예약의 성공 여부다.</summary>
    public Func<bool>? Reserve { get; set; }
    /// <summary>부모의 건설 표시를 생성하며0초 입력은 true다.</summary>
    public Action<int, bool>? Effect { get; set; }
    /// <summary>필드·타입·부모·flags0x10으로 부착하고 현재 소유 필드를 반환한다.</summary>
    public Func<ConstructionProcessFields, uint, int, uint, ConstructionProcessFields?>? Attach { get; set; }
    /// <summary>현재 소리 이름을 항목 번호로 조회한다.</summary>
    public Func<string, uint>? LoadSound { get; set; }
    /// <summary>원본0x28바이트 진행음 프로세스 예약이다.</summary>
    public Func<bool>? ReserveSound { get; set; }
    /// <summary>부모·진행음·대체음0·부모 위치 반복flags1을 부착한다.</summary>
    public Action<int, uint, uint, uint>? AttachSound { get; set; }
    /// <summary>화면 갱신을 요청한다.</summary>
    public Action? Refresh { get; set; }
    /// <summary>현재 부모의 실제 최대HP다.</summary>
    public Func<int, int>? MaxHitPoints { get; set; }
    /// <summary>HP 쓰기 뒤 부모의 가상 표시를 갱신한다.</summary>
    public Action<int>? UpdateDisplay { get; set; }
    /// <summary>현재 부모에서 지정 타입의 첫contained를 삭제한다.</summary>
    public Action<int, uint>? RemoveContained { get; set; }
    /// <summary>현재 위치에서 이름으로 소리를 한 번 낸다.</summary>
    public Action<float, float, string, uint>? PlayAt { get; set; }
    /// <summary>현재TYPE의 준비 소리 항목이다.</summary>
    public Func<uint, uint>? ReadySound { get; set; }
    /// <summary>지정 소리 항목을 전역으로 한 번 재생한다.</summary>
    public Action<uint>? Play { get; set; }
    /// <summary>실제 Unpop은void 상태를 켜야 한다.</summary>
    public Action<int, uint>? Unpop { get; set; }
    /// <summary>abstract 해제 뒤flags9로 현재 공간에 재등록한다.</summary>
    public Action<int, uint>? Repop { get; set; }
    /// <summary>완성한 발자국의 유닛을 현재 위치·TYPE·소유자로 밀어낸다.</summary>
    public Action<float, float, uint, uint>? Push { get; set; }
    /// <summary>선택된 진단에서 현재Form의 부모 체인 연결을 확인한다.</summary>
    public Func<int, bool>? FormRobust { get; set; }
    /// <summary>Construction.cpp1c4 원문 진단이며 복귀하면HP 진행을 계속한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
    /// <summary>C++ 값형 갈고리처럼 대리자 사본을 보관한다.</summary>
    public ConstructionProcessHooks Copy() => (ConstructionProcessHooks)MemberwiseClone();
}

/// <summary>00443fe0·00443c40·004428c0·00443e20·00444030의 실제 건설 수명이다.</summary>
/// <remarks>근거: cpppj/src/o/RawConstructionProcess.cpp·h. 2026-11-20 0.670 이식.</remarks>
public sealed class RawConstructionProcess
{
    /// <summary>PATCH와CD가 함께 쓰는0초 입력의 실제 기본 시간이다.</summary>
    private const double DefaultSeconds = 10.0;
    /// <summary>현재 부모HP·abstract를 저장하는 같은 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>경계 뒤 부모의 현재genus를 재조회하는 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>같은 장기 게임 전역이다.</summary>
    private readonly ConstructionProcessState _state;
    /// <summary>독립 효과 함수 사본이다.</summary>
    private readonly ConstructionProcessHooks _hooks;
    /// <summary>있으면 실제Kernel 프로세스와Form을 부착한다.</summary>
    private readonly SquidProcessHost? _host;

    /// <summary>첫 예약·쓰기 전에 판본별TYPE 폭과 필수 효과·같은Host 풀을 확인한다.</summary>
    public RawConstructionProcess(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, ConstructionProcessState state,
        ConstructionProcessHooks hooks, SquidProcessHost? host = null)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) ||
            (host is not null && !ReferenceEquals(pool, host.Pool())) || hooks.Reserve is null || hooks.Effect is null ||
            (hooks.Attach is null && host is null) || hooks.LoadSound is null || hooks.ReserveSound is null ||
            hooks.AttachSound is null || hooks.Refresh is null || hooks.MaxHitPoints is null || hooks.UpdateDisplay is null ||
            hooks.RemoveContained is null || hooks.PlayAt is null || hooks.ReadySound is null || hooks.Play is null ||
            hooks.Unpop is null || hooks.Repop is null || hooks.Push is null)
            throw new ArgumentException("건설 프로세스 자료/효과 연결 오류");
        _pool = pool; _types = types; _state = state; _hooks = hooks.Copy(); _host = host;
    }

    /// <summary>예약 실패는 효과 없이 끝내며 성공하면 현재 상태로 생성한다.</summary>
    public void Start(int parent, float duration)
    {
        if (!_hooks.Reserve!()) return;
        Construct(parent, duration);
    }

    /// <summary>최대HP 조회 뒤 현재 시각+10초로 끝만 바꾸며 시작·소리·다른dirty 비트를 보존한다.</summary>
    public void ResetEnd(ConstructionProcessFields fields, int parent, uint damage)
    {
        int maximum = _hooks.MaxHitPoints!(parent), candidate = maximum;
        if (!Patch)
        {
            int progress = Ftol(((_state.Time() - fields.Start) / (fields.End - fields.Start)) * maximum);
            candidate = unchecked((int)((uint)progress - damage));
            if (candidate <= 1) candidate = 1;
        }
        // CD helper의 부모·후보HP 인자는 원문이 읽지 않는다.
        _ = candidate;
        fields.End = _state.Time() + DefaultSeconds; fields.Dirty |= 4;
    }

    /// <summary>같은 실제Host의 현재Form에서 건설 프로세스를 찾아 끝 시각을 변경한다.</summary>
    public void ResetEndFromForm(int form, uint damage)
    {
        if (_host is null) throw new InvalidOperationException("건설 끝 시각 변경에 프로세스 host가 필요합니다");
        if (_host.FromForm(form) is not ConstructionProcess process || process.Form() != form)
            throw new ArgumentException("건설 끝 시각 변경의 form 연결 오류", nameof(form));
        ResetEnd(process.Fields, process.Parent(), damage);
    }

    /// <summary>현재 시각의HP를 계산하고 표시 뒤abstract·권한·Kill 후 생존 여부를 다시 읽는다.</summary>
    public void Run(ConstructionProcessFields fields, int parent, Func<uint, bool> kill,
        Func<int>? currentForm = null, Func<int>? currentParent = null)
    {
        if (Patch && _state.Checking.IsEnabled)
        {
            int form = (currentForm ?? throw new InvalidOperationException("건설 현재 Form 조회 장치 미연결"))();
            if (_hooks.FormRobust is null) throw new InvalidOperationException("건설 현재 Form 연결 진단 장치 미연결");
            if (!_hooks.FormRobust(form))
                (_hooks.Assertion ?? throw new InvalidOperationException("건설 Form 원문 진단 미연결"))(
                    "getForm().isRobust()", "\\Ns\\O\\Construction.cpp", 0x1c4);
        }
        if (currentParent is not null) parent = currentParent();
        int maximum = _hooks.MaxHitPoints!(parent);
        int value = Ftol(((_state.Time() - fields.Start) / (fields.End - fields.Start)) * maximum);
        if (_state.QuickBuild.IsEnabled || (_state.QuickVortex.IsEnabled && (Genus(parent) & 0x200) != 0)) value = maximum;
        if (value > maximum) value = maximum;
        if (value < 0) value = 0;
        RawSquidHitPoints.Set(_pool, parent, value);
        _hooks.UpdateDisplay!(parent);
        if ((_pool.Slot(parent)[Extra] & 1) == 0 && !kill(0)) return;
        if (value == maximum && _state.IsBoss()) _ = kill(0);
    }

    /// <summary>진행음 삭제·갱신·완료음 뒤 현재 권한으로abstract를 풀고 실제 공간에 재등록한다.</summary>
    public void Complete(int parent)
    {
        int maximum = _hooks.MaxHitPoints!(parent), hitPoints = HitPoints(parent);
        _hooks.RemoveContained!(parent, _state.SoundProcessType);
        _hooks.Refresh!();
        if (_pool.Slot(parent)[Owner] == _state.Local() && Confirmed(parent))
        {
            _hooks.PlayAt!(Float(parent, 14), Float(parent, 18), "buildDone.wav", 0);
            uint ready = _hooks.ReadySound!(TypeOf(parent));
            if (ready != 0) _hooks.Play!(ready);
        }
        if (!_state.IsBoss() || !Confirmed(parent) || hitPoints != maximum || (_pool.Slot(parent)[Extra] & 1) == 0) return;
        _hooks.Unpop!(parent, 0);
        if ((_pool.Slot(parent)[11] & 4) == 0) throw new InvalidOperationException("건설 완료의 abstract 해제는 void 상태여야 합니다");
        _pool.AllocatedBytes(parent)[Extra] &= 0xfe;
        _hooks.Repop!(parent, 9);
        if ((_state.PushMask() & Genus(parent)) == 0) return;
        _hooks.Push!(Float(parent, 14), Float(parent, 18), _pool.Slot(parent)[10], _pool.Slot(parent)[Owner]);
    }

    /// <summary>완료 뒤 현재free/dead·받침 여부로 마지막HP 채움을 결정한다.</summary>
    public void FormDestroyed(int parent)
    {
        Complete(parent);
        if ((_pool.Slot(parent)[11] & 3) != 0 || _pool.Slot(parent)[10] == _state.DaisType) return;
        RawSquidHitPoints.Set(_pool, parent, _hooks.MaxHitPoints!(parent));
    }

    /// <summary>같은 풀의 보상 장치로 최대HP를 연결하며 다른 효과는 사본으로 보존한다.</summary>
    public static ConstructionProcessHooks MakeRewardHooks(RawSidPool pool, RawSquidReward reward, ConstructionProcessHooks hooks)
        => MakeHooks(pool, reward, null, hooks);

    /// <summary>같은 풀의 실제 보상·선택적 밀어내기를 연결하고 나머지 효과는 사본으로 보존한다.</summary>
    public static ConstructionProcessHooks MakeHooks(RawSidPool pool, RawSquidReward reward, RawConstructionEffects? effects, ConstructionProcessHooks hooks)
    {
        if (!ReferenceEquals(pool, reward.Pool) || (effects is not null && !ReferenceEquals(pool, effects.Pool)))
            throw new ArgumentException("건설 프로세스 어댑터의 풀 연결 오류");
        ConstructionProcessHooks result = hooks.Copy(); result.MaxHitPoints = reward.MaxHitPoints;
        if (effects is not null) result.Push = (x, y, type, owner) => { _ = effects.Displace(x, y, type, owner); };
        return result;
    }
    /// <summary>실제 월드 조립의 풀 동일성 확인용이다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>현재 풀의 실제 판본이다.</summary>
    private bool Patch => _pool.Edition == OriginalEdition.Patch1078;
    /// <summary>판본별 소유자BYTE 위치다.</summary>
    private int Owner => Patch ? 34 : 32;
    /// <summary>판본별abstract BYTE 위치다.</summary>
    private int Extra => Patch ? 40 : 35;
    /// <summary>현재 부모TYPE를 읽고 표 경계를 확인한다.</summary>
    private uint TypeOf(int sid)
    {
        uint type = _pool.Slot(sid)[10];
        if (type >= (uint)_types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "건설 프로세스 부모 타입 오류");
        return type;
    }
    /// <summary>효과 뒤 부모의 현재genus를 다시 조회한다.</summary>
    private uint Genus(int sid) => _types[(int)TypeOf(sid)].Flags2;
    /// <summary>PATCH는signed DWORD, CD는signed WORD로 현재HP를 읽는다.</summary>
    private int HitPoints(int sid) => Patch ? BinaryPrimitives.ReadInt32LittleEndian(_pool.Slot(sid).Slice(26, 4))
        : BinaryPrimitives.ReadInt16LittleEndian(_pool.Slot(sid).Slice(26, 2));
    /// <summary>원본 판본의 예측 영역만 제외한다. 관리CD 하네스 할당 경계와 구별한다.</summary>
    private bool Confirmed(int sid) => sid < 5 || sid >= (Patch ? RawSidPool.ServerFirst : RawSidPool.CdServerFirst);
    /// <summary>원본fistp64의 하위signed DWORD를 유지하며 비유한·범위 밖은0이다.</summary>
    private static int Ftol(double value) => !double.IsFinite(value) || Math.Abs(value) >= 9223372036854775808.0
        ? 0 : unchecked((int)(uint)(long)value);
    /// <summary>현재 저장 좌표의float 비트를 변환한다.</summary>
    private float Float(int sid, int offset) => BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(sid).Slice(offset, 4)));

    /// <summary>표시 효과 뒤 현재 시각으로 필드를 만들고 실제 부착 뒤 진행음을 연결한다.</summary>
    private void Construct(int parent, float duration)
    {
        var fields = new ConstructionProcessFields(); bool instant = duration == 0f;
        _hooks.Effect!(parent, instant);
        fields.Start = _state.Time(); fields.Dirty |= 2;
        if (instant) { fields.End = _state.Time() + DefaultSeconds; fields.Dirty |= 4; duration = (float)DefaultSeconds; }
        fields.End = (double)duration + _state.Time(); fields.Dirty |= 4;
        ConstructionProcessFields? attached;
        if (_host is not null)
        {
            var process = (ConstructionProcess?)_host.Attach(new ConstructionProcess(this, fields), _state.Type(), parent, 0x10);
            attached = process?.Fields;
        }
        else attached = _hooks.Attach!(fields, _state.Type(), parent, 0x10);
        ConstructionProcessFields live = attached ?? fields;
        bool audible = Patch ? duration > 1f : BitConverter.SingleToInt32Bits(duration) > BitConverter.SingleToInt32Bits(1f);
        if (audible)
        {
            live.Sound = _hooks.LoadSound!("jimBuild.wav");
            if (_hooks.ReserveSound!()) _hooks.AttachSound!(parent, live.Sound, 0, 1);
        }
        else live.Sound = 0;
        _hooks.Refresh!();
    }
}

/// <summary>실제Kernel이 소유하고 부모Form 삭제 통지로 완료하는 건설 프로세스다.</summary>
public sealed class ConstructionProcess : SquidProcess
{
    /// <summary>현재 프로세스보다 오래 유지할 원본 진행 본체다.</summary>
    private readonly RawConstructionProcess _logic;
    /// <summary>부착 뒤 프로세스가 소유하는 원본 필드다.</summary>
    public ConstructionProcessFields Fields { get; }
    /// <summary>시작이 계산한 초기 필드를 복사한다.</summary>
    public ConstructionProcess(RawConstructionProcess logic, ConstructionProcessFields fields)
    {
        _logic = logic; Fields = fields.Copy();
    }
    /// <summary>같은Kernel의 다른 건설 owner를 보존하도록 현재 진행 본체의 정체성을 확인한다.</summary>
    public bool BelongsTo(RawConstructionProcess logic) => ReferenceEquals(_logic, logic);
    /// <summary>Kill 후 등록 여부는 미리 읽은Host·PID로 확인하며 제거된 멤버를 다시 읽지 않는다.</summary>
    public override void RunFrame()
    {
        SquidProcessHost? host = Host(); if (host is null) return;
        uint pid = Pid();
        _logic.Run(Fields, 0, flags => { host.Kill(this, flags); return host.IsRegistered(pid, this); }, Form, Parent);
    }
    /// <summary>원본이 읽지 않는flags는 전달하지 않고 현재 부모의 완료를 실행한다.</summary>
    public override void OnFormDestroy(uint flags) => _logic.FormDestroyed(Parent());
}
