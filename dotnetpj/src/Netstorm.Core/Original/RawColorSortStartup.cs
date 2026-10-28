namespace Netstorm.Core.Original;

/// <summary>
/// 같은 Client 수명의 실제 포인터 수·개수·불러옴 여부를 직접 빌리는 ColorSort 시작 전역 묶음.
/// 근거: cpppj/src/o/RawColorSortStartup.h 의 ColorSortStartupGlobals. (R2d-1, 2026-10-28)
/// C++의 참조 멤버를 이 클론에서는 호출자가 소유한 같은 객체로 둔다.
/// </summary>
public sealed class ColorSortStartupGlobals
{
    /// <summary>현재 0x3800 ColorSort 주소(00555d88).</summary>
    public uint Primary;

    /// <summary>현재 0x5008 검색 객체 주소(00555dd8).</summary>
    public uint Search;

    /// <summary>현재 0x5020 기본 색띠 주소(00555d78).</summary>
    public uint Gradient;

    /// <summary>현재 개수. 첫 부모가 10을 쓰며 색띠 할당 뒤 현재 값을 캡처한다(0053199c).</summary>
    public uint Count;

    /// <summary>현재 팔레트 불러옴 여부(00555e08). 0이면 진단 뒤 계속한다.</summary>
    public uint PaletteLoaded;
}

/// <summary>
/// 토큰은 실제 호스트 포인터가 아니며 생성·정렬·색띠는 원본 하위와 같은 필수 구현을 연결해야 한다.
/// 근거: cpppj/src/o/RawColorSortStartup.h 의 ColorSortStartupHooks. (R2d-1, 2026-10-28)
/// </summary>
public sealed class ColorSortStartupHooks
{
    /// <summary>원본 Colorsort.cpp 451행의 불러옴 진단을 호출자에게 알린다.</summary>
    public Action<uint>? Assertion { get; set; }

    /// <summary>현재 비어 있지 않은 슬롯을 비운다. 0 쓰기는 부모에 남긴다.</summary>
    public Action<uint>? Release { get; set; }

    /// <summary>0x3800·0x5008·0x5020의 실제 할당 결과(0은 실패)다.</summary>
    public Func<uint, uint>? Allocate { get; set; }

    /// <summary>439de0(this, 같은 논리 포인터)의 실제 반환 주소. 입력은 이 시점의 현재 내용이다.</summary>
    public Func<uint, uint, uint>? Construct { get; set; }

    /// <summary>43a8e0(this)의 실제 검색 생성. 자체 진입에서 현재 primary를 읽는다.</summary>
    public Action<uint>? ConstructSearch { get; set; }

    /// <summary>직접 vtable 저장을 위한 같은 실제 할당의 안정된 뷰다.</summary>
    public Func<uint, byte[]>? Memory { get; set; }

    /// <summary>qsort(검색+8, 256, 80, 0x43a1e0)의 실제 정렬이다.</summary>
    public Action<uint, uint, uint, uint>? Sort { get; set; }

    /// <summary>43ad20(this, from, to, 현재 count, 2.4)의 실제 색띠 계산·반환 주소.</summary>
    public Func<uint, double[], double[], uint, double, uint>? ConstructGradient { get; set; }
}

/// <summary>
/// 원본 43b430/43b300 생성 부모의 현재 슬롯·호출 순서만 복원하며 하위 실제 계산은 필수 경계로 분리한다.
/// 근거: cpppj/src/o/RawColorSortStartup.cpp. (R2d-1, 2026-10-28)
/// </summary>
public static class RawColorSortStartup
{
    /// <summary>원본 00506698/00503678의 정확한 double 회색 입력.</summary>
    private static readonly double Gray = BitConverter.UInt64BitsToDouble(0x3fcb9b9b9b9b9b9cul);

    /// <summary>원본 색띠 너비 2.4의 double 입력.</summary>
    private const double Width = 2.4;

    /// <summary>43b502가 직접 쓰는 파생 vtable 식별자.</summary>
    private const uint SearchVtableValue = 0x506658;

    /// <summary>qsort 비교자 0x43a1e0.</summary>
    private const uint SortComparator = 0x43a1e0;

    /// <summary>검색 정렬의 항목 수·기록 크기.</summary>
    private const uint SortCount = 256;

    /// <summary>검색 정렬의 기록 크기.</summary>
    private const uint SortRecordBytes = 80;

    /// <summary>검색 본문 시작 오프셋(search+8).</summary>
    private const uint SortBodyOffset = 8;

    /// <summary>첫 부모가 쓰는 개수.</summary>
    private const uint FirstCount = 10;

    /// <summary>primary 슬롯의 할당 크기.</summary>
    private const uint PrimaryBytes = 0x3800;

    /// <summary>검색 슬롯의 할당 크기.</summary>
    private const uint SearchBytes = 0x5008;

    /// <summary>색띠 슬롯의 할당 크기.</summary>
    private const uint GradientBytes = 0x5020;

    /// <summary>
    /// 같은 검색 객체의 첫 DWORD만 리틀 엔디언 vtable로 쓴다. 인접 미작성 바이트는 보존한다.
    /// </summary>
    /// <param name="memory">같은 실제 할당의 안정된 버퍼.</param>
    private static void SearchVtable(byte[] memory)
    {
        ArgumentNullException.ThrowIfNull(memory);
        if (memory.Length < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(memory), "ColorSort 검색 vtable 물리 범위 오류");
        }
        // 원본 low byte부터 네 바이트를 쓴다
        for (int i = 0; i < 4; i++)
        {
            memory[i] = (byte)(SearchVtableValue >> (i * 8));
        }
    }

    /// <summary>
    /// 43b300: 현재 색띠의 조건부 비움·0→새 할당→현재 개수·고정 RGB→실제 색띠 생성→반환 주소 대입이다.
    /// </summary>
    /// <param name="globals">같은 Client 수명의 현재 슬롯 묶음.</param>
    /// <param name="hooks">실제 할당·색띠 생성의 외부 장치 연결.</param>
    public static void InitializeColorSortGradient(ColorSortStartupGlobals globals, ColorSortStartupHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (globals.Gradient != 0)
        {
            (hooks.Release ?? throw new InvalidOperationException("ColorSort 색띠 해제가 연결되지 않았습니다"))(globals.Gradient);
            globals.Gradient = 0;
        }
        uint allocated = (hooks.Allocate ?? throw new InvalidOperationException("ColorSort 색띠 할당이 연결되지 않았습니다"))(GradientBytes);
        if (allocated != 0)
        {
            uint count = globals.Count;
            // 이후 생성자 콜백 중 바뀌어도 현재 인자는 캡처값이다
            globals.Gradient = (hooks.ConstructGradient ?? throw new InvalidOperationException("ColorSort 색띠 생성이 연결되지 않았습니다"))(
                allocated, [Gray, Gray, Gray], [1.0, 0.0, 0.0], count, Width);
        }
        else
        {
            globals.Gradient = 0;
        }
    }

    /// <summary>
    /// 43b430: 불러옴 진단→두 현재 슬롯 비움·0→primary 생성→검색 생성·vtable·qsort→검색 대입→개수 10→색띠 부모다.
    /// </summary>
    /// <param name="logical">같은 논리 포인터의 내용. 생성자 진입에서 처음 읽는다.</param>
    /// <param name="globals">같은 Client 수명의 현재 슬롯 묶음.</param>
    /// <param name="hooks">실제 할당·생성·정렬의 외부 장치 연결.</param>
    public static void InitializeColorSort(uint logical, ColorSortStartupGlobals globals, ColorSortStartupHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (globals.PaletteLoaded == 0)
        {
            (hooks.Assertion ?? throw new InvalidOperationException("ColorSort 불러옴 진단이 연결되지 않았습니다"))(451);
        }
        if (globals.Primary != 0)
        {
            (hooks.Release ?? throw new InvalidOperationException("ColorSort primary 해제가 연결되지 않았습니다"))(globals.Primary);
            globals.Primary = 0;
        }
        if (globals.Search != 0)
        {
            (hooks.Release ?? throw new InvalidOperationException("ColorSort 검색 해제가 연결되지 않았습니다"))(globals.Search);
            globals.Search = 0;
        }
        uint primary = (hooks.Allocate ?? throw new InvalidOperationException("ColorSort primary 할당이 연결되지 않았습니다"))(PrimaryBytes);
        // 같은 논리 포인터의 내용은 뒤 생성자 진입에서 처음 읽는다
        globals.Primary = primary != 0
            ? (hooks.Construct ?? throw new InvalidOperationException("ColorSort primary 생성이 연결되지 않았습니다"))(primary, logical)
            : 0u;
        uint search = (hooks.Allocate ?? throw new InvalidOperationException("ColorSort 검색 할당이 연결되지 않았습니다"))(SearchBytes);
        if (search != 0)
        {
            (hooks.ConstructSearch ?? throw new InvalidOperationException("ColorSort 검색 생성이 연결되지 않았습니다"))(search);
            SearchVtable((hooks.Memory ?? throw new InvalidOperationException("ColorSort 검색 메모리가 연결되지 않았습니다"))(search));
            (hooks.Sort ?? throw new InvalidOperationException("ColorSort 검색 정렬이 연결되지 않았습니다"))(unchecked(search + SortBodyOffset), SortCount, SortRecordBytes, SortComparator);
        }
        globals.Search = search;
        globals.Count = FirstCount;
        InitializeColorSortGradient(globals, hooks);
    }
}
