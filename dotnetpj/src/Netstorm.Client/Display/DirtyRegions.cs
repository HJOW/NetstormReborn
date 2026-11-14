// Renderer의 공통 변경영역 표를 실제 프레임/출력 소비자가 공유한다.
namespace Netstorm.Client.Display;

/// <summary>원문 변경영역과 현재 플래그다.</summary>
/// <param name="Rect">자르기 전 화면 사각형이다.</param>
/// <param name="Flags">원문 무시/불투명 표식이다.</param>
public readonly record struct DirtyRegion(ScreenRect Rect, uint Flags);

/// <summary>Renderer.cpp의100항목 변경표·첫병합·전체갱신/첫flags잔여를 보존한다.</summary>
public sealed class DirtyRegions
{
    /// <summary>원문 표의 최대 항목 수다.</summary>
    private const int Capacity = 100;
    /// <summary>화면 바깥 추가를 거부하는 가로 범위다.</summary>
    private readonly int _width;
    /// <summary>화면 바깥 추가를 거부하는 세로 범위다.</summary>
    private readonly int _height;
    /// <summary>카메라와 같은owner주소를 유지하는 실제 목록이다.</summary>
    private readonly List<DirtyRegion> _entries = [];
    /// <summary>Count0 뒤에도 물리 첫flags는 남는다.</summary>
    private uint _clearedFirstFlags;
    /// <summary>넘침/전체갱신 요청 표식이다.</summary>
    public bool FullRedraw { get; private set; }
    /// <summary>구멍/무시표식까지 원문 순서의 현재 항목이다.</summary>
    public IReadOnlyList<DirtyRegion> Entries => _entries;
    /// <summary>원문497960 카메라가 직접 이동/확장할 같은 목록이다.</summary>
    public List<DirtyRegion> CameraEntries => _entries;
    /// <summary>현재 첫항목 또는Count0 뒤 남은flagsDWORD다.</summary>
    public uint FirstFlags => _entries.Count == 0 ? _clearedFirstFlags : _entries[0].Flags;
    /// <summary>양수 화면 범위를 확인하며 표는 비어 있는 상태로 시작한다.</summary>
    public DirtyRegions(int width, int height)
    {
        if (width <= 0 || height <= 0) throw new ArgumentOutOfRangeException(nameof(width), "변경영역 화면크기 오류");
        _width = width; _height = height;
    }
    /// <summary>정수폭을 먼저 넓혀 원문 half-open면적을 계산한다.</summary>
    private static long Area(ScreenRect rect) => ((long)rect.Right - rect.Left) * ((long)rect.Bottom - rect.Top);
    /// <summary>실제 출력 때 자르므로 여기서는 자르지 않고 첫 병합대상에서만 반환한다.</summary>
    public void Add(ScreenRect rect, uint flags = 0)
    {
        if (!(rect.Left < _width && rect.Top < _height && rect.Right > 0 && rect.Bottom > 0)) return;
        if (_entries.Count >= Capacity) { FullRedraw = true; return; }
        long area = Area(rect);
        // 무시되지 않은 기존항목을 원문 순서로 검사하며 첫 병합 뒤 재병합하지 않는다.
        for (int i = 0; i < _entries.Count; i++)
        {
            DirtyRegion entry = _entries[i]; if ((entry.Flags & 1) != 0) continue;
            long oldArea = Area(entry.Rect); var united = new ScreenRect(Math.Min(entry.Rect.Left, rect.Left), Math.Min(entry.Rect.Top, rect.Top), Math.Max(entry.Rect.Right, rect.Right), Math.Max(entry.Rect.Bottom, rect.Bottom)); long unitedArea = Area(united);
            if ((flags & 4) != 0 && unitedArea == area) _entries[i] = entry with { Flags = entry.Flags | 1 };
            else
            {
                if ((entry.Flags & 4) != 0 && unitedArea == oldArea) return;
                if (unitedArea - unitedArea / 8 < oldArea + area) { _entries[i] = entry with { Rect = united }; return; }
            }
        }
        _entries.Add(new(rect, flags & ~1u));
    }
    /// <summary>현재 항목은 보존하며 전체갱신만 요청한다.</summary>
    public void InvalidateAll() => FullRedraw = true;
    /// <summary>출력 뒤 표/전체표식은 지우고 물리 첫flagsDWORD는 보존한다.</summary>
    public void Clear()
    {
        if (_entries.Count != 0) _clearedFirstFlags = _entries[0].Flags;
        FullRedraw = false; _entries.Clear();
    }
}
