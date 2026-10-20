namespace Netstorm.Core.Original;

/// <summary>
/// 원본 Player 의 신전 네 필드를 유지한다. 복사본을 읽지 않도록 실행 월드가 이 저장소를 공유한다.
/// </summary>
public sealed class AssetPostPopPlayer
{
    /// <summary>초기 신전.</summary>
    public uint InitialTemple { get; set; }

    /// <summary>초기 신전 복사.</summary>
    public uint InitialTempleCopy { get; set; }

    /// <summary>새 신전.</summary>
    public uint NewTemple { get; set; }

    /// <summary>새 신전 복사.</summary>
    public uint NewTempleCopy { get; set; }
}

/// <summary>
/// Gun·거주 구역·간헐천·신전 재정의가 쓰는 전역과 목록(원본 AssetPostPopState).
/// 이 인스턴스는 모듈보다 오래 살아야 하며 호출자가 직접 채운다.
/// </summary>
public sealed class AssetPostPopState
{
    /// <summary>내부 전투 여부.</summary>
    public bool InBattle { get; set; }

    /// <summary>보스 여부. 켜지면 신전이 같은 영역의 다른 비매립 신전을 가상 삭제한다.</summary>
    public bool Boss { get; set; } = true;

    /// <summary>요새 모드 여부. 꺼진 썬 디스크 발사기 Gun 의 원본 assert 경로를 가린다.</summary>
    public bool FortMode { get; set; }

    /// <summary>검사 Form 여부. 켜지면 Gun 생성 뒤 checkForms assert 를 검사한다.</summary>
    public bool CheckingForms { get; set; }

    /// <summary>거주 구역 애니메이션 여부. 켜지면 잔여 프레임을 2로 나눈 나머지와 묶는다.</summary>
    public bool ResidenceAnimation { get; set; }

    /// <summary>로딩 깊이. 0이 아니면 최초 처리를 건너뛰는 쪽으로 분기한다.</summary>
    public uint LoadingDepth { get; set; }

    /// <summary>신전 개수. 신전 공통 몸체가 최초 등록마다 1 늘린다.</summary>
    public uint TempleCount { get; set; }

    /// <summary>현재 선택한 객체·컨트롤의 번호 또는 토큰. 신전의 배치 취소 조건에 쓴다.</summary>
    public uint Selected { get; set; }

    /// <summary>로컬 소유자 번호.</summary>
    public byte LocalOwner { get; set; } = 1;

    /// <summary>섬 자산을 타입 표에서 찾는 번호.</summary>
    public uint IslandType { get; set; } = 94;

    /// <summary>섬 받침 자산을 타입 표에서 찾는 번호.</summary>
    public uint StalagType { get; set; } = 95;

    /// <summary>생산 칸 자산을 타입 표에서 찾는 번호.</summary>
    public uint SlotType { get; set; } = 133;

    /// <summary>소유자별 Player 신전 기록(0~8번). 바깥 기록을 묶으면 그쪽을 쓰고 이 사본은 쓰지 않는다.</summary>
    public AssetPostPopPlayer[] Players { get; } = Enumerable.Range(0, 9).Select(_ => new AssetPostPopPlayer()).ToArray();

    /// <summary>거주 구역 목록.</summary>
    public RawPostPopList Residences { get; } = new();

    /// <summary>신전 목록.</summary>
    public RawPostPopList Temples { get; } = new();

    /// <summary>간헐천 목록.</summary>
    public RawPostPopList Geysers { get; } = new();

    /// <summary>신전·아웃포스트의 섬 소유권 재계산에 쓰는 별도 목록.</summary>
    public RawPostPopList Outposts { get; } = new();
}

/// <summary>
/// 외부 몸체를 생략하지 않고 호출 순서대로 연결하는 명시 경계(원본 AssetPostPopHooks).
/// 필요한 경계가 없으면 공간 쓰기 전에 사전 검사에서 거부한다.
/// </summary>
public sealed class AssetPostPopHooks
{
    /// <summary>원본 각각의 확보(new 0x28) 성공 여부다.</summary>
    public Func<bool>? ReserveRegular { get; set; }

    /// <summary>실제 Regular 생성자에 사건·번호·지연을 넘긴다.</summary>
    public Action<int, uint, float>? AddRegular { get; set; }

    /// <summary>생성·GunProcess 생성·부착의 전체 경계다. 바리케이드 최초·Gun 분기에서 쓴다.</summary>
    public Action<int>? CreateGunProcess { get; set; }

    /// <summary>실제 0045ec50 의 생성·부착·Damageable 전체 몸체다(훨리베이스).</summary>
    public Action<int, uint>? SunAviaryPostPop { get; set; }

    /// <summary>실제 0045ed10 의 편집 관문·타입 60 생성·부착·Damageable 전체 몸체다(맨 오 워 풀).</summary>
    public Action<int, uint>? RainAviaryPostPop { get; set; }

    /// <summary>썬 디스크 발사기의 현재 애니메이션을 제거한다.</summary>
    public Action<int>? RemoveAnimation { get; set; }

    /// <summary>켜진 진단의 checkForms assert 를 검사한다.</summary>
    public Action<int>? CheckForms { get; set; }

    /// <summary>현재 총 각도에서 실제 총 애니메이션 프레임을 조회한다.</summary>
    public Func<int, int>? GunFrame { get; set; }

    /// <summary>그래프 바이트(+30)를 쓴다(원본 004ad3e0).</summary>
    public Action<int, int>? SetGraph { get; set; }

    /// <summary>거주 구역의 현재 지면 번호를 조회한다.</summary>
    public Func<float, float, int>? Surface { get; set; }

    /// <summary>가상 +0x74. 소용돌이·거주 구역 파생 분배를 포함한다.</summary>
    public Action<int, uint>? SetOwner { get; set; }

    /// <summary>연결을 재평가하는 004214a0 이다.</summary>
    public Action<int>? NotifySurface { get; set; }

    /// <summary>비로딩 바리케이드의 0046ae80 대응 몸체다.</summary>
    public Action<int>? StartBlocker { get; set; }

    /// <summary>간헐천 등록 00416720(번호, 1)이다.</summary>
    public Action<int, uint>? GeyserRegistration { get; set; }

    /// <summary>실제 한 점 반복자 순서의 주변 번호 목록이다.</summary>
    public Func<float, float, int[]>? AtPosition { get; set; }

    /// <summary>가상 +0x48 이다.</summary>
    public Action<int, uint>? Unpop { get; set; }

    /// <summary>가상 +0x4c 이다.</summary>
    public Action<int, uint>? Repop { get; set; }

    /// <summary>현재 타입의 +0x74·+0x130·+0x134 을 읽는다.</summary>
    public Func<int, uint, uint>? TypeDword { get; set; }

    /// <summary>타입 +0x74 충격음 토큰을 실제 소리 재생에 넘긴다.</summary>
    public Action<uint>? PlayImpactSound { get; set; }

    /// <summary>선택된 소용돌이 부류의 배치 커서를 취소한다.</summary>
    public Action? ClearPlacement { get; set; }

    /// <summary>현재 지면의 섬 번호다.</summary>
    public Func<float, float, int>? IslandNumber { get; set; }

    /// <summary>실제 섬 목록 존재 검사다.</summary>
    public Func<int, bool>? IslandExists { get; set; }

    /// <summary>섬의 소용돌이 조회다. CD 의 기존 섬 신전 assert 에만 쓴다(10.78 경로에서 쓰지 않는다).</summary>
    public Func<int, int>? IslandTemple { get; set; }

    /// <summary>실제 영역 지도에 섬의 소용돌이를 기록한다.</summary>
    public Action<int, int>? RegisterTemple { get; set; }

    /// <summary>영역 번호 조회다. 보스 삭제의 같은 영역 판정에 쓴다.</summary>
    public Func<float, float, int>? RegionNumber { get; set; }

    /// <summary>같은 풀의 가상 삭제를 호출한다.</summary>
    public Action<int, uint>? Destroy { get; set; }

    /// <summary>현재 내용물 목록의 생산 칸·0x10 조회다.</summary>
    public Func<int, uint, bool>? HasContent { get; set; }

    /// <summary>생산 칸 생성 연결이다.</summary>
    public Action<int, uint>? CreateSlot { get; set; }

    /// <summary>소용돌이 비로딩 생산 확인 뒤의 0047d890 이다.</summary>
    public Action? RefreshProduction { get; set; }

    /// <summary>실제 00457560 의 이웃·연결선·Regular·Damageable 전체 몸체다(썬 바리케이드).</summary>
    public Action<int, uint>? SunFencePostPop { get; set; }

    /// <summary>실제 00484f90 의 조회·두 할당·게임 난수·Regular·Damageable 전체 몸체다(너겟).</summary>
    public Action<int, uint>? NuggetPostPop { get; set; }
}

/// <summary>
/// 실제 가상 표 +0x20 에 따라 초기 요새 건물의 전용 몸체를 분배한다(원본 RawAssetPostPop, 10.78).
/// 제네레이터·Gun·거주 구역·바리케이드·간헐천·신전 재정의가 공통·Damageable 장부를 각각 정확히 한 번 호출한다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 가상 표·추가 바이트 폭·거주 구역 예약·소용돌이 좌표가 달라서 옮기지 않았다).
/// 근거: cpppj/src/o/RawAssetPostPop.cpp (W02 Asset 조각, 2026-10-20).
/// </summary>
public sealed class RawAssetPostPop
{
    /// <summary>제네레이터 몸체 부류. 타입 +0x98 순수 조회 뒤 Damageable 로 이어진다.</summary>
    private const uint GeneratorKind = 1;

    /// <summary>각도·그래프를 초기화하는 Gun 몸체 부류.</summary>
    private const uint GunAngleKind = 2;

    /// <summary>최초 Gun 몸체 부류.</summary>
    private const uint GunFirstKind = 3;

    /// <summary>캐논 계열 Gun 몸체 부류.</summary>
    private const uint GunSunKind = 4;

    /// <summary>썬 디스크 발사기 Gun 몸체 부류.</summary>
    private const uint GunArcherKind = 5;

    /// <summary>Gun 장식 몸체 부류. 최초 때 사건 0 하나만 예약하고 공통 몸체로 간다.</summary>
    private const uint GunDecorationKind = 6;

    /// <summary>거주 구역 몸체 부류.</summary>
    private const uint ResidenceKind = 7;

    /// <summary>바리케이드 몸체 부류.</summary>
    private const uint BlockerKind = 8;

    /// <summary>간헐천 몸체 부류.</summary>
    private const uint GeyserKind = 9;

    /// <summary>레인 신전 포장 몸체 부류.</summary>
    private const uint VortexRainKind = 10;

    /// <summary>윈드 신전 포장 몸체 부류.</summary>
    private const uint VortexWindKind = 11;

    /// <summary>썬더 신전 포장 몸체 부류.</summary>
    private const uint VortexThunderKind = 12;

    /// <summary>훨리베이스 전용 몸체 부류. 다른 Damageable 공통 몸체만 호출하지 않는다.</summary>
    private const uint SunAviaryKind = 13;

    /// <summary>썬 바리케이드 전용 몸체 부류.</summary>
    private const uint SunFenceKind = 14;

    /// <summary>맨 오 워 풀 전용 몸체 부류. 훨리베이스 상태 기계로 대체하지 않는다.</summary>
    private const uint RainAviaryKind = 15;

    /// <summary>너겟 전용 몸체 부류.</summary>
    private const uint NuggetKind = 16;

    /// <summary>추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>프레임 DWORD의 위치(10.78). 각도 Gun 의 그래프 바이트(+30)와 다른 필드다.</summary>
    private const int WordOffset = 36;

    /// <summary>슬롯 안 소유자 바이트의 위치(10.78).</summary>
    private const int OwnerOffset = 34;

    /// <summary>슬롯 안 그래프 번호 바이트의 위치(10.78). 각도 Gun 이 프레임 조회값을 쓴다.</summary>
    private const int GraphOffset = 30;

    /// <summary>슬롯 안 현재 X 좌표(float)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 현재 Y 좌표(float)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>실제 Player 한 기록의 바이트 크기(10.78).</summary>
    private const int PlayerStride = 0xb4;

    /// <summary>실제 Player 기록 안 첫 신전 DWORD의 위치(10.78 +0x7c).</summary>
    private const int PlayerTempleBase = 0x7c;

    /// <summary>묶은 Player 기록 수(0~8번).</summary>
    private const int PlayerRecordCount = 9;

    /// <summary>원본 서버·클라이언트 영역을 가르는 첫 서버 번호. Gun 원본 assert 경로의 상한에 쓴다.</summary>
    private const int ServerFirst = 15000;

    /// <summary>10.78 가상 표(+0x20 몸체 부류) 대응표. 같은 주소가 여럿이면 같은 부류로 분배한다.</summary>
    private static readonly (uint Vtable, uint Kind)[] KindTable =
    [
        (0x00501ea0, GeneratorKind), (0x0050b6e8, GunAngleKind), (0x0050b7c0, GunFirstKind),
        (0x0050b9b8, GunSunKind), (0x0050bad0, GunSunKind), (0x0050b888, GunArcherKind),
        (0x0050b570, GunDecorationKind), (0x0050f540, ResidenceKind), (0x0050e940, BlockerKind),
        (0x0050ea08, BlockerKind), (0x0050ead0, BlockerKind), (0x0050da88, GeyserKind),
        (0x005196f0, VortexRainKind), (0x00519860, VortexWindKind), (0x005197a8, VortexWindKind),
        (0x00519918, VortexThunderKind), (0x00509930, SunAviaryKind), (0x00509470, SunFenceKind),
        (0x005099e8, RainAviaryKind), (0x0050dc30, NuggetKind),
    ];

    /// <summary>슬롯을 읽고 쓸 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호 → 타입 레코드. 플래그·비용을 읽는다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>파생 처리에서 공통 동작을 위임하는 기본 객체.</summary>
    private readonly RawPostPop _base;

    /// <summary>피해 가능한 객체의 HP·삭제·후처리 연결.</summary>
    private readonly RawCarrierPostPop _damageable;

    /// <summary>Gun·거주 구역·간헐천·신전 재정의가 쓰는 전역과 목록.</summary>
    private readonly AssetPostPopState _state;

    /// <summary>외부 저장소 조회와 가상 동작을 연결하는 묶음.</summary>
    private readonly AssetPostPopHooks _hooks;

    /// <summary>Player 생성·AI·경제와 공유하는 현재 9기록. 비어 있으면 state 사본을 쓴다.</summary>
    private byte[] _playerRecords = [];

    /// <summary>효과가 바꿀 수 있는 현재 원본 소유자 DWORD를 읽는 함수. 비어 있으면 state 값을 쓴다.</summary>
    private Func<uint>? _localOwner;

    /// <summary>
    /// 같은 풀·공통 장부·Damageable 와 상태를 받는다. 참조 대상은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">슬롯 풀. 기본 몸체·Damageable 몸체와 같아야 한다.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드. 10.78 은 188개여야 한다.</param>
    /// <param name="base">파생 처리에서 공통 동작을 위임하는 기본 객체.</param>
    /// <param name="damageable">피해 가능한 객체의 후처리 연결.</param>
    /// <param name="state">공유 상태. 이 모듈보다 오래 살아야 한다.</param>
    /// <param name="hooks">외부 효과 연결 묶음. 분기마다 필요한 항목이 있어야 한다.</param>
    public RawAssetPostPop(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawPostPop @base, RawCarrierPostPop damageable, AssetPostPopState state, AssetPostPopHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(@base);
        ArgumentNullException.ThrowIfNull(damageable);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        // 세 모듈이 다른 풀을 보면 원본 iterator 와 달라지므로 거부한다
        if (!ReferenceEquals(pool, @base.Pool) || !ReferenceEquals(pool, damageable.Pool))
        {
            throw new ArgumentException("자산 postPop 풀 불일치", nameof(pool));
        }
        // CD 10.72(171개)는 가상 표·추가 바이트 폭이 다르므로 받지 않는다
        if (types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException($"타입 레코드가 {types.Count}개입니다 (10.78 은 {OriginalConstructors.TypeCount}개)", nameof(types));
        }
        _pool = pool;
        _types = types;
        _base = @base;
        _damageable = damageable;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>이 모듈이 쓰는 슬롯 풀. 기본·Damageable 몸체와 같은 풀인지 확인하는 데 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 실제 복원한 가상 표인지 돌려준다. 타입 번호로 다른 파생 재정의를 대체하지 않는다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    public bool Handles(int sid) => Kind(sid) != 0;

    /// <summary>
    /// 공간 쓰기 전에 현재 분기가 요구하는 외부 효과와 공통 장부를 확인한다.
    /// 쓰기 전에 거부하므로 풀·목록·기록은 바뀌지 않는다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    public void Validate(int sid, uint flags)
    {
        uint kind = Kind(sid);
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        // 지원 표에 없고 타입 번호가 표를 벗어나면 미복원 효과다
        if (kind == 0 || raw[RawSidPool.TypeOffset] >= _types.Count)
        {
            throw new ArgumentException("자산 postPop 가상 표·타입 미복원", nameof(sid));
        }
        _base.ValidateBase(sid, flags);
        if (kind == NuggetKind)
        {
            if (_hooks.NuggetPostPop is null)
            {
                throw new InvalidOperationException("너겟 실제 전체 후처리 미연결");
            }
            return;
        }
        if (kind == SunAviaryKind)
        {
            if (_hooks.SunAviaryPostPop is null)
            {
                throw new InvalidOperationException("훨리베이스 실제 전체 후처리 미연결");
            }
            return;
        }
        if (kind == RainAviaryKind)
        {
            if (_hooks.RainAviaryPostPop is null)
            {
                throw new InvalidOperationException("맨 오 워 풀 실제 전체 후처리 미연결");
            }
            return;
        }
        if (kind == SunFenceKind)
        {
            if (_hooks.SunFencePostPop is null)
            {
                throw new InvalidOperationException("썬 바리케이드 실제 전체 후처리 미연결");
            }
            return;
        }
        if (kind == GeneratorKind)
        {
            return;
        }
        if (kind == BlockerKind)
        {
            // 전투 중 비로딩 최초인데 바리케이드 몸체가 없으면 거부한다
            if (_state.InBattle && (flags & 1) != 0 && _state.LoadingDepth == 0 && _hooks.StartBlocker is null)
            {
                throw new ArgumentException("바리케이드 최초 프로세스 경계 누락", nameof(sid));
            }
            return;
        }
        if (kind >= GunAngleKind && kind <= GunArcherKind)
        {
            // Gun 분기의 필수 효과를 검사한다
            if (_hooks.CreateGunProcess is null
                || (kind == GunAngleKind && (_hooks.GunFrame is null || _hooks.SetGraph is null))
                || (kind == GunArcherKind && _hooks.RemoveAnimation is null)
                || (_state.CheckingForms && _hooks.CheckForms is null))
            {
                throw new ArgumentException("Gun postPop 효과 누락", nameof(sid));
            }
            // 원본 서버·로딩 assert 경로. 전투 중 비추가의 최초 호출이 서버·예측 번호가 아니면 원본처럼 멈춘다
            if (_state.InBattle && (raw[ExtraOffset] & 1) == 0 && (flags & 1) != 0
                && ((kind == GunSunKind && _state.LoadingDepth == 0) || (kind == GunArcherKind && !_state.FortMode))
                && ((flags & 0x50) != 0 || (sid >= RawSidPool.FirstOrdinary && sid < ServerFirst)))
            {
                throw new InvalidOperationException("Gun postPop 원본 서버·로딩 assert 경로");
            }
            return;
        }
        // 비최초 호출의 Gun 장식·거주 구역·바리케이드·간헐천·신전 앞부분은 공통 검사만으로 충분하다
        if ((flags & 1) == 0 && kind < VortexRainKind)
        {
            return;
        }
        if (_hooks.ReserveRegular is null || _hooks.AddRegular is null)
        {
            throw new ArgumentException("자산 postPop Regular 경계 누락", nameof(sid));
        }
        if (kind == ResidenceKind && (_hooks.Surface is null || _hooks.SetOwner is null || _hooks.NotifySurface is null))
        {
            throw new ArgumentException("거주 구역 지면·소유자·연결 경계 누락", nameof(sid));
        }
        if (kind == GeyserKind && (_hooks.GeyserRegistration is null || _hooks.AtPosition is null || _hooks.Unpop is null || _hooks.Repop is null))
        {
            throw new ArgumentException("간헐천 등록·공간 경계 누락", nameof(sid));
        }
        if (kind >= VortexRainKind && (_hooks.TypeDword is null || _hooks.PlayImpactSound is null || _hooks.SetOwner is null
            || _hooks.IslandNumber is null || _hooks.IslandExists is null || _hooks.RegisterTemple is null
            || _hooks.RegionNumber is null || _hooks.Destroy is null || _hooks.HasContent is null
            || _hooks.CreateSlot is null || _hooks.RefreshProduction is null || _hooks.ClearPlacement is null))
        {
            throw new ArgumentException("Vortex 전체 postPop 효과 누락", nameof(sid));
        }
    }

    /// <summary>
    /// Gun·제네레이터·거주 구역·바리케이드·간헐천·신전 전체 재정의 순서를 실행한다.
    /// 각 몸체는 공통·Damageable 장부를 내부에서 정확히 한 번 호출한다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    public void PostPop(int sid, uint flags)
    {
        Validate(sid, flags);
        uint kind = Kind(sid);
        if (kind == SunAviaryKind)
        {
            _hooks.SunAviaryPostPop!(sid, flags);
        }
        else if (kind == NuggetKind)
        {
            _hooks.NuggetPostPop!(sid, flags);
        }
        else if (kind == RainAviaryKind)
        {
            _hooks.RainAviaryPostPop!(sid, flags);
        }
        else if (kind == SunFenceKind)
        {
            _hooks.SunFencePostPop!(sid, flags);
        }
        else if (kind == GeneratorKind)
        {
            _damageable.DamageablePostPop(sid, flags);
        }
        else if (kind >= GunAngleKind && kind <= GunArcherKind)
        {
            Gun(sid, flags, kind);
        }
        else if (kind == GunDecorationKind)
        {
            // Gun 장식은 최초 때 사건 0 하나만 예약한다
            if ((flags & 1) != 0)
            {
                Regular(sid, 0, 0.05f);
            }
            _base.PostPopBase(sid, flags);
        }
        else if (kind == ResidenceKind)
        {
            Residence(sid, flags);
        }
        else if (kind == BlockerKind)
        {
            // 전투 중 비로딩 최초 바리케이드는 전용 몸체를 먼저 부른다
            if (_state.InBattle && (flags & 1) != 0 && _state.LoadingDepth == 0)
            {
                _hooks.StartBlocker!(sid);
            }
            _damageable.DamageablePostPop(sid, flags);
        }
        else if (kind == GeyserKind)
        {
            Geyser(sid, flags);
        }
        else
        {
            Vortex(sid, flags, kind);
        }
    }

    /// <summary>
    /// 실제 9 Player 기록을 묶는다. 전체 크기가 맞지 않으면 거부하며, 묶은 뒤에는 state 사본을 쓰지 않는다.
    /// </summary>
    /// <param name="records">9개 기록 전체 바이트(10.78 은 1,620바이트). 이 모듈보다 오래 살아야 한다.</param>
    /// <param name="currentLocalOwner">효과가 바꾼 현재 원본 소유자 DWORD를 읽는 함수. 비우면 state 값을 쓴다.</param>
    public void BindPlayerRecords(byte[] records, Func<uint>? currentLocalOwner = null)
    {
        ArgumentNullException.ThrowIfNull(records);
        // 실제 한 기록의 바이트 크기로 전체 크기를 검사한다
        if (records.Length != PlayerRecordCount * PlayerStride)
        {
            throw new ArgumentException("자산 postPop 실제9 Player 기록 크기 오류", nameof(records));
        }
        _playerRecords = records;
        _localOwner = currentLocalOwner;
    }

    /// <summary>두 가상 주소를 실제 몸체 부류로 분류한다. 표에 없으면 0이다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    private uint Kind(int sid)
    {
        uint address = Read(_pool.Slot(sid), 0);
        // 같은 주소의 실제 몸체 부류를 반환한다
        foreach ((uint vtable, uint kind) in KindTable)
        {
            if (vtable == address)
            {
                return kind;
            }
        }
        return 0;
    }

    /// <summary>실패 가능한 두 Regular 확보를 각각 실행한다. 확보 실패 뒤에도 다음 확보·공통 후처리를 계속한다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="event">예약할 사건 번호.</param>
    /// <param name="delay">예약 지연(초).</param>
    private void Regular(int sid, uint @event, float delay)
    {
        if (_hooks.ReserveRegular!())
        {
            _hooks.AddRegular!(sid, @event, delay);
        }
    }

    /// <summary>Gun 몸체의 네 분기·현재 raw 재조회·추가 비트 0x1000의 워드·추가 쓰기다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    /// <param name="kind">Gun 세부 부류.</param>
    private void Gun(int sid, uint flags, uint kind)
    {
        // 각도 Gun 은 하위 3비트가 켜졌을 때 현재 각도로 그래프 바이트를 먼저 정한다
        if (kind == GunAngleKind && (flags & 7) != 0)
        {
            int angle = unchecked((int)Read(_pool.Slot(sid), WordOffset));
            int graph = _hooks.GunFrame!(angle);
            _hooks.SetGraph!(sid, graph);
        }
        bool branch = kind == GunAngleKind ? (flags & 7) != 0 : (flags & 1) != 0;
        // 썬 캐논은 현재 추가 비트가 꺼졌을 때만 분기한다
        if (branch && (kind != GunSunKind || (_pool.Slot(sid)[ExtraOffset] & 1) == 0))
        {
            // 전투 중 비추가이면 GunProcess 생성·부착의 전체 경계로 간다
            if (_state.InBattle && (_pool.Slot(sid)[ExtraOffset] & 1) == 0)
            {
                if (kind == GunArcherKind)
                {
                    _hooks.RemoveAnimation!(sid);
                }
                _hooks.CreateGunProcess!(sid);
                if (_state.CheckingForms)
                {
                    _hooks.CheckForms!(sid);
                }
            }
            // 로딩 중 최초 캐논의 타입 0x1000 은 현재 HP 워드를 추가 위치에 복사한다
            if (kind == GunSunKind && _state.LoadingDepth != 0 && (_types[_pool.Slot(sid)[RawSidPool.TypeOffset]].Flags1 & 0x1000) != 0)
            {
                Write(_pool.AllocatedBytes(sid), WordOffset, Read(_pool.Slot(sid), 12, 2));
            }
        }
        // 썬 캐논의 타입 0x1000 은 현재 추가 위치 값을 HP 워드로 쓴다
        if (kind == GunSunKind && (_types[_pool.Slot(sid)[RawSidPool.TypeOffset]].Flags1 & 0x1000) != 0)
        {
            Write(_pool.AllocatedBytes(sid), 12, Read(_pool.Slot(sid), WordOffset), 2);
        }
        _damageable.DamageablePostPop(sid, flags);
    }

    /// <summary>거주 구역 최초 소유자·목록·예약·연결·잔여 프레임을 처리한다. 마지막에는 공통 몸체를 직접 호출한다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    private void Residence(int sid, uint flags)
    {
        if ((flags & 1) != 0)
        {
            ReadOnlySpan<byte> raw = _pool.Slot(sid);
            int surface = _hooks.Surface!(X(raw, XOffset), X(raw, YOffset));
            // 현재 지면의 소유자가 있으면 그 소유자로 재지정한다
            if (surface != 0)
            {
                uint owner = _pool.Slot(surface)[OwnerOffset];
                if (owner != 0)
                {
                    _hooks.SetOwner!(sid, owner);
                }
            }
            Add(_state.Residences, sid, false);
            Regular(sid, 0, 0);
            Regular(sid, 1, 0);
            Regular(sid, 3, 0);
            _pool.AllocatedBytes(sid)[12] &= 0xf0;
            _hooks.NotifySurface!(sid);
            int value = unchecked((int)Read(_pool.Slot(sid), WordOffset));
            // 애니메이션이 켜지면 잔여 프레임을 2로 나눈 나머지와 현재 하위 니블로 묶는다
            if (_state.ResidenceAnimation)
            {
                value = value / 2 * 2 + (_pool.Slot(sid)[12] & 15);
                int group = value < 6 ? 0 : value < 12 ? 1 : value < 18 ? 2 : 3;
                value -= group * 6;
            }
            Write(_pool.AllocatedBytes(sid), 42, unchecked((uint)value));
        }
        _base.PostPopBase(sid, flags);
    }

    /// <summary>간헐천 최초 비용 워드·목록·주변 받침 중립 프레임·Damageable 을 처리한다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    private void Geyser(int sid, uint flags)
    {
        // 최초 비추가의 간헐천만 등록·비용·예약을 처리한다
        if ((flags & 1) != 0 && (_pool.Slot(sid)[ExtraOffset] & 9) == 0)
        {
            Add(_state.Geysers, sid, false);
            _hooks.GeyserRegistration!(sid, 1);
            Write(_pool.AllocatedBytes(sid), 12, Truncate(_types[_pool.Slot(sid)[RawSidPool.TypeOffset]].Cost), 2);
            Regular(sid, 0, 0.1f);
            ReadOnlySpan<byte> raw = _pool.Slot(sid);
            int[] neighbors = _hooks.AtPosition!(X(raw, XOffset), X(raw, YOffset));
            // 현재 지점에 걸린 섬 받침·종유석만 중립 프레임으로 다시 등록한다
            foreach (int neighbor in neighbors)
            {
                byte type = _pool.Slot(neighbor)[RawSidPool.TypeOffset];
                if (type == _state.IslandType || type == _state.StalagType)
                {
                    _hooks.Unpop!(neighbor, 0);
                    Write(_pool.AllocatedBytes(neighbor), WordOffset, 8);
                    _hooks.Repop!(neighbor, 0x40);
                }
            }
        }
        _damageable.DamageablePostPop(sid, flags);
    }

    /// <summary>신전 공통 몸체 후 각 원소 포장의 추가·예약을 처리한다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    /// <param name="kind">신전 세부 부류.</param>
    private void Vortex(int sid, uint flags, uint kind)
    {
        VortexBase(sid, flags);
        if ((flags & 1) != 0)
        {
            // 윈드 신전은 공통 몸체가 이미 추가 위치를 썼으므로 원소 추가를 건너뛴다
            if (kind != VortexWindKind)
            {
                Write(_pool.AllocatedBytes(sid), WordOffset, _hooks.TypeDword!(sid, 0x134));
            }
            Regular(sid, 1, kind == VortexThunderKind ? 0.1f : 0.08f);
        }
    }

    /// <summary>
    /// 신전 Damageable 뒤 Player·목록·영역·다른 신전 삭제·생산 칸 생성을 처리한다.
    /// 공통 Damageable 을 먼저 호출하여 공통 깊이를 줄인다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    private void VortexBase(int sid, uint flags)
    {
        _damageable.DamageablePostPop(sid, flags);
        if ((flags & 2) != 0)
        {
            // 새 신전 비트의 소유자는 1~8 이어야 한다
            uint owner = _pool.Slot(sid)[OwnerOffset];
            if (owner == 0 || owner > 8)
            {
                throw new InvalidOperationException("Vortex postPop 새 신전 소유자 assert");
            }
            SetPlayerTemple(owner, 2, (uint)sid);
            SetPlayerTemple(owner, 3, (uint)sid);
        }
        if ((flags & 1) == 0)
        {
            return;
        }
        ++_state.TempleCount;
        // 비추가의 최초 신전은 충격음을 내고 HP 를 지운다
        if ((_pool.Slot(sid)[ExtraOffset] & 8) == 0)
        {
            _hooks.PlayImpactSound!(_hooks.TypeDword!(sid, 0x74));
            Write(_pool.AllocatedBytes(sid), 12, 0, 2);
            uint owner = _pool.Slot(sid)[OwnerOffset];
            // 초기 신전이 없고 비추가이면 공유 기록의 초기 신전 두 필드를 채운다
            if (owner != 0 && owner <= 8 && PlayerTemple(owner, 0) == 0 && (_pool.Slot(sid)[ExtraOffset] & 1) == 0)
            {
                SetPlayerTemple(owner, 0, (uint)sid);
                SetPlayerTemple(owner, 1, (uint)sid);
            }
        }
        // 비추가·비매립의 최초 신전은 목록·선택 취소·추가·영역 등록을 처리한다
        if ((_pool.Slot(sid)[ExtraOffset] & 9) == 0)
        {
            Add(_state.Temples, sid, true);
            Add(_state.Residences, sid, true);
            // 선택된 폼이 소용돌이 부류이면 배치 커서를 취소한다
            if (_state.Selected != 0 && _pool.Slot(sid)[OwnerOffset] == LocalOwner())
            {
                ReadOnlySpan<byte> form = _pool.Slot((int)_state.Selected);
                uint type = Read(form, 18, 2);
                if (type < _types.Count && (_types[(int)type].Flags2 & 0x200) != 0)
                {
                    _hooks.ClearPlacement!();
                }
            }
            Write(_pool.AllocatedBytes(sid), WordOffset, _hooks.TypeDword!(sid, 0x130));
            ReadOnlySpan<byte> raw = _pool.Slot(sid);
            int island = _hooks.IslandNumber!(X(raw, XOffset), X(raw, YOffset));
            // 섬이 목록에 없으면 원본처럼 멈춘다
            if (!_hooks.IslandExists!(island))
            {
                throw new InvalidOperationException("Vortex postPop 섬 목록 assert");
            }
            _hooks.RegisterTemple!(island, sid);
            _hooks.SetOwner!(sid, _pool.Slot(sid)[OwnerOffset]);
            // 보스이면 원본 반복자처럼 다음 번호를 미리 확보하고 같은 영역의 다른 소용돌이를 삭제한다
            if (_state.Boss)
            {
                uint current = 2;
                // 최초 번호는 해제 슬롯을 건너뛰며 첫 할당 자산·폼을 그대로 반환한다
                while (current < (uint)_pool.Capacity && (_pool.Slot((int)current)[RawSidPool.StateOffset] & 1) != 0)
                {
                    current++;
                }
                // 가상 삭제의 외부 효과가 다음 슬롯을 바꿔도 원본 반복자의 현재 번호를 유지한다
                while (current < (uint)_pool.Capacity)
                {
                    int other = (int)current;
                    uint next = current + 1;
                    while (next < (uint)_pool.Capacity && (_pool.Slot((int)next)[RawSidPool.StateOffset] & 1) != 0)
                    {
                        next++;
                    }
                    ReadOnlySpan<byte> otherRaw = _pool.Slot(other);
                    byte otherType = otherRaw[RawSidPool.TypeOffset];
                    if ((otherRaw[ExtraOffset] & 8) == 0 && otherType < _types.Count
                        && (_types[otherType].Flags2 & 0x200) != 0 && other != sid)
                    {
                        float x = X(otherRaw, XOffset);
                        float y = X(otherRaw, YOffset);
                        if (_hooks.RegionNumber!(x, y) == island)
                        {
                            _hooks.Destroy!(other, 0x100000);
                        }
                    }
                    current = next;
                }
            }
        }
        // 비로딩의 비추가·비매립 신전은 생산 칸 확인·생산 갱신을 처리한다
        if (_state.LoadingDepth == 0 && (_pool.Slot(sid)[ExtraOffset] & 9) == 0)
        {
            if (!_hooks.HasContent!(sid, _state.SlotType))
            {
                _hooks.CreateSlot!(sid, _state.SlotType);
            }
            _hooks.RefreshProduction!();
        }
    }

    /// <summary>현재 Player 의 신전 네 DWORD 를 실제 기록 저장소에서 읽는다. 묶지 않은 모듈은 state 사본을 쓴다.</summary>
    /// <param name="owner">소유자 번호(0~8).</param>
    /// <param name="field">신전 필드(0~3: 초기·초기 복사·새·새 복사).</param>
    private uint PlayerTemple(uint owner, int field)
    {
        int offset = PlayerTempleOffset(owner, field);
        if (_playerRecords.Length == 0)
        {
            AssetPostPopPlayer player = _state.Players[owner];
            return field switch
            {
                0 => player.InitialTemple,
                1 => player.InitialTempleCopy,
                2 => player.NewTemple,
                _ => player.NewTempleCopy,
            };
        }
        return Read(_playerRecords, offset);
    }

    /// <summary>초기·새 신전 및 각각의 복사 필드를 정확 DWORD 폭으로 쓰고 다른 바이트를 보존한다.</summary>
    /// <param name="owner">소유자 번호(0~8).</param>
    /// <param name="field">신전 필드(0~3).</param>
    /// <param name="sid">쓸 번호.</param>
    private void SetPlayerTemple(uint owner, int field, uint sid)
    {
        int offset = PlayerTempleOffset(owner, field);
        if (_playerRecords.Length == 0)
        {
            AssetPostPopPlayer player = _state.Players[owner];
            switch (field)
            {
                case 0: player.InitialTemple = sid; break;
                case 1: player.InitialTempleCopy = sid; break;
                case 2: player.NewTemple = sid; break;
                default: player.NewTempleCopy = sid; break;
            }
            return;
        }
        Write(_playerRecords, offset, sid);
    }

    /// <summary>실제 9기록의 판본별 신전 필드 주소를 계산한다. 필드 0~3 은 초기·복사·새·복사 순서다.</summary>
    /// <param name="owner">소유자 번호(0~8).</param>
    /// <param name="field">신전 필드(0~3).</param>
    private static int PlayerTempleOffset(uint owner, int field)
    {
        // 소유자·필드가 기록을 벗어나면 쓰기 전에 거부한다
        if (owner >= PlayerRecordCount || field < 0 || field >= 4)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), owner, "자산 postPop Player 신전 필드 범위 오류");
        }
        return (int)owner * PlayerStride + PlayerTempleBase + field * 4;
    }

    /// <summary>현재 외부 소유자 DWORD 또는 state 의 로컬 소유자 바이트를 조회한다.</summary>
    private uint LocalOwner() => _localOwner is null ? _state.LocalOwner : _localOwner();

    /// <summary>원본 전체 목록은 자리가 남은 경우만 덧붙인다(중복 검사는 호출자가 정한다).</summary>
    /// <param name="list">대상 목록.</param>
    /// <param name="sid">덧붙일 번호.</param>
    /// <param name="unique">참이면 이미 있을 때 생략한다.</param>
    private static void Add(RawPostPopList list, int sid, bool unique)
    {
        if (list.Count >= (uint)list.Entries.Length)
        {
            return;
        }
        // 중복이면 덧붙이지 않고 기존 항목을 보존한다
        if (unique && list.Contains(sid))
        {
            return;
        }
        list.Entries[(int)list.Count++] = (uint)sid;
    }

    /// <summary>현재 float 좌표의 비트 패턴을 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static float X(ReadOnlySpan<byte> raw, int offset) => BitConverter.UInt32BitsToSingle(Read(raw, offset));

    /// <summary>정렬되지 않은 리틀 엔디언 값을 필요한 폭만큼 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽을 바이트 수(1~4).</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        uint value = 0;
        // 필요한 바이트만 현재 슬롯에서 조합한다
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }
        return value;
    }

    /// <summary>원본 바이트 쓰기 폭을 유지한다. 지정한 필드 폭 바깥의 값을 보존한다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수(1~4).</param>
    private static void Write(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>CRT 절삭의 유한 부호 64비트 범위에서 낮은 DWORD 를 반환한다.</summary>
    /// <param name="value">자를 실수 비용.</param>
    private static uint Truncate(float value)
    {
        double wide = value;
        // 무한·비수·64비트 범위를 벗어나면 원본처럼 거부한다
        if (!double.IsFinite(wide) || wide < -9223372036854775808.0 || wide >= 9223372036854775808.0)
        {
            throw new ArgumentOutOfRangeException(nameof(value), value, "자산 postPop CRT 정수 범위 오류");
        }
        return unchecked((uint)(long)wide);
    }
}
