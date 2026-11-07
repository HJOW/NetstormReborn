// 원본 CanonDecoder.cpp: 패턴 표(여러 칸으로 이루어진 모양)를 회전을 반영해 한 칸씩 풀어 주는 반복자다 (cpppj o/CanonDecoder.h/.cpp, 2026-11-07 F2c-12 이식).
using System;

namespace Netstorm.Core.Original;

/// <summary>
/// 원본 72바이트 <c>CanonPattern</c> 안의 4바이트 셀 하나(변형 글자, 방향 글자, 셋째 바이트, 번호 글자)다 (2026-11-07 F2c-12 이식).
/// </summary>
/// <param name="Byte0">변형 숫자 글자('0'~).</param>
/// <param name="Byte1">방향 글자('A'~'P', 빈 칸은 '.').</param>
/// <param name="Byte2">원본 셋째 바이트.</param>
/// <param name="Byte3">번호 글자('a'~, 없으면 0).</param>
public readonly record struct CanonPatternCell(byte Byte0, byte Byte1, byte Byte2, byte Byte3)
{
    /// <summary>문자 리터럴로 4바이트 셀을 초기화한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="b0">변형 문자.</param>
    /// <param name="b1">방향 문자.</param>
    /// <param name="b2">셋째 바이트.</param>
    /// <param name="b3">번호 문자.</param>
    public CanonPatternCell(char b0, char b1, byte b2, char b3)
        : this((byte)b0, (byte)b1, b2, (byte)b3)
    {
    }

    /// <summary>셀 안의 0~3번 바이트를 인덱스로 읽는다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="index">바이트 인덱스(0..3).</param>
    /// <returns>해당 위치의 바이트.</returns>
    public byte this[int index] => index switch
    {
        0 => Byte0,
        1 => Byte1,
        2 => Byte2,
        3 => Byte3,
        _ => throw new ArgumentOutOfRangeException(nameof(index)),
    };
}

/// <summary>
/// 원본 72바이트 패턴 레코드(정수 셋과 4바이트 셀 15개)를 표현하는 클래스다(cpppj <c>o/CanonDecoder.h</c>의 <c>CanonPattern</c>).
/// 셀은 줄 우선으로 <c>Width × Height</c>만큼 사용한다 (2026-11-07 F2c-12 이식).
/// </summary>
public sealed class CanonPattern
{
    /// <summary>다리의 추첨 가중치다. 영역 패턴은 모두 1이다 (2026-11-07 F2c-12 이식).</summary>
    public int First { get; set; }

    /// <summary>패턴 가로 크기(+4)다 (2026-11-07 F2c-12 이식).</summary>
    public int Width { get; set; }

    /// <summary>패턴 세로 크기(+8)다 (2026-11-07 F2c-12 이식).</summary>
    public int Height { get; set; }

    /// <summary>고정 15개 셀 배열이다 (2026-11-07 F2c-12 이식).</summary>
    public CanonPatternCell[] Cells { get; }

    /// <summary>빈 15개 셀을 가진 패턴 레코드를 만든다 (2026-11-07 F2c-12 이식).</summary>
    public CanonPattern()
    {
        Cells = new CanonPatternCell[15];
    }

    /// <summary>가중치·폭·높이와 15개 셀 배열로 패턴 레코드를 만든다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="first">추첨 가중치 또는 첫 정수.</param>
    /// <param name="width">가로 크기.</param>
    /// <param name="height">세로 크기.</param>
    /// <param name="cells">15개 셀 배열.</param>
    public CanonPattern(int first, int width, int height, CanonPatternCell[] cells)
    {
        ArgumentNullException.ThrowIfNull(cells);
        if (cells.Length != 15)
        {
            throw new ArgumentException("CanonPattern 셀 배열은 15개여야 합니다.", nameof(cells));
        }
        First = first;
        Width = width;
        Height = height;
        Cells = cells;
    }
}

/// <summary>
/// 반복자보다 오래 사는 현재 checking 뷰와 원문 진단 장치 묶음이다(cpppj <c>o/CanonDecoder.h</c>의 <c>CanonDecoderDiagnostics</c>).
/// 사본도 같은 Client owner를 읽는다 (2026-11-07 F2c-12 이식).
/// </summary>
public sealed class CanonDecoderDiagnostics
{
    /// <summary>005e4794의 현재 DWORD를 진단 소비 위치마다 읽는 뷰다 (2026-11-07 F2c-12 이식).</summary>
    public RawCheckingFlag Checking { get; set; } = new();

    /// <summary>식·파일·행을 전달하며 복귀하면 원래 순회를 계속하는 진단 대리자다 (2026-11-07 F2c-12 이식).</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>
/// 원본 <c>CanonDecoder</c>(0x54바이트 반복자 객체, 패치 <c>00425c20</c>/<c>00425860</c>/<c>00425700</c>/<c>00425c00</c> ↔ CD <c>0041fcf0</c>/<c>0041feb0</c>/<c>0041fbb0</c>/<c>00420370</c>)를 복원한 클래스다.
/// 영역(puzzlePiece)·다리(bridge)·특수 패턴과 비패턴 한 칸 경로의 회전·프레임 검색·좌표 순회를 수행할 때 사용한다 (2026-11-07 F2c-12 이식).
/// </summary>
public sealed partial class CanonDecoder
{
    /// <summary>빈 칸을 뜻하는 방향 글자('.')다 (2026-11-07 F2c-12 이식).</summary>
    private const char EmptyOrientation = '.';

    /// <summary>방향 글자 허용 범위의 시작('A')이다 (2026-11-07 F2c-12 이식).</summary>
    private const char FirstOrientation = 'A';

    /// <summary>방향 글자 허용 범위의 끝('P')이다 (2026-11-07 F2c-12 이식).</summary>
    private const char LastOrientation = 'P';

    /// <summary>프레임을 찾을 때 변형 글자로 쓰는 회전 표의 열(16번째 글자, 인덱스 15)이다 (2026-11-07 F2c-12 이식).</summary>
    private const int VariantColumn = 15;

    /// <summary>번호 글자의 기준('a' = 0x61)이다 (2026-11-07 F2c-12 이식).</summary>
    private const int LabelBase = 0x61;

    /// <summary>변형 숫자 글자의 기준('0' = 0x30)이다 (2026-11-07 F2c-12 이식).</summary>
    private const int VariationBase = 0x30;

    /// <summary>변형 값의 허용 범위 끝(10)이다 (2026-11-07 F2c-12 이식).</summary>
    private const int MaxVariation = 10;

    /// <summary>실제 owner의 현재 설정·콜백을 빌리는 진단 묶음이다 (2026-11-07 F2c-12 이식).</summary>
    private readonly CanonDecoderDiagnostics? _diagnostics;

    /// <summary>cracked 프레임 진단이 없는 CD와 패치 부모를 구별하는 판본이다 (2026-11-07 F2c-12 이식).</summary>
    private readonly OriginalEdition _edition;

    /// <summary>타입 프레임의 코드와 방향별 번호 구간을 조회하는 표다 (2026-11-07 F2c-12 이식).</summary>
    private readonly RiftTypeFrames _frames;

    /// <summary>칸별 방향·변형·라벨을 펼칠 Canon 패턴 자료(비패턴 경로는 null)다 (2026-11-07 F2c-12 이식).</summary>
    private readonly CanonPattern? _pattern;

    /// <summary>실제 FindNumber 실패의 보고 후 계속 진행을 요청하는 콜백이다 (2026-11-07 F2c-12 이식).</summary>
    private readonly Action? _missingFrameDiagnostic;

    /// <summary>패턴을 회전할 방향 값(+8)이다 (2026-11-07 F2c-12 이식).</summary>
    private readonly int _rotation;

    /// <summary>좌우 반전 여부(+0xc, 영역 패턴에서는 항상 거짓)다 (2026-11-07 F2c-12 이식).</summary>
    private readonly bool _mirrored = false;

    /// <summary>타입에서 사용할 그림 프레임 번호(+0x10)다 (2026-11-07 F2c-12 이식).</summary>
    private int _frame = -1;

    /// <summary>현재 자료나 순회 항목을 읽을 수 있는지(+0x14)다 (2026-11-07 F2c-12 이식).</summary>
    private bool _valid = true;

    /// <summary>현재 칸의 번호 글자에서 'a'를 뺀 값(+0x18)이다 (2026-11-07 F2c-12 이식).</summary>
    private int _label;

    /// <summary>좌표 전진 단계(+0x1c, 기본 1)다 (2026-11-07 F2c-12 이식).</summary>
    private readonly int _step = 1;

    /// <summary>현재 칸의 X 좌표(+0x20)다 (2026-11-07 F2c-12 이식).</summary>
    private float _x;

    /// <summary>현재 칸의 Y 좌표(+0x24)다 (2026-11-07 F2c-12 이식).</summary>
    private float _y;

    /// <summary>좌표 계산의 기준 X 원점(+0x2c)이다 (2026-11-07 F2c-12 이식).</summary>
    private readonly float _originX;

    /// <summary>좌표 계산의 기준 Y 원점(+0x30)이다 (2026-11-07 F2c-12 이식).</summary>
    private readonly float _originY;

    /// <summary>패턴 바깥 반복의 칸 개수(+0x38)다 (2026-11-07 F2c-12 이식).</summary>
    private readonly int _outerCount;

    /// <summary>패턴 안쪽 반복의 칸 개수(+0x3c)다 (2026-11-07 F2c-12 이식).</summary>
    private readonly int _innerCount;

    /// <summary>현재 Canon 패턴의 X 인덱스(+0x40)다 (2026-11-07 F2c-12 이식).</summary>
    private int _patternX;

    /// <summary>현재 Canon 패턴의 Y 인덱스(+0x44)다 (2026-11-07 F2c-12 이식).</summary>
    private int _patternY;

    /// <summary>현재 패턴 바깥 반복 인덱스(+0x48)다 (2026-11-07 F2c-12 이식).</summary>
    private int _outer;

    /// <summary>현재 패턴 안쪽 반복 인덱스(+0x4c)다 (2026-11-07 F2c-12 이식).</summary>
    private int _inner;

    /// <summary>패턴 없는 타입의 기본 프레임이다 (2026-11-07 F2c-12 이식).</summary>
    private readonly int _defaultFrame;

    /// <summary>패턴 없는 타입의 원본 첫 인자다 (2026-11-07 F2c-12 이식).</summary>
    private readonly int _argument;

    /// <summary>첫 인자를 프레임 번호로 사용할지(+0x50)다 (2026-11-07 F2c-12 이식).</summary>
    private readonly bool _explicitFrame;

    /// <summary>영역 패턴 표(패치 005300f0, 68개)를 반환한다 (2026-11-07 F2c-12 이식).</summary>
    /// <returns>68개 영역 패턴 스팬.</returns>
    public static ReadOnlySpan<CanonPattern> TerritoryPatterns() => TerritoryPatternTable;

    /// <summary>다리 패턴 표(패치 0052f998, CD 00514a80, 26개)를 반환한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="edition">원본 판본.</param>
    /// <returns>26개 다리 패턴 스팬.</returns>
    public static ReadOnlySpan<CanonPattern> BridgePatterns(OriginalEdition edition = OriginalEdition.Patch1078)
    {
        return edition == OriginalEdition.Cd1072 ? CdBridgePatternTable : BridgePatternTable;
    }

    /// <summary>원본 004257c0 ↔ CD 0041fc70: 누적 가중치와 signed 나머지를 &lt;=로 비교해 다리 패턴 번호를 고른다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="random">난수 입력.</param>
    /// <param name="edition">원본 판본.</param>
    /// <returns>선택된 다리 패턴 인덱스.</returns>
    public static int SelectBridgePattern(int random, OriginalEdition edition = OriginalEdition.Patch1078)
    {
        ReadOnlySpan<CanonPattern> patterns = BridgePatterns(edition);
        int total = 0;
        // 원본 26개 항목의 가중치 합을 구한다
        for (int i = 0; i < patterns.Length; ++i)
        {
            total += patterns[i].First;
        }
        int value = random % total;
        int accumulated = 0;
        // 누적 경계는 엄격한 <가 아닌 <=다
        for (int i = 0; i < patterns.Length; ++i)
        {
            accumulated += patterns[i].First;
            if (value <= accumulated)
            {
                return i;
            }
        }
        throw new InvalidOperationException("Bridge pattern weights");
    }

    /// <summary>원본 FUN_00425c20 ↔ CD 0041fcf0의 영역·다리·특수 패턴 경로로 반복자를 생성하고 첫 유효 칸으로 전진한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="frames">그 타입의 프레임 코드 표.</param>
    /// <param name="pattern">회전·순회할 패턴 레코드.</param>
    /// <param name="direction">방향 값(회전은 direction / 2).</param>
    /// <param name="x">첫 칸의 X 좌표.</param>
    /// <param name="y">첫 칸의 Y 좌표.</param>
    /// <param name="missingFrameDiagnostic">프레임 검색 실패 시 호출할 진단 콜백.</param>
    /// <param name="edition">원본 판본.</param>
    /// <param name="diagnostics">Checking·단언 진단 묶음.</param>
    public CanonDecoder(
        RiftTypeFrames frames,
        CanonPattern pattern,
        int direction,
        float x,
        float y,
        Action? missingFrameDiagnostic = null,
        OriginalEdition edition = OriginalEdition.Patch1078,
        CanonDecoderDiagnostics? diagnostics = null)
    {
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(pattern);
        _diagnostics = diagnostics;
        _edition = edition;
        _frames = frames;
        _pattern = pattern;
        _missingFrameDiagnostic = missingFrameDiagnostic;
        _rotation = direction / 2;
        _originX = x;
        _originY = y;
        if (_rotation < 0 || _rotation > 3)
        {
            throw new ArgumentOutOfRangeException(nameof(direction), "CanonDecoder rotation");
        }
        int width = pattern.Width;
        int height = pattern.Height;
        _outerCount = (_rotation & 1) != 0 ? width : height;
        _innerCount = (_rotation & 1) != 0 ? height : width;
        _patternX = (width - 1) * StartX[_rotation];
        _patternY = (height - 1) * StartY[_rotation];
        Advance();
    }

    /// <summary>패턴 없는 타입의 한 칸 경로로 반복자를 생성하고 첫 칸으로 전진한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="frames">그 타입의 프레임 코드 표.</param>
    /// <param name="defaultFrame">기본 프레임 번호.</param>
    /// <param name="argument">명시 프레임 후보 인자.</param>
    /// <param name="direction">방향 값(CD는 홀수를 거부).</param>
    /// <param name="x">X 좌표.</param>
    /// <param name="y">Y 좌표.</param>
    /// <param name="explicitFrame">argument를 프레임 번호로 쓸지 여부.</param>
    /// <param name="edition">원본 판본.</param>
    /// <param name="diagnostics">Checking·단언 진단 묶음.</param>
    public CanonDecoder(
        RiftTypeFrames frames,
        int defaultFrame,
        int argument,
        int direction,
        float x,
        float y,
        bool explicitFrame = false,
        OriginalEdition edition = OriginalEdition.Patch1078,
        CanonDecoderDiagnostics? diagnostics = null)
    {
        ArgumentNullException.ThrowIfNull(frames);
        _diagnostics = diagnostics;
        _edition = edition;
        _frames = frames;
        _pattern = null;
        _rotation = 0;
        _originX = x;
        _originY = y;
        _outerCount = 1;
        _innerCount = 1;
        _defaultFrame = defaultFrame;
        _argument = argument;
        _explicitFrame = explicitFrame;
        if (edition == OriginalEdition.Cd1072 && (direction & 1) != 0)
        {
            throw new ArgumentOutOfRangeException(nameof(direction), "CD 비패턴 CanonDecoder 홀수 방향 오류");
        }
        if (!float.IsFinite(x) || !float.IsFinite(y))
        {
            throw new ArgumentException("패턴 없는 CanonDecoder 좌표 오류", nameof(x));
        }
        Advance();
    }

    /// <summary>00425b90 / CD 004202e0: 현재 좌표 절삭에서 순회 개수×현재 발자국을 뺀 미리보기 사각형 [left, top, right, bottom]을 계산한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="footX">가로 발자국.</param>
    /// <param name="footY">세로 발자국.</param>
    /// <returns>정수 4개 배열.</returns>
    public int[] Bounds(int footX, int footY)
    {
        double x = Math.Truncate(_x);
        double y = Math.Truncate(_y);
        double left = x - (double)_innerCount * footX;
        double top = y - (double)_outerCount * footY;
        ReadOnlySpan<double> values = [left, top, x, y];
        // 원본 int 산술·CRT 범위를 벗어나는 입력은 별도 계약으로 진단한다
        for (int i = 0; i < values.Length; ++i)
        {
            double value = values[i];
            if (!double.IsFinite(value) || value < int.MinValue || value > int.MaxValue)
            {
                throw new ArgumentOutOfRangeException(nameof(footX), "CanonDecoder 범위 산술 오류");
            }
        }
        return [(int)left, (int)top, (int)x, (int)y];
    }

    /// <summary>아직 유효한 칸이 남았는지(+0x14) 반환한다 (2026-11-07 F2c-12 이식).</summary>
    /// <returns>유효 칸이 있으면 true.</returns>
    public bool Valid() => _valid;

    /// <summary>선택한 원문 진단을 보고한다. 장치 누락은 해당 위치에서 예외로 거부한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="expression">원문 단언 식 문자열.</param>
    /// <param name="line">원문 소스 행 번호.</param>
    private void Diagnose(string expression, uint line)
    {
        if (_diagnostics is null || _diagnostics.Assertion is null)
        {
            throw new ArgumentOutOfRangeException(nameof(line), $"CanonDecoder 원문 진단 장치 미연결 {line}");
        }
        _diagnostics.Assertion(expression, "\\Ns\\O\\Canondecoder.cpp", line);
    }

    /// <summary>원본 FUN_00425860 ↔ CD 0041feb0: 다음 유효 칸으로 간다. 빈 칸과 프레임을 찾지 못한 칸은 건너뛴다 (2026-11-07 F2c-12 이식).</summary>
    public void Advance()
    {
        int rotation = _rotation;
        // 프레임이 있는 칸을 만날 때까지 간다
        while (true)
        {
            if (_inner >= _innerCount)
            {
                // 한 줄이 끝났다: 패턴 좌표를 바깥 방향으로 한 칸 옮기고 다음 줄로 간다
                int outerStepX = OuterStepX[rotation];
                if (_mirrored)
                {
                    outerStepX = -outerStepX;
                }
                if (OuterStepX[rotation] != 0)
                {
                    _patternX = (outerStepX + _outerCount + _patternX) % _outerCount;
                }
                if (OuterStepY[rotation] != 0)
                {
                    _patternY = (OuterStepY[rotation] + _outerCount + _patternY) % _outerCount;
                }
                if (++_outer >= _outerCount)
                {
                    _valid = false;
                    return;
                }
                _inner = 0;
            }
            _frame = -1;
            // 셀 해석(00425700): 줄 우선 배열에서 (패턴 x, 패턴 y)의 셀을 읽는다
            if (_pattern is null)
            {
                _frame = _explicitFrame ? _argument : _defaultFrame;
                if (_explicitFrame && (_frame < 0 || _frame >= _frames.Codes.Count))
                {
                    if (_edition != OriginalEdition.Patch1078)
                    {
                        throw new ArgumentOutOfRangeException(nameof(_frame), "CD CanonDecoder 명시 프레임 오류");
                    }
                    Diagnose("0 <= frame && frame < ::getType(type)->numFrames", 0x305u);
                }
            }
            else
            {
                int cellIndex = _pattern.Width * _patternY + _patternX;
                if (cellIndex < 0 || cellIndex >= _pattern.Cells.Length)
                {
                    throw new ArgumentOutOfRangeException(nameof(cellIndex), "CanonDecoder 셀 인덱스 범위 오류");
                }
                CanonPatternCell cell = _pattern.Cells[cellIndex];
                int label = cell[3];
                if (label != 0 && (label < 0x61 || label > 0x7a))
                {
                    throw new InvalidOperationException("num==0 || ('a' <= num && num <= 'z')");
                }
                _label = label - LabelBase;
                char orientation = (char)cell[1];
                int variation = cell[0] - VariationBase;
                if (orientation != EmptyOrientation)
                {
                    if (orientation < FirstOrientation || orientation > LastOrientation)
                    {
                        throw new InvalidOperationException("'A' <= orientation && orientation <= 'P'");
                    }
                    if (variation < 0 || variation > MaxVariation)
                    {
                        throw new InvalidOperationException("0 <= variation && variation <= 10");
                    }
                    // 회전에 따라 방향 글자를 바꿔 그 방향·변형·번호의 프레임을 찾는다(원본 FUN_0049a940)
                    byte side = (byte)RotatedSides[rotation][orientation - FirstOrientation];
                    byte variant = (byte)RotatedSides[rotation][VariantColumn];
                    _frame = _frames.FindNumber(side, variant, (byte)variation);
                    // 원본 assert lookup은 보고한 뒤 -1을 반환하며 동일 Advance가 그 칸을 계속 건너뛴다
                    if (_frame == -1 && _missingFrameDiagnostic is not null)
                    {
                        _missingFrameDiagnostic();
                    }
                }
            }
            // 패치는 현재 checking에서 현재 프레임 코드의 금 간 비트를 읽는다. CD에는 이 진단이 없다
            if (_edition == OriginalEdition.Patch1078 && _diagnostics is not null && _diagnostics.Checking && _frame != -1)
            {
                if (_frame < 0 || _frame >= _frames.Codes.Count)
                {
                    throw new ArgumentOutOfRangeException(nameof(_frame), "CanonDecoder 진단 복귀 뒤 물리 프레임 코드 범위 오류");
                }
                if ((_frames.Codes[_frame].Flags & 0x20u) != 0u)
                {
                    Diagnose("frame == EMPTY_FRAME || !(typeStruct->frameInfo[frame].flags & FF_CRACKED)", 0x325u);
                }
            }
            _x = (float)(_step * _inner) + _originX;
            _y = (float)(_outer * _step) + _originY;
            // 패턴 좌표를 안쪽 방향으로 한 칸 옮긴다
            int innerStepX = InnerStepX[rotation];
            if (_mirrored)
            {
                innerStepX = -innerStepX;
            }
            if (InnerStepX[rotation] != 0)
            {
                _patternX = (innerStepX + _innerCount + _patternX) % _innerCount;
            }
            if (InnerStepY[rotation] != 0)
            {
                _patternY = (InnerStepY[rotation] + _innerCount + _patternY) % _innerCount;
            }
            ++_inner;
            if (_frame != -1)
            {
                return;
            }
        }
    }

    /// <summary>현재 칸의 프레임 번호(+0x10)를 반환한다 (2026-11-07 F2c-12 이식).</summary>
    /// <returns>프레임 번호.</returns>
    public int Frame() => _frame;

    /// <summary>현재 칸의 X 좌표(+0x20)를 반환한다 (2026-11-07 F2c-12 이식).</summary>
    /// <returns>X 좌표.</returns>
    public float X() => _x;

    /// <summary>현재 칸의 Y 좌표(+0x24)를 반환한다 (2026-11-07 F2c-12 이식).</summary>
    /// <returns>Y 좌표.</returns>
    public float Y() => _y;

    /// <summary>현재 칸의 번호 글자에서 'a'를 뺀 값(+0x18, 번호가 없으면 -0x61)을 반환한다 (2026-11-07 F2c-12 이식).</summary>
    /// <returns>라벨 값.</returns>
    public int Label() => _label;

    /// <summary>원본 FUN_00425c00: 현재 프레임 코드의 방향 글자를 반환한다 (2026-11-07 F2c-12 이식).</summary>
    /// <returns>방향 글자.</returns>
    public char Side()
    {
        if (_frame < 0 || _frame >= _frames.Codes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(_frame), "CanonDecoder 방향 조회 물리 프레임 범위 오류");
        }
        return (char)_frames.Codes[_frame].Side;
    }
}
