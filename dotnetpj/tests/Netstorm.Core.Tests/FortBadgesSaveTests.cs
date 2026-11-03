// cpppj FortBadgesSaveTests.cpp의 2개 TEST_CASE를 xUnit으로 옮긴 뱃지 저장 검사다. (F2c-6, 2026-11-03)
using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 뱃지 저장 검사를 모아 둔 네임스페이스다.
namespace Netstorm.Core.Tests;

/// <summary>
/// 실제 저장 부모/getter의 최초 관측과 전체 raw 장부/stream/출력/부분 중단을 비교한다.
/// cpppj <c>FortBadgesSaveTests</c>의 전수 1352행과 누락 장치 직접 1종을 옮겼다. (F2c-6, 2026-11-03)
/// </summary>
public sealed class FortBadgesSaveTests
{
    /// <summary>최초 PE 격리 입력의 두 살아 있는 할당 주소다.</summary>
    private const uint BadgesBufferA = 0x12008000u;

    /// <summary>최초 PE 격리 입력의 두 살아 있는 할당 주소다.</summary>
    private const uint BadgesBufferB = 0x12009000u;

    /// <summary>raw Template에 저장하는 실제 가상 manager 포인터다.</summary>
    private const uint BadgesManager11 = 0x12000100u;

    /// <summary>raw Template에 저장하는 실제 가상 manager 포인터다.</summary>
    private const uint BadgesManager22 = 0x12000200u;

    /// <summary>
    /// 마지막 빈 칸도 보존하는 최초 입력/관측 필드 분리다.
    /// </summary>
    /// <param name="source">분리할 원문이다.</param>
    /// <param name="separator">구분자다.</param>
    private static string[] BadgesFields(string source, char separator)
    {
        // 각 원래 구분자와 빈 문자열의 순서를 유지한다
        return source.Split(separator);
    }

    /// <summary>
    /// 해당 네 BYTE를 little endian DWORD로 읽는다.
    /// </summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    private static uint BadgesGet(ReadOnlySpan<byte> bytes, int at)
    {
        uint value = 0;
        // 주변 padding은 읽지 않는다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>
    /// 지정 네 BYTE만 바꾸고 주변 입력은 그대로 보존한다.
    /// </summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void BadgesPut(Span<byte> bytes, int at, uint value)
    {
        // 원본 DWORD 폭만 쓴다
        for (int index = 0; index < 4; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// 실제 물리 메모리의 모든 BYTE를 관측과 같은 소문자 hex로 표현한다.
    /// </summary>
    /// <param name="bytes">표시할 바이트다.</param>
    private static string BadgesHex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        StringBuilder result = new();
        // 한 BYTE의 두 글자 표현에 쓰는 고정 표다
        foreach (byte value in bytes)
        {
            result.Append(digits[value >> 4]);
            result.Append(digits[value & 15]);
        }
        return result.ToString();
    }

    /// <summary>
    /// 전체 6160BYTE를 무손실 RLE로 표현해 type/count 사이 212BYTE도 검증한다.
    /// </summary>
    /// <param name="bytes">표시할 바이트다.</param>
    private static string BadgesRle(ReadOnlySpan<byte> bytes)
    {
        StringBuilder result = new();
        int start = 0;
        // 연속한 같은 BYTE의 길이와 값만 출력한다
        while (start < bytes.Length)
        {
            int end = start + 1;
            // 다음 run의 경계만 찾는다
            while (end < bytes.Length && bytes[end] == bytes[start])
            {
                end++;
            }
            if (result.Length != 0)
            {
                result.Append(',');
            }
            result.Append(end - start);
            result.Append(':');
            result.Append(BadgesHex(bytes.Slice(start, 1)));
            start = end;
        }
        return result.ToString();
    }

    /// <summary>
    /// manager 장치 ID만 실제 raw 포인터로 표현한다.
    /// </summary>
    /// <param name="manager">장치 ID다.</param>
    private static uint BadgesPointer(uint manager)
    {
        if (manager == 0)
        {
            return 0;
        }
        if (manager == 11)
        {
            return BadgesManager11;
        }
        if (manager == 22)
        {
            return BadgesManager22;
        }
        throw new ArgumentOutOfRangeException(nameof(manager), manager, "Badges manager ID 범위");
    }

    /// <summary>
    /// raw Template의 포인터를 외부 장치 ID로만 정규화한다.
    /// </summary>
    /// <param name="pointer">raw 포인터다.</param>
    private static uint BadgesManager(uint pointer)
    {
        if (pointer == 0)
        {
            return 0;
        }
        if (pointer == BadgesManager11)
        {
            return 11;
        }
        if (pointer == BadgesManager22)
        {
            return 22;
        }
        throw new ArgumentOutOfRangeException(nameof(pointer), pointer, "Badges manager 포인터 범위");
    }

    /// <summary>원본 emu_stop과 같은 지정 경계의 중단 표식이다.</summary>
    internal sealed class BadgesStop : Exception;

    /// <summary>최초 입력과 외부 장치 응답만 제공한다. 출력/조회 순서는 제품이 결정한다.</summary>
    private sealed class BadgesDevices
    {
        /// <summary>최초 입력의 종류 DWORD다.</summary>
        public uint Kind;

        /// <summary>최초 입력의 장부 profile DWORD다.</summary>
        public uint Profile;

        /// <summary>최초 입력의 조회 type DWORD다.</summary>
        public uint Query;

        /// <summary>최초 입력의 수량 DWORD다.</summary>
        public uint Count;

        /// <summary>최초 입력의 manager DWORD다.</summary>
        public uint Manager;

        /// <summary>최초 입력의 stream 위치 DWORD다.</summary>
        public uint Position;

        /// <summary>최초 입력의 stream 끝 DWORD다.</summary>
        public uint End;

        /// <summary>최초 입력의 stream 용량 DWORD다.</summary>
        public uint Capacity;

        /// <summary>명시 장치의 실제 Create 응답 횟수다.</summary>
        public uint Created;

        /// <summary>명시 장치가 반환한 현재 stream 토큰이다.</summary>
        public uint Token;

        /// <summary>getter 진입점의 실제 반환 DWORD다.</summary>
        public uint Result;

        /// <summary>용량 진단 뒤 부분 상태인 실제 호출 횟수다.</summary>
        public uint CapacityCalls;

        /// <summary>현재 반환 stream 번호다.</summary>
        public uint Active = 1;

        /// <summary>현재 count 할당의 토큰이다.</summary>
        public uint CounterToken;

        /// <summary>현재 count 할당의 위치다.</summary>
        public uint CounterPosition;

        /// <summary>실제 Template1의 전체 20BYTE다.</summary>
        public byte[] Header = new byte[20];

        /// <summary>명시 Create가 반환할 두 실제 stream 16바이트다.</summary>
        public byte[][] Nodes = [new byte[16], new byte[16]];

        /// <summary>data 변경 뒤에도 최초 할당이 살아 있는 두 실제 256바이트 출력이다.</summary>
        public byte[][] Buffers = [new byte[256], new byte[256]];

        /// <summary>의미 없는 padding도 입력 그대로 가진 실제 28개 220BYTE 장부다.</summary>
        public byte[] Badges = new byte[FortBadgeSaveShape.RecordCount * FortBadgeSaveShape.RecordStride];

        /// <summary>원문 외부 DataManager의 현재 값이다.</summary>
        public Dictionary<uint, uint> Currents = new() { { 0, 90 }, { 11, 91 }, { 22, 92 } };

        /// <summary>실제 외부 경계별 호출 횟수다.</summary>
        public Dictionary<string, int> Calls = [];

        /// <summary>최초 입력의 callback 변경이다.</summary>
        public string Mutation = "";

        /// <summary>실제 발생한 전체 경계 순서다.</summary>
        public string Trace = "";

        /// <summary>
        /// 최초 fixture의 입력열만 파싱하며 결과열은 제품 실행 뒤에만 사용한다.
        /// </summary>
        /// <param name="input">쉼표로 나눈 최초 입력 9칸이다.</param>
        public BadgesDevices(string input)
        {
            string[] fields = BadgesFields(input, ',');
            if (fields.Length != 9)
            {
                throw new ArgumentException("Badges 저장 입력 칸 수 오류", nameof(input));
            }
            Kind = uint.Parse(fields[0], CultureInfo.InvariantCulture);
            Profile = uint.Parse(fields[1], CultureInfo.InvariantCulture);
            Query = uint.Parse(fields[2], CultureInfo.InvariantCulture);
            Count = uint.Parse(fields[3], CultureInfo.InvariantCulture);
            Manager = uint.Parse(fields[4], CultureInfo.InvariantCulture);
            Position = uint.Parse(fields[5], CultureInfo.InvariantCulture);
            End = uint.Parse(fields[6], CultureInfo.InvariantCulture);
            Capacity = uint.Parse(fields[7], CultureInfo.InvariantCulture);
            Mutation = fields[8];
            // 전체 Template 초기 입력이다
            for (int index = 0; index < Header.Length; index++)
            {
                Header[index] = (byte)index;
            }
            BadgesPut(Header, 0, BadgesPointer(Manager));
            Array.Fill(Badges, (byte)0xa5);
            Array.Fill(Buffers[0], (byte)0xcc);
            Array.Fill(Buffers[1], (byte)0xdd);
            // 두 명시 stream은 같은 pos/end/capacity와 서로 다른 data를 갖는다
            for (int index = 0; index < Nodes.Length; index++)
            {
                BadgesPut(Nodes[index], 0, index == 0 ? BadgesBufferA : BadgesBufferB);
                BadgesPut(Nodes[index], 4, Position);
                BadgesPut(Nodes[index], 8, End);
                BadgesPut(Nodes[index], 12, Capacity);
            }
            // 최초 도구와 같은 장부 입력만 구성하며 저장 기대값을 만들지 않는다
            for (uint index = 0; index < 28; index++)
            {
                uint type = index;
                uint quantity = 0;
                if (Profile == 0)
                {
                    type = index == 0 ? Query : 70 + index;
                    quantity = index == 0 ? Count : 0;
                }
                else if (Profile == 1)
                {
                    bool duplicate = index == 0 || index == 13 || index == 27;
                    type = duplicate ? Query : 100 + index;
                    quantity = duplicate ? Count + (index == 0 ? 0u : index == 13 ? 1u : 2u) : 0u;
                }
                else if (Profile == 2)
                {
                    type = index < 3 ? new uint[] { 0, 254, 255 }[index] : index == 27 ? 0u : 100 + index;
                    quantity = index < 3 ? Count : index == 27 ? Count + 7u : 0u;
                }
                else if (Profile == 3)
                {
                    type = index < 27 ? index : 254u;
                    quantity = Count + index;
                }
                else if (Profile == 4)
                {
                    type = 1000 + index;
                    quantity = Count;
                }
                BadgesPut(Badges, (int)(index * 220), type);
                BadgesPut(Badges, (int)(index * 220 + 216), quantity);
            }
        }

        /// <summary>
        /// 실제 buffer 포인터를 같은 할당의 span으로만 변환한다.
        /// </summary>
        /// <param name="pointer">조회할 buffer 토큰이다.</param>
        public Span<byte> Buffer(uint pointer)
        {
            if (pointer == BadgesBufferA)
            {
                return Buffers[0];
            }
            if (pointer == BadgesBufferB)
            {
                return Buffers[1];
            }
            throw new ArgumentOutOfRangeException(nameof(pointer), pointer, "Badges 명시 물리 buffer 범위");
        }

        /// <summary>
        /// 최초 원문 EDI 포인터가 가리킨 같은 BYTE만 읽는다.
        /// </summary>
        public int Counter()
        {
            if (CounterToken == 0)
            {
                return -1;
            }
            return Buffers[(int)CounterToken - 1][(int)CounterPosition];
        }

        /// <summary>
        /// native 경계와 같은 한 시간순 trace에 기록한다.
        /// </summary>
        /// <param name="name">경계 이름이다.</param>
        /// <param name="text">기록 본문이다.</param>
        public int Event(string name, string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += text;
            int count = Calls.GetValueOrDefault(name) + 1;
            Calls[name] = count;
            return count;
        }

        /// <summary>
        /// 원본에 공급한 지정 외부효과만 반영한다. 이후 stream 선택/분기는 제품이 수행한다.
        /// </summary>
        /// <param name="boundary">효과를 적용할 경계 이름이다.</param>
        public void Mutate(string boundary)
        {
            int split = Mutation.LastIndexOf('-');
            if (split < 0)
            {
                return;
            }
            if (!string.Equals(Mutation[..split], boundary, StringComparison.Ordinal))
            {
                return;
            }
            string action = Mutation[(split + 1)..];
            Span<byte> node = Nodes[(int)Active - 1];
            if (action.StartsWith("manager", StringComparison.Ordinal))
            {
                BadgesPut(Header, 0, BadgesPointer(uint.Parse(action[7..], CultureInfo.InvariantCulture)));
            }
            else if (action == "dataA")
            {
                BadgesPut(node, 0, BadgesBufferA);
            }
            else if (action == "dataB")
            {
                BadgesPut(node, 0, BadgesBufferB);
            }
            else if (action.StartsWith("pos", StringComparison.Ordinal))
            {
                BadgesPut(node, 4, uint.Parse(action[3..], CultureInfo.InvariantCulture));
            }
            else if (action.StartsWith("end", StringComparison.Ordinal))
            {
                BadgesPut(node, 8, uint.Parse(action[3..], CultureInfo.InvariantCulture));
            }
            else if (action.StartsWith("cap", StringComparison.Ordinal))
            {
                BadgesPut(node, 12, uint.Parse(action[3..], CultureInfo.InvariantCulture));
            }
            else if (action.StartsWith("counter", StringComparison.Ordinal))
            {
                Buffers[(int)CounterToken - 1][(int)CounterPosition] = (byte)uint.Parse(action[7..], CultureInfo.InvariantCulture);
            }
            else if (action == "table999")
            {
                BadgesPut(Badges, 216, 999);
            }
            else if (action == "current2")
            {
                // 다른 current를 골라도 반환 stream을 교체하지 않는다
                foreach (uint key in Currents.Keys.ToArray())
                {
                    Currents[key] = 2;
                }
            }
            else if (action == "stop")
            {
                throw new BadgesStop();
            }
        }

        /// <summary>
        /// 원문 Create/진단/getter 반환 callback과 같은 명시 응답만 구성한다.
        /// </summary>
        public FortBadgesSaveHooks Hooks()
        {
            return new FortBadgesSaveHooks
            {
                Manager = slot =>
                {
                    Assert.Equal(1, slot);
                    return BadgesManager(BadgesGet(Header, 0));
                },
                Create = (selected, name) =>
                {
                    Assert.Equal("Badges", name);
                    Created++;
                    Token = Mutation == "create-return2" ? 2u : 1u;
                    Active = Token;
                    Event("create", $"create:{selected}:Badges");
                    Currents[selected] = Token;
                    Mutate("create");
                    return Token;
                },
                Assertion = reason =>
                {
                    Span<byte> node = Nodes[(int)Active - 1];
                    uint line = (uint)reason;
                    int index = Event("assert", $"assert:{line}:{BadgesGet(node, 0)}:{BadgesGet(node, 4)}:{BadgesGet(node, 8)}:{BadgesGet(node, 12)}:{Counter()}");
                    Mutate(line == 36 ? $"capacity{++CapacityCalls}" : $"assert{index}");
                },
                Count = type =>
                {
                    uint value = RawFortBadgesSave.ReadFortBadgeCount(Badges, type);
                    Event("count", $"count:{type}:{value}:{Counter()}");
                    Mutate($"count{type}");
                    return value;
                },
                Access = (selected, operation) =>
                {
                    Assert.Equal(Token, selected);
                    byte[] node = Nodes[(int)selected - 1];
                    if (CounterToken == 0)
                    {
                        CounterToken = BadgesGet(node, 0) == BadgesBufferA ? 1u : 2u;
                        CounterPosition = BadgesGet(node, 4);
                    }
                    MemStreamHooks memory = new()
                    {
                        Buffer = pointer => Buffer(pointer),
                    };
                    operation(node, memory);
                },
            };
        }

        /// <summary>
        /// 한 행의 제품 부모 또는 실제 장부 getter만 명시 실행한다.
        /// </summary>
        /// <param name="hooks">연결할 저장 경계다.</param>
        public void Run(FortBadgesSaveHooks hooks)
        {
            if (Kind == 0)
            {
                RawFortBadgesSave.SaveFortBadges(1, hooks);
            }
            else
            {
                Result = RawFortBadgesSave.ReadFortBadgeCount(Badges, Query);
            }
        }

        /// <summary>
        /// 전체 현재 외부 상태 7DWORD를 최초 원문과 같은 순서로 표현한다.
        /// </summary>
        public string State()
        {
            return $"{BadgesManager(BadgesGet(Header, 0))},{Created},{Token},{Result},{Currents[0]},{Currents[11]},{Currents[22]}";
        }

        /// <summary>
        /// 사용하지 않은 두 번째 stream/버퍼와 Template 부가 필드도 비교한다.
        /// </summary>
        public string Raw()
        {
            return BadgesHex(Header) + BadgesHex(Nodes[0]) + BadgesHex(Nodes[1]) + BadgesHex(Buffers[0]) + BadgesHex(Buffers[1]);
        }
    }

    /// <summary>
    /// 최초 676입력에 두 정밀도를 둔 모든 관측과 80회 부분 중단을 비교한다.
    /// cpppj <c>FortBadgesSave_IndependentNative1352</c>를 옮겼다.
    /// </summary>
    [Fact]
    public void IndependentNative1352()
    {
        string[][] rows = X86Fixture.Read("fort-badges-save");
        int compared = 0;
        int stopped = 0;
        int counts = 0;
        int assertions = 0;
        int creates = 0;
        HashSet<(string, string)> unique = [];
        // 관측 기대열은 제품 실행이 끝난 뒤에만 사용한다
        foreach (string[] row in rows)
        {
            Assert.Equal(7, row.Length);
            string trace = row[6].EndsWith('\r') ? row[6][..^1] : row[6];
            Assert.True(unique.Add((row[0], row[1])));
            BadgesDevices devices = new(row[1]);
            bool completed = true;
            try
            {
                devices.Run(devices.Hooks());
            }
            catch (BadgesStop)
            {
                completed = false;
                stopped++;
            }
            Assert.Equal(completed, row[2] == "1");
            Assert.Equal(devices.State(), row[3]);
            Assert.Equal(devices.Raw(), row[4]);
            Assert.Equal(BadgesRle(devices.Badges), row[5]);
            Assert.Equal(devices.Trace, trace);
            compared++;
            counts += devices.Calls.GetValueOrDefault("count");
            assertions += devices.Calls.GetValueOrDefault("assert");
            creates += devices.Calls.GetValueOrDefault("create");
        }
        Assert.Equal(1352, compared);
        Assert.Equal(80, stopped);
        Assert.Equal(167834, counts);
        Assert.Equal(3788, assertions);
        Assert.Equal(710, creates);
    }

    /// <summary>
    /// raw getter의 장부 물리 계약과 선택된 누락 child의 선행 효과를 명시 검사한다.
    /// cpppj <c>FortBadgesSave_MissingDevicesAndPhysicalLedger</c>를 옮겼다.
    /// </summary>
    [Fact]
    public void MissingDevicesAndPhysicalLedger()
    {
        BadgesDevices missing = new("0,0,0,1,11,0,0,128,none");
        FortBadgesSaveHooks hooks = missing.Hooks();
        hooks.Count = null;
        bool rejected = false;
        try
        {
            missing.Run(hooks);
        }
        catch (InvalidOperationException)
        {
            rejected = true;
        }
        Assert.True(rejected && missing.Created == 1 && BadgesGet(missing.Nodes[0], 4) == 1 && missing.Buffers[0][0] == 0);
        BadgesDevices getter = new("1,1,255,0,11,0,0,128,none");
        Assert.Equal(0u, RawFortBadgesSave.ReadFortBadgeCount(getter.Badges, 255));
        Assert.Equal(0u, RawFortBadgesSave.ReadFortBadgeCount(getter.Badges, 0xffffffffu));
        rejected = false;
        try
        {
            RawFortBadgesSave.ReadFortBadgeCount(getter.Badges.AsSpan(0, 6159), 0);
        }
        catch (ArgumentException)
        {
            rejected = true;
        }
        Assert.True(rejected);
    }
}
