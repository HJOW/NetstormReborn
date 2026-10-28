using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 요새 저장용 SP writer(RawFortSpWrite)를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/FortSpWriteTests.cpp 와 같다: 명시 stream 16바이트·버퍼·난수로 Write 한 번을 실행한다.
/// 행마다 입력 비트·시드·위치·끝·용량·무늬·버퍼 방식·변경으로 장치를 세우고 상태·난수·호출 수·세 버퍼 전체·자취를 대조한다.
/// 기준 판본은 10.78 이다 (R0-4 신규 이식, 2026-10-28).
/// </summary>
public sealed class X86RawFortSpWriteTests
{
    /// <summary>원본과 같은 물리를 나타내는 불투명 버퍼 식별자 3개.</summary>
    private static readonly uint[] SpBuffers = [0x12001000u, 0x12002000u, 0x12003000u];

    /// <summary>원본 테스트와 같은 명시 중단 표시. operator_new 정상 반환 전 중단을 흉내낸 검사 입력이며 게임 규칙과 무관하다.</summary>
    private sealed class SpStopException : Exception
    {
    }

    /// <summary>
    /// 실제 stream 16바이트와 버퍼·난수를 소유하며 원본 출력 기대가 아닌 입력·외부 효과만 구성한다.
    /// cpppj/tests/FortSpWriteTests.cpp 의 SpDevice와 같은 자리다.
    /// </summary>
    private sealed class SpDevice
    {
        /// <summary>세 배열 앞 16바이트가 바로 같은 실제 stream인 256바이트 버퍼 3개.</summary>
        private readonly byte[][] _buffers = [new byte[256], new byte[256], new byte[256]];

        /// <summary>검사 쪽에서 읽는 Native 실제 4558c0 대응 공유 난수.</summary>
        private readonly NetstormRandom _random = new(0);

        /// <summary>원시 float 입력 비트.</summary>
        private uint _bits;

        /// <summary>실제 난수 호출 관측 수.</summary>
        private uint _randoms;

        /// <summary>실제 진단 호출 관측 수.</summary>
        private uint _assertions;

        /// <summary>현재 비트와 같은 난수가 나온 횟수.</summary>
        private uint _equal;

        /// <summary>명시 하위 변경·중단 입력.</summary>
        private string _mutation = string.Empty;

        /// <summary>실제 경계 순서 자취.</summary>
        private string _trace = string.Empty;

        /// <summary>난수 호출 수.</summary>
        public uint RandomCalls => _randoms;

        /// <summary>진단 호출 수.</summary>
        public uint AssertionCalls => _assertions;

        /// <summary>현재 비트와 같은 난수 횟수.</summary>
        public uint EqualDraws => _equal;

        /// <summary>실제 경계 순서 자취.</summary>
        public string Trace => _trace;

        /// <summary>공유 난수(진단용).</summary>
        public NetstormRandom Random => _random;

        /// <summary>
        /// 기록의 원시 입력만 해석하고 Native 출력을 역산하지 않는다. 입력 칸은 8개다.
        /// </summary>
        /// <param name="input">비트·시드·위치·끝·용량·무늬·버퍼 방식·변경.</param>
        public SpDevice(string[] input)
        {
            if (input.Length != 8)
            {
                throw new InvalidOperationException("SP writer 입력 칸 수");
            }
            _bits = X86Fixture.UInt(input[0]);
            _random.SetState(X86Fixture.UInt(input[1]));
            _mutation = input[7];
            uint pattern = X86Fixture.UInt(input[5]);
            // 원시 물리 전체를 먼저 명시 초기화한다
            for (int index = 0; index < _buffers.Length; index++)
            {
                for (int b = 0; b < 256; b++)
                {
                    _buffers[index][b] = (byte)(pattern + (uint)index * 37 + (uint)b * 11);
                }
            }
            SpPut(Stream(), 0, SpBuffers[int.Parse(input[6])]);
            for (int field = 1; field < 4; field++)
            {
                SpPut(Stream(), field * 4, X86Fixture.UInt(input[field + 1]));
            }
        }

        /// <summary>원시 float 입력 비트.</summary>
        public float Bits => BitConverter.UInt32BitsToSingle(_bits);

        /// <summary>셋째 출력 물리의 같은 첫 16바이트를 빌린다. 별도 헤더 사본은 두지 않는다.</summary>
        public Span<byte> Stream() => _buffers[2].AsSpan(0, 16);

        /// <summary>세 버퍼 전체(진단·체크섬용).</summary>
        public byte[][] Buffers => _buffers;

        /// <summary>
        /// 외부 하위의 현재 필드 변경만 제공하며 부모 분기나 원문 기대는 계산하지 않는다.
        /// </summary>
        /// <param name="boundary">이번 선택 하위의 경계 이름.</param>
        public void Mutate(string boundary)
        {
            int split = _mutation.IndexOf('-');
            if (split < 0 || _mutation.Substring(0, split) != boundary)
            {
                return;
            }
            string action = _mutation.Substring(split + 1);
            // 이번 선택 하위의 명시 효과다
            switch (action)
            {
                case "stop":
                    throw new SpStopException();
                case "seed0":
                    _random.SetState(0);
                    break;
                case "baseA":
                case "baseB":
                    SpPut(Stream(), 0, SpBuffers[action == "baseB" ? 1 : 0]);
                    SpPut(Stream(), 4, 5);
                    SpPut(Stream(), 8, 1);
                    SpPut(Stream(), 12, 128);
                    break;
                case "stream":
                    SpPut(Stream(), 0, SpBuffers[2]);
                    SpPut(Stream(), 4, 8);
                    break;
                case "position240":
                    SpPut(Stream(), 4, 240);
                    SpPut(Stream(), 12, 256);
                    break;
                case "capacity0":
                    SpPut(Stream(), 12, 0);
                    break;
                case "endHigh":
                    SpPut(Stream(), 8, 0x80000000u);
                    break;
                case "position17":
                    SpPut(Stream(), 4, 17);
                    SpPut(Stream(), 12, 256);
                    break;
                default:
                    throw new InvalidOperationException("SP writer 미해석 입력 변경");
            }
        }

        /// <summary>같은 게임 난수와 현재 물리·선택 진단을 실제 새 부모에 연결한다.</summary>
        public FortSpWriteHooks Hooks()
        {
            var hooks = new FortSpWriteHooks();
            hooks.NextRandom = limit =>
            {
                Assert.Equal(32u, limit);
                uint value = (uint)_random.Next((int)limit);
                _randoms++;
                if (value == (_randoms - 1) / 16)
                {
                    _equal++;
                }
                if (_trace.Length != 0)
                {
                    _trace += ';';
                }
                _trace += $"R:{_randoms}:{value}:{_random.State}";
                Mutate("r" + _randoms);
                return value;
            };
            hooks.Stream.Buffer = token =>
            {
                for (int index = 0; index < SpBuffers.Length; index++)
                {
                    if (token == SpBuffers[index])
                    {
                        return _buffers[index];
                    }
                }
                throw new ArgumentOutOfRangeException(nameof(token), token, "SP writer 검사 미등록 버퍼");
            };
            hooks.Stream.Assertion = kind =>
            {
                Assert.Equal(MemStreamAssertion.WriteCapacity, kind);
                _assertions++;
                if (_trace.Length != 0)
                {
                    _trace += ';';
                }
                _trace += "A:36";
                Mutate("a" + _assertions);
            };
            return hooks;
        }
    }

    /// <summary>명시 초기 입력·하위 콜백의 같은 DWORD만 리틀 엔디언으로 쓴다.</summary>
    /// <param name="bytes">쓸 물리.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void SpPut(Span<byte> bytes, int offset, uint value)
    {
        // 부모 출력 기대를 생성하지 않는다
        for (int b = 0; b < 4; b++)
        {
            bytes[offset + b] = (byte)(value >> (b * 8));
        }
    }

    /// <summary>실제 현재 출력 폭을 검사 관측용 DWORD로 읽는다.</summary>
    /// <param name="bytes">읽을 물리.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽을 폭. 생략하면 DWORD 4다.</param>
    private static uint SpRead(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        uint value = 0;
        // 관측 폭 밖의 바이트는 보지 않는다
        for (int b = 0; b < width; b++)
        {
            value |= (uint)bytes[offset + b] << (b * 8);
        }
        return value;
    }

    /// <summary>미작성 인접 바이트도 포함하는 전체 물리 출력이다.</summary>
    /// <param name="bytes">바꿀 물리 전체.</param>
    private static string SpHex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        var text = new System.Text.StringBuilder(bytes.Length * 2);
        // NUL이나 미사용 구간을 버리지 않는다
        foreach (byte value in bytes)
        {
            text.Append(digits[value >> 4]);
            text.Append(digits[value & 15]);
        }
        return text.ToString();
    }

    /// <summary>
    /// Native 178 정상·20 부분중단과 실제 _ftol·RNG·같은 stream 자체 alias를 모두 대조한다.
    /// </summary>
    [Fact]
    public void FortSpWriteReplaysIndependentNative198()
    {
        string[][] rows = X86Fixture.Read("fort-sp-write");
        int count = 0;
        int normal = 0;
        int stops = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(13, row.Length);
            string[] fields = row[1].Split(',');
            var device = new SpDevice(fields);
            string status = "normal";
            // 부분 중단을 정상 반환에 합치지 않는다
            try
            {
                RawFortSpWrite.Write(device.Stream(), device.Bits, device.Hooks());
            }
            catch (SpStopException)
            {
                status = "stop";
            }
            Assert.Equal(row[2], status);
            Assert.Equal(X86Fixture.UInt(row[4]), device.Random.State);
            Assert.Equal(X86Fixture.UInt(row[5]), device.RandomCalls);
            Assert.Equal(X86Fixture.UInt(row[6]), device.AssertionCalls);
            Assert.Equal(X86Fixture.UInt(row[7]), device.EqualDraws);
            Assert.Equal(row[8], SpHex(device.Buffers[0]));
            Assert.Equal(row[9], SpHex(device.Buffers[1]));
            Assert.Equal(row[10], SpHex(device.Buffers[2]));
            // 내부 변환은 출력으로 검증하며 가짜 C# 변환 hook를 만들지 않는다
            string conversion = "F:" + row[3] + ';';
            Assert.StartsWith(conversion, row[11]);
            Assert.Equal(device.Trace, row[11].Substring(conversion.Length));
            if (status == "normal" && fields[6] == "0" && fields[7] == "none")
            {
                uint decoded = 0;
                // 보호 비트를 실제 88바이트 출력에서만 모아 원문 EAX와 대조한다
                int offset = int.Parse(fields[2]);
                for (uint bit = 0; bit < 32; bit++)
                {
                    int width = bit < 8 ? 1 : bit < 16 ? 2 : 4;
                    decoded |= SpRead(device.Buffers[0], offset, width) & (1u << (int)bit);
                    offset += width;
                }
                Assert.Equal(X86Fixture.UInt(row[3]), decoded);
            }
            count++;
            if (status == "normal")
            {
                normal++;
                Assert.Equal(512u, device.RandomCalls);
            }
            else
            {
                stops++;
            }
            return true;
        });
        Assert.Equal(198, count);
        Assert.Equal(178, normal);
        Assert.Equal(20, stops);
    }

    /// <summary>
    /// 해당 비트만 반환하는 명시 난수 입력으로 잡음 생략·8바이트·8워드·16더블워드·총 88바이트를 독립 확인한다.
    /// </summary>
    [Fact]
    public void FortSpWriteEqualNoiseKeepsOwnBitAndExactWidths()
    {
        byte[] stream = new byte[16];
        byte[] payload = new byte[96];
        Array.Fill(payload, (byte)0xa5);
        SpPut(stream, 0, 1);
        SpPut(stream, 12, 88);
        uint calls = 0;
        var hooks = new FortSpWriteHooks();
        // 512회 모두 현재 비트와 같아 OR만 생략해야 한다
        hooks.NextRandom = limit =>
        {
            Assert.Equal(32u, limit);
            return calls++ / 16;
        };
        hooks.Stream.Buffer = token =>
        {
            Assert.Equal(1u, token);
            return payload;
        };
        RawFortSpWrite.Write(stream, 65535.0f, hooks);
        Assert.Equal(512u, calls);
        Assert.Equal(88u, SpRead(stream, 4));
        Assert.Equal(88u, SpRead(stream, 8));
        int offset = 0;
        // 각 인라인 쓰기의 실제 연속 위치다
        for (uint bit = 0; bit < 32; bit++)
        {
            int width = bit < 8 ? 1 : bit < 16 ? 2 : 4;
            Assert.Equal(bit < 16 ? 1u << (int)bit : 0u, SpRead(payload, offset, width));
            offset += width;
        }
        // 88바이트 밖은 쓰지 않는다
        for (int b = 88; b < payload.Length; b++)
        {
            Assert.Equal(0xa5, payload[b]);
        }
    }

    /// <summary>
    /// 선택한 버퍼·진단이 없으면 첫 16 난수 효과는 남고 할당·일반 writer로 우회하지 않는다.
    /// </summary>
    [Fact]
    public void FortSpWriteMissingSelectedDevicesPreserveRandomPrefix()
    {
        foreach (bool missingAssertion in new[] { false, true })
        {
            // 자연 정상 용량과 실제 선택 진단의 누락을 구별한다
            var device = new SpDevice((missingAssertion ? "1199570688,55,3,9,0,165,0,none" : "1199570688,55,3,9,128,165,0,none").Split(','));
            byte[][] before = device.Buffers.Select(buffer => buffer.ToArray()).ToArray();
            FortSpWriteHooks hooks = device.Hooks();
            if (missingAssertion)
            {
                hooks.Stream.Assertion = null;
            }
            else
            {
                hooks.Stream.Buffer = null;
            }
            bool rejected = false;
            try
            {
                RawFortSpWrite.Write(device.Stream(), 65535.0f, hooks);
            }
            catch (InvalidOperationException)
            {
                rejected = true;
            }
            Assert.True(rejected);
            Assert.Equal(16u, device.RandomCalls);
            for (int index = 0; index < before.Length; index++)
            {
                Assert.Equal(before[index], device.Buffers[index]);
            }
        }
        var seeded = new SpDevice("1199570688,55,3,9,128,165,0,none".Split(','));
        byte[][] seededBefore = seeded.Buffers.Select(buffer => buffer.ToArray()).ToArray();
        FortSpWriteHooks seededHooks = seeded.Hooks();
        seededHooks.NextRandom = null;
        bool seededRejected = false;
        try
        {
            RawFortSpWrite.Write(seeded.Stream(), 65535.0f, seededHooks);
        }
        catch (InvalidOperationException)
        {
            seededRejected = true;
        }
        Assert.True(seededRejected);
        Assert.Equal(0u, seeded.RandomCalls);
        for (int index = 0; index < seededBefore.Length; index++)
        {
            Assert.Equal(seededBefore[index], seeded.Buffers[index]);
        }
    }

    /// <summary>
    /// 버퍼 물리가 짧으면 이미 쓴 앞 두 바이트와 48회 난수를 유지하고 그 뒤 위치·끝을 자동 복구하지 않는다.
    /// </summary>
    [Fact]
    public void FortSpWritePhysicalFailurePreservesPriorWrites()
    {
        byte[] stream = new byte[16];
        byte[] payload = [0xa5, 0xa5];
        SpPut(stream, 0, 1);
        SpPut(stream, 12, 88);
        uint calls = 0;
        var hooks = new FortSpWriteHooks();
        // 외부 난수 효과는 실제 선택 횟수만 관측한다
        hooks.NextRandom = limit =>
        {
            Assert.Equal(32u, limit);
            return calls++ / 16;
        };
        hooks.Stream.Buffer = _ => payload;
        bool rejected = false;
        try
        {
            RawFortSpWrite.Write(stream, 1.0f, hooks);
        }
        catch (ArgumentOutOfRangeException)
        {
            rejected = true;
        }
        Assert.True(rejected);
        Assert.Equal(48u, calls);
        Assert.Equal(1, payload[0]);
        Assert.Equal(0, payload[1]);
        Assert.Equal(2u, SpRead(stream, 4));
        Assert.Equal(2u, SpRead(stream, 8));
    }
}
