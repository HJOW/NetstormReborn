using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort 원시 Contents 로더(<see cref="RawFortContentsLoad"/>)를 독립 원시 입력과 대조한다.
/// cpppj <c>FortContentsLoadTests</c>의 최초 668행 전수와 선택·물리 경계 직접 검사를 옮겼다. (F2b, 2026-11-02)
/// </summary>
public sealed class FortContentsLoadTests
{
    /// <summary>빈 마지막 필드도 보존하는 원시 구분자 분리다. 제품 기대는 생성하지 않는다.</summary>
    /// <param name="source">분리할 원문이다.</param>
    /// <param name="delimiter">구분자다.</param>
    private static string[] ContentsFields(string source, char delimiter)
    {
        List<string> result = [];
        int start = 0;
        // 구분자 뒤 빈 칸을 버리지 않는다
        for (; ; )
        {
            int at = source.IndexOf(delimiter, start);
            result.Add(at < 0 ? source[start..] : source.Substring(start, at - start));
            if (at < 0)
            {
                return [.. result];
            }
            start = at + 1;
        }
    }

    /// <summary>
    /// callback 정지와 실제 변환 주소 0 물리 실패를 정상 false와 구별하는 검사 표식이다.
    /// </summary>
    private sealed class ContentsStop : Exception;

    /// <summary>실제 변환 주소 0의 물리 실패 표식이다.</summary>
    private sealed class ContentsFault : Exception;

    /// <summary>네이티브가 소유하지 않는 외부 장치·원시 입력만 유지한다.</summary>
    private sealed class ContentsDevices
    {
        /// <summary>fixture의 앞 10개 원시 DWORD다.</summary>
        public uint[] Input { get; } = new uint[10];

        /// <summary>원문 뒤 재조회할 현재값과 명시 경계 순번이다.</summary>
        public uint Conversion { get; set; }

        /// <summary>현재 checking DWORD다.</summary>
        public uint Checking { get; set; }

        /// <summary>같은 테스트의 현재 깊이다(본문은 쓰지 않는다).</summary>
        public uint Depth = 3;

        /// <summary>진입 부모의 현재 그릇 flags다.</summary>
        public uint ParentFlags { get; set; }

        /// <summary>TYPE callback 순번이다.</summary>
        public int TypeCalls { get; set; }

        /// <summary>부모 flags callback 순번이다.</summary>
        public int ParentCalls { get; set; }

        /// <summary>Create callback 순번이다.</summary>
        public int Creates { get; set; }

        /// <summary>진단 순번이다.</summary>
        public int Assertions { get; set; }

        /// <summary>현재 조회 중인 TYPE 번호다.</summary>
        public uint CurrentType { get; set; }

        /// <summary>마지막 변환 관측 TYPE 번호다.</summary>
        public uint LastConversionType { get; set; }

        /// <summary>해당 Create 호출에만 반환 0을 공급한다.</summary>
        public bool ZeroReturn { get; set; }

        /// <summary>독립 입력 callback과 실제 호출 관측이다.</summary>
        public string Mutation;

        /// <summary>실제 호출 trace다.</summary>
        public string Trace = "";

        /// <summary>두 실제 MemStream 필드다.</summary>
        public byte[][] Streams { get; } = [new byte[16], new byte[16]];

        /// <summary>논리 end와 다른 실제 물리 입력이다.</summary>
        public byte[][] Buffers { get; } = [new byte[2048], new byte[2048]];

        /// <summary>현재 토큰 11·22가 가리키는 서로 다른 변환표다.</summary>
        public byte[][] Tables { get; } = [new byte[255], new byte[255]];

        /// <summary>현재 TYPE 두 필드만 명시 공급한다.</summary>
        public FortContentsLoadType[] Types { get; } = new FortContentsLoadType[256];

        /// <summary>
        /// 입력열만으로 네이티브와 동일한 외부 저장소를 준비한다. 출력열은 여기서 읽지 않는다.
        /// </summary>
        /// <param name="fields">입력 11개 문자열이다.</param>
        public ContentsDevices(string[] fields)
        {
            for (int index = 0; index < Input.Length; index++)
            {
                Input[index] = uint.Parse(fields[index], CultureInfo.InvariantCulture);
            }
            Mutation = fields[10];
            Conversion = Input[1];
            Checking = Input[2];
            ParentFlags = Input[8];
            Types[2] = new FortContentsLoadType(0x1000, 0x104);
            Types[3] = new FortContentsLoadType(0x2000, 0);
            Types[4] = new FortContentsLoadType(0x3000, 0);
            Types[5] = new FortContentsLoadType(0x20000, 0);
            for (int index = 0; index < Tables.Length; index++)
            {
                for (int type = 0; type < 255; type++)
                {
                    Tables[index][type] = (byte)type;
                }
                Tables[index][245] = index == 0 ? (byte)2 : (byte)0; // 독립 원시 변환 입력이다
            }
            Tables[1][0] = 1;
            Tables[1][1] = 2;
            Tables[1][2] = 3;
            Tables[1][3] = 0;
            Tables[1][4] = 1;
            byte[] program = ContentsProgram(Input[0]); // 앞으로 소비할 원시 BYTE 입력이다
            for (int stream = 0; stream < Streams.Length; stream++)
            {
                // 각 반환 stream에 다른 물리 버퍼를 공급한다
                WriteU32(Streams[stream], 0, Input[4] != 0 ? (uint)(stream + 1) : 0);
                WriteU32(Streams[stream], 4, Input[6]);
                WriteU32(Streams[stream], 8, Input[5]);
                WriteU32(Streams[stream], 12, 2048);
                Array.Fill(Buffers[stream], (byte)(stream == 0 ? 0xcc : 0xdd));
                foreach (int position in new[] { 0, 512 })
                {
                    Array.Copy(program, 0, Buffers[stream], position, program.Length); // callback pos도 독립 원문을 갖는다
                }
            }
        }

        /// <summary>
        /// PE에도 공급한 count·type·QA·QB·listFlags·중첩 원시 입력이다.
        /// </summary>
        /// <param name="profile">입력 종류 번호다.</param>
        private static byte[] ContentsProgram(uint profile)
        {
            byte[][] inputs =
            [
                [1, 1, 0x11],
                [0],
                [3, 2, 254, 3, 128, 255, 1, 4, 85, 52, 128, 16],
                [3, 1, 16, 1, 1, 1, 0],
                [2, 5, 16, 2, 1, 0, 2, 7, 1, 4],
                [2, 1, 1, 0],
                [2, 5, 1, 2, 1, 1, 0, 1, 4],
                [3, 245, 0, 246, 0, 255, 0],
                [],
                [1, 3, 128, 255, 0],
            ];
            if (profile != 8)
            {
                return inputs[profile];
            }
            List<byte> bytes = [255];
            for (int index = 0; index < 255; index++)
            {
                bytes.Add(1);
                bytes.Add(0);
            }
            return [.. bytes]; // BYTE 개수 255의 실제 저장 입력이다
        }

        /// <summary>현재 네 BYTE 조립값이다. 이웃 필드를 섞지 않는다.</summary>
        /// <param name="bytes">읽을 바이트다.</param>
        /// <param name="at">읽을 위치다.</param>
        private static uint ReadU32(ReadOnlySpan<byte> bytes, int at)
        {
            uint value = 0;
            for (int index = 0; index < 4; index++)
            {
                value |= (uint)bytes[at + index] << (index * 8);
            }
            return value;
        }

        /// <summary>명시 입력·하위 효과의 한 DWORD만 쓴다. 다른 필드·패딩은 유지한다.</summary>
        /// <param name="bytes">쓸 바이트다.</param>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        private static void WriteU32(Span<byte> bytes, int at, uint value)
        {
            for (int index = 0; index < 4; index++)
            {
                bytes[at + index] = (byte)(value >> (index * 8));
            }
        }

        /// <summary>실제 하위 진입의 인자와 순서를 기록한다.</summary>
        /// <param name="text">기록할 항목이다.</param>
        public void Record(string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += text;
        }

        /// <summary>명시 외부 효과만 바꾸며 본문의 후속 분기·반환을 대신 계산하지 않는다.</summary>
        /// <param name="boundary">효과 경계 이름이다.</param>
        public void Mutate(string boundary)
        {
            int separator = Mutation.IndexOf('-');
            if (separator < 0)
            {
                return;
            }
            string trigger = Mutation[..separator];
            string action = Mutation[(separator + 1)..];
            if (trigger != boundary)
            {
                return;
            }
            byte[] fields = Streams[(int)Input[9]]; // 그때 실제 선택한 stream이다
            if (action == "stop")
            {
                throw new ContentsStop();
            }
            else if (action.StartsWith("conversion", StringComparison.Ordinal))
            {
                Conversion = uint.Parse(action[10..], CultureInfo.InvariantCulture);
            }
            else if (action == "dataB")
            {
                WriteU32(fields, 0, 2);
            }
            else if (action.StartsWith("pos", StringComparison.Ordinal))
            {
                WriteU32(fields, 4, uint.Parse(action[3..], CultureInfo.InvariantCulture));
            }
            else if (action.StartsWith("end", StringComparison.Ordinal))
            {
                WriteU32(fields, 8, uint.Parse(action[3..], System.Globalization.NumberStyles.HexNumber, CultureInfo.InvariantCulture));
            }
            else if (action == "flags0")
            {
                Types[CurrentType] = Types[CurrentType] with { Flags1 = 0 };
            }
            else if (action == "flagsQB")
            {
                Types[CurrentType] = Types[CurrentType] with { Flags1 = 0x2000 };
            }
            else if (action.StartsWith("list", StringComparison.Ordinal))
            {
                Types[CurrentType] = Types[CurrentType] with { ContentListFlags = uint.Parse(action[4..], CultureInfo.InvariantCulture) };
            }
            else if (action is "parent4" or "flags4")
            {
                ParentFlags = 4;
            }
            else if (action == "checking0")
            {
                Checking = 0;
            }
            else if (action == "depth7")
            {
                Depth = 7;
            }
            else if (action == "return0")
            {
                ZeroReturn = true;
            }
            else
            {
                throw new InvalidOperationException("Contents 검사 효과 미해석");
            }
        }

        /// <summary>실제 직접 읽기와 동일한 저장소를 연결하며 TYPE·Create·진단은 명시 외부 응답으로 둔다.</summary>
        public FortContentsLoadHooks Hooks()
        {
            // 본문의 stream·TYPE·flags 계산은 대체하지 않는다
            return new FortContentsLoadHooks
            {
                Access = (token, operation) =>
                {
                    if (token is < 1 or > 2)
                    {
                        throw new ArgumentOutOfRangeException(nameof(token), token, "Contents 검사 stream 물리 토큰");
                    }
                    MemStreamHooks memory = new()
                    {
                        Buffer = data =>
                        {
                            if (data is < 1 or > 2)
                            {
                                throw new ArgumentOutOfRangeException(nameof(data), data, "Contents 검사 data 물리 범위");
                            }
                            return Buffers[data - 1];
                        },
                    };
                    operation(Streams[token - 1], memory);
                },
                ConversionBuffer = token =>
                {
                    if (token == 0)
                    {
                        Record("F:" + LastConversionType.ToString(CultureInfo.InvariantCulture) + ":1");
                        throw new ContentsFault();
                    }
                    if (token != 11 && token != 22)
                    {
                        throw new ArgumentOutOfRangeException(nameof(token), token, "Contents 검사 conversion 물리 범위");
                    }
                    byte[] fields = Streams[(int)Input[9]];
                    uint data = ReadU32(fields, 0);
                    uint position = ReadU32(fields, 4); // 변환 진단 전 처음 인덱스만 관측한다
                    if (position != 0 && position <= 2048 && data >= 1 && data <= 2)
                    {
                        LastConversionType = Buffers[data - 1][position - 1];
                    }
                    return Tables[token == 11 ? 0 : 1];
                },
                Type = type =>
                {
                    if (type >= (uint)Types.Length)
                    {
                        throw new ArgumentOutOfRangeException(nameof(type), type, "Contents 검사 TYPE 물리 범위");
                    }
                    CurrentType = type;
                    Record("T:" + type.ToString(CultureInfo.InvariantCulture));
                    Mutate("type" + (++TypeCalls).ToString(CultureInfo.InvariantCulture));
                    return Types[type];
                },
                ParentListFlags = parent =>
                {
                    Record("P:" + parent.ToString(CultureInfo.InvariantCulture));
                    Mutate("parent" + (++ParentCalls).ToString(CultureInfo.InvariantCulture));
                    return ParentFlags;
                },
                Create = (parent, type, quantity, flags, attach, quality) =>
                {
                    Record("C:" + parent.ToString(CultureInfo.InvariantCulture) + ':' + type.ToString(CultureInfo.InvariantCulture) + ':' + quantity.ToString(CultureInfo.InvariantCulture) + ':' + flags.ToString(CultureInfo.InvariantCulture) + ':' + attach.ToString(CultureInfo.InvariantCulture) + ':' + quality.ToString(CultureInfo.InvariantCulture));
                    ZeroReturn = false;
                    Mutate("create" + (++Creates).ToString(CultureInfo.InvariantCulture));
                    return ZeroReturn ? 0u : 20u + (uint)Creates;
                },
                Assertion = line =>
                {
                    Record("A:" + ((uint)line).ToString(CultureInfo.InvariantCulture));
                    Mutate("assert" + (++Assertions).ToString(CultureInfo.InvariantCulture));
                },
            };
        }
    }

    /// <summary>
    /// 최초 고정한 모든 직접 cursor·현재 변환·TYPE·Create·재귀와 실제 정지·물리 실패를 비교한다.
    /// </summary>
    [Fact]
    public void IndependentNative664Rows()
    {
        string[][] rows = X86Fixture.Read("fort-contents-load");
        int compared = 0;
        // 앞 두 열은 입력이고 뒤 네 열은 제품 전에 독립 고정한 기대다
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            ContentsDevices devices = new(ContentsFields(row[1], ','));
            FortContentsLoadHooks hooks = devices.Hooks();
            FortContentsLoadGlobals globals = new()
            {
                GetConversion = () => devices.Conversion,
                SetConversion = value => devices.Conversion = value,
                Checking = () => devices.Checking,
            };
            string status = "normal";
            bool result = false;
            try
            {
                result = RawFortContentsLoad.LoadFortContents(10, devices.Input[9] + 1, devices.Input[7], devices.Input[3], globals, hooks);
            }
            catch (ContentsStop)
            {
                status = "stop";
            }
            catch (ContentsFault)
            {
                status = "fault";
            }
            Assert.Equal(row[2], status);
            string[] state = ContentsFields(row[3], ',');
            Assert.Equal(6, state.Length);
            Assert.Equal(uint.Parse(state[0], CultureInfo.InvariantCulture), result ? 1u : 0u);
            Assert.Equal(uint.Parse(state[1], CultureInfo.InvariantCulture), devices.Depth);
            Assert.Equal(uint.Parse(state[2], CultureInfo.InvariantCulture), devices.Conversion);
            Assert.Equal(uint.Parse(state[3], CultureInfo.InvariantCulture), devices.Checking);
            Assert.Equal(uint.Parse(state[4], CultureInfo.InvariantCulture), (uint)devices.Creates);
            Assert.Equal(uint.Parse(state[5], CultureInfo.InvariantCulture), devices.ParentFlags);
            string[] streamState = ContentsFields(row[4], ',');
            Assert.Equal(8, streamState.Length);
            for (int stream = 0; stream < 2; stream++)
            {
                for (int field = 0; field < 4; field++)
                {
                    Assert.Equal(uint.Parse(streamState[stream * 4 + field], CultureInfo.InvariantCulture), ReadU32(devices.Streams[stream], field * 4)); // 모든 현재 stream 필드를 비교한다
                }
            }
            Assert.Equal(row[5], devices.Trace.Length == 0 ? "-" : devices.Trace);
            compared++;
        }
        Assert.Equal(668, compared);
    }

    /// <summary>
    /// 현재 네 BYTE 조립값이다. 이웃 필드를 섞지 않는다.
    /// </summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    private static uint ReadU32(ReadOnlySpan<byte> bytes, int at)
    {
        uint value = 0;
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>
    /// container 분기는 flags1의 정확히 0x20000 비트를 본다. 하위 비트만 있는 타입은 중첩 재귀로 들어가지 않는다.
    /// </summary>
    [Fact]
    public void ContainerBitRequiresExactFlag()
    {
        // count 1·kind 6(flags1 하위 비트만)·listFlags 0x10 뒤에 자식 항목 2개를 둔다
        byte[] fields = new byte[16];
        WriteU32(fields, 0, 1);
        WriteU32(fields, 4, 0);
        WriteU32(fields, 8, 1);
        WriteU32(fields, 12, 2048);
        byte[] buffer = [1, 6, 0x10, 2, 99, 0, 0, 0];
        FortContentsLoadType[] types = new FortContentsLoadType[256];
        types[6] = new FortContentsLoadType(0x1, 0);
        List<string> trace = [];
        FortContentsLoadHooks hooks = new()
        {
            Access = (token, operation) =>
            {
                Assert.Equal(1u, token);
                operation(fields, new MemStreamHooks { Buffer = _ => buffer });
            },
            ConversionBuffer = _ => throw new InvalidOperationException("변환표가 없는 직접 입력"),
            Type = type =>
            {
                trace.Add("T:" + type.ToString(CultureInfo.InvariantCulture));
                return types[type];
            },
            ParentListFlags = parent =>
            {
                trace.Add("P:" + parent.ToString(CultureInfo.InvariantCulture));
                return 0;
            },
            Create = (parent, type, quantity, flags, attach, quality) =>
            {
                trace.Add("C:" + parent.ToString(CultureInfo.InvariantCulture) + ':' + type.ToString(CultureInfo.InvariantCulture) + ':' + quantity.ToString(CultureInfo.InvariantCulture) + ':' + flags.ToString(CultureInfo.InvariantCulture) + ':' + attach.ToString(CultureInfo.InvariantCulture) + ':' + quality.ToString(CultureInfo.InvariantCulture));
                return 20;
            },
            Assertion = _ => throw new InvalidOperationException("예상 밖 진단"),
        };
        FortContentsLoadGlobals globals = new()
        {
            GetConversion = () => 0,
            SetConversion = _ => { },
            Checking = () => 0,
        };
        Assert.True(RawFortContentsLoad.LoadFortContents(10, 1, 9, 0, globals, hooks));
        Assert.Equal(["T:6", "T:6", "P:10", "C:10:6:0:16:17:0"], trace);
    }

    /// <summary>
    /// 도달한 호스트 물리·장치 미연결 예외만 확인한다.
    /// </summary>
    /// <param name="function">실행할 동작이다.</param>
    private static bool Rejects(Action function)
    {
        try
        {
            function();
        }
        catch (Exception)
        {
            return true;
        }
        return false;
    }

    /// <summary>
    /// empty 정상 false·선택 장치 누락·논리 end와 별개인 물리 거부·현재 conversion을 구별한다.
    /// </summary>
    [Fact]
    public void SelectedDevicesAndPhysicalBounds()
    {
        ContentsDevices empty = new(ContentsFields("0,11,1,0,0,1,0,9,0,0,none", ','));
        FortContentsLoadHooks hooks = empty.Hooks();
        hooks.Type = null;
        hooks.Create = null;
        hooks.ConversionBuffer = null;
        Assert.False(RawFortContentsLoad.LoadFortContents(10, 1, 9, 0, new FortContentsLoadGlobals { GetConversion = () => empty.Conversion, SetConversion = value => empty.Conversion = value, Checking = () => 0 }, hooks));
        Assert.Equal(0u, ReadU32(empty.Streams[0], 4));
        ContentsDevices missing = new(ContentsFields("0,11,0,1,1,1,0,9,0,0,none", ','));
        FortContentsLoadHooks selected = missing.Hooks();
        selected.Create = null;
        Assert.True(Rejects(() => RawFortContentsLoad.LoadFortContents(10, 1, 9, 1, new FortContentsLoadGlobals { GetConversion = () => missing.Conversion, SetConversion = value => missing.Conversion = value, Checking = () => missing.Checking }, selected)));
        Assert.Equal(3u, ReadU32(missing.Streams[0], 4));
        Assert.Equal(0, missing.Creates); // Create 누락도 앞선 count·type·listFlags 소비 뒤 실패한다
        ContentsDevices physical = new(ContentsFields("0,11,0,1,1,1,0,9,0,0,none", ','));
        FortContentsLoadHooks bounded = physical.Hooks();
        WriteU32(physical.Streams[0], 4, 2048);
        Assert.True(Rejects(() => RawFortContentsLoad.LoadFortContents(10, 1, 9, 1, new FortContentsLoadGlobals { GetConversion = () => physical.Conversion, SetConversion = value => physical.Conversion = value, Checking = () => physical.Checking }, bounded)));
        Assert.Equal(2048u, ReadU32(physical.Streams[0], 4)); // 논리 end 1로 읽기를 생략하지 않고 물리 끝에서만 거부한다
    }

    /// <summary>
    /// 명시 입력·하위 효과의 한 DWORD만 쓴다. 다른 필드·패딩은 유지한다.
    /// </summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void WriteU32(Span<byte> bytes, int at, uint value)
    {
        for (int index = 0; index < 4; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }
}
