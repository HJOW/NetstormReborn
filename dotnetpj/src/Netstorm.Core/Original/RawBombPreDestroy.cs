namespace Netstorm.Core.Original;

/// <summary>
/// Point Blast 계열 Bomb 의 가상 전처리(원본 0041e960)가 바깥에 요구하는 효과 경계 묶음
/// (원본 BombPreDestroyHooks). 폭발·현재좌표 소리·공통 전처리 세 갈래다.
/// </summary>
public sealed class BombPreDestroyHooks
{
    /// <summary>같은 월드의 실제 폭발 효과(원본 004605d0). 폭발체·피해·표시를 모두 포함한다.</summary>
    public Action<int>? Explosion { get; set; }

    /// <summary>현재 좌표의 <c>explosion.wav</c> 재생(원본 loop 0). 폭발 뒤 좌표를 다시 읽어 넘긴다.</summary>
    public Action<float, float>? Sound { get; set; }

    /// <summary>가상이 아닌 공통 사전 파괴(원본 004b0950). 폭발 여부와 무관하게 항상 부른다.</summary>
    public Action<int, uint>? Base { get; set; }
}

/// <summary>
/// Point Blast 계열 Bomb 의 가상 사전 파괴(원본 0041e960, RawBombPreDestroy, 10.78).
/// 같은 Bomb 파생 중 공통 전처리만 가진 가상 표는 이 몸체로 분배하지 않는다.
/// 순서는 원본 그대로 "현재 추가 비트와 폭발 플래그 확인 → 폭발 → 현재 좌표 읽기 → 소리 → 공통 전처리"다.
/// 폭발 효과가 좌표·가상 표를 바꿀 수 있어 소리 좌표는 폭발 뒤에 다시 읽는다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 의 다른 전처리를 같은 것으로 추정하지 않는다).
/// 근거: cpppj/src/o/RawBombPreDestroy.cpp (W03 다리 밖 자산 삭제 재정의 조각, 2026-10-23).
/// </summary>
public sealed class RawBombPreDestroy
{
    /// <summary>10.78 Point Blast Bomb 클래스의 가상 표 값(원본 0x502370). 이 값만 이 몸체가 처리한다.</summary>
    private const uint PatchBombVtable = 0x00502370;

    /// <summary>슬롯 안 추가 비트(abstract 1·매립 8) 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>가림·매립 비트 마스크. 하나라도 켜지면 폭발을 건너뛴다.</summary>
    private const byte SkipMask = 9;

    /// <summary>폭발을 요구하는 삭제 플래그 비트(원본 0x100000).</summary>
    private const uint ExplosionFlag = 0x100000;

    /// <summary>슬롯 안 x 좌표(float 4바이트)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 y 좌표(float 4바이트)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>원본과 같은 50바이트 슬롯을 읽는 풀. 생성 뒤 바뀌지 않는다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>폭발·소리·공통 전처리의 실제 소비자.</summary>
    private readonly BombPreDestroyHooks _hooks;

    /// <summary>
    /// Bomb 전처리를 만든다. 풀은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">Bomb 슬롯을 가진 풀.</param>
    /// <param name="hooks">폭발·소리·공통 전처리 경계. 실제로 선택되는 경계는 호출 시점에 검사한다.</param>
    public RawBombPreDestroy(RawSidPool pool, BombPreDestroyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hooks);
        _pool = pool;
        _hooks = hooks;
    }

    /// <summary>이 몸체가 읽는 슬롯 풀. 연결 어댑터가 같은 월드인지 확인할 때 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 슬롯 앞 4바이트가 10.78 Point Blast Bomb 가상 표인지 확인한다(원본 0041e960 의 진입 조건).
    /// 입력 TYPE 이름으로 가상 표를 넓히지 않는다.
    /// </summary>
    /// <param name="sid">확인할 번호.</param>
    public bool Handles(int sid) => UInt(_pool.Slot(sid), 0) == PatchBombVtable;

    /// <summary>
    /// 원본 0041e960 전체다. 폭발 플래그가 있고 가림 비트가 없을 때만 폭발·소리를 내고,
    /// 폭발이 바꾼 현재 좌표로 소리를 낸 뒤 공통 전처리를 같은 플래그로 한 번 부른다.
    /// </summary>
    /// <param name="sid">삭제되는 Bomb 번호.</param>
    /// <param name="flags">삭제 플래그. 폭발 요구 비트와 공통 전처리에 그대로 쓴다.</param>
    public void PreDestroy(int sid, uint flags)
    {
        // 가상 표가 다르면 효과를 내기 전에 거부한다
        if (!Handles(sid))
        {
            throw new InvalidOperationException("Bomb 고유 전처리의 실제 가상 표 불일치");
        }
        // 최초 추가 비트와 폭발 플래그를 한 번만 검사한다
        if ((_pool.Slot(sid)[ExtraOffset] & SkipMask) == 0 && (flags & ExplosionFlag) != 0)
        {
            Need(_hooks.Explosion, "폭발 효과").Invoke(sid);
            // 폭발 효과가 좌표를 바꿨을 수 있어 소리 전에 다시 읽는다
            ReadOnlySpan<byte> now = _pool.Slot(sid);
            Need(_hooks.Sound, "폭발 소리").Invoke(
                BitConverter.UInt32BitsToSingle(UInt(now, XOffset)),
                BitConverter.UInt32BitsToSingle(UInt(now, YOffset)));
        }
        // 폭발 여부와 무관하게 공통 사전 파괴를 같은 플래그로 부른다
        Need(_hooks.Base, "공통 사전 파괴").Invoke(sid, flags);
    }

    /// <summary>슬롯의 4바이트 값을 리틀 엔디언으로 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint UInt(ReadOnlySpan<byte> raw, int offset)
    {
        // 호스트 바이트 순서에 기대지 않고 낮은 바이트부터 합친다
        return (uint)(raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24));
    }

    /// <summary>선택된 경계가 없으면 앞선 효과를 더 진행하지 않고 명시 거부한다.</summary>
    /// <param name="value">확인할 경계.</param>
    /// <param name="name">누락 진단에 쓸 경계 이름.</param>
    private static T Need<T>(T? value, string name)
        where T : class
    {
        // null 경계는 그 효과를 내기 전에 거부한다
        if (value is null)
        {
            throw new InvalidOperationException("Bomb 실제 전처리 경계 미연결: " + name);
        }
        return value;
    }
}
