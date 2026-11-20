using System.Buffers.Binary;

// 같은 월드의 시설 업그레이드 그림·완료·프로세스 수명을 처리한다.
namespace Netstorm.Core.Original;

/// <summary>각 원본 조회 지점에서 다시 읽는 전역 장치다.</summary>
public sealed class FactoryUpgradeState
{
    /// <summary>완료 효과를 모두 생략하는 현재 아웃포스트 TYPE다.</summary>
    public Func<uint>? Outpost { get; set; }
    /// <summary>현재 DependFinder가 선택하는 TYPE다.</summary>
    public Func<uint>? DependType { get; set; }
    /// <summary>완료 안내 소리를 들을 현재 로컬 플레이어다.</summary>
    public Func<uint>? LocalPlayer { get; set; }
    /// <summary>dead 부모 finder의 현재 진단 DWORD다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();
    /// <summary>dead 부모 진단을 생략하는 도전 모드다.</summary>
    public Func<bool>? Challenge { get; set; }
    /// <summary>dead 부모 진단을 생략하는 현재 권한이다.</summary>
    public Func<bool>? Boss { get; set; }
}

/// <summary>실제 가상 호출·건설·효과·AI 저장소에 연결한다. 선택한 하위만 요구한다.</summary>
public sealed class FactoryUpgradeHooks
{
    /// <summary>현재 가상80 레벨이다.</summary>
    public Func<int, uint>? Level { get; set; }
    /// <summary>가상84 레벨 쓰기와 표시 갱신이다.</summary>
    public Action<int, uint>? SetLevel { get; set; }
    /// <summary>현재 최대 HP다.</summary>
    public Func<int, int>? MaxHp { get; set; }
    /// <summary>현재 상태 글자의 프레임을 보정한다.</summary>
    public Action<int>? AdjustFrame { get; set; }
    /// <summary>현재 객체 표시를 갱신한다.</summary>
    public Action<int>? Display { get; set; }
    /// <summary>위치 소리를 loop0으로 재생한다.</summary>
    public Action<float, float, string>? SoundAt { get; set; }
    /// <summary>전역 완료 소리를 재생한다.</summary>
    public Action<string>? Sound { get; set; }
    /// <summary>실제 개수·사건의 플레어다.</summary>
    public Action<int, int, uint>? Flare { get; set; }
    /// <summary>같은 TYPE·flags2로 실제 그림을 생성한다.</summary>
    public Func<uint, uint, int>? Create { get; set; }
    /// <summary>그림의 현재 소유자를 지정한다.</summary>
    public Action<int, uint>? Owner { get; set; }
    /// <summary>그림을 같은 월드 공간에 등록한다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }
    /// <summary>그림의 실제 건설 진행을 생성한다.</summary>
    public Action<int, float>? Construction { get; set; }
    /// <summary>실제 watched/watcher 감시 쌍을 연결한다.</summary>
    public Action<int, int>? Depend { get; set; }
    /// <summary>현재 감시 Form의 상대 감시자 SID다.</summary>
    public Func<int, int>? Watcher { get; set; }
    /// <summary>그림을 가상10·flags0으로 실제 삭제한다.</summary>
    public Action<int, uint>? Destroy { get; set; }
    /// <summary>현재 소유자와 Player+b0의 AI 존재 여부다.</summary>
    public Func<int, bool>? AiPresent { get; set; }
    /// <summary>현재 AI의 생산 큐·지갑에 완료를 통지한다.</summary>
    public Action<uint, int>? NotifyAi { get; set; }
    /// <summary>원본 종류3/4의 진행 로그다.</summary>
    public Action<uint, string>? Log { get; set; }
    /// <summary>원본 진단 행 번호다. 반환 뒤 계속 진행한다.</summary>
    public Action<uint>? Assertion { get; set; }
}

/// <summary>004549c0 사건3의 그림 생성과 004545e0 완료·종속 그림 정리를 실행한다.</summary>
/// <remarks>근거: cpppj/src/o/RawFactoryUpgrade.cpp·h. 2026-11-20 0.667에서 이식했다.</remarks>
public sealed class RawFactoryUpgrade
{
    /// <summary>원본 PE00509144의 정확한 그림 Y 오프셋이다.</summary>
    private const float PictureOffset = 0.0010000000474974513f;
    /// <summary>원본 예측 그림의 클라이언트 SID 상한이다.</summary>
    private const int PictureClientLimit = 15000;
    /// <summary>실제 시설·그림·감시 Form의 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>실제 Regular/SharedRegular 검색·삭제를 수행하는 같은 호스트다.</summary>
    private readonly SquidProcessHost _host;
    /// <summary>현재 전역을 빌리는 장치다.</summary>
    private readonly FactoryUpgradeState _state;
    /// <summary>선택된 실제 가상·장치·AI 효과다.</summary>
    private readonly FactoryUpgradeHooks _hooks;

    /// <summary>게임 효과 없이 같은 PATCH 풀·호스트 계약을 확인한다.</summary>
    public RawFactoryUpgrade(RawSidPool pool, SquidProcessHost host, FactoryUpgradeState state, FactoryUpgradeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(host);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || !ReferenceEquals(pool, host.Pool()))
        {
            throw new ArgumentException("시설 업그레이드 같은 PATCH 풀/Host 누락");
        }
        _pool = pool;
        _host = host;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>상한3과 상한 미만의 두 번째 가상 레벨 재조회를 보존한다.</summary>
    public uint NextLevel(int parent)
    {
        uint next = unchecked(Need(_hooks.Level)(parent) + 1u);
        return unchecked((int)next) > 3 ? 3u : unchecked(Need(_hooks.Level)(parent) + 1u);
    }

    /// <summary>다음 레벨 추상 클라이언트 그림→실제 건설→감시 쌍을 생성한다.</summary>
    public void Begin(int parent, float payload)
    {
        int image = Need(_hooks.Create)(_pool.Slot(parent)[10], 2);
        uint level = NextLevel(parent);
        Need(_hooks.SetLevel)(image, level);
        _pool.AllocatedBytes(image)[40] |= 8;
        if ((_pool.Slot(image)[11] & 4) == 0)
        {
            Need(_hooks.Assertion)(0x282);
        }
        _pool.AllocatedBytes(image)[40] |= 1;
        BinaryPrimitives.WriteUInt16LittleEndian(_pool.AllocatedBytes(image).Slice(12, 2), 0);
        Need(_hooks.Owner)(image, _pool.Slot(parent)[34]);
        ReadOnlySpan<byte> current = _pool.Slot(parent);
        float x = Float(current, 14);
        float y = (float)((double)Float(current, 18) + PictureOffset);
        Need(_hooks.Pop)(image, x, y, 0);
        Need(_hooks.Log)(4, "making the client upgrade picture");
        Need(_hooks.Construction)(image, payload);
        Need(_hooks.Depend)(parent, image);
    }

    /// <summary>레벨·HP·표시·소리·플레어 뒤 사건4/3과 모든 예측 그림을 정리한다.</summary>
    public void Complete(int parent)
    {
        if (_pool.Slot(parent)[10] == Need(_state.Outpost)())
        {
            return;
        }
        uint level = NextLevel(parent);
        Need(_hooks.SetLevel)(parent, level);
        int hp = Need(_hooks.MaxHp)(parent);
        RawSquidHitPoints.Set(_pool, parent, hp);
        Need(_hooks.AdjustFrame)(parent);
        Need(_hooks.Display)(parent);
        ReadOnlySpan<byte> raw = _pool.Slot(parent);
        Need(_hooks.SoundAt)(Float(raw, 14), Float(raw, 18), "upgradeDone.wav");
        if (_pool.Slot(parent)[34] == Need(_state.LocalPlayer)())
        {
            Need(_hooks.Sound)("upgradeComplete.wav");
        }
        Need(_hooks.Flare)(parent, 6, 2);
        // 완료 효과가 추가한 같은 사건까지 원본 순서대로 반복 검색·삭제한다.
        while (_host.FindEvent(parent, 4) is { } fourth)
        {
            _host.Kill(fourth, 0);
        }
        // 현재 실행 중인 사건3도 호스트의 실제 수명·등록 검사에 맡긴다.
        while (_host.FindEvent(parent, 3) is { } third)
        {
            _host.Kill(third, 0);
        }
        int next = First(parent), steps = 0;
        // 삭제가 현재 감시 Form의 링크를 바꿔도 먼저 저장한 next를 유지한다.
        for (int form = Next(ref next, ref steps); form != 0; form = Next(ref next, ref steps))
        {
            int watcher = Need(_hooks.Watcher)(form);
            if (watcher == parent)
            {
                Need(_hooks.Assertion)(0xdd);
            }
            if (Picture(parent, watcher))
            {
                Need(_hooks.Destroy)(watcher, 0);
            }
        }
    }

    /// <summary>로그→완료→현재 소유자/AI 재조회·통지→-1 반환 순서다. 아웃포스트에도 통지는 적용한다.</summary>
    public float Finish(int parent)
    {
        Need(_hooks.Log)(3, "Upgrading!\n");
        Complete(parent);
        if (Need(_hooks.AiPresent)(parent))
        {
            Need(_hooks.NotifyAi)(_pool.Slot(parent)[34], parent);
        }
        return -1f;
    }

    /// <summary>같은 TYPE·extra8·클라이언트 범위의 첫 감시 그림에서 멈춘다.</summary>
    public bool HasPicture(int parent)
    {
        int next = First(parent), steps = 0;
        // 원본 현재 TYPE 필터와 저장 next로 감시 Form을 방문한다.
        for (int form = Next(ref next, ref steps); form != 0; form = Next(ref next, ref steps))
        {
            int watcher = Need(_hooks.Watcher)(form);
            if (watcher == parent)
            {
                Need(_hooks.Assertion)(0xad);
            }
            if (Picture(parent, watcher))
            {
                return true;
            }
        }
        return false;
    }

    /// <summary>그림→사건4→사건3 순서의 단락 조회다.</summary>
    public bool IsUpgrading(int parent) => HasPicture(parent) || _host.FindEvent(parent, 4) is not null || _host.FindEvent(parent, 3) is not null;
    /// <summary>실제 세션 조립이 같은 풀인지 확인한다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>finder 진단 뒤 현재 부모 head WORD를 읽는다.</summary>
    private int First(int parent)
    {
        if (_state.Checking.Read() != 0 && !Need(_state.Challenge)() && (_pool.Slot(parent)[11] & 2) != 0 && !Need(_state.Boss)())
        {
            Need(_hooks.Assertion)(0x158);
        }
        return BinaryPrimitives.ReadUInt16LittleEndian(_pool.Slot(parent).Slice(6, 2));
    }

    /// <summary>후보를 반환하기 전에 next를 저장하고 현재 TYPE 필터만 적용한다.</summary>
    private int Next(ref int next, ref int steps)
    {
        // 다른 Form만 건너뛰며 free/dead 조건을 추가하지 않는다.
        while (next != 0)
        {
            if (next < 0 || next >= _pool.Capacity || ++steps > _pool.Capacity)
            {
                throw new InvalidOperationException("시설 업그레이드 종속 체인 손상");
            }
            int current = next;
            ReadOnlySpan<byte> raw = _pool.Slot(current);
            next = BinaryPrimitives.ReadUInt16LittleEndian(raw.Slice(4, 2));
            if (raw[10] < 70 && raw[10] == Need(_state.DependType)())
            {
                return current;
            }
        }
        return 0;
    }

    /// <summary>그림은 TYPE·extra8·SID 범위만 판정한다. 소유자·free/dead 조건은 추가하지 않는다.</summary>
    private bool Picture(int parent, int candidate)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(candidate);
        return raw[10] == _pool.Slot(parent)[10] && (raw[40] & 8) != 0 && candidate > 4 && candidate < PictureClientLimit;
    }
    /// <summary>현재 슬롯의 float 비트를 원문 그대로 읽는다.</summary>
    private static float Float(ReadOnlySpan<byte> raw, int offset) => BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(offset, 4)));
    /// <summary>선택된 미연결을 빈 성공으로 대신하지 않는다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("시설 업그레이드 실제 하위 미연결");
}
