// 간헐천의 실제 전처리와 후처리를 기존 Damageable 수명에 연결한다.
// cpppj/src/o/RawGeyserLifetime.h·cpp 의 RawGeyserLifetime 본체를 이식한다.
// 0.583에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Geyser 우선)
namespace Netstorm.Core.Original;

/// <summary>
/// 가상 기본 프레임·표시와 큰 AI·섬·공통 수명 효과의 실제 경계다.
/// (원본 GeyserLifetimeHooks, 10.78).
/// </summary>
public sealed class GeyserLifetimeHooks
{
    /// <summary>가상+78의 현재 TYPE+128 기본 프레임이다.</summary>
    public Func<int, uint>? DefaultFrame { get; set; }

    /// <summary>기본 프레임을 raw+24에 쓴 뒤 가상+88 한 번이다.</summary>
    public Action<int>? Update { get; set; }

    /// <summary>현재 Player AI 제거 요청이다.</summary>
    public Action<int, uint>? RegisterAi { get; set; }

    /// <summary>기존 실제 공통 수명이다.</summary>
    public Action<int, uint>? DamageablePre { get; set; }

    /// <summary>기존 실제 공통 수명이다.</summary>
    public Action<int, uint>? DamageablePost { get; set; }

    /// <summary>편집 모드에서만 현재 좌표/flags/owner로 호출한다.</summary>
    public Action<float, float, uint, uint>? DropIsland { get; set; }
}

/// <summary>
/// 현재 지면 목록은 PostPop/선택기와 같은 저장소이며 복사본이 아니다.
/// 간헐천 고유 수명 접두를 일반 Damageable로 대체하지 않는다.
/// (원본 RawGeyserLifetime, 10.78).
/// </summary>
public sealed class RawGeyserLifetime
{
    /// <summary>실제 간헐천 가상 표 값이다.</summary>
    private const uint GeyserVtable = 0x0050da88u;

    /// <summary>슬롯 안 추가 비트 바이트의 위치다.</summary>
    private const int ExtraOffset = 40;

    /// <summary>abstract·매립·contained 마스크다.</summary>
    private const uint BlockedMask = 9u;

    /// <summary>죽은 상태도 읽는 실제 raw 저장소다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>원래 현재 목록이다.</summary>
    private readonly RawPostPopList _geysers;

    /// <summary>현재 편집 모드 참조다.</summary>
    private readonly UIntBox? _editor;

    /// <summary>실제 Native owner만 켜며 일반 직접 입력은 기존 false다.</summary>
    private bool _nativeBody;

    /// <summary>선택된 효과 뒤에는 현재 raw/목록을 다시 읽는다.</summary>
    private readonly GeyserLifetimeHooks _hooks;

    /// <summary>
    /// 간헐천 수명 본체를 만든다. 풀·목록·편집 모드 참조는 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">객체 슬롯 풀.</param>
    /// <param name="geysers">원래 현재 목록이다.</param>
    /// <param name="editor">현재 편집 모드 참조다. null이면 편집 분기를 건너뛴다.</param>
    /// <param name="hooks">외부 효과 묶음이다.</param>
    public RawGeyserLifetime(RawSidPool pool, RawPostPopList geysers, UIntBox? editor, GeyserLifetimeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(geysers);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("간헐천 수명 판본 오류", nameof(pool));
        }

        if (hooks.DefaultFrame is null || hooks.Update is null || hooks.RegisterAi is null
            || hooks.DamageablePre is null || hooks.DamageablePost is null || hooks.DropIsland is null)
        {
            throw new ArgumentException("간헐천 수명 하위 효과 누락", nameof(hooks));
        }

        _pool = pool;
        _geysers = geysers;
        _editor = editor;
        _hooks = hooks;
    }

    /// <summary>같은 실제 raw 풀이다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>실제 PATCH 장기 owner는 원문 물리 접근만 선택하고 값을 변경하지 않는다.</summary>
    public void EnableNativeBody() => _nativeBody = true;

    /// <summary>타입 번호가 같더라도 다른 가상 표면 파생으로 확장하지 않는다.</summary>
    /// <param name="sid">확인할 객체 번호.</param>
    public bool Handles(int sid) => Read(_pool.Slot(sid), 0) == GeyserVtable;

    /// <summary>원본 앞방향 압축과 남은 물리 꼬리를 그대로 보존한다.</summary>
    /// <param name="sid">제거할 객체 번호.</param>
    private void Remove(int sid)
    {
        _ = _geysers.Items();
        uint removed = 0;
        // AI 제거 효과 뒤의 현재 count/내용으로 모든 중복을 제거한다.
        for (uint i = 0; i < _geysers.Count; i++)
        {
            uint value = _geysers.Entries[i];
            if (value == unchecked((uint)sid))
            {
                removed++;
            }
            else
            {
                _geysers.Entries[i - removed] = value;
            }
        }

        _geysers.Count -= removed;
    }

    /// <summary>원문은 signed count를 매 반복 조회하며 사용하지 않는 꼬리를 지우지 않는다.</summary>
    /// <param name="sid">제거할 객체 번호.</param>
    private void RemoveNative(int sid)
    {
        uint removed = 0;
        uint index = 0;
        // 앞선 store가 바꾼 현재 count/물리도 다음 반복에서 읽는다.
        while (unchecked((int)index) < unchecked((int)_geysers.Count))
        {
            uint[] storage = _geysers.Entries;
            if (index >= (uint)storage.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(sid), "Native 간헐천 제거의 현재 DWORD 물리 부족");
            }

            uint value = storage[index];
            if (value == unchecked((uint)sid))
            {
                removed++;
            }
            else
            {
                uint destination = index - removed;
                if (destination >= (uint)storage.Length)
                {
                    throw new ArgumentOutOfRangeException(nameof(sid), "Native 간헐천 압축의 현재 DWORD 물리 부족");
                }

                storage[destination] = value;
            }

            index++;
        }

        _geysers.Count -= removed;
    }

    /// <summary>getter 뒤 프레임 직접 저장이며 일반 SetFrame의 추가 Unpop/두 표시 갱신을 넣지 않는다.</summary>
    /// <param name="sid">전처리할 객체 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트다.</param>
    public void PreDestroy(int sid, uint flags)
    {
        uint frame = Need(_hooks.DefaultFrame)(sid);
        System.Span<byte> raw = _nativeBody ? _pool.RawSlot(sid) : _pool.AllocatedBytes(sid);
        // 현재 기본 프레임 DWORD만 기록한다.
        for (int i = 0; i < 4; i++)
        {
            raw[36 + i] = (byte)(frame >> (i * 8));
        }

        Need(_hooks.Update)(sid);
        if ((_pool.Slot(sid)[ExtraOffset] & BlockedMask) == 0)
        {
            Need(_hooks.RegisterAi)(sid, 0);
            if (_nativeBody)
            {
                RemoveNative(sid);
            }
            else
            {
                Remove(sid);
            }
        }

        Need(_hooks.DamageablePre)(sid, flags);
    }

    /// <summary>원본의 진입 조건을 한 번 평가하고 섬 정리 뒤에는 동일 flags로 공통 수명을 호출한다.</summary>
    /// <param name="sid">후처리할 객체 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트다.</param>
    public void PostDestroy(int sid, uint flags)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (_editor is { } editor && editor.Value != 0 && (raw[ExtraOffset] & BlockedMask) == 0)
        {
            Need(_hooks.DropIsland)(ReadSingle(raw, 14), ReadSingle(raw, 18), flags, raw[34]);
        }

        Need(_hooks.DamageablePost)(sid, flags);
    }

    /// <summary>현재 비정렬 raw DWORD를 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int offset) =>
        (uint)(raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24));

    /// <summary>원문 비트를 float 그대로 읽는다.</summary>
    private static float ReadSingle(ReadOnlySpan<byte> raw, int offset) =>
        BitConverter.UInt32BitsToSingle(Read(raw, offset));

    /// <summary>실제 선택된 큰 하위를 빈 성공으로 바꾸지 않는다.</summary>
    private static T Need<T>(T? hook) where T : class =>
        hook ?? throw new InvalidOperationException("간헐천 수명의 실제 하위 미연결");
}
