// 004566d0의 실제 fencemark 끝점 태그/현재 finder/가상 삭제를 같은 월드에서 실행한다.
// cpppj/src/o/RawFenceMarkerClear.h·cpp 의 RawFenceMarkerClear 본체를 이식한다.
// 0.589에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Fence 정리 효과)
namespace Netstorm.Core.Original;

// 펜스 표식 정리 모듈을 담는 네임스페이스다. 0.589에서 추가됐다. (R4-1 펜스 정리 효과)

/// <summary>
/// 화면의 포스 필드(TYPE170)와 별도인 fencemark(TYPE90)의 기존 연결 표식을 지우는 본체다.
/// (원본 RawFenceMarkerClear, 10.78).
/// </summary>
public sealed class RawFenceMarkerClear
{
    /// <summary>원본이 x87에서 더한 뒤 절삭하는 float 상수다. 0.589에서 추가됐다.</summary>
    private const float MarkerCeilBias = 0.9999f;

    /// <summary>표식 삭제에 쓰는 가상 flags다. 0.589에서 추가됐다.</summary>
    private const uint DestroyFlags = 0x40u;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치. 0.589에서 추가됐다.</summary>
    private const int TypeOffset = 10;

    /// <summary>현재 두 끝점과 후보의 전체 원본 슬롯을 읽는 풀. 0.589에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>삭제 도중 Next가 현재 체인을 읽는 실제 공간 해시. 0.589에서 추가됐다.</summary>
    private readonly RawSquidHash _hash;

    /// <summary>finder의 실제 발자국 표. 0.589에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>같은 풀의 004adea0 현재 중심을 조회하는 함수. 0.589에서 추가됐다.</summary>
    private readonly Func<int, float[]> _center;

    /// <summary>DAT005411c0의 현재 fencemark 타입 번호 상자. 0.589에서 추가됐다.</summary>
    private readonly UIntBox _markerTypeBox;

    /// <summary>선택 위치의 실제 가상 삭제 소비자. 비우면 Clear에서 명시 실패한다. 0.589에서 추가됐다.</summary>
    private readonly Action<int, uint>? _destroy;

    /// <summary>
    /// 같은 원문 중심 getter를 표식 생성과 공유하고 실제 후보/삭제 본문을 유지한다. 참조 대상은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">현재 두 끝점과 후보의 전체 원본 슬롯 풀.</param>
    /// <param name="hash">삭제 도중 Next가 현재 체인을 읽는 실제 공간 해시.</param>
    /// <param name="types">finder의 실제 발자국 표.</param>
    /// <param name="markerType">DAT005411c0의 현재 fencemark 타입 번호 상자.</param>
    /// <param name="destroy">선택 위치의 실제 가상 삭제 소비자.</param>
    /// <param name="center">같은 풀의 현재 중심 조회 함수.</param>
    public RawFenceMarkerClear(
        RawSidPool pool,
        RawSquidHash hash,
        IReadOnlyList<OriginalTypeRecord> types,
        UIntBox markerType,
        Action<int, uint>? destroy,
        Func<int, float[]> center)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(markerType);
        ArgumentNullException.ThrowIfNull(center);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("fencemark 기존 표식 삭제 10.78 풀 계약 오류", nameof(pool));
        }

        if (types.Count != 188)
        {
            throw new ArgumentException("fencemark 기존 표식 삭제 타입 표 크기 오류", nameof(types));
        }

        _pool = pool;
        _hash = hash;
        _types = types;
        _markerTypeBox = markerType;
        _destroy = destroy;
        _center = center;
    }

    /// <summary>실제 단일 풀을 연결 단계에서 확인한다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>현재 fencemark 타입 번호를 읽는다.</summary>
    public uint MarkerType => _markerTypeBox.Value;

    /// <summary>
    /// 원본은 다른 끝점 중심을 먼저 읽고 Rect의 들어오는 corner만 1..255로 clamp한다.
    /// </summary>
    /// <param name="parent">기준 펜스 번호.</param>
    /// <param name="other">이웃 펜스 번호.</param>
    public SquidSearchArea Bounds(int parent, int other)
    {
        float[] second = _center(other);
        float[] first = _center(parent);
        if (!float.IsFinite(first[0]) || !float.IsFinite(first[1]) || !float.IsFinite(second[0]) || !float.IsFinite(second[1]))
        {
            throw new ArgumentOutOfRangeException(nameof(parent), "fencemark 비유한 중심 좌표");
        }

        float x = Math.Clamp(second[0], 1.0f, 255.0f);
        float y = Math.Clamp(second[1], 1.0f, 255.0f);
        bool valid = first[0] > 0 && first[1] > 0 && first[0] < 256 && first[1] < 256;
        float left = valid ? Math.Min(first[0], x) : x;
        float top = valid ? Math.Min(first[1], y) : y;
        float right = valid ? Math.Max(first[0], x) : x;
        float bottom = valid ? Math.Max(first[1], y) : y;
        return new SquidSearchArea(
            MarkerInt(left), MarkerInt(top),
            MarkerInt((double)right + MarkerCeilBias), MarkerInt((double)bottom + MarkerCeilBias));
    }

    /// <summary>
    /// QA 또는 packed QB/graph가 현재 부모인 표식만 가상+10(flags40)으로 지운다. 실제 flags1은 표면을 건너뛰되 삭제되는 항목의 next는 같은 finder가 미리 보관한다.
    /// </summary>
    /// <param name="parent">기준 펜스 번호.</param>
    /// <param name="other">이웃 펜스 번호.</param>
    public void Clear(int parent, int other)
    {
        var finder = new RawSquidFinder(_pool, _hash, _types);
        // 모든 후보를 현재 순서대로 읽고 효과 뒤의 체인/태그 변경을 반영한다.
        for (int sid = finder.Begin(Bounds(parent, other), 1); sid != 0; sid = finder.Next())
        {
            ReadOnlySpan<byte> raw = _pool.Slot(sid);
            if (raw[TypeOffset] != MarkerType)
            {
                continue;
            }

            if (ReadUint(raw, 12, 2) != (uint)parent
                && (((ReadUint(raw, 26) << 8) | raw[30]) != (uint)parent))
            {
                continue;
            }

            Action<int, uint> destroy = _destroy
                ?? throw new InvalidOperationException("fencemark 기존 표식의 실제 가상 삭제 미연결");
            destroy(sid, DestroyFlags);
        }
    }

    /// <summary>지정한 WORD/DWORD 폭만 little endian으로 읽는다.</summary>
    /// <param name="raw">후보 슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽는 바이트 폭.</param>
    private static uint ReadUint(ReadOnlySpan<byte> raw, int offset, int width = 4)
    {
        uint value = 0;
        // 현재 후보의 비정렬 기록값이다. 이웃 필드에 걸치지 않는다.
        for (int index = 0; index < width; index++)
        {
            value |= (uint)raw[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>보드 중심의 유효 float를 원본 CRT 절삭으로 바꾼다.</summary>
    /// <param name="value">절삭할 값.</param>
    private static int MarkerInt(double value)
    {
        if (!double.IsFinite(value) || value < -2147483648.0 || value >= 2147483648.0)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "fencemark 검색 CRT 범위 오류");
        }

        return (int)value;
    }
}
