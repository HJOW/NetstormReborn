using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort TypeNames 로더(<see cref="RawFortTypeNamesLoad"/>)를 독립 원시 입력과 대조한다.
/// cpppj <c>FortTypeNamesLoadTests</c>의 최초 664행 전수와 선택·부분 상태 직접 검사를 옮겼다. (F2b, 2026-11-02)
/// </summary>
public sealed class FortTypeNamesLoadTests
{
    /// <summary>최대 255 hash와 pos 경계를 별도로 관측하는 물리 범위다.</summary>
    private const int BufferBytes = 1088;

    /// <summary>원문 명시 data 주소를 보존하는 토큰이다.</summary>
    private static readonly uint[] DataTokens = [0x12001000, 0x12002000];

    /// <summary>0·기존·새 할당·교체 주소다.</summary>
    private static readonly uint[] BlockTokens = [0, 0x12004000, 0x12004200, 0x12004400];

    /// <summary>원시 signed char·NUL·긴 이름·중복 입력이다.</summary>
    private static readonly string[] TypeNames = ["Alpha", "Beta", "", "ÿA\x80\x7f", "Alpha", "LongNameWithMoreThanTwentyCharacters123", "Case", "case"];

    /// <summary>저장 hash 입력의 네 종류를 고정한다. 제품 해시나 예상 변환표에서 값을 얻지 않는다.</summary>
    /// <param name="mode">입력 종류 번호다.</param>
    private static uint[] StoredHashes(uint mode)
    {
        const uint alpha = 0x68706ca2;
        const uint beta = 0x61746542;
        const uint signed = 0x7e8040ff;
        return mode switch
        {
            0 => [alpha, beta, 0u, signed],
            1 => [alpha, alpha, beta, 0u],
            _ => [0xffffffffu, 0x12345678u, 0x80000000u, 0x7fffffffu],
        };
    }

    /// <summary>
    /// 마지막 빈 필드도 포함해 fixture를 분리한다.
    /// </summary>
    /// <param name="text">분리할 원문이다.</param>
    /// <param name="separator">구분자다.</param>
    private static string[] TypesFields(string text, char separator)
    {
        List<string> result = [];
        int start = 0;
        // 구분자 뒤 빈 칸을 버리지 않는다
        for (; ; )
        {
            int at = text.IndexOf(separator, start);
            result.Add(at < 0 ? text[start..] : text.Substring(start, at - start));
            if (at < 0)
            {
                return [.. result];
            }
            start = at + 1;
        }
    }

    /// <summary>
    /// 명시 원시 필드의 DWORD만 little endian으로 읽는다. 정확한 네 BYTE의 합이다.
    /// </summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    private static uint TypesRead(ReadOnlySpan<byte> bytes, int offset)
    {
        uint result = 0;
        // 낮은 BYTE부터 조립한다
        for (int index = 0; index < 4; index++)
        {
            result |= (uint)bytes[offset + index] << (index * 8);
        }
        return result;
    }

    /// <summary>
    /// 초기 입력·callback의 지정 DWORD만 쓴다. 다른 필드·패딩은 유지한다.
    /// </summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void TypesPut(Span<byte> bytes, int offset, uint value)
    {
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// 전체 물리 BYTE를 NUL 뒤도 보존해 hex로 관측한다. 원문 순서로 관측한다.
    /// </summary>
    /// <param name="bytes">관측할 바이트다.</param>
    private static string TypesHex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        StringBuilder result = new();
        // 한 BYTE의 두 글자 표현이다
        foreach (byte value in bytes)
        {
            result.Append(digits[value >> 4]);
            result.Append(digits[value & 15]);
        }
        return result.ToString();
    }

    /// <summary>
    /// TYPE 장치의 명시 NUL 앞 바이트만 관측한다.
    /// </summary>
    /// <param name="bytes">관측할 바이트다.</param>
    /// <exception cref="ArgumentOutOfRangeException">TypeNames 장치 NUL 범위다.</exception>
    private static ReadOnlySpan<byte> TypesText(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0);
        if (end < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), bytes.Length, "TypeNames 장치 NUL 범위");
        }
        return bytes[..end];
    }

    /// <summary>
    /// 실제 메모리에서 지정 BYTE만 써서 이름 getter 뒤 변경을 만든다.
    /// </summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="name">쓸 이름이다.</param>
    private static void TypesInput(Span<byte> bytes, string name)
    {
        byte[] encoded = Encoding.Latin1.GetBytes(name);
        if (encoded.Length >= bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(name), name.Length, "TypeNames 장치 이름 크기");
        }
        encoded.CopyTo(bytes);
        bytes[encoded.Length] = 0;
    }

    /// <summary>
    /// 명시 하위 중단(원인 포함)을 본문·cleanup의 같은 성공으로 합치지 않는다.
    /// </summary>
    /// <param name="Cause">중단 경계 이름이다.</param>
    private sealed class TypesStop(string Cause) : Exception
    {
        /// <summary>중단 경계 이름이다.</summary>
        public string Cause { get; } = Cause;
    }

    /// <summary>기대열을 읽지 않고 원문과 동일한 입력·하위 효과만 구성한다.</summary>
    private sealed class TypesDevices
    {
        /// <summary>현재 전역·실제 경계 횟수다.</summary>
        public uint Slot { get; set; }

        /// <summary>0·기존·새 할당·교체 주소 중 현재 변환표다.</summary>
        public uint Conversion { get; set; }

        /// <summary>새 할당 주소다.</summary>
        public uint Allocated { get; set; }

        /// <summary>현재 signed typeCount다. TYPE callback이 바꿀 수 있다.</summary>
        public int Count { get; set; }

        /// <summary>선택된 stream 번호다.</summary>
        public uint Selected { get; set; }

        /// <summary>현재 깊이다.</summary>
        public uint Depth { get; set; }

        /// <summary>현재 조회 중인 TYPE 번호다.</summary>
        public uint CurrentType { get; set; }

        /// <summary>선택·진단·할당·기록 callback 순번이다.</summary>
        public uint Selects { get; set; }

        /// <summary>진단 순번이다.</summary>
        public uint Asserts { get; set; }

        /// <summary>할당 순번이다.</summary>
        public uint Allocations { get; set; }

        /// <summary>TYPE 조회 순번이다.</summary>
        public uint Records { get; set; }

        /// <summary>명시 효과와 실제 순서·접근 대상이다.</summary>
        public string Mutation;

        /// <summary>실제 호출 trace다.</summary>
        public string Trace = "";

        /// <summary>첫 버퍼 읽기·hash 읽기 구별이다. 비어 있으면 물리 실패가 아니다.</summary>
        public string Physical = "";

        /// <summary>고정 this의 9×20BYTE 실제 표다.</summary>
        public byte[] Headers { get; } = new byte[180];

        /// <summary>서로 다른 선택 stream의 실제 pos·end·data다.</summary>
        public byte[][] Streams { get; } = [new byte[16], new byte[16]];

        /// <summary>count·hash의 명시 물리 입력이다.</summary>
        public byte[][] Data { get; } = [new byte[BufferBytes], new byte[BufferBytes]];

        /// <summary>기존·새 할당·교체 변환표를 모두 관측한다.</summary>
        public byte[][] Blocks { get; } = [new byte[255], new byte[255], new byte[255]];

        /// <summary>count 증가 전에 모든 실제 이름 버퍼가 존재한다.</summary>
        public byte[][] Names { get; }

        /// <summary>
        /// 파일 hash는 입력으로만 설치하고 어느 칸이 갱신될지는 본문이 결정한다.
        /// </summary>
        /// <param name="input">입력 13개 문자열이다.</param>
        public TypesDevices(string[] input)
        {
            if (input.Length != 13)
            {
                throw new ArgumentException("TypeNames 입력 칸 수", nameof(input));
            }
            Slot = uint.Parse(input[0], CultureInfo.InvariantCulture);
            uint manager = uint.Parse(input[1], CultureInfo.InvariantCulture);
            Selected = uint.Parse(input[2], CultureInfo.InvariantCulture);
            bool present = input[3] != "0";
            uint end = uint.Parse(input[4], CultureInfo.InvariantCulture);
            uint position = uint.Parse(input[5], CultureInfo.InvariantCulture);
            uint stored = uint.Parse(input[6], CultureInfo.InvariantCulture);
            uint mode = uint.Parse(input[7], CultureInfo.InvariantCulture);
            Count = int.Parse(input[8], CultureInfo.InvariantCulture);
            Conversion = BlockTokens[uint.Parse(input[9], CultureInfo.InvariantCulture) / 11];
            Allocated = BlockTokens[uint.Parse(input[10], CultureInfo.InvariantCulture) / 11];
            Depth = uint.Parse(input[11], CultureInfo.InvariantCulture);
            Mutation = input[12];
            // 고정 헤더의 비대상 바이트도 원문 패턴을 그대로 공급한다
            for (int index = 0; index < Headers.Length; index++)
            {
                Headers[index] = (byte)(0x70 + index);
            }
            TypesPut(Headers, (int)Slot * 20, manager);
            // 두 stream의 data 주소와 저장 hash 입력을 구별한다
            for (int index = 0; index < Streams.Length; index++)
            {
                TypesPut(Streams[index], 0, present ? DataTokens[index] : 0);
                TypesPut(Streams[index], 4, position);
                TypesPut(Streams[index], 8, end);
                TypesPut(Streams[index], 12, BufferBytes);
                for (int address = 0; address < BufferBytes; address++)
                {
                    Data[index][address] = (byte)(0xa0 + address + index * 7); // NUL이 아닌 주변 입력도 보존한다
                }
                if (position < BufferBytes)
                {
                    Data[index][(int)position] = (byte)stored;
                    for (uint number = 0; number < stored; number++)
                    {
                        ulong target = (ulong)position + 1 + number * 4u;
                        if (target + 4 <= BufferBytes)
                        {
                            TypesPut(Data[index], (int)target, StoredHashes(mode)[(int)((number + index) % 4)]); // 물리 안의 입력만 설치한다
                        }
                    }
                }
            }
            // 새 할당도 원본이 0으로 만들기 전에는 독립 비영 입력을 가진다
            for (int index = 0; index < Blocks.Length; index++)
            {
                for (int address = 0; address < 255; address++)
                {
                    Blocks[index][address] = (byte)(0x60 + (index + 1) * 11 + address);
                }
            }
            Names = new byte[262][];
            // 긴 이름은 20자에서 자르지 않고 원본 NUL 위치를 공급한다
            for (int index = 0; index < Names.Length; index++)
            {
                Names[index] = new byte[128];
                Array.Fill(Names[index], (byte)0xa7);
                TypesInput(Names[index], TypeNames[index % TypeNames.Length]);
            }
        }

        /// <summary>실제 하위 진입 순서를 연결한다.</summary>
        /// <param name="value">기록할 항목이다.</param>
        public void Record(string value)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += value;
        }

        /// <summary>각 경계에서 지정한 현재 DWORD·버퍼만 바꾸며 본문의 재조회를 복제하지 않는다.</summary>
        /// <param name="boundary">효과 경계 이름이다.</param>
        public void Mutate(string boundary)
        {
            byte[] stream = Streams[(int)Selected]; // 명시 callback 진입 시점에 선택된 실제 주소다
            if (Mutation == "select-stream1" && boundary == "select")
            {
                Selected = 1;
            }
            else if (Mutation == "select-manager0" && boundary == "select")
            {
                TypesPut(Headers, (int)Slot * 20, 0);
            }
            else if (Mutation == boundary + "-depth7")
            {
                Depth = 7;
            }
            else if (Mutation == boundary + "-depthffffffff")
            {
                Depth = 0xffffffffu;
            }
            else if (Mutation == boundary + "-conversion0")
            {
                Conversion = 0;
            }
            else if (Mutation == boundary + "-conversion11")
            {
                Conversion = BlockTokens[1];
            }
            else if (Mutation == boundary + "-conversion22")
            {
                Conversion = BlockTokens[2];
            }
            else if (Mutation == boundary + "-data1")
            {
                TypesPut(stream, 0, DataTokens[1]);
            }
            else if (Mutation == boundary + "-pos9")
            {
                TypesPut(stream, 4, 9);
            }
            else if (Mutation == boundary + "-endffffffff")
            {
                TypesPut(stream, 8, 0xffffffffu);
            }
            else if (Mutation == boundary + "-count0")
            {
                Count = 0;
            }
            else if (Mutation == boundary + "-count260")
            {
                Count = 260;
            }
            else if (Mutation == "record1-name" && boundary == "record1")
            {
                TypesInput(Names[0], "Beta");
            }
            else if (Mutation == "record1-bufferhash" && boundary == "record1")
            {
                TypesPut(Data[0], 1, StoredHashes(0)[1]);
            }
            else if (Mutation == boundary + "-stop")
            {
                throw new TypesStop(boundary);
            }
        }

        /// <summary>두 현재 DWORD를 제품과 같은 실체로 빌린다.</summary>
        public FortTypeNamesLoadGlobals Globals() => new()
        {
            GetDepth = () => Depth,
            SetDepth = value => Depth = value,
            GetConversion = () => Conversion,
            SetConversion = value => Conversion = value,
        };

        /// <summary>Select·할당·TYPE만 경계로 두고 실제 검색·zero·pos·end는 제품이 수행한다.</summary>
        public FortTypeNamesLoadHooks Hooks() => new()
        {
            Manager = fixedSlot => TypesRead(Headers, fixedSlot * 20),
            Select = (manager, name, writable) =>
            {
                if (name != "TypeNames" || writable != 1)
                {
                    throw new InvalidOperationException("TypeNames 실제 Select 인자");
                }
                Selects++;
                Record("S:" + manager.ToString(CultureInfo.InvariantCulture) + ":TypeNames:1");
                Mutate("select");
                return Selected;
            },
            Stream = token => Streams[(int)token],
            Buffer = token =>
            {
                Physical = BuffersRead == 0 ? "count-read" : "hash-read";
                BuffersRead++;
                int found = Array.IndexOf(DataTokens, token);
                if (found < 0)
                {
                    throw new ArgumentOutOfRangeException(nameof(token), token, "TypeNames 현재 data 주소");
                }
                return Data[found];
            },
            Allocate = size =>
            {
                if (size != 255)
                {
                    throw new InvalidOperationException("TypeNames 실제 255BYTE 할당");
                }
                Allocations++;
                Record("N:255");
                Mutate("allocate");
                return Allocated;
            },
            ConversionBuffer = token =>
            {
                ConversionAccesses++;
                if (token == 0)
                {
                    throw new TypesStop(ConversionAccesses == 1 ? "allocation0" : "conversion-write");
                }
                int found = Array.IndexOf(BlockTokens, token);
                if (found < 1 || found >= BlockTokens.Length)
                {
                    throw new ArgumentOutOfRangeException(nameof(token), token, "TypeNames 현재 conversion 주소");
                }
                return Blocks[found - 1];
            },
            TypeCount = () =>
            {
                Record("C:" + Count.ToString(CultureInfo.InvariantCulture));
                return Count;
            },
            TypeName = index =>
            {
                CurrentType = (uint)index;
                Records++;
                Record("R:" + index.ToString(CultureInfo.InvariantCulture) + ':' + TypesHex(TypesText(Names[CurrentType])));
                Mutate("record" + Records.ToString(CultureInfo.InvariantCulture));
                return Names[CurrentType];
            },
            Assertion = line =>
            {
                if (line != 608)
                {
                    throw new InvalidOperationException("TypeNames 실제 진단행");
                }
                Asserts++;
                Record("A:608");
                Mutate("assert");
            },
            LoadingDepthChanged = value => Record("L:" + value.ToString(CultureInfo.InvariantCulture)),
        };

        /// <summary>버퍼 읽기 횟수다. 첫 읽기와 hash 읽기를 구별한다.</summary>
        public int BuffersRead { get; set; }

        /// <summary>변환 접근 횟수다.</summary>
        public int ConversionAccesses { get; set; }

        /// <summary>
        /// 본문·별도 cleanup 시점의 같은 현재값을 관측한다.
        /// </summary>
        public string State()
        {
            int found = Array.IndexOf(BlockTokens, Conversion);
            if (found < 0)
            {
                throw new InvalidOperationException("TypeNames 관측 pointer");
            }
            return Depth.ToString(CultureInfo.InvariantCulture) + ',' + (found * 11).ToString(CultureInfo.InvariantCulture) + ',' + Count.ToString(CultureInfo.InvariantCulture) + ',' + Selected.ToString(CultureInfo.InvariantCulture) + ',' + Selects.ToString(CultureInfo.InvariantCulture) + ',' + Asserts.ToString(CultureInfo.InvariantCulture) + ',' + Allocations.ToString(CultureInfo.InvariantCulture) + ',' + Records.ToString(CultureInfo.InvariantCulture);
        }

        /// <summary>
        /// CPU·callback이 바꾼 전체 raw를 NUL 뒤도 대조한다.
        /// </summary>
        public string Raw() => TypesHex(Headers) + ':' + TypesHex(Streams[0]) + ':' + TypesHex(Streams[1]) + ':' + TypesHex(Data[0]) + ':' + TypesHex(Data[1]) + ':' + TypesHex(Blocks[0]) + ':' + TypesHex(Blocks[1]) + ':' + TypesHex(Blocks[2]) + ':' + TypesHex(Names[0]) + ':' + TypesHex(Names[1]);
    }

    /// <summary>
    /// 최초 332입력의 664본문과 38별도 cleanup을 원문 전체 BYTE·순서와 직접 대조한다.
    /// </summary>
    [Fact]
    public void IndependentNative664Rows()
    {
        string[][] rows = X86Fixture.Read("fort-type-names-load");
        int compared = 0;
        int stopped = 0;
        HashSet<(string, string)> unique = [];
        // 입력열만 실행한 뒤 제품 작성 전에 고정한 기대열을 읽는다
        foreach (string[] row in rows)
        {
            Assert.Equal(7, row.Length);
            Assert.True(unique.Add((row[0], row[1])));
            TypesDevices devices = new(TypesFields(row[1], ','));
            FortTypeNamesLoadHooks hooks = devices.Hooks();
            string completion = "1";
            try
            {
                RawFortTypeNamesLoad.LoadFortTypeNames((int)devices.Slot, devices.Globals(), hooks);
            }
            catch (TypesStop stop)
            {
                completion = stop.Cause;
            }
            catch (ArgumentOutOfRangeException)
            {
                if (devices.Physical.Length == 0)
                {
                    throw;
                }
                completion = devices.Physical;
            }
            string body = devices.State();
            string raw = devices.Raw();
            string trace = devices.Trace;
            string cleanup = "-";
            if (completion != "1")
            {
                stopped++;
                devices.Trace = "";
                RawFortTypeNamesLoad.UnwindFortTypeNamesLoad(devices.Globals(), hooks);
                cleanup = devices.State() + '/' + devices.Trace;
                Assert.Equal(raw, devices.Raw());
            }
            Assert.Equal(row[2], completion);
            Assert.Equal(row[3], body);
            Assert.Equal(row[4], raw);
            Assert.Equal(row[5], trace);
            Assert.Equal(row[6], cleanup);
            compared++;
        }
        Assert.Equal(664, compared);
        Assert.Equal(38, stopped);
    }

    /// <summary>
    /// 빈 stream의 미도달 경계·부분 zero·원문 NUL 누락을 원문 fixture와 별도로 검사한다.
    /// </summary>
    [Fact]
    public void SelectedBoundariesAndPartialState()
    {
        TypesDevices empty = new(TypesFields("1,11,0,0,1,0,4,0,7,11,22,1,none", ','));
        FortTypeNamesLoadHooks hooks = empty.Hooks();
        hooks.Allocate = null;
        hooks.TypeName = null;
        hooks.TypeCount = null;
        hooks.Assertion = null;
        hooks.Buffer = null;
        hooks.ConversionBuffer = null;
        RawFortTypeNamesLoad.LoadFortTypeNames(1, empty.Globals(), hooks);
        Assert.Equal(0u, empty.Conversion);
        Assert.Equal(1u, empty.Depth);
        Assert.Equal(0u, empty.Allocations);
        TypesDevices missing = new(TypesFields("1,11,0,1,1,0,4,0,7,11,22,1,none", ','));
        hooks = missing.Hooks();
        hooks.Assertion = null;
        bool rejected = false;
        try
        {
            RawFortTypeNamesLoad.LoadFortTypeNames(1, missing.Globals(), hooks);
        }
        catch (InvalidOperationException)
        {
            rejected = true;
        }
        Assert.True(rejected);
        Assert.Equal(2u, missing.Depth);
        Assert.Equal(1u, TypesRead(missing.Streams[0], 4));
        Assert.Equal(0u, missing.Allocations);
        Assert.Equal(BlockTokens[1], missing.Conversion);
        RawFortTypeNamesLoad.UnwindFortTypeNamesLoad(missing.Globals(), hooks);
        Assert.Equal(1u, missing.Depth);
        TypesDevices shortBlock = new(TypesFields("1,11,0,1,1,0,4,0,7,0,22,1,none", ','));
        hooks = shortBlock.Hooks();
        hooks.ConversionBuffer = _ => shortBlock.Blocks[1].AsSpan(0, 254);
        rejected = false;
        try
        {
            RawFortTypeNamesLoad.LoadFortTypeNames(1, shortBlock.Globals(), hooks);
        }
        catch (ArgumentOutOfRangeException)
        {
            rejected = true;
        }
        Assert.True(rejected);
        Assert.Equal(2u, shortBlock.Depth);
        Assert.Equal(BlockTokens[2], shortBlock.Conversion);
        Assert.Equal(0, shortBlock.Blocks[1][0]);
        Assert.Equal(0, shortBlock.Blocks[1][253]);
        Assert.NotEqual(0, shortBlock.Blocks[1][254]);
        RawFortTypeNamesLoad.UnwindFortTypeNamesLoad(shortBlock.Globals(), hooks);
        Assert.Equal(1u, shortBlock.Depth);
        TypesDevices noNul = new(TypesFields("1,11,0,1,1,0,4,0,1,0,22,1,none", ','));
        hooks = noNul.Hooks();
        byte[] missingNul = [(byte)'A', (byte)'B'];
        hooks.TypeName = _ => missingNul;
        rejected = false;
        try
        {
            RawFortTypeNamesLoad.LoadFortTypeNames(1, noNul.Globals(), hooks);
        }
        catch (ArgumentOutOfRangeException)
        {
            rejected = true;
        }
        Assert.True(rejected);
        Assert.Equal(2u, noNul.Depth);
        Assert.Equal(BlockTokens[2], noNul.Conversion);
        RawFortTypeNamesLoad.UnwindFortTypeNamesLoad(noNul.Globals(), hooks);
        Assert.Equal(1u, noNul.Depth);
    }
}
