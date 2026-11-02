using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort 팔레트 전환(<see cref="RawFortPaletteTransition"/>)을 독립 원본 입력과 대조한다.
/// cpppj <c>FortPaletteTransitionTests</c>의 최초 676행 전수와 선택·물리 경계 직접 검사를 옮겼다. (F2c-1, 2026-11-03)
/// </summary>
public sealed class FortPaletteTransitionTests
{
    /// <summary>원문 고정 이름 주소다(trace에만 표현한다).</summary>
    private const uint PaletteNameAddress = 0x5c8910u;

    /// <summary>Kencloud 이름은 팔레트 이름과 별개 주소다.</summary>
    private const uint CloudNameAddress = 0x56b7a0u;

    /// <summary>격리 x86 장치의 두 512바이트 입력 위치다.</summary>
    private const uint InputAddress = 0x12400000u;

    /// <summary>격리 PathSpec이 돌려주는 두 256바이트 위치다.</summary>
    private const uint PathAddress = 0x12401000u;

    /// <summary>실제 할당기 대신 명시한 cache 토큰 주소의 시작이다.</summary>
    private const uint PoolAddress = 0x12000000u;

    /// <summary>명시 하위 callback 또는 물리 접근 중단만 나타내며 부모의 정상 반환과 구별한다.</summary>
    internal sealed class PaletteStop : Exception
    {
        /// <summary>fixture의 중단 원인을 그대로 보존한다.</summary>
        public string Cause { get; }

        /// <summary>중단 원인을 보관한다.</summary>
        /// <param name="cause">중단 원인 이름이다.</param>
        public PaletteStop(string cause)
        {
            Cause = cause;
        }
    }

    /// <summary>동일한 실제 버퍼를 빌리고 하위 효과만 기록한다. 부모의 제어 흐름은 이 장치가 계산하지 않는다.</summary>
    private sealed class PaletteDevice
    {
        /// <summary>고정 이름 앞 1바이트와 뒤 256바이트 전체 물리다.</summary>
        public byte[] Palette = new byte[257];

        /// <summary>고정 구름 이름 전체 물리다.</summary>
        public byte[] Cloud = new byte[257];

        /// <summary>입력 인자 변수 변경과 같은 입력 내용 변경을 구별하는 두 입력이다.</summary>
        public byte[][] Inputs = [new byte[512], new byte[512]];

        /// <summary>PathSpec 반환 주소의 실제 물리다.</summary>
        public byte[][] Paths = [new byte[256], new byte[256]];

        /// <summary>stack의 현재 인자 변수다.</summary>
        public Memory<byte> Argument;

        /// <summary>parent가 캡처한 주소다.</summary>
        public byte[]? CapturedArray;

        /// <summary>캡처한 주소의 배열 안 위치다.</summary>
        public int CapturedOffset;

        /// <summary>원문이 직접 재조회하는 같은 현재 DWORD들이다.</summary>
        public uint Cache;

        /// <summary>원문이 직접 재조회하는 현재 개수다.</summary>
        public uint Count;

        /// <summary>원문이 직접 재조회하는 현재 렌더러 존재 값이다.</summary>
        public uint Renderer;

        /// <summary>원문이 직접 재조회하는 현재 dirty 값이다.</summary>
        public uint Dirty;

        /// <summary>build가 쓴 뒤의 cache 값이다.</summary>
        public uint Built;

        /// <summary>현재 PathSpec 선택(-1은 NULL)이다.</summary>
        public int PathIndex;

        /// <summary>독립 입력 번호다.</summary>
        public int NameCase;

        /// <summary>진입점이다.</summary>
        public string Kind = "";

        /// <summary>명시 하위 변화만 선택한다.</summary>
        public string Mutation = "";

        /// <summary>free·build·step·load·maps 횟수를 fixture와 비교한다.</summary>
        public Dictionary<string, uint> Calls = [];

        /// <summary>실제 순서대로 누적한 명시 하위 호출이다.</summary>
        public List<string> TraceList = [];

        /// <summary>논리 cache 입력을 격리 원문과 같은 주소 토큰으로 바꾼다.</summary>
        /// <param name="logical">논리 cache 값이다.</param>
        public static uint Token(uint logical)
        {
            return logical == 0 ? 0 : PoolAddress + logical / 11 * 0x1000;
        }

        /// <summary>NUL까지 쓰되 주변 바이트는 유지하여 alias·남은 물리를 검증한다.</summary>
        /// <param name="target">쓸 대상이다.</param>
        /// <param name="value">쓸 문자열이다.</param>
        public static void Put(Span<byte> target, string value)
        {
            byte[] bytes = Encoding.Latin1.GetBytes(value);
            if (bytes.Length + 1 > target.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(target), "검사 장치 입력 길이");
            }
            bytes.CopyTo(target);
            target[bytes.Length] = 0;
        }

        /// <summary>C++ 버퍼 주소를 격리 PE의 실제 주소와 대응시킨다.</summary>
        /// <param name="span">주소로 바꿀 span이다.</param>
        public uint Address(ReadOnlySpan<byte> span)
        {
            if (span.IsEmpty)
            {
                return 0;
            }
            int located = Locate(span);
            if (located < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(span), "검사 장치 주소 대응 누락");
            }
            return (uint)located;
        }

        /// <summary>span의 첫 바이트가 속한 실제 배열과 위치를 찾아 격리 주소로 바꾼다.</summary>
        /// <param name="span">찾을 span이다.</param>
        private int Locate(ReadOnlySpan<byte> span)
        {
            ref byte reference = ref MemoryMarshal.GetReference(span);
            // 각 실제 소유 버퍼 안에서 같은 주소의 바이트를 찾는다
            for (int offset = 0; offset < Palette.Length; offset++)
            {
                if (Unsafe.AreSame(ref reference, ref Palette[offset]))
                {
                    return (int)((PaletteNameAddress - 1) + (uint)offset);
                }
            }
            for (int offset = 0; offset < Cloud.Length; offset++)
            {
                if (Unsafe.AreSame(ref reference, ref Cloud[offset]))
                {
                    return (int)((CloudNameAddress - 1) + (uint)offset);
                }
            }
            for (int offset = 0; offset < Inputs[0].Length; offset++)
            {
                if (Unsafe.AreSame(ref reference, ref Inputs[0][offset]))
                {
                    return (int)(InputAddress + (uint)offset);
                }
            }
            for (int offset = 0; offset < Inputs[1].Length; offset++)
            {
                if (Unsafe.AreSame(ref reference, ref Inputs[1][offset]))
                {
                    return (int)(InputAddress + 0x400 + (uint)offset);
                }
            }
            for (int offset = 0; offset < Paths[0].Length; offset++)
            {
                if (Unsafe.AreSame(ref reference, ref Paths[0][offset]))
                {
                    return (int)(PathAddress + (uint)offset);
                }
            }
            for (int offset = 0; offset < Paths[1].Length; offset++)
            {
                if (Unsafe.AreSame(ref reference, ref Paths[1][offset]))
                {
                    return (int)(PathAddress + 0x200 + (uint)offset);
                }
            }
            return -1;
        }

        /// <summary>캡처한 span이 속한 실제 배열과 위치를 찾는다.</summary>
        /// <param name="span">찾을 span이다.</param>
        /// <param name="array">찾은 배열이다.</param>
        /// <param name="offset">찾은 위치다.</param>
        private bool TryLocate(ReadOnlySpan<byte> span, out byte[] array, out int offset)
        {
            array = null!;
            offset = 0;
            if (span.IsEmpty)
            {
                return false;
            }
            ref byte reference = ref MemoryMarshal.GetReference(span);
            // 같은 주소의 바이트가 있는 배열을 찾는다
            for (int index = 0; index < Palette.Length; index++)
            {
                if (Unsafe.AreSame(ref reference, ref Palette[index]))
                {
                    array = Palette;
                    offset = index;
                    return true;
                }
            }
            for (int index = 0; index < Cloud.Length; index++)
            {
                if (Unsafe.AreSame(ref reference, ref Cloud[index]))
                {
                    array = Cloud;
                    offset = index;
                    return true;
                }
            }
            for (int index = 0; index < Inputs[0].Length; index++)
            {
                if (Unsafe.AreSame(ref reference, ref Inputs[0][index]))
                {
                    array = Inputs[0];
                    offset = index;
                    return true;
                }
            }
            for (int index = 0; index < Inputs[1].Length; index++)
            {
                if (Unsafe.AreSame(ref reference, ref Inputs[1][index]))
                {
                    array = Inputs[1];
                    offset = index;
                    return true;
                }
            }
            for (int index = 0; index < Paths[0].Length; index++)
            {
                if (Unsafe.AreSame(ref reference, ref Paths[0][index]))
                {
                    array = Paths[0];
                    offset = index;
                    return true;
                }
            }
            for (int index = 0; index < Paths[1].Length; index++)
            {
                if (Unsafe.AreSame(ref reference, ref Paths[1][index]))
                {
                    array = Paths[1];
                    offset = index;
                    return true;
                }
            }
            return false;
        }

        /// <summary>읽기 범위 안에 NUL이 없는 경우 실제 하위 접근 실패를 명시 중단으로 돌린다.</summary>
        /// <param name="bytes">검사할 구간이다.</param>
        /// <param name="cause">중단 원인 이름이다.</param>
        public static ReadOnlySpan<byte> CString(ReadOnlySpan<byte> bytes, string cause)
        {
            if (bytes.IsEmpty)
            {
                throw new PaletteStop(cause);
            }
            int at = bytes.IndexOf((byte)0);
            if (at < 0)
            {
                throw new PaletteStop(cause);
            }
            return bytes[..at];
        }

        /// <summary>하위 효과가 같은 captured source의 내용을 바꾸도록 실제 가변 물리를 찾는다.</summary>
        public void ChangeSource()
        {
            if (CapturedArray is null)
            {
                throw new ArgumentOutOfRangeException(nameof(CapturedArray), "검사 source 쓰기 주소 누락");
            }
            byte[] array = CapturedArray;
            int offset = CapturedOffset;
            Span<byte> region;
            if (ReferenceEquals(array, Palette))
            {
                region = Palette.AsSpan(offset);
            }
            else if (ReferenceEquals(array, Cloud))
            {
                region = Cloud.AsSpan(offset);
            }
            else if (ReferenceEquals(array, Inputs[0]))
            {
                region = Inputs[0].AsSpan(offset);
            }
            else if (ReferenceEquals(array, Inputs[1]))
            {
                region = Inputs[1].AsSpan(offset);
            }
            else if (ReferenceEquals(array, Paths[0]))
            {
                region = Paths[0].AsSpan(offset);
            }
            else if (ReferenceEquals(array, Paths[1]))
            {
                region = Paths[1].AsSpan(offset);
            }
            else
            {
                throw new ArgumentOutOfRangeException(nameof(CapturedArray), "검사 source 쓰기 주소 누락");
            }
            Put(region, "ChangedByChild");
        }

        /// <summary>부모 밖에서만 발생하도록 지정한 callback 변화·중단을 적용한다.</summary>
        /// <param name="boundary">효과 경계 이름이다.</param>
        public void Mutate(string boundary)
        {
            if (Mutation == boundary + "-argumentB")
            {
                Argument = new Memory<byte>(Inputs[1]);
            }
            else if (Mutation == boundary + "-source")
            {
                ChangeSource();
            }
            else if (Mutation == boundary + "-palette")
            {
                Put(Palette.AsSpan(1), "ChangedPalette");
            }
            else if (Mutation == boundary + "-cache11")
            {
                Cache = Token(11);
            }
            else if (Mutation == boundary + "-cache33")
            {
                Cache = Token(33);
            }
            else if (Mutation == boundary + "-cache0")
            {
                Cache = 0;
            }
            else if (Mutation == boundary + "-count0")
            {
                Count = 0;
            }
            else if (Mutation == boundary + "-count3")
            {
                Count = 3;
            }
            else if (Mutation == boundary + "-renderer0")
            {
                Renderer = 0;
            }
            else if (Mutation == boundary + "-renderer1")
            {
                Renderer = 1;
            }
            else if (Mutation == boundary + "-dirty77")
            {
                Dirty = 77;
            }
            else if (Mutation == "path-return1" && boundary == "path")
            {
                PathIndex = 1;
            }
            else if (Mutation == "path-return0" && boundary == "path")
            {
                PathIndex = -1;
            }
            else if (Mutation == boundary + "-stop")
            {
                throw new PaletteStop(boundary);
            }
        }

        /// <summary>
        /// 각 관측을 새 장치로 초기화하고 NUL 뒤·별칭 영역까지 독립 입력을 넣는다.
        /// </summary>
        /// <param name="values">쉼표 9칸 입력이다.</param>
        public PaletteDevice(string[] values)
        {
            if (values.Length != 9)
            {
                throw new InvalidOperationException("palette fixture 입력 칸 수");
            }
            Kind = values[0];
            NameCase = int.Parse(values[1], CultureInfo.InvariantCulture);
            Cache = Token(uint.Parse(values[2], CultureInfo.InvariantCulture));
            Count = uint.Parse(values[3], CultureInfo.InvariantCulture);
            Renderer = uint.Parse(values[4], CultureInfo.InvariantCulture);
            Dirty = uint.Parse(values[5], CultureInfo.InvariantCulture);
            Built = Token(uint.Parse(values[6], CultureInfo.InvariantCulture));
            PathIndex = int.Parse(values[7], CultureInfo.InvariantCulture);
            Mutation = values[8];
            Array.Fill(Palette, (byte)0xa1);
            Palette[0] = (byte)'Z';
            Array.Fill(Cloud, (byte)0xb2);
            Cloud[0] = (byte)'Y';
            Put(Palette.AsSpan(1), NameCase == 1 || NameCase == 2 ? "gifcloud" : "OldPalette");
            Put(Cloud.AsSpan(1), "LegacySort");
            // 각 입력의 NUL 뒤 바이트까지 서로 다른 초기값을 보존한다
            for (int input = 0; input < Inputs.Length; input++)
            {
                for (int address = 0; address < Inputs[input].Length; address++)
                {
                    Inputs[input][address] = (byte)(0xc0 + input * 7 + address);
                }
            }
            string name = NameCase == 1 ? "GIFCLOUD" : NameCase == 3 ? "" : "gifcloud";
            if (NameCase == 6)
            {
                StringBuilder longName = new();
                // 210자 입력도 원문 길이대로 복사한다
                for (int part = 0; part < 14; part++)
                {
                    longName.Append("LongPaletteName");
                }
                name = longName.ToString();
            }
            Put(Inputs[0], name);
            Put(Inputs[1], "SecondPalette");
            if (NameCase == 8)
            {
                Array.Fill(Inputs[0], (byte)'Q');
            }
            Argument = new Memory<byte>(Inputs[0]);
            if (NameCase == 4)
            {
                Argument = new Memory<byte>(Palette, 2, Palette.Length - 2);
            }
            else if (NameCase == 5)
            {
                Argument = new Memory<byte>(Cloud, 2, Cloud.Length - 2);
            }
            else if (NameCase == 7)
            {
                Argument = new Memory<byte>(Palette);
            }
            else if (NameCase == 9)
            {
                Argument = new Memory<byte>(Cloud, 1, Cloud.Length - 1);
            }
            else if (NameCase == 10)
            {
                Argument = new Memory<byte>(Cloud);
            }
            // 실제 PathSpec 반환 둘의 주소와 내용을 서로 구별한다
            for (int path = 0; path < Paths.Length; path++)
            {
                for (int address = 0; address < Paths[path].Length; address++)
                {
                    Paths[path][address] = (byte)(0xd0 + path * 5 + address);
                }
                Put(Paths[path], path == 0 ? "Work/First.col" : "Work/Second.col");
            }
            if (TryLocate(Argument.Span, out byte[] initialArray, out int initialOffset))
            {
                CapturedArray = initialArray;
                CapturedOffset = initialOffset;
            }
            else
            {
                CapturedArray = null;
                CapturedOffset = 0;
            }
            // 원문 const span& 참조 의미를 보존한다. log보다 뒤에 캡처하는 source가 log 중 바뀐 인자를 보게 한다.
            // C#은 Span을 값으로 전달하므로 log-argumentB만 미리 반영하고 Mutate는 같은 값으로 다시 둔다.
            // 최종 trace·값·raw는 원문과 같다. 캡처는 미리 반영 전의 옛 인자를 유지한다.
            if (Mutation == "log-argumentB")
            {
                Argument = new Memory<byte>(Inputs[1]);
            }
        }

        /// <summary>실제 DWORD·고정 버퍼의 같은 수명 참조를 새 부모에 전달한다.</summary>
        public FortPaletteTransitionGlobals Globals() => new()
        {
            PaletteName = new Memory<byte>(Palette, 1, 256),
            CloudName = new Memory<byte>(Cloud, 1, 256),
            GetCache = () => Cache,
            SetCache = value => Cache = value,
            Count = () => Count,
            Renderer = () => Renderer,
            GetDirty = () => Dirty,
            SetDirty = value => Dirty = value,
        };

        /// <summary>제품 본체와 독립인 명시 CRT·계산·파일·Screen 장치다. 실제 GUI나 파일은 열지 않는다.</summary>
        public FortPaletteTransitionHooks Hooks() => new()
        {
            Log = (level, text) =>
            {
                Assert.Equal(29u, level);
                Assert.Equal("init palette\n", text);
                TraceList.Add("LOG:" + level.ToString(CultureInfo.InvariantCulture) + ":" + PaletteHex(Encoding.Latin1.GetBytes(text)));
                Mutate("log");
            },
            Compare = (first, second) =>
            {
                if (TryLocate(second, out byte[] array, out int offset))
                {
                    CapturedArray = array;
                    CapturedOffset = offset;
                }
                else
                {
                    CapturedArray = null;
                    CapturedOffset = 0;
                }
                TraceList.Add("COMPARE:" + Address(first).ToString(CultureInfo.InvariantCulture) + ":" + Address(second).ToString(CultureInfo.InvariantCulture));
                ReadOnlySpan<byte> left = CString(first, "compare-physical");
                ReadOnlySpan<byte> right = CString(second, "compare-physical");
                bool equal = left.Length == right.Length;
                // 입력 전체가 ASCII인 명시 CRT 장치다
                if (equal)
                {
                    for (int index = 0; index < left.Length; index++)
                    {
                        byte a = left[index] >= (byte)'A' && left[index] <= (byte)'Z' ? (byte)(left[index] + 32) : left[index];
                        byte b = right[index] >= (byte)'A' && right[index] <= (byte)'Z' ? (byte)(right[index] + 32) : right[index];
                        if (a != b)
                        {
                            equal = false;
                            break;
                        }
                    }
                }
                Mutate("compare");
                return equal ? 0 : 1;
            },
            Release = selected =>
            {
                Calls["free"] = Calls.TryGetValue("free", out uint value) ? value + 1 : 1;
                TraceList.Add("FREE:" + selected.ToString(CultureInfo.InvariantCulture));
                Mutate("free");
            },
            BuildCloud = () =>
            {
                Calls["build"] = Calls.TryGetValue("build", out uint value) ? value + 1 : 1;
                TraceList.Add("BUILD:" + Cache.ToString(CultureInfo.InvariantCulture) + ":" + Count.ToString(CultureInfo.InvariantCulture));
                Cache = Built;
                Mutate("build");
            },
            StepCloud = () =>
            {
                uint index = Calls.TryGetValue("step", out uint value) ? value + 1 : 1;
                Calls["step"] = index;
                TraceList.Add("STEP:" + index.ToString(CultureInfo.InvariantCulture) + ":" + Cache.ToString(CultureInfo.InvariantCulture) + ":" + Count.ToString(CultureInfo.InvariantCulture));
                Mutate("step" + index.ToString(CultureInfo.InvariantCulture));
            },
            PathSpec = (key, source, first, second) =>
            {
                Assert.Equal("GamePalSpec", key);
                Assert.Equal(0u, first);
                Assert.Equal(0u, second);
                TraceList.Add("PATH:" + Address(source).ToString(CultureInfo.InvariantCulture) + ":" + first.ToString(CultureInfo.InvariantCulture) + ":" + second.ToString(CultureInfo.InvariantCulture));
                TraceList.Add("PATH-NAME:" + PaletteHex(CString(source, "path-physical")));
                Mutate("path");
                return PathIndex < 0 ? [] : Paths[PathIndex];
            },
            LoadPalette = (path, clear) =>
            {
                Calls["load"] = Calls.TryGetValue("load", out uint value) ? value + 1 : 1;
                Assert.Equal(1u, clear);
                TraceList.Add("LOAD:" + Address(path).ToString(CultureInfo.InvariantCulture) + ":" + clear.ToString(CultureInfo.InvariantCulture));
                TraceList.Add("LOAD-PATH:" + PaletteHex(CString(path, "load-physical")));
                Mutate("load");
            },
            BuildMaps = () =>
            {
                Calls["maps"] = Calls.TryGetValue("maps", out uint value) ? value + 1 : 1;
                TraceList.Add("MAPS:" + Renderer.ToString(CultureInfo.InvariantCulture) + ":" + Dirty.ToString(CultureInfo.InvariantCulture));
                Mutate("maps");
            },
        };

        /// <summary>네 선택 진입점만 실행하고 명시 중단·물리 예외를 정직하게 반환한다.</summary>
        public string Run()
        {
            FortPaletteTransitionGlobals globals = Globals();
            FortPaletteTransitionHooks hooks = Hooks();
            try
            {
                if (Kind == "transition")
                {
                    RawFortPaletteTransition.TransitionFortPalette(Argument.Span, globals, hooks);
                }
                else if (Kind == "cache")
                {
                    RawFortPaletteTransition.ReleaseFortPaletteCloudCache(globals, hooks);
                }
                else if (Kind == "sort")
                {
                    RawFortPaletteTransition.SetFortPaletteCloudName(Argument.Span, globals, hooks);
                }
                else if (Kind == "dirty")
                {
                    RawFortPaletteTransition.MarkFortPaletteDirty(globals);
                }
                else
                {
                    throw new InvalidOperationException("palette fixture 진입점");
                }
            }
            catch (PaletteStop stopped)
            {
                return stopped.Cause;
            }
            catch (ArgumentOutOfRangeException error) when (error.ParamName == "target")
            {
                return Kind == "transition" && NameCase == 7 ? "palette-write" : "sort-write";
            }
            return "1";
        }

        /// <summary>독립 원문과 같은 11개 현재 필드의 최종·중단 시점을 표시한다.</summary>
        public string Values()
        {
            return Cache.ToString(CultureInfo.InvariantCulture) + "," + Count.ToString(CultureInfo.InvariantCulture) + "," + Renderer.ToString(CultureInfo.InvariantCulture) + "," + Dirty.ToString(CultureInfo.InvariantCulture) + "," + Address(Argument.Span).ToString(CultureInfo.InvariantCulture) + "," + PathIndex.ToString(CultureInfo.InvariantCulture) + "," + Calls.GetValueOrDefault("free").ToString(CultureInfo.InvariantCulture) + "," + Calls.GetValueOrDefault("build").ToString(CultureInfo.InvariantCulture) + "," + Calls.GetValueOrDefault("step").ToString(CultureInfo.InvariantCulture) + "," + Calls.GetValueOrDefault("load").ToString(CultureInfo.InvariantCulture) + "," + Calls.GetValueOrDefault("maps").ToString(CultureInfo.InvariantCulture);
        }

        /// <summary>이름 앞 바이트·NUL 뒤 잔여·입력 두 개·경로 두 개의 모든 물리를 비교한다.</summary>
        public string Raw()
        {
            return PaletteHex(Palette) + ":" + PaletteHex(Cloud) + ":" + PaletteHex(Inputs[0]) + ":" + PaletteHex(Inputs[1]) + ":" + PaletteHex(Paths[0]) + ":" + PaletteHex(Paths[1]);
        }

        /// <summary>trace를 원문과 같은 순서로 한 줄에 결합한다.</summary>
        public string Trace()
        {
            return string.Join(';', TraceList);
        }
    }

    /// <summary>쉼표·탭·trace 분리는 마지막 빈 칸도 보존하여 fixture 형식 오류를 숨기지 않는다.</summary>
    /// <param name="source">분리할 원문이다.</param>
    /// <param name="separator">구분자다.</param>
    private static string[] SplitPalette(string source, char separator)
    {
        List<string> result = [];
        int start = 0;
        // 끝 빈 trace도 하나의 칸으로 남긴다
        for (; ; )
        {
            int at = source.IndexOf(separator, start);
            result.Add(at < 0 ? source[start..] : source.Substring(start, at - start));
            if (at < 0)
            {
                return [.. result];
            }
            start = at + 1;
        }
    }

    /// <summary>전체 물리와 CString trace를 같은 소문자 hex로 표시한다.</summary>
    /// <param name="bytes">표시할 바이트다.</param>
    private static string PaletteHex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        StringBuilder result = new();
        // NUL 뒤 바이트도 raw에서는 생략하지 않는다
        foreach (byte value in bytes)
        {
            result.Append(digits[value >> 4]);
            result.Append(digits[value & 15]);
        }
        return result.ToString();
    }

    /// <summary>네 원문 본문의 모든 최초 관측을 callback·물리 중단까지 포함하여 대조한다.</summary>
    [Fact]
    public void OriginalX86Fixture()
    {
        string[][] rows = X86Fixture.Read("fort-palette-transition");
        int observations = 0;
        int stops = 0;
        // 두 FPCW 관측 모두 독립 초기 상태에서 실행한다
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            PaletteDevice device = new(SplitPalette(row[1], ','));
            string cause = device.Run();
            Assert.Equal(row[2], cause);
            Assert.Equal(row[3], device.Values());
            Assert.Equal(row[4], device.Raw());
            Assert.Equal(row[5], device.Trace());
            observations++;
            if (cause != "1")
            {
                stops++;
            }
        }
        Assert.Equal(676, observations);
        Assert.Equal(34, stops);
    }

    /// <summary>자연 생략은 미연결 하위를 부르지 않으며 도달한 필수 Screen 경계의 누락은 예외와 이전 효과를 보존한다.</summary>
    [Fact]
    public void RequiredBoundariesAndNaturalReturn()
    {
        PaletteDevice same = new(SplitPalette("transition,1,11,3,1,42,22,0,none", ','));
        FortPaletteTransitionGlobals globals = same.Globals();
        FortPaletteTransitionHooks hooks = same.Hooks();
        hooks.Release = null;
        hooks.BuildCloud = null;
        hooks.StepCloud = null;
        hooks.PathSpec = null;
        hooks.LoadPalette = null;
        hooks.BuildMaps = null;
        RawFortPaletteTransition.TransitionFortPalette(same.Argument.Span, globals, hooks);
        Assert.Equal(PaletteDevice.Token(11), same.Cache);
        Assert.Equal(42u, same.Dirty);
        Assert.Equal(2, same.TraceList.Count);
        PaletteDevice missing = new(SplitPalette("transition,0,11,0,1,42,22,0,none", ','));
        FortPaletteTransitionGlobals missingGlobals = missing.Globals();
        FortPaletteTransitionHooks missingHooks = missing.Hooks();
        missingHooks.LoadPalette = null;
        bool rejected = false;
        try
        {
            RawFortPaletteTransition.TransitionFortPalette(missing.Argument.Span, missingGlobals, missingHooks);
        }
        catch (InvalidOperationException)
        {
            rejected = true;
        }
        Assert.True(rejected);
        Assert.Equal(PaletteDevice.Token(22), missing.Cache);
        Assert.Equal(42u, missing.Dirty);
        Assert.Equal(1u, missing.Calls["free"]);
        Assert.Equal(1u, missing.Calls["build"]);
        Assert.Equal(0u, missing.Calls.GetValueOrDefault("maps"));
        Assert.Equal("676966636c6f7564", PaletteHex(PaletteDevice.CString(missing.Palette.AsSpan(1), "test")));
    }
}
