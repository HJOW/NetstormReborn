namespace Netstorm.Client.Display;

/// <summary>
/// <see cref="Screen"/>의 결과를 실제 화면(창·전체화면)으로 내보내는 호스트. MonoGame 호스트가 구현한다.
/// 원본의 GDI BitBlt·DwmFlush에 해당하는 부분만 호스트에 맡기고, 버퍼·팔레트·자르기 영역의 의미는 <see cref="Screen"/>이 소유한다.
/// (2026-10-28 R1에서 추가: 0절 결정 1 — 게임 화면은 8비트 논리 화면에 그리고 MonoGame은 최종 출력만 맡는다)
/// </summary>
public interface IScreenOutput
{
    /// <summary>논리 화면의 사각형이 갱신됐음을 알린다. 호스트는 인덱스를 현재 팔레트로 RGBA로 바꿔 내보낸다. 원본 004a1800 의 창 복사에 해당한다.</summary>
    /// <param name="screen">갱신된 화면.</param>
    /// <param name="rect">화면 안으로 잘린 갱신 사각형(비어 있지 않다).</param>
    void Present(Screen screen, ScreenRect rect);

    /// <summary>팔레트 적용 뒤 수직 동기를 기다린다. 원본 004a1fb0 에 해당하며 호스트가 필요 없으면 비워 둔다.</summary>
    /// <param name="screen">대상 화면.</param>
    void WaitVerticalBlank(Screen screen);
}

/// <summary>
/// 원본 Screen.cpp 의 화면 장치를 플랫폼 독립으로 옮긴 8비트 논리 화면: 행 간격이 있는 인덱스 버퍼, 256색 현재·저장·논리 팔레트, 모드 플래그, 잠금 깊이, 자르기 영역.
/// 창·DIB·GDI는 두지 않는다. 호스트가 <see cref="Output"/>으로 결과를 받아 화면에 낸다. 행 간격은 Win32 DIB와 같게 DWORD(4바이트) 정렬이다(예: 폭 1366 → 1368).
/// 사용: 생성 → <see cref="SetMode"/> → <see cref="LoadPalette"/> → <see cref="Lock"/>으로 얻은 버퍼에 그린 뒤 <see cref="Unlock"/> → <see cref="Update"/>.
/// 근거: 원본 004a1270·004a4570·004a4fe0·004a2640·004a14e0·004a1580·004a1800, cpppj/src/client/Screen.cpp. (2026-10-28 R1에서 이식)
/// </summary>
public sealed class Screen
{
    /// <summary>논리 팔레트 항목의 플래그(원본이 쓰는 4 = PC_NOCOLLAPSE).</summary>
    private const byte PaletteEntryFlags = 4;

    /// <summary>팔레트의 색 개수.</summary>
    private const uint PaletteSize = 256;

    /// <summary>파일 팔레트 적용 뒤 채우는 원소별 RGB 색이다. 바람=노랑, 비=파랑, 천둥=빨강, 해=갈색 순서(원본 004a4850 의 DAT_005b5da8..005b5db4).</summary>
    private static readonly int[][] WeatherRgb = [[255, 255, 22], [22, 22, 255], [255, 22, 22], [181, 140, 111]];

    /// <summary>행 간격을 포함한 인덱스 버퍼. 원본 DIB 섹션의 비트에 해당한다.</summary>
    private readonly byte[] _pixels;

    /// <summary>현재 표시 팔레트(DAT_0059afe0).</summary>
    private readonly ScreenColor[] _current = new ScreenColor[256];

    /// <summary>저장 팔레트(DAT_005acd28). 파일 팔레트와 모드 전환 때 복구하는 값이다.</summary>
    private ScreenColor[] _saved = new ScreenColor[256];

    /// <summary>논리 팔레트(DAT_005c7954): 빨강·초록·파랑·플래그 DWORD 256개. 색 검색은 이 값을 읽는다.</summary>
    private readonly uint[] _logical = new uint[256];

    /// <summary>현재 화면 모드 플래그(DAT_005c78d4).</summary>
    private uint _flags;

    /// <summary>중첩된 잠금 요청의 수(DAT_005c7920).</summary>
    private int _lockDepth;

    /// <summary>잠긴 동안만 값이 있는 그리기 버퍼(DAT_005c78f8).</summary>
    private byte[]? _buffer;

    /// <summary>현재 자르기 영역(DAT_005c7a44~50). Right·Bottom은 포함하지 않는다.</summary>
    private ScreenRect _clip;

    /// <summary>팔레트 읽음 DWORD(00555e08). 실제 Client가 소유하게 되면 R4에서 그 전역을 빌린다.</summary>
    private uint _paletteLoaded;

    /// <summary>파일 팔레트에서 계산한 기본·날씨 색 번호 표. 일시 팔레트 효과에는 갱신하지 않는다.</summary>
    private PaletteColorTable _colors = new();

    /// <summary>파일 팔레트에서 계산한 밝음·어두움 색 번호 변환표.</summary>
    private GumpShadeMaps _shadeMaps = new();

    /// <summary>논리 화면을 만든다. 모든 픽셀은 0번 색, 팔레트는 검정이며 모드는 아직 정해지지 않았다(flags 0).</summary>
    /// <param name="width">논리 폭(양수).</param>
    /// <param name="height">논리 높이(양수).</param>
    public Screen(int width, int height)
    {
        if (width <= 0 || height <= 0) throw new ArgumentOutOfRangeException(nameof(width), "화면 크기는 양수여야 한다.");
        Width = width;
        Height = height;
        Pitch = (width + 3) & ~3;
        _pixels = new byte[(long)Pitch * height];
        _clip = default;
    }

    /// <summary>실제 화면으로 내보내는 호스트. null이면 갱신을 내보내지 않는다(헤드리스 검사).</summary>
    public IScreenOutput? Output { get; set; }

    /// <summary>논리 폭.</summary>
    public int Width { get; }

    /// <summary>논리 높이.</summary>
    public int Height { get; }

    /// <summary>그리기 표면의 행 간격(DAT_005c78f0). Win32 DIB와 같이 4바이트 정렬이다.</summary>
    public int Pitch { get; }

    /// <summary>현재 화면 모드 플래그.</summary>
    public uint Flags => _flags;

    /// <summary>팔레트 읽음 DWORD. 파일 팔레트를 읽은 뒤 1이다.</summary>
    public uint PaletteLoaded => _paletteLoaded;

    /// <summary>현재 표시 팔레트. 호스트가 인덱스를 색으로 바꿀 때 읽는다.</summary>
    public ReadOnlySpan<ScreenColor> Palette => _current;

    /// <summary>논리 팔레트(R·G·B·플래그 DWORD 256개). 색 검색의 입력이다.</summary>
    public ReadOnlySpan<uint> LogicalPalette => _logical;

    /// <summary>호스트가 출력용으로 읽는 인덱스 버퍼 전체(행 간격 포함). 잠금 수를 바꾸지 않으며 쓰지 않는다.</summary>
    public ReadOnlySpan<byte> Pixels => _pixels;

    /// <summary>현재 자르기 영역을 바꾸지 않고 읽는다. Right·Bottom은 포함하지 않는다.</summary>
    public ScreenRect Clip => _clip;

    /// <summary>현재 자르기 영역(양 끝 포함 — VFX pane 좌표, DAT_005b5de0~ec).</summary>
    public ScreenRect Pane => new(_clip.Left, _clip.Top, _clip.Right - 1, _clip.Bottom - 1);

    /// <summary>
    /// 화면 모드를 정한다(원본 004a4fe0 의 모드 부분). 요청 값을 Windows 10/11 호환 모드로 정규화하고(<see cref="ScreenPresentation.ModernScreenMode"/>),
    /// 이미 같은 모드면 잠금 깊이만 0으로 하고 성공한다. 허용되지 않는 조합이면 거부한다. 새 모드면 현재 팔레트를 저장하고 저장 팔레트를 다시 적용한다.
    /// 창 위치·크기·커서 제한은 호스트의 몫이다.
    /// </summary>
    /// <param name="flags">요청할 <see cref="ScreenMode"/> 비트.</param>
    /// <returns>모드가 정해졌으면 true, 허용되지 않는 조합이면 false.</returns>
    public bool SetMode(uint flags)
    {
        flags = ScreenPresentation.ModernScreenMode(flags);
        _lockDepth = 0;
        _buffer = null;
        if (flags == _flags) return true;
        if (!ScreenGeometry.ScreenModeIsLegal(flags, true)) return false;
        _saved = (ScreenColor[])_current.Clone();
        _flags = flags;
        SetPalette(0, PaletteSize, null, true);
        return true;
    }

    /// <summary>
    /// 팔레트의 [start, start+count)를 colors로 바꾼다(원본 004a2640). colors가 null이면 저장 팔레트를 쓴다.
    /// 논리 팔레트에는 항상 반영하고(apply=false 준비도 색 검색에 보인다), apply가 참이면 현재 표시 팔레트에도 복사한다.
    /// 0번은 검정, 255번은 흰색으로 고정하며 이 두 값은 호출자가 넘긴 배열에도 써서 원본과 같이 보존한다.
    /// 전체화면 출력은 이미 낸 RGB 픽셀도 다시 칠해야 번개·날씨가 전체에 적용되므로 화면 전체를 갱신한다.
    /// </summary>
    /// <param name="start">시작 번호.</param>
    /// <param name="count">개수(0이고 apply면 전체 256색).</param>
    /// <param name="colors">색 배열. colors[0]이 start번 색이다. null이면 저장 팔레트.</param>
    /// <param name="apply">현재 표시 팔레트에 반영할지.</param>
    public void SetPalette(uint start, uint count, ScreenColor[]? colors, bool apply)
    {
        colors ??= _saved;
        uint end = unchecked(start + count);
        // 논리 팔레트 항목: 빨강·초록·파랑·플래그 순서.
        for (uint i = start; i < end && i < PaletteSize; i++)
        {
            ScreenColor color = colors[i - start];
            _logical[i] = color.Red | ((uint)color.Green << 8) | ((uint)color.Blue << 16) | ((uint)PaletteEntryFlags << 24);
        }
        // 0번은 검정, 255번은 흰색으로 고정한다(플래그 없음).
        _logical[0] = 0;
        _logical[255] = 0x00ffffff;
        if (start == 0) colors[0] = default;
        // 원본은 colors[255]에 쓴다(start가 0이라고 가정한 코드). 부분 배열을 넘겨도 범위를 넘지 않게 끝 항목에 쓴다.
        if (end == PaletteSize) colors[PaletteSize - 1 - start] = new ScreenColor(0xff, 0xff, 0xff, 0);
        if (!apply) return;
        if (count == 0) { start = 0; count = PaletteSize; }
        // 현재 표시 팔레트에 복사한다.
        for (uint i = 0; i < count && start + i < PaletteSize; i++) _current[start + i] = colors[i];
        if ((_flags & ScreenMode.FullScreen) != 0) Update(new ScreenRect(0, 0, Width, Height));
    }

    /// <summary>
    /// 파일 팔레트의 256색을 저장 팔레트에 RGBQUAD 순서로 넣고 적용한다(원본 004a4850). clear가 0이 아니면 적용 전에 화면을 검정으로 지우고
    /// 표시한 뒤 수직 동기를 기다린다(<see cref="ScreenPaletteApply.ApplyScreenPaletteClear"/>). RGB COL은 예약 바이트를 읽지 않으므로 기존 저장값을 유지한다.
    /// 파일 로더에서만 기본·날씨 색 표(<see cref="PaletteColors"/>)와 밝음·어두움 변환표(<see cref="PaletteShade"/>)를 다시 계산한다.
    /// </summary>
    /// <param name="palette">읽은 팔레트 파일.</param>
    /// <param name="clear">지우기 요청(0이면 지우지 않는다).</param>
    /// <param name="patch1078">10.78이면 참이다. 거짓이면 CD판처럼 12~19번 추가 색을 빼고 색 표를 계산한다.</param>
    public void LoadPalette(GamePalette palette, uint clear = 0, bool patch1078 = true)
    {
        // 색 번호를 그대로 유지한다.
        for (int i = 0; i < _saved.Length; i++)
        {
            PaletteColor color = palette.Color((byte)i);
            byte reserved = palette.Reserved((byte)i) ?? _saved[i].Reserved;
            _saved[i] = new ScreenColor(color.Blue, color.Green, color.Red, reserved);
        }
        var devices = new ScreenPaletteApplyHooks
        {
            Lock = () => Lock(),
            FillBlack = () =>
            {
                if ((_flags & ScreenMode.ModeSet) == 0) throw new InvalidOperationException("팔레트전환 Surface.fillBlt 미복원");
                FillRect(new ScreenRect(0, 0, Width, Height), 0);
            },
            Unlock = Unlock,
            WaitVerticalBlank = () => Output?.WaitVerticalBlank(this),
            PresentPreservingClip = () =>
            {
                ScreenRect previous = _clip;
                Update(new ScreenRect(0, 0, Width, Height));
                _clip = previous;
            },
            Flip = () =>
            {
                if ((_flags & ScreenMode.Flipping) != 0) throw new InvalidOperationException("팔레트전환 DirectDraw flip 장치 미복원");
            },
        };
        ScreenPaletteApply.ApplyScreenPaletteClear(clear, _flags, ref _paletteLoaded, devices);
        SetPalette(0, PaletteSize, null, true);
        // 파일 로더에서만 색 표를 다시 계산한다. 일시 팔레트 효과(SetPalette·번개)는 표를 바꾸지 않는다
        _colors = PaletteColors.BuildPaletteColorTable(_logical, patch1078);
        _shadeMaps = PaletteShade.BuildGumpShadeMaps(_logical);
    }

    /// <summary>현재 논리 팔레트에서 RGB와 가장 가까운 색 번호를 찾는다. <see cref="SetPalette"/>(apply=false)로 준비한 논리 팔레트도 반영한다.</summary>
    /// <param name="red">빨강.</param>
    /// <param name="green">초록.</param>
    /// <param name="blue">파랑.</param>
    public uint FindColor(int red, int green, int blue) => PaletteSearch.FindPaletteColor(_logical, red, green, blue);

    /// <summary>파일 팔레트에서 계산한 기본·날씨 색 번호 표. 일시 팔레트 효과에는 바뀌지 않는다.</summary>
    public PaletteColorTable Colors() => _colors;

    /// <summary>파일 팔레트에서 계산한 밝음·어두움 색 번호 변환표.</summary>
    public GumpShadeMaps ShadeMaps() => _shadeMaps;

    /// <summary>현재 논리 팔레트에서 날씨 RGB 네 색(바람·비·천둥·해)에 가장 가까운 번호를 찾는다. 팔레트 로더의 저장 별칭이 아니라 계산 도우미다.</summary>
    public uint[] WeatherTints()
    {
        var result = new uint[WeatherRgb.Length];
        // 각 원소의 RGB에 가장 가까운 현재 팔레트 번호를 기록한다.
        for (int i = 0; i < result.Length; i++) result[i] = FindColor(WeatherRgb[i][0], WeatherRgb[i][1], WeatherRgb[i][2]);
        return result;
    }

    /// <summary>
    /// 그리기 표면을 잠그고 인덱스 버퍼를 돌려준다(원본 004a14e0). 중첩 횟수를 세며 처음 잠글 때만 버퍼를 얻는다. 모드를 정하기 전에는 flags가 0이어서
    /// 그리기 함수가 아무것도 쓰지 않는다. 반드시 <see cref="Unlock"/>과 짝을 맞춘다.
    /// </summary>
    public byte[] Lock()
    {
        if (_lockDepth++ == 0) _buffer = _pixels;
        return _buffer!;
    }

    /// <summary>마지막 잠금이 풀릴 때 표면을 푼다(원본 004a1550). 잠그지 않은 상태에서 부르면 예외다.</summary>
    public void Unlock()
    {
        if (_lockDepth <= 0) throw new InvalidOperationException("Screen.Unlock: 잠금 깊이가 0이다.");
        if (--_lockDepth == 0) _buffer = null;
    }

    /// <summary>그리기 자르기 영역을 정한다(원본 004a1580). Right·Bottom은 포함하지 않는다.</summary>
    public void SetClip(int left, int top, int right, int bottom) => _clip = new ScreenRect(left, top, right, bottom);

    /// <summary>
    /// 잠근 그리기 버퍼의 사각형을 한 색으로 채운다(원본 004a2060). 현재 자르기 영역으로 먼저 자른다.
    /// 모드가 정해지기 전(ModeSet 없음)이거나 잠그지 않았으면 아무것도 쓰지 않는다.
    /// </summary>
    /// <param name="rect">채울 사각형.</param>
    /// <param name="color">팔레트 번호.</param>
    public void FillRect(ScreenRect rect, byte color)
    {
        rect = ScreenGeometry.ClipScreenRect(rect, _clip);
        if ((_flags & ScreenMode.ModeSet) == 0 || _buffer is null) return;
        // 위에서 아래로 한 줄씩 채운다.
        for (int y = rect.Top; y < rect.Bottom; y++)
            _buffer.AsSpan(Pitch * y + rect.Left, Math.Max(rect.Right - rect.Left, 0)).Fill(color);
    }

    /// <summary>잠그고 채우고 푼다(원본 004a4790). 화면 모드가 정해지기 전(flags 0)에는 아무것도 하지 않는다.</summary>
    /// <param name="rect">지울 사각형.</param>
    /// <param name="color">팔레트 번호.</param>
    public void Clear(ScreenRect rect, byte color)
    {
        if (_flags == 0) return;
        Lock();
        FillRect(rect, color);
        Unlock();
    }

    /// <summary>
    /// 원시 인덱스 사각형을 잠근 버퍼에 복사한다(원본 004a1fe0 → 004a0c10 의 DIB 경로). 원본 사각형 크기를 대상 크기로 바꾼 뒤 화면과 현재 자르기 영역으로 자르고,
    /// 행마다 4바이트 단위(마지막 0~3바이트는 1바이트씩)로 먼저 읽은 뒤 쓴다. 좌표 계산은 DWORD로 감긴다.
    /// 잠그지 않았거나 실제 물리 범위를 벗어나면 예외다.
    /// </summary>
    /// <param name="x">대상 X.</param>
    /// <param name="y">대상 Y.</param>
    /// <param name="source">원본 인덱스 버퍼.</param>
    /// <param name="sourcePitch">원본 행 간격.</param>
    /// <param name="sourceRect">원본 사각형.</param>
    public void BlitBuffer(int x, int y, ReadOnlySpan<byte> source, int sourcePitch, ScreenRect sourceRect)
    {
        static int Add(int a, int b) => unchecked(a + b);
        static int Subtract(int a, int b) => unchecked(a - b);
        ScreenRect clip = ScreenGeometry.ClipScreenRect(_clip, new ScreenRect(0, 0, Width, Height));
        ScreenRect target = ScreenGeometry.ClipScreenRect(
            new ScreenRect(x, y, Add(x, Subtract(sourceRect.Right, sourceRect.Left)), Add(y, Subtract(sourceRect.Bottom, sourceRect.Top))), clip);
        if (_buffer is null) throw new InvalidOperationException("구름 buffer 복사의 실제 잠금 누락");
        int width = Subtract(target.Right, target.Left), height = Subtract(target.Bottom, target.Top);
        if (width < 0 || height < 0 || sourcePitch < 0) throw new ArgumentOutOfRangeException(nameof(sourceRect), "구름 buffer 복사의 현재 물리 폭");
        long first = (long)Add(sourceRect.Top, Subtract(target.Top, y)) * sourcePitch + Add(sourceRect.Left, Subtract(target.Left, x));
        Span<byte> word = stackalloc byte[4];
        // 행마다 현재 원시 source와 같은 실제 잠긴 버퍼를 사용한다.
        for (int row = 0; row < height; row++)
        {
            long sourceRow = first + (long)row * sourcePitch, destinationRow = (long)(target.Top + row) * Pitch + target.Left;
            // 4바이트 단위 앞으로 복사와 마지막 0~3바이트를 같은 순서로 실행한다.
            for (int column = 0; column < width;)
            {
                int bytes = width - column >= 4 ? 4 : 1;
                long from = sourceRow + column, to = destinationRow + column;
                if (from < 0 || (ulong)from + (ulong)bytes > (ulong)source.Length || to < 0 || (ulong)to + (ulong)bytes > (ulong)Pitch * (ulong)Height)
                    throw new ArgumentOutOfRangeException(nameof(source), "구름 buffer 복사의 실제 물리 범위");
                source.Slice((int)from, bytes).CopyTo(word);
                word[..bytes].CopyTo(_buffer.AsSpan((int)to, bytes)); // 겹친 입력도 먼저 해당 단어를 읽은 뒤 쓴다.
                column += bytes;
            }
        }
    }

    /// <summary>
    /// 그리기 표면의 사각형을 화면으로 내보낸다(원본 004a1800). 화면으로 자른 뒤 자르기 영역을 화면 전체로 되돌린다. 비어 있으면 아무것도 내보내지 않는다.
    /// 실제 복사와 확대는 <see cref="Output"/>이 맡는다.
    /// </summary>
    /// <param name="rect">갱신할 사각형.</param>
    public void Update(ScreenRect rect)
    {
        var screen = new ScreenRect(0, 0, Width, Height);
        rect = ScreenGeometry.ClipScreenRect(rect, screen);
        _clip = screen;
        if (rect.Right <= rect.Left || rect.Bottom <= rect.Top) return;
        Output?.Present(this, rect);
    }

    /// <summary>
    /// 원본 4a1ad0 의 DIB 행 이동을 현재 버퍼에서 수행한다. 이동은 DIB 모드에서만 가능하다. 사용: 카메라 이동의 화면 스크롤.
    /// </summary>
    /// <param name="x">영역 왼쪽.</param>
    /// <param name="y">영역 위쪽.</param>
    /// <param name="width">영역 폭.</param>
    /// <param name="height">영역 높이.</param>
    /// <param name="dx">가로 이동량.</param>
    /// <param name="dy">세로 이동량.</param>
    public void Scroll(int x, int y, int width, int height, int dx, int dy)
    {
        if ((_flags & ScreenMode.BackBufferDib) == 0) throw new InvalidOperationException("화면 이동의 현재 DIB 장치 필요");
        if (dx == 0 && dy == 0) return;
        byte[] pixels = Lock();
        try { ScreenPresentation.ScrollScreenPixels(pixels, Pitch, Width, Height, x, y, width, height, dx, dy); }
        finally { Unlock(); }
    }

    /// <summary>
    /// 호환 전체화면의 현재 실제 출력 범위를 구한다. 전체화면이 아니면 논리 화면 전체, 전체화면이면 클라이언트 크기에 비율대로 맞춘 사각형이다.
    /// </summary>
    /// <param name="clientWidth">클라이언트 영역 폭.</param>
    /// <param name="clientHeight">클라이언트 영역 높이.</param>
    public ScreenRect PresentationRect(int clientWidth, int clientHeight)
    {
        if ((_flags & ScreenMode.FullScreen) == 0 || clientWidth <= 0 || clientHeight <= 0) return new ScreenRect(0, 0, Width, Height);
        return ScreenPresentation.FitScreenPresentation(Width, Height, clientWidth, clientHeight);
    }

    /// <summary>
    /// 실제 클라이언트 좌표를 논리 입력 좌표로 바꾼다. 창 모드는 1:1을 유지하고 호환 전체화면만 배율과 레터박스를 되돌린다.
    /// </summary>
    /// <param name="point">클라이언트 좌표.</param>
    /// <param name="clientWidth">클라이언트 영역 폭.</param>
    /// <param name="clientHeight">클라이언트 영역 높이.</param>
    public ScreenPoint ToLogicalPoint(ScreenPoint point, int clientWidth, int clientHeight) =>
        (_flags & ScreenMode.FullScreen) != 0
            ? ScreenPresentation.MapScreenPresentationPoint(point, PresentationRect(clientWidth, clientHeight), Width, Height)
            : point;
}
