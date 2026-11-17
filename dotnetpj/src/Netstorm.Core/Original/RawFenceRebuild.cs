// PATCH 썬 바리케이드의 동맹 이웃·사거리·재생성 호출 순서를 같은 SID 풀에 적용한다.
// cpppj/src/o/RawFenceRebuild.h·cpp 의 RawFenceRebuild 본체를 이식한다.
// 0.590에서 추가됐다. (R4-1 잔여 중 펜스 수명 실제 이웃 결선)
namespace Netstorm.Core.Original;

// 펜스 재생성 모듈을 담는 네임스페이스다. 0.590에서 추가됐다. (R4-1 펜스 재생성 효과)

/// <summary>
/// 실제 현재 동맹 이웃·사거리·표식·포스 필드 몸체를 제공하는 효과 경계다.
/// (원본 FenceRebuildHooks, 10.78).
/// </summary>
public sealed class FenceRebuildHooks
{
    /// <summary>00455fb0의 동맹 최근접 검색이다. 0.590에서 추가됐다.</summary>
    public Func<int, uint, int>? Neighbor { get; set; }

    /// <summary>00455f00의 실제 현재 사거리 판정이다. 0.590에서 추가됐다.</summary>
    public Func<int, float[], bool>? WithinRange { get; set; }

    /// <summary>00456460의 내부 표식 생성이다. 0.590에서 추가됐다.</summary>
    public Action<int, int>? Markers { get; set; }

    /// <summary>00455b20(mode1)의 현재 좌표/owner/끝점 SID 생성이다. 0.590에서 추가됐다.</summary>
    public Action<float[], uint, uint, int, int>? Shields { get; set; }
}

/// <summary>
/// 원본의 부모 extra 9/대상 extra 21 검사를 원래 위치에서 수행하는 재생성 본체다.
/// (원본 RawFenceRebuild, 10.78).
/// </summary>
public sealed class RawFenceRebuild
{
    /// <summary>슬롯 안 추가 비트 바이트의 위치. 0.590에서 추가됐다.</summary>
    private const int ExtraOffset = 40;

    /// <summary>부모 진입 검사의 abstract·매립·contained 마스크(9)다. 0.590에서 추가됐다.</summary>
    private const uint BlockedMask = 9u;

    /// <summary>대상 건너뜀 검사의 마스크(0x21)다. 0.590에서 추가됐다.</summary>
    private const uint TargetSkipMask = 0x21u;

    /// <summary>슬롯 안 소유자 바이트의 위치. 0.590에서 추가됐다.</summary>
    private const int OwnerOffset = 34;

    /// <summary>슬롯 안 X 좌표 DWORD(4바이트)의 위치. 0.590에서 추가됐다.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 Y 좌표 DWORD(4바이트)의 위치. 0.590에서 추가됐다.</summary>
    private const int YOffset = 18;

    /// <summary>현재 TYPE/extra/owner/좌표를 소유한 실제 풀. 0.590에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>네 방향을 처리하는 실제 소비자다. 0.590에서 추가됐다.</summary>
    private readonly FenceRebuildHooks _hooks;

    /// <summary>
    /// 펜스 재생성 본체를 만든다. 풀과 효과 묶음은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">좌표·owner를 읽는 단일 raw 풀.</param>
    /// <param name="hooks">실제 하위 효과 묶음이다.</param>
    public RawFenceRebuild(RawSidPool pool, FenceRebuildHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("썬 바리케이드 재생성 판본 오류", nameof(pool));
        }

        if (hooks.Neighbor is null || hooks.WithinRange is null || hooks.Markers is null || hooks.Shields is null)
        {
            throw new ArgumentException("썬 바리케이드 재생성 효과 누락", nameof(hooks));
        }

        _pool = pool;
        _hooks = hooks;
    }

    /// <summary>실제 단일 풀을 연결 단계에서 확인한다. 0.590에서 추가됐다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>형식화한 사본을 오래 보관하지 않고 호출 직전에 현재 좌표를 읽는다.</summary>
    /// <param name="parent">기준 펜스 번호.</param>
    /// <param name="other">이웃 펜스 번호.</param>
    private float[] Endpoints(int parent, int other)
    {
        ReadOnlySpan<byte> a = _pool.Slot(parent);
        ReadOnlySpan<byte> b = _pool.Slot(other);
        return [ReadSingle(a, XOffset), ReadSingle(a, YOffset), ReadSingle(b, XOffset), ReadSingle(b, YOffset)];
    }

    /// <summary>
    /// 동맹 이웃→거리→대상 extra→표식→현재 포스 필드 인수의 순서로 재생성한다.
    /// 부모 extra는 진입 때만 검사하고 하위 효과 뒤 대상 extra/owner/좌표는 다시 읽는다.
    /// </summary>
    /// <param name="parent">재생성할 펜스 번호.</param>
    public void Rebuild(int parent)
    {
        if ((_pool.Slot(parent)[ExtraOffset] & BlockedMask) != 0)
        {
            return;
        }

        // 원본 네 방향 0/2/4/6의 순서다.
        for (uint direction = 0; direction < 8; direction += 2)
        {
            int other = Need(_hooks.Neighbor)(parent, direction);
            if (other == 0)
            {
                continue;
            }

            bool within = Need(_hooks.WithinRange)(parent, Endpoints(parent, other));
            if (!within || ((_pool.Slot(other)[ExtraOffset] & TargetSkipMask) != 0))
            {
                continue;
            }

            Need(_hooks.Markers)(parent, other);
            // 표식 생성 뒤 현재 owner들을 다시 읽는다.
            uint first = _pool.Slot(parent)[OwnerOffset];
            uint second = _pool.Slot(other)[OwnerOffset];
            Need(_hooks.Shields)(Endpoints(parent, other), first, second, parent, other);
        }
    }

    /// <summary>비정렬 float 좌표의 비트를 그대로 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static float ReadSingle(ReadOnlySpan<byte> raw, int offset) =>
        BitConverter.UInt32BitsToSingle(ReadUint(raw, offset));

    /// <summary>비정렬 raw DWORD를 낮은 바이트부터 조립한다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint ReadUint(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        // 정확한 네 바이트만 결합한다. 이웃 바이트는 읽지 않는다.
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)raw[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>실제 선택된 효과가 빠졌을 때 빈 처리로 넘어가지 않는다.</summary>
    /// <typeparam name="T">효과 대리자 형식.</typeparam>
    /// <param name="hook">호출할 효과.</param>
    private static T Need<T>(T? hook) where T : class =>
        hook ?? throw new InvalidOperationException("썬 바리케이드 재생성의 실제 하위 미연결");
}
