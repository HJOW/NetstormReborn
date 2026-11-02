using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort Subscriber 로더(<see cref="RawFortSubscriberLoad"/>)를 독립 원시 입력과 대조한다.
/// cpppj <c>FortSubscriberLoadTests</c>의 최초 694행 전수와 선택·부분 상태 직접 검사를 옮겼다. (F2b, 2026-11-02)
/// </summary>
public sealed class FortSubscriberLoadTests
{
    /// <summary>fixture 원시 구분자를 분리하며 마지막 빈 필드도 유지한다. 원문·출력열을 가공 없나 나눈다.</summary>
    /// <param name="source">분리할 원문이다.</param>
    /// <param name="delimiter">구분자다.</param>
    private static string[] SubscriberFields(string source, char delimiter)
    {
        List<string> result = [];
        int start = 0;
        // 끝 빈 칸도 하나의 열이다
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

    /// <summary>명시 원시 입력과 관측 출력의 DWORD를 읽는다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    private static uint SubscriberU(ReadOnlySpan<byte> bytes, int at)
    {
        uint value = 0;
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value; // 다른 필드를 섞지 않는다
    }

    /// <summary>하위 장치·입력만 현재 DWORD에 쓴다. 전체 32비트를 보존한다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void SubscriberPut(Span<byte> bytes, int at, uint value)
    {
        for (int index = 0; index < 4; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>출력 전체의 원시 BYTE를 hex로 표시해 내부 NUL과 stale 꼬리도 비교한다. 문자열 첫 NUL에서 잘라 비교하지 않는다.</summary>
    /// <param name="bytes">표시할 바이트다.</param>
    private static string SubscriberHex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        StringBuilder result = new();
        foreach (byte value in bytes)
        {
            result.Append(digits[value >> 4]);
            result.Append(digits[value & 15]);
        }
        return result.ToString();
    }

    /// <summary>명시 외부 중단은 물리 접근 실패와 구별한다.</summary>
    internal sealed class SubscriberStop : Exception;

    /// <summary>원문 밖 장치의 명시 입력·현재 상태만 소유하고 제품의 cursor·분기를 대신 계산하지 않는다.</summary>
    private sealed class SubscriberDevices
    {
        /// <summary>fixture 원시 DWORD열이다. 뒤 출력열로 장치를 만들지 않는다.</summary>
        public uint[] Input { get; } = new uint[12];

        /// <summary>원문이 그때 관측하는 현재 ID·고정 this·선택 반환과 진단 순번이다.</summary>
        public uint Subscriber { get; set; }

        /// <summary>현재 고정 manager다.</summary>
        public uint Manager { get; set; }

        /// <summary>선택 반환 번호다.</summary>
        public uint Selected { get; set; }

        /// <summary>진단 순번이다.</summary>
        public uint Assertions { get; set; }

        /// <summary>같은 선택 반환의 실제 fields다.</summary>
        public byte[][] Streams { get; } = [new byte[16], new byte[16]];

        /// <summary>논리 end와 별개인 두 실제 물리 할당이다.</summary>
        public byte[][] Buffers { get; } = [new byte[512], new byte[512]];

        /// <summary>전달된 고정 출력 주소의 stale BYTE까지 유지한다.</summary>
        public byte[] Output { get; } = new byte[512];

        /// <summary>명시 외부 변형과 실제 선택 경계 trace다.</summary>
        public string Mutation;

        /// <summary>실제 선택 경계 trace다.</summary>
        public string Trace = "";

        /// <summary>
        /// PE와 같은 원시 입력만 준비한다. 결과 ID·cursor·이름은 계산하지 않는다.
        /// </summary>
        /// <param name="fields">입력 13개 문자열이다.</param>
        public SubscriberDevices(string[] fields)
        {
            for (int index = 0; index < Input.Length; index++)
            {
                Input[index] = uint.Parse(fields[index], CultureInfo.InvariantCulture);
            }
            Mutation = fields[12];
            Manager = Input[2];
            Selected = Input[3];
            Subscriber = Input[0] != 0 ? Input[7] : 0x12345678u;
            Array.Fill(Output, (byte)0xa5); // 실제 출력 초깃값이다
            byte[] head = new byte[6];
            SubscriberPut(head, 0, Input[7]);
            head[4] = (byte)Input[8];
            head[5] = (byte)(Input[8] >> 8); // 파일 ID와 signed WORD의 원시 저장 폭이다
            List<byte> raw = [.. head];
            for (uint index = 0; index < Math.Min(Input[8], 300u); index++)
            {
                raw.Add((byte)(index * 37u + 65u)); // 내부 NUL도 있는 명시 입력이다
            }
            for (int index = 0; index < Streams.Length; index++)
            {
                // 두 반환 노드의 실제 data·pos·end를 독립적으로 설정한다
                SubscriberPut(Streams[index], 0, Input[4] != 0 ? (uint)(index + 1) : 0u);
                SubscriberPut(Streams[index], 4, Input[6]);
                SubscriberPut(Streams[index], 8, Input[5]);
                SubscriberPut(Streams[index], 12, 512);
                for (int address = 0; address < Buffers[index].Length; address++)
                {
                    Buffers[index][address] = (byte)(address * 17 + index * 91); // 나머지 물리 메모리도 PE와 같은 독립 입력이다
                }
                foreach (int position in new[] { 0, 64 })
                {
                    Array.Copy(raw.ToArray(), 0, Buffers[index], position, raw.Count); // callback이 옮긴 pos도 명시 원문을 읽는다
                }
            }
        }

        /// <summary>실제 경계 진입 순서를 기록한다.</summary>
        /// <param name="item">기록할 항목이다.</param>
        public void Record(string item)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += item;
        }

        /// <summary>입력이 명시한 외부 효과만 바꾼다. 그 뒤 처리는 원문 대응 제품이 한다.</summary>
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
            string plain = boundary.TrimEnd('0', '1', '2', '3', '4', '5', '6', '7', '8', '9'); // 하위 장치별 1기반 진단과 단일 경계를 구별한다
            if (trigger != boundary && trigger != plain)
            {
                return;
            }
            byte[] fields = Streams[(int)Selected]; // 외부 효과가 진입했을 때의 선택 반환 노드다
            if (action == "stop")
            {
                throw new SubscriberStop();
            }
            else if (action == "manager22")
            {
                Manager = 22;
            }
            else if (action == "manager0")
            {
                Manager = 0;
            }
            else if (action == "subscriber9")
            {
                Subscriber = 9;
            }
            else if (action == "stream1")
            {
                Selected = 1;
            }
            else if (action.StartsWith("pos", StringComparison.Ordinal))
            {
                SubscriberPut(fields, 4, uint.Parse(action[3..], CultureInfo.InvariantCulture));
            }
            else if (action == "data0")
            {
                SubscriberPut(fields, 0, 0);
            }
            else if (action == "dataB")
            {
                SubscriberPut(fields, 0, 2);
            }
            else if (action == "endffffffff")
            {
                SubscriberPut(fields, 8, 0xffffffffu);
            }
            else
            {
                throw new InvalidOperationException("Subscriber 검사 변형 미해석");
            }
        }

        /// <summary>실제 물리 저장소·Config·새 ID 장치만 연결한다.</summary>
        public FortSubscriberLoadHooks Hooks() => new()
        {
            Manager = slot =>
            {
                Assert.Equal(unchecked((int)Input[1]), slot);
                return Manager;
            },
            Select = (current, name, writable) =>
            {
                Record("S:" + current.ToString(CultureInfo.InvariantCulture) + ':' + name + ':' + writable.ToString(CultureInfo.InvariantCulture));
                Mutate("select1");
                return Selected + 1;
            },
            Access = (token, operation) =>
            {
                if (token == 0 || token > Streams.Length)
                {
                    throw new ArgumentOutOfRangeException(nameof(token), token, "Subscriber 검사 stream 물리 범위");
                }
                MemStreamHooks memory = new()
                {
                    Buffer = pointer =>
                    {
                        if (pointer == 0 || pointer > Buffers.Length)
                        {
                            throw new ArgumentOutOfRangeException(nameof(pointer), pointer, "Subscriber 검사 data 물리 범위");
                        }
                        return Buffers[pointer - 1];
                    },
                };
                operation(Streams[token - 1], memory);
            },
            Assertion = line =>
            {
                Record("A:" + ((uint)line).ToString(CultureInfo.InvariantCulture));
                Mutate("assert" + (++Assertions).ToString(CultureInfo.InvariantCulture));
            },
            ReadRegistered = (string key, ref uint target) =>
            {
                Record("R:" + key);
                target = Input[10];
                Mutate("read1");
                return Input[9] != 0;
            },
            Generate = () =>
            {
                Record("G");
                Mutate("generate1");
                return Input[11];
            },
            Configure = expression =>
            {
                Record("C:" + expression);
                Mutate("config1");
            },
        };

        /// <summary>두 현재 DWORD를 제품과 같은 실체로 빌린다.</summary>
        public FortSubscriberLoadGlobals Globals() => new()
        {
            GetSubscriber = () => Subscriber,
            SetSubscriber = value => Subscriber = value,
        };
    }

    /// <summary>도달한 호스트 선택 장치·물리 범위 예외만 확인한다.</summary>
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
    /// 최초 PE 694행의 정상·중단·부분 실패와 모든 현재 물리 상태를 대조한다.
    /// </summary>
    [Fact]
    public void IndependentNativeBodyAndRegistration()
    {
        string[][] rows = X86Fixture.Read("fort-subscriber-load");
        int compared = 0;
        // 앞 두 열의 입력만 장치에 주고 나머지는 제품 실행 뒤 비교한다
        foreach (string[] row in rows)
        {
            Assert.Equal(7, row.Length);
            SubscriberDevices devices = new(SubscriberFields(row[1], ','));
            FortSubscriberLoadHooks hooks = devices.Hooks();
            string status = "normal";
            try
            {
                if (devices.Input[0] != 0)
                {
                    RawFortSubscriberLoad.FinishFortSubscriberRegistration(devices.Globals(), hooks);
                }
                else
                {
                    RawFortSubscriberLoad.LoadFortSubscriber(unchecked((int)devices.Input[1]), devices.Output, devices.Globals(), hooks);
                }
            }
            catch (SubscriberStop)
            {
                status = "stop";
            }
            catch (ArgumentOutOfRangeException)
            {
                status = "fault";
            }
            Assert.Equal(row[2], status);
            string[] state = SubscriberFields(row[3], ',');
            Assert.Equal(2, state.Length);
            Assert.Equal(uint.Parse(state[0], CultureInfo.InvariantCulture), devices.Subscriber);
            Assert.Equal(uint.Parse(state[1], CultureInfo.InvariantCulture), devices.Manager);
            string[] raw = SubscriberFields(row[4], ',');
            Assert.Equal(8, raw.Length);
            for (int node = 0; node < 2; node++)
            {
                for (int field = 0; field < 4; field++)
                {
                    Assert.Equal(uint.Parse(raw[node * 4 + field], CultureInfo.InvariantCulture), SubscriberU(devices.Streams[node], field * 4)); // 양쪽 현재 stream을 비교한다
                }
            }
            Assert.Equal(row[5], SubscriberHex(devices.Output));
            Assert.Equal(row[6], devices.Trace.Length == 0 ? "-" : devices.Trace);
            compared++;
        }
        Assert.Equal(694, compared);
    }

    /// <summary>
    /// 현재 ID 관문의 미선택 장치·부족한 실제 handle 부분 쓰기·선택 장치 누락 효과를 구별한다.
    /// </summary>
    [Fact]
    public void SelectedDevicesAndPhysicalPrefix()
    {
        uint known = 1;
        RawFortSubscriberLoad.FinishFortSubscriberRegistration(new FortSubscriberLoadGlobals { GetSubscriber = () => known, SetSubscriber = value => known = value }, new FortSubscriberLoadHooks());
        Assert.Equal(1u, known); // 이미 등록된 ID는 Config·생성 장치가 없어도 정상이다
        SubscriberDevices devices = new(SubscriberFields("0,1,11,0,1,512,0,1,127,0,77,88,none", ','));
        FortSubscriberLoadHooks hooks = devices.Hooks();
        byte[] shortHandle = new byte[64];
        Array.Fill(shortHandle, (byte)0xa5);
        Assert.True(Rejects(() => RawFortSubscriberLoad.LoadFortSubscriber(1, shortHandle, devices.Globals(), hooks)));
        Assert.Equal(1u, devices.Subscriber);
        Assert.Equal(6u, SubscriberU(devices.Streams[0], 4));
        Assert.Equal(devices.Buffers[0].AsSpan(6, 64).ToArray(), shortHandle); // 뒤 실패가 앞선 64BYTE를 되돌리지 않는다
        SubscriberDevices missing = new(SubscriberFields("0,1,11,0,1,512,0,4294967295,3,0,77,88,none", ','));
        FortSubscriberLoadHooks selected = missing.Hooks();
        selected.Generate = null;
        Assert.True(Rejects(() => RawFortSubscriberLoad.LoadFortSubscriber(1, missing.Output, missing.Globals(), selected)));
        Assert.Equal(77u, missing.Subscriber);
        Assert.Equal(9u, SubscriberU(missing.Streams[0], 4));
        Assert.Equal(0, missing.Output[3]); // 파일 이름 복사가 끝난 뒤 필수 새 ID 장치 위치에서만 거부한다
    }

    /// <summary>
    /// 현재 pos DWORD는 4바이트 전체를 읽는다. 상위 바이트가 있으면 그대로 써서 물리 범위를 벗어난다.
    /// </summary>
    [Fact]
    public void PositionReadsFullDword()
    {
        byte[] fields = new byte[16];
        SubscriberPut(fields, 0, 1);
        SubscriberPut(fields, 4, 0x10001u);
        SubscriberPut(fields, 8, 0x20000u);
        SubscriberPut(fields, 12, 2048);
        byte[] buffer = new byte[2048];
        buffer[1] = 0x22;
        buffer[2] = 0x22;
        buffer[3] = 0x22;
        buffer[4] = 0x22;
        buffer[5] = 3;
        buffer[6] = 0;
        buffer[7] = (byte)'a';
        buffer[8] = (byte)'b';
        buffer[9] = (byte)'c';
        FortSubscriberLoadHooks hooks = new()
        {
            Manager = _ => 11,
            Select = (_, _, _) => 1,
            Access = (token, operation) =>
            {
                Assert.Equal(1u, token);
                operation(fields, new MemStreamHooks { Buffer = _ => buffer });
            },
            Assertion = _ => throw new InvalidOperationException("예상 밖 진단"),
            ReadRegistered = null,
            Generate = null,
            Configure = null,
        };
        FortSubscriberLoadGlobals globals = new()
        {
            GetSubscriber = () => 0,
            SetSubscriber = _ => { },
        };
        byte[] output = new byte[512];
        // 원본은 pos 0x10001에서 ID를 읽다 물리 범위를 벗어나고, 하위 2바이트만 읽으면 끝까지 진행한다
        Assert.True(Rejects(() => RawFortSubscriberLoad.LoadFortSubscriber(1, output, globals, hooks)));
        Assert.Equal(0x10001u, SubscriberU(fields, 4));
    }
}
