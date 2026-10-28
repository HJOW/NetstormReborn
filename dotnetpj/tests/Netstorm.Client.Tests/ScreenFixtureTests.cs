using System.Globalization;
using Netstorm.Client.Display;

namespace Netstorm.Tests;

/// <summary>
/// 원본 기계어 fixture 3개(palettecolor·screen-scroll·screen-palette-clear)로 화면 계층의 L1 일치를 검사한다.
/// 근거: cpppj/tests/ScreenTests.cpp, ScreenScrollTests.cpp, ScreenPaletteApplyTests.cpp. 기대값은 cpppj가 원본 exe를 격리 실행해 얻은 값이며
/// C# 결과로 바꾸지 않는다. (2026-10-28 R1에서 이식)
/// </summary>
public sealed class ScreenFixtureTests
{
    /// <summary>색 검색 fixture의 행 수(원본 세 몸체의 두 x87 관찰).</summary>
    private const int PaletteColorRows = 786;

    /// <summary>스크롤 fixture의 행 수(보호 4a1ad0이 실제 고른 행 순서로 만든 전체 144바이트).</summary>
    private const int ScrollRows = 357;

    /// <summary>팔레트 clear fixture의 전체 관측 수와 정상 checkpoint 수.</summary>
    private const int ClearRows = 166, ClearCheckpoints = 122;

    /// <summary>리틀 엔디언 16진수 문자열을 DWORD 배열로 바꾼다. 최상위 플래그 바이트도 보존한다.</summary>
    private static uint[] Dwords(string hex)
    {
        var result = new uint[hex.Length / 8];
        // DWORD마다 네 바이트를 원본 순서(낮은 주소가 낮은 자리)로 합친다.
        for (int i = 0; i < result.Length; i++)
            for (int b = 0; b < 4; b++)
                result[i] |= uint.Parse(hex.AsSpan(i * 8 + b * 2, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture) << (b * 8);
        return result;
    }

    /// <summary>색 검색이 원본 세 몸체의 786개 관측과 모두 같은 번호를 고른다(거리 동률·float 비교 포함).</summary>
    [Fact]
    public void NearestColorMatchesNativeBodies()
    {
        string[][] rows = X86Fixture.Read("palettecolor");
        Assert.Equal(PaletteColorRows, rows.Length);
        // 관찰 행마다 논리 팔레트 256개를 복원해 같은 입력으로 검색한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(2048, row[4].Length);
            uint[] palette = Dwords(row[4]);
            uint expected = X86Fixture.UInt(row[5]);
            Assert.Equal(expected, PaletteSearch.FindPaletteColor(palette, X86Fixture.Int(row[1]), X86Fixture.Int(row[2]), X86Fixture.Int(row[3])));
        }
    }

    /// <summary>화면 이동이 원본 4a1ad0의 전체 144바이트 결과(padding 포함) 357개와 바이트 단위로 같다.</summary>
    [Fact]
    public void ScrollMatchesNativeDibPixels()
    {
        string[][] rows = X86Fixture.Read("screen-scroll");
        Assert.Equal(ScrollRows, rows.Length);
        // 관측마다 같은 독립 초기 버퍼를 공급한다. 원문 입력만 만들고 기대 이동은 계산하지 않는다.
        foreach (string[] row in rows)
        {
            var pixels = new byte[144];
            for (int i = 0; i < pixels.Length; i++) pixels[i] = (byte)i;
            ScreenPresentation.ScrollScreenPixels(pixels, 16, 13, 9, 2, 1, 9, 7, X86Fixture.Int(row[0]), X86Fixture.Int(row[1]));
            Assert.Equal(row[2], Convert.ToHexStringLower(pixels));
        }
    }

    /// <summary>
    /// 팔레트 clear 관문의 최초 PE prefix 166관측(순서·중단 원인·13개 현재 필드·전체 64바이트 물리·trace)이 모두 같다.
    /// 장치는 원문에서 명시 입력이던 Lock/Fill/Unlock/Update만 모델링하며 호출 순서는 <see cref="ScreenPaletteApply"/>가 소유한다.
    /// </summary>
    [Fact]
    public void PaletteClearMatchesFirstPePrefixFixture()
    {
        string[][] rows = X86Fixture.Read("screen-palette-clear");
        Assert.Equal(ClearRows, rows.Length);
        int checkpoints = 0;
        // 관측마다 장치를 독립 초기화하고 이전 관측의 버퍼나 전역에 기대지 않는다.
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            var device = new ClearDevice(row[1].Split(','));
            string cause = device.Run();
            Assert.Equal(row[2], cause);
            Assert.Equal(row[3], device.Values());
            Assert.Equal(row[4], device.Raw());
            Assert.Equal(row[5], device.Trace());
            if (cause == "checkpoint") checkpoints++;
        }
        Assert.Equal(ClearCheckpoints, checkpoints);
    }

    /// <summary>clear가 0이거나 flags가 0이면 loaded만 쓰고, 도달한 필수 하위 장치가 연결돼 있지 않으면 loaded 기록 뒤에 예외를 던진다.</summary>
    [Fact]
    public void PaletteClearNaturalSkipAndRequiredDevices()
    {
        uint loaded = 77;
        var devices = new ScreenPaletteApplyHooks(); // 미연결 하위에 기본 성공을 넣지 않은 입력이다.
        ScreenPaletteApply.ApplyScreenPaletteClear(1, 0, ref loaded, devices);
        Assert.Equal(1u, loaded);
        loaded = 77;
        ScreenPaletteApply.ApplyScreenPaletteClear(0, 0x8a, ref loaded, devices);
        Assert.Equal(1u, loaded);
        loaded = 77;
        Assert.Throws<InvalidOperationException>(() =>
        {
            uint local = 77;
            try { ScreenPaletteApply.ApplyScreenPaletteClear(1, 0x8a, ref local, devices); }
            finally { loaded = local; }
        });
        Assert.Equal(1u, loaded);
    }

    /// <summary>팔레트 clear fixture의 중단 지점을 나타낸다(정상 checkpoint와 구별한다).</summary>
    private sealed class StopException(string cause) : Exception
    {
        /// <summary>원문 명시 경계 이름.</summary>
        public string Cause { get; } = cause;
    }

    /// <summary>fixture 한 행의 입력으로 만든 격리 장치. 원문 Lock/Fill/Unlock/Update의 현재 필드 변경과 중단만 반영한다.</summary>
    private sealed class ClearDevice
    {
        /// <summary>원문과 동일한 명시 장치 토큰이다.</summary>
        private const uint Surface = 0x12001000, Other = 0x12002000, Pixels = 0x12005000;

        /// <summary>같은 실제 현재 필드의 격리 입력이다.</summary>
        private uint _flags, _depth, _draw = Surface, _buffer, _pitch, _vfx, _loaded = 77, _width = 8, _height = 4, _clear;

        /// <summary>현재 자르기 영역(4칸)과 고정 viewport. 둘은 서로 다르다.</summary>
        private uint[] _clip = new uint[4], _view = [0, 0, 8, 4];

        /// <summary>prefix 하위의 명시 물리 입력(64바이트)이며 GDI 표면이 아니다.</summary>
        private readonly byte[] _pixels = new byte[64];

        /// <summary>호출 순서와 하위 효과를 원문 형식으로 기록한다.</summary>
        private readonly List<string> _trace = [];

        /// <summary>어느 하위에서 어떤 변경을 일으킬지 정하는 입력 이름.</summary>
        private readonly string _mutation;

        /// <summary>fixture의 "clear,flags,depth,smallClip,mutation" 다섯 칸으로 장치를 만든다.</summary>
        public ClearDevice(string[] fields)
        {
            if (fields.Length != 5) throw new InvalidDataException("palette clear 입력칸");
            _clear = uint.Parse(fields[0], CultureInfo.InvariantCulture);
            _flags = uint.Parse(fields[1], CultureInfo.InvariantCulture);
            _depth = uint.Parse(fields[2], CultureInfo.InvariantCulture);
            _clip = fields[3] == "1" ? [1, 1, 7, 3] : [.. _view];
            _mutation = fields[4];
            if (_depth != 0) { _buffer = _vfx = Pixels; _pitch = 8; }
            // 모든 픽셀을 서로 다른 번호로 초기화한다.
            for (int i = 0; i < _pixels.Length; i++) _pixels[i] = (byte)i;
        }

        /// <summary>정수 목록을 쉼표로 이은 fixture 표기.</summary>
        private static string Numbers(IEnumerable<uint> values) => string.Join(',', values);

        /// <summary>경계 이름에 맞는 원문 장치의 현재 값 변경이나 중단을 일으킨다.</summary>
        private void Mutate(string boundary)
        {
            if (_mutation == boundary + "-stop") throw new StopException(boundary);
            if (_mutation == "lock-size" && boundary == "lock") { _width = 6; _height = 3; }
            else if (_mutation == "lock-flags0" && boundary == "lock") _flags = 0;
            else if (_mutation == "fill-draw" && boundary == "fill") _draw = Other;
            else if (_mutation == "fill-flags1" && boundary == "fill") _flags = 0x81;
            else if (_mutation == "unlock-buffer" && boundary == "unlock") { _buffer = _vfx = Pixels + 16; _pitch = 99; }
            else if (_mutation == "update-clip" && boundary == "update") _clip = [2, 2, 3, 3];
            else if (_mutation == "update-flags4" && boundary == "update") _flags = 0x84;
        }

        /// <summary>현재 필드 13개를 fixture 형식으로 표시한다.</summary>
        public string Values() => Numbers([_flags, _depth, _draw, _buffer, _pitch, _vfx, _loaded, _width, _height, _clip[0], _clip[1], _clip[2], _clip[3]]);

        /// <summary>지우기 전후의 모든 픽셀을 생략 없이 16진수로 표시한다.</summary>
        public string Raw() => Convert.ToHexStringLower(_pixels);

        /// <summary>관측 trace를 원문과 같은 순서로 합친다.</summary>
        public string Trace() => string.Join(';', _trace);

        /// <summary>실제 호출자와 같은 clear 관문을 helper에 맡기고 하위 장치만 명시한다. 중단 원인 또는 "checkpoint"를 돌려준다.</summary>
        public string Run()
        {
            var hooks = new ScreenPaletteApplyHooks
            {
                Lock = () =>
                {
                    uint before = _depth++;
                    _trace.Add("LOCK:" + before);
                    if (before == 0) { _buffer = _vfx = Pixels; _pitch = 8; }
                    Mutate("lock");
                },
                FillBlack = () =>
                {
                    uint[] rect = [0, 0, _width, _height];
                    _trace.Add("FILL:" + Numbers(rect) + ":0:" + Numbers(_clip)); // lock 뒤의 현재 크기와 clip이다.
                    if ((_flags & 0x80) != 0 && _buffer != 0)
                    {
                        uint left = Math.Max(_clip[0], Math.Min(rect[0], _clip[2])), top = Math.Max(_clip[1], Math.Min(rect[1], _clip[3]));
                        uint right = Math.Max(_clip[0], Math.Min(rect[2], _clip[2])), bottom = Math.Max(_clip[1], Math.Min(rect[3], _clip[3]));
                        // FillRect 자체는 선택 prefix 밖의 기존 primitive이며 같은 명시 물리에만 쓴다.
                        for (uint y = top; y < bottom; y++)
                            for (uint x = left; x < right; x++) _pixels[(int)(_buffer - Pixels + y * _pitch + x)] = 0;
                    }
                    Mutate("fill");
                },
                Unlock = () =>
                {
                    _depth--;
                    if (_depth == 0)
                    {
                        _trace.Add("UNLOCK:" + _draw);
                        Mutate("unlock");
                        _buffer = _pitch = _vfx = 0;
                    }
                },
                WaitVerticalBlank = () =>
                {
                    _trace.Add("WAIT:" + _flags);
                    if ((_flags & 1) != 0) throw new StopException("directdraw-vblank");
                },
                PresentPreservingClip = () =>
                {
                    _trace.Add("PRESERVE:" + Numbers(_clip));
                    uint[] previous = _clip;
                    _trace.Add("UPDATE:" + Numbers(_view));
                    _clip = _view;
                    Mutate("update");
                    _clip = previous;
                },
                Flip = () =>
                {
                    _trace.Add("FLIP:" + _flags);
                    if ((_flags & 4) != 0) throw new StopException("directdraw-flip");
                },
            };
            try
            {
                ScreenPaletteApply.ApplyScreenPaletteClear(_clear, _flags, ref _loaded, hooks);
                _trace.Add("APPLY:0,256,0,1");
            }
            catch (StopException stop) { return stop.Cause; }
            return "checkpoint";
        }
    }
}
