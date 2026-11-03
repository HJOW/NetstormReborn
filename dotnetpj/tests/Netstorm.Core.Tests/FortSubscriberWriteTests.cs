using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort Subscriber 저장(<see cref="RawFortSubscriberWrite"/>)을 독립 원본 입력과 대조한다.
/// cpppj <c>FortSubscriberWriteTests</c>의 첫 번째 TEST_CASE(90행 전수 재생)만 옮겼다. (F2c-6, 2026-11-03)
/// 두 번째 TEST_CASE(실제 저장소)는 Client 저장소를 쓰므로 Core가 아니라
/// <c>Netstorm.Client.Tests.FortSubscriberWriteStorageTests</c>에서 잇는다.
/// </summary>
public sealed class FortSubscriberWriteTests
{
    /// <summary>원본 관측과 같은 두 물리 버퍼 토큰이다. 호스트 주소가 아니다.</summary>
    private static readonly uint[] SubWriteBuffers = [0x12040000u, 0x12060000u];

    /// <summary>전체 output·이름의 명시 물리 용량이다.</summary>
    private const int SubWritePhysical = 0x11000;

    /// <summary>독립 TSV의 쉼표 입력 13칸을 그대로 분리한다. 마지막 빈 칸도 유지한다.</summary>
    /// <param name="source">분리할 원문이다.</param>
    private static string[] SubWriteFields(string source)
    {
        List<string> result = [];
        int start = 0;
        // 구분자가 끝날 때까지 차례대로 나눈다
        for (; ; )
        {
            int at = source.IndexOf(',', start);
            result.Add(at < 0 ? source[start..] : source.Substring(start, at - start));
            if (at < 0)
            {
                return [.. result];
            }
            start = at + 1;
        }
    }

    /// <summary>같은 16바이트 stream의 지정 DWORD만 읽는다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    private static uint SubWriteGet(ReadOnlySpan<byte> bytes, int at)
    {
        uint value = 0;
        // 낮은 바이트부터 원문 순서로 조립한다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>같은 16바이트 stream의 지정 DWORD만 쓴다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void SubWritePut(Span<byte> bytes, int at, uint value)
    {
        // 주변 필드·바이트는 유지한다
        for (int index = 0; index < 4; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>모든 물리 바이트를 독립 FNV1a 64비트로 계산해 원문 전체 출력 관측과 대조한다.</summary>
    /// <param name="bytes">해시할 바이트다.</param>
    private static string SubWriteHash(ReadOnlySpan<byte> bytes)
    {
        ulong value = 0xcbf29ce484222325ul;
        // unsigned64 곱셈의 감김까지 같은 관측 형식이다
        foreach (byte item in bytes)
        {
            value = (value ^ item) * 0x100000001b3ul;
        }
        const string digits = "0123456789abcdef";
        StringBuilder result = new(16, 16);
        // 상위 nibble부터 고정 16글자로 표시한다
        for (int index = 0; index < 16; index++)
        {
            result.Append(digits[(int)((value >> ((15 - index) * 4)) & 15)]);
        }
        return result.ToString();
    }

    /// <summary>출력 64바이트 prefix를 NUL에서 자르지 않고 원시 hex로 대조한다.</summary>
    /// <param name="bytes">표시할 바이트다.</param>
    private static string SubWriteHex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        StringBuilder result = new();
        // 실제 순서의 각 바이트만 표시한다
        foreach (byte value in bytes)
        {
            result.Append(digits[value >> 4]);
            result.Append(digits[value & 15]);
        }
        return result.ToString();
    }

    /// <summary>명시 callback·잘못된 manager 경계를 원문의 정상 반환과 구별한다.</summary>
    internal sealed class SubWriteStop : Exception
    {
        /// <summary>fixture가 관측한 도달 상태만 담는다.</summary>
        public string Status { get; }

        /// <summary>중단 상태를 보관한다.</summary>
        /// <param name="status">도달 상태다.</param>
        public SubWriteStop(string status)
        {
            Status = status;
        }
    }

    /// <summary>원문과 같은 외부 입력·현재 장치만 준비하고 기대 관측은 보관하지 않는다.</summary>
    private sealed class SubWriteScene
    {
        /// <summary>fixture의 입력 12칸이다.</summary>
        public uint[] Input = new uint[12];

        /// <summary>지정 외부 효과 이름이다.</summary>
        public string Mutation = "";

        /// <summary>실제 순서의 외부 경계 trace다.</summary>
        public string Trace = "";

        /// <summary>실제 helper가 바꾸는 현재 manager다.</summary>
        public uint Manager;

        /// <summary>실제 등록 helper가 바꾸는 현재 Subscriber ID다.</summary>
        public uint Subscriber;

        /// <summary>현재 이름 포인터 선택 번호다.</summary>
        public uint CurrentHandle;

        /// <summary>선택된 stream의 write 순번이다.</summary>
        public uint Writes;

        /// <summary>캡처 포인터·대체 포인터의 서로 다른 이름 저장소다.</summary>
        public byte[][] Names = [new byte[SubWritePhysical], new byte[SubWritePhysical]];

        /// <summary>두 토큰이 구별되는 물리 output이다.</summary>
        public byte[][] Output = [new byte[SubWritePhysical], new byte[SubWritePhysical]];

        /// <summary>실제 두 stream의 16바이트 필드다.</summary>
        public byte[][] Stream = [new byte[16], new byte[16]];

        /// <summary>원문과 같은 초기 물리 guard·stream·이름을 명시 입력으로 둔다.</summary>
        /// <param name="text">쉼표 13칸 입력이다.</param>
        public SubWriteScene(string text)
        {
            string[] fields = SubWriteFields(text);
            if (fields.Length != 13)
            {
                throw new ArgumentException("Subscriber 저장 입력 열 수 오류", nameof(text));
            }
            for (int index = 0; index < Input.Length; index++)
            {
                Input[index] = uint.Parse(fields[index], CultureInfo.InvariantCulture);
            }
            Mutation = fields[12];
            Manager = Input[1];
            Subscriber = Input[3];
            for (int index = 0; index < 2; index++)
            {
                // 원문과 같은 초기 물리 guard·stream을 명시 입력한다
                Array.Fill(Names[index], (byte)0xcc);
                Array.Fill(Output[index], (byte)0xcc);
                SubWritePut(Stream[index], 0, SubWriteBuffers[index]);
                SubWritePut(Stream[index], 4, Input[9]);
                SubWritePut(Stream[index], 8, Input[10]);
                SubWritePut(Stream[index], 12, Input[11]);
            }
            Encoding.Latin1.GetBytes("Other\0").CopyTo(Names[1], 0);
            if (Input[4] >= 4)
            {
                // 길이 WORD 감김의 실제 전체 입력을 유지한다
                uint length = Input[4] == 4 ? 255u : Input[4] == 5 ? 256u : 65536u;
                for (uint index = 0; index < length; index++)
                {
                    Names[0][index] = (byte)(65 + index % 23);
                }
                Names[0][length] = 0;
            }
            else if (Input[4] == 2)
            {
                Names[0][0] = 0;
            }
            else if (Input[4] == 3)
            {
                new byte[] { (byte)'A', 0, (byte)'B', (byte)'C', 0 }.CopyTo(Names[0], 0);
            }
            else
            {
                Encoding.Latin1.GetBytes("Alpha\0").CopyTo(Names[0], 0);
            }
        }

        /// <summary>실제 경계의 외부 응답·내용만 바꾸고 부모의 조건·재조회는 계산하지 않는다.</summary>
        /// <param name="boundary">효과 경계 이름이다.</param>
        /// <param name="text">기록할 본문이다.</param>
        /// <exception cref="SubWriteStop">명시 중단·잘못된 manager 경계다.</exception>
        public void Event(string boundary, string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += text;
            int dash = Mutation.IndexOf('-');
            if (dash < 0 || Mutation[..dash] != boundary)
            {
                return;
            }
            string action = Mutation[(dash + 1)..];
            if (action == "stop")
            {
                throw new SubWriteStop("stop:" + boundary);
            }
            else if (action == "manager22")
            {
                Manager = 22;
            }
            else if (action == "manager0")
            {
                Manager = 0;
            }
            else if (action == "id9")
            {
                Subscriber = 9;
            }
            else if (action == "pointer2")
            {
                CurrentHandle = 1;
            }
            else if (action == "name")
            {
                Names[0][0] = (byte)'Z';
                Names[0][1] = 0;
                Names[0][2] = (byte)'Y';
            }
            else if (action == "grow")
            {
                Encoding.Latin1.GetBytes("Grown Word\0").CopyTo(Names[0], 0);
            }
            else if (action == "streamB")
            {
                byte[] selected = Stream[(int)Input[8]];
                SubWritePut(selected, 0, SubWriteBuffers[1]);
                SubWritePut(selected, 4, 4);
                SubWritePut(selected, 8, 2);
                SubWritePut(selected, 12, (uint)SubWritePhysical);
            }
            else
            {
                throw new InvalidOperationException("Subscriber 저장 미해석 외부 변경");
            }
        }

        /// <summary>실제 일반 MemStream 몸체에 물리 버퍼만 제공한다.</summary>
        public MemStreamHooks Memory()
        {
            MemStreamHooks hooks = new();
            hooks.Buffer = pointer =>
            {
                for (int index = 0; index < SubWriteBuffers.Length; index++)
                {
                    if (pointer == SubWriteBuffers[index])
                    {
                        return Output[index];
                    }
                }
                throw new ArgumentOutOfRangeException(nameof(pointer), pointer, "Subscriber 저장 물리 output 토큰");
            };
            hooks.Assertion = value =>
            {
                if (value != MemStreamAssertion.WriteCapacity)
                {
                    throw new InvalidOperationException("Subscriber 저장 예상 밖 할당 진단");
                }
                Event("capacity", "A:36");
            };
            return hooks;
        }

        /// <summary>실제 등록 helper·문자열·일반 writer를 연결하고 Create·Config만 명시 장치로 제공한다.</summary>
        public FortSubscriberWriteHooks Hooks()
        {
            return new FortSubscriberWriteHooks
            {
                Manager = slot =>
                {
                    Assert.Equal(Input[0], slot);
                    return Manager;
                },
                Assertion = line => Event(line == 0x1cfu ? "assert" : "null", "A:" + line.ToString(CultureInfo.InvariantCulture)),
                RegisterSubscriber = () =>
                {
                    Event("register", "H");
                    FortSubscriberLoadGlobals globals = new()
                    {
                        GetSubscriber = () => Subscriber,
                        SetSubscriber = value => Subscriber = value,
                    };
                    FortSubscriberLoadHooks registration = new()
                    {
                        ReadRegistered = (string key, ref uint target) =>
                        {
                            target = Input[6];
                            Event("read", "R:" + key);
                            return Input[5] != 0;
                        },
                        Generate = () =>
                        {
                            Event("generate", "G");
                            return Input[7];
                        },
                        Configure = value => Event("config", "F:" + value),
                    };
                    RawFortSubscriberLoad.FinishFortSubscriberRegistration(globals, registration);
                },
                Create = (selected, section) =>
                {
                    Assert.Equal("Subscriber", section);
                    Event("create", "C:" + selected.ToString(CultureInfo.InvariantCulture));
                    if (selected == 0)
                    {
                        throw new SubWriteStop("fault:manager0");
                    }
                    return Input[8] + 1;
                },
                Write = (selected, bytes) =>
                {
                    Writes++;
                    Event("write" + Writes.ToString(CultureInfo.InvariantCulture), "W:" + Writes.ToString(CultureInfo.InvariantCulture) + ':' + bytes.Length.ToString(CultureInfo.InvariantCulture) + ':' + SubWriteHash(bytes));
                    RawMemStream.Write(Stream[(int)selected - 1], bytes, (uint)bytes.Length, Memory());
                },
            };
        }

        /// <summary>실제 두 stream의 필드를 포인터만 fixture 번호로 바꾸어 관측한다.</summary>
        public string Streams()
        {
            List<string> parts = [];
            // 다른 stream이 바뀌지 않았는지도 같은 8DWORD에 포함한다
            foreach (byte[] selected in Stream)
            {
                uint pointer = SubWriteGet(selected, 0);
                for (int index = 0; index < 4; index++)
                {
                    uint field = index == 0 ? (pointer == SubWriteBuffers[0] ? 1u : pointer == SubWriteBuffers[1] ? 2u : 0u) : SubWriteGet(selected, index * 4);
                    parts.Add(field.ToString(CultureInfo.InvariantCulture));
                }
            }
            return string.Join(',', parts);
        }
    }

    /// <summary>
    /// 부모·등록·기존 문자열·실제 MemStream의 상태와 전체 물리 output을 최초 Native 관측 90행에 대조한다.
    /// </summary>
    [Fact]
    public void ReplaysNative90CapturedIdAndDelayedNameWrites()
    {
        string[][] rows = X86Fixture.Read("fort-subscriber-write");
        int compared = 0;
        // 입력 12칸+mutation만 장치에 쓰고 뒤 관측 6칸은 제품 실행 뒤에 비교한다
        foreach (string[] row in rows)
        {
            Assert.Equal(9, row.Length);
            SubWriteScene scene = new(row[2]);
            string status = "normal";
            try
            {
                RawFortSubscriberWrite.SaveFortSubscriber(scene.Input[0], scene.Input[2], scene.Input[4] == 1 ? null : scene.Names[0], scene.Hooks());
            }
            catch (SubWriteStop stopped)
            {
                status = stopped.Status;
            }
            Assert.Equal(row[3], status);
            Assert.Equal(row[4], scene.Subscriber.ToString(CultureInfo.InvariantCulture) + ',' + scene.Manager.ToString(CultureInfo.InvariantCulture) + ',' + scene.CurrentHandle.ToString(CultureInfo.InvariantCulture));
            Assert.Equal(row[5], scene.Streams());
            Assert.Equal(row[6], SubWriteHash(scene.Output[0]) + ':' + SubWriteHash(scene.Output[1]));
            Assert.Equal(row[7], SubWriteHex(scene.Output[0].AsSpan(0, 64)) + ':' + SubWriteHex(scene.Output[1].AsSpan(0, 64)));
            Assert.Equal(row[8], scene.Trace);
            compared++;
        }
        Assert.Equal(90, compared);
    }
}
