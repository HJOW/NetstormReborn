using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 타입 변경(RawSquidFactory.Retype)과 비석·주문 전환(RawObjectConversion)을 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/ObjectConversionTests.cpp 와 같다: 같은 원본 슬롯·타입·생성자 몸체를 조립해 Retype 128행과 전환 명령 36행을 재생한다.
/// 슬롯 50바이트 전체와 진단·공간·부모 순서, 풀 카운터를 대조한다. 기준 판본은 10.78 이다 (R0-4 재대조, 2026-10-28).
/// </summary>
public sealed class X86RawObjectConversionTests
{
    /// <summary>원문의 실제 this에 해당하는 풀 슬롯 번호(50바이트 물리 저장소).</summary>
    private const int ConversionSid = 50;

    /// <summary>주문 Unpop 뒤 읽는 현재 비석 타입.</summary>
    private const uint BuriedType = 112;

    /// <summary>신규 타입의 명시 미등록 생성자 주소. Retype이 타입 바이트를 쓴 뒤 생성자 조회에서 거부한다.</summary>
    private const uint UnregisteredConstructor = 0xffffffffu;

    /// <summary>
    /// 큰 게임 장치 없이 같은 원본 슬롯·타입·생성자 몸체를 조립한다.
    /// cpppj/tests/ObjectConversionTests.cpp 의 ConversionScene과 같은 자리다.
    /// </summary>
    private sealed class ConversionScene
    {
        /// <summary>원문의 50바이트 물리 저장소.</summary>
        private readonly RawSidPool _pool = new(24000, true);

        /// <summary>실제 번호순의 명시 타입 입력 188개.</summary>
        private readonly OriginalTypeRecord[] _types = Enumerable.Range(0, OriginalConstructors.TypeCount)
            .Select(_ => new OriginalTypeRecord()).ToArray();

        /// <summary>원본 생성자 recipe와 같은 풀을 사용하는 팩토리.</summary>
        private RawSquidFactory _factory;

        /// <summary>Unpop 경계의 명시 효과 입력.</summary>
        private uint _mutation;

        /// <summary>원문과 같은 진단·공간·부모 순서.</summary>
        private string _trace = string.Empty;

        /// <summary>장면을 만들고 팩토리를 연결한다.</summary>
        public ConversionScene()
        {
            _factory = new RawSquidFactory(_pool, _types);
        }

        /// <summary>같은 풀. 생성 효과 어댑터가 같은 풀인지 확인하는 데 쓴다.</summary>
        public RawSidPool Pool => _pool;

        /// <summary>현재 팩토리. 타입 표를 직접 바꾼 뒤에는 같은 배열 참조라 새로 만들 필요가 없다.</summary>
        public RawSquidFactory Factory => _factory;

        /// <summary>실제 번호순의 명시 타입 입력. 미등록 생성자 같은 경계 입력에 쓴다.</summary>
        public OriginalTypeRecord[] Types => _types;

        /// <summary>Unpop 경계의 명시 효과 입력.</summary>
        public uint Mutation
        {
            get => _mutation;
            set => _mutation = value;
        }

        /// <summary>원문과 같은 진단·공간·부모 순서.</summary>
        public string Trace => _trace;

        /// <summary>검사 입력만 물리 슬롯에 직접 설치한다. 제품 API의 상태 관문을 완화하지 않는다.</summary>
        public Span<byte> Raw() => _pool.RawSlot(ConversionSid);

        /// <summary>현재 자취에 하나의 실제 외부 효과를 추가한다.</summary>
        /// <param name="text">추가할 효과 기록.</param>
        public void Emit(string text)
        {
            if (_trace.Length != 0)
            {
                _trace += ';';
            }
            _trace += text;
        }

        /// <summary>원문 부모의 진단 복귀를 기록하며 기대 출력이나 상태는 고치지 않는다.</summary>
        /// <param name="line">원문 진단 줄 번호.</param>
        public void Diagnose(uint line) => Emit("A," + line);

        /// <summary>Native와 같은 sentinel·생성자·foot 값을 새 입력으로 공급한다.</summary>
        /// <param name="old">바꾸기 전 타입.</param>
        /// <param name="next">바꿀 타입.</param>
        /// <param name="state">슬롯 상태 바이트.</param>
        /// <param name="mismatch">foot 불일치 입력(비트 0: 가로, 비트 1: 세로).</param>
        public void Prepare(uint old, uint next, uint state, uint mismatch)
        {
            _trace = string.Empty;
            _mutation = 0;
            Span<byte> raw = Raw();
            // 모든 미사용 바이트도 독립 입력과 일치시킨다
            for (int index = 0; index < raw.Length; index++)
            {
                raw[index] = (byte)(index * 7 + 9);
            }
            SpPut(raw, 0, old == 112 ? 0x50a748u : 0x502370u);
            raw[10] = (byte)old;
            raw[11] = (byte)state;
            SpPut(raw, 12, next, 2);
            SpPut(raw, 14, BitConverter.SingleToUInt32Bits(100.5f));
            SpPut(raw, 18, BitConverter.SingleToUInt32Bits(122.25f));
            for (int index = 0; index < _types.Length; index++)
            {
                _types[index] = new OriginalTypeRecord();
            }
            // 원문의 현재 네 타입 입력이다
            foreach (uint kind in new uint[] { 112u, 114u, 116u, 80u })
            {
                _types[kind].FootX = 1;
                _types[kind].FootY = 1;
            }
            _types[112].ConstructorAddress = 0x4625f0u;
            _types[114].ConstructorAddress = 0x420e40u;
            _types[116].ConstructorAddress = 0x420e60u;
            if (old != next)
            {
                _types[next].FootX = (mismatch & 1) != 0 ? 2 : 1;
                _types[next].FootY = (mismatch & 2) != 0 ? 3 : 1;
            }
            _factory = new RawSquidFactory(_pool, _types);
        }

        /// <summary>원문에서 분리한 실제 장치 효과만 제공하며 객체 전환은 제품 본체가 수행한다.</summary>
        public ObjectConversionHooks Hooks()
        {
            var hooks = new ObjectConversionHooks();
            hooks.Unpop = (sid, flags) =>
            {
                Assert.Equal(ConversionSid, sid);
                Emit("U," + flags);
                Span<byte> raw = Raw();
                raw[11] |= 4;
                if (_mutation == 1)
                {
                    SpPut(raw, 12, 116, 2);
                }
                else if (_mutation == 2)
                {
                    raw[10] = 116;
                }
            };
            hooks.Repop = (sid, flags) =>
            {
                Assert.Equal(ConversionSid, sid);
                Span<byte> raw = Raw();
                Emit("P," + SpGet(raw, 14) + ',' + SpGet(raw, 18) + ',' + flags);
                raw[11] = (byte)(raw[11] & ~4u);
            };
            hooks.Retype = (sid, type) => _factory.Retype(sid, type, Diagnose);
            hooks.DefaultFrame = type => type switch
            {
                112 => 17u,
                114 => 21u,
                116 => 33u,
                80 => 5u,
                _ => throw new InvalidOperationException("명시 원본 기본프레임 TYPE 범위"),
            };
            hooks.Frame = (sid, value) =>
            {
                Assert.Equal(ConversionSid, sid);
                SpPut(Raw(), 36, value);
            };
            hooks.Qa = (sid, value) =>
            {
                Assert.Equal(ConversionSid, sid);
                SpPut(Raw(), 12, value, 2);
            };
            hooks.Base = (sid, code) =>
            {
                Assert.Equal(ConversionSid, sid);
                Emit("B," + code);
                return 77u;
            };
            return hooks;
        }

        /// <summary>fixture 50바이트만 읽고 타입·가상 표·프레임·QA 밖 바이트까지 그대로 대조한다.</summary>
        /// <param name="hex">기대 100자리 hex.</param>
        public bool Same(string hex)
        {
            if (hex.Length != 100)
            {
                return false;
            }
            ReadOnlySpan<byte> raw = Raw();
            // 각 두 hex 문자로 한 바이트를 비교한다
            for (int index = 0; index < raw.Length; index++)
            {
                if (raw[index] != Convert.ToByte(hex.Substring(index * 2, 2), 16))
                {
                    return false;
                }
            }
            return true;
        }
    }

    /// <summary>명시 초기 입력·하위 콜백의 같은 DWORD를 리틀 엔디언으로 쓴다.</summary>
    /// <param name="bytes">쓸 물리.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓸 폭. 생략하면 DWORD 4다.</param>
    private static void SpPut(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 부모 출력 기대를 생성하지 않는다
        for (int b = 0; b < width; b++)
        {
            bytes[offset + b] = (byte)(value >> (b * 8));
        }
    }

    /// <summary>실제 현재 출력 폭을 검사 관측용 DWORD로 읽는다.</summary>
    /// <param name="bytes">읽을 물리.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint SpGet(ReadOnlySpan<byte> bytes, int offset)
    {
        uint value = 0;
        // 관측 폭 밖의 바이트는 보지 않는다
        for (int b = 0; b < 4; b++)
        {
            value |= (uint)bytes[offset + b] << (b * 8);
        }
        return value;
    }

    /// <summary>풀 전체 슬롯 바이트를 복사한다. Retype·Command가 다른 슬롯을 바꾸지 않는지 대조한다.</summary>
    /// <param name="pool">복사할 풀.</param>
    private static byte[] SnapshotPool(RawSidPool pool)
    {
        var bytes = new byte[pool.Capacity * RawSidPool.Stride];
        // 슬롯마다 현재 바이트를 순서대로 모은다
        for (int sid = 0; sid < pool.Capacity; sid++)
        {
            pool.Slot(sid).CopyTo(bytes.AsSpan(sid * RawSidPool.Stride, RawSidPool.Stride));
        }
        return bytes;
    }

    /// <summary>
    /// 원문 타입 변경 128행의 전체 슬롯과 효과 순서를 허용오차 없이 재생한다.
    /// Editor 바깥 호출도 원문의 진단 복귀 의미를 유지한다.
    /// </summary>
    [Fact]
    public void SquidRetypeReplaysNative128AndPreservesOtherSlots()
    {
        string[][] rows = X86Fixture.Read("squid-retype");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] == "old")
            {
                return true;
            }
            Assert.Equal(6, row.Length);
            var scene = new ConversionScene();
            scene.Prepare(X86Fixture.UInt(row[0]), X86Fixture.UInt(row[1]), X86Fixture.UInt(row[2]), X86Fixture.UInt(row[3]));
            byte[] before = SnapshotPool(scene.Pool);
            uint free = scene.Pool.FreeCount;
            scene.Factory.Retype(ConversionSid, X86Fixture.UInt(row[1]), scene.Diagnose);
            Assert.True(scene.Same(row[4]));
            Assert.Equal(row[5], scene.Trace);
            Assert.Equal(free, scene.Pool.FreeCount);
            // 대상 슬롯에 현재 바이트를 덮은 뒤 풀 전체가 같아야 한다
            scene.Raw().CopyTo(before.AsSpan(ConversionSid * RawSidPool.Stride, RawSidPool.Stride));
            byte[] after = SnapshotPool(scene.Pool);
            Assert.Equal(before, after);
            count++;
            return true;
        });
        Assert.Equal(128, count);
    }

    /// <summary>
    /// 상위 플래그·부모 반환과 Unpop 뒤 타입·QA 재조회를 실제 두 몸체의 독립 출력에 맞춘다.
    /// </summary>
    [Fact]
    public void ObjectConversionReplaysNative36CommandsAndLateReads()
    {
        string[][] rows = X86Fixture.Read("object-conversion");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] == "body")
            {
                return true;
            }
            Assert.Equal(7, row.Length);
            bool buried = row[0] == "Buried";
            var scene = new ConversionScene();
            scene.Prepare(buried ? 112u : 114u, 114u, X86Fixture.UInt(row[1]), 0);
            scene.Mutation = X86Fixture.UInt(row[3]);
            uint body = RawObjectConversion.ObjectConversionBody(SpGet(scene.Raw(), 0));
            Assert.Equal(buried ? 0x462290u : 0x41e400u, body);
            var conversion = new RawObjectConversion(scene.Pool, BuriedType, scene.Hooks());
            Assert.Equal(X86Fixture.UInt(row[4]), conversion.Command(ConversionSid, X86Fixture.UInt(row[2]), body));
            Assert.True(scene.Same(row[5]));
            Assert.Equal(row[6], scene.Trace);
            count++;
            return true;
        });
        Assert.Equal(36, count);
        Assert.Equal(0u, RawObjectConversion.ObjectConversionBody(0));
        Assert.Equal(0u, RawObjectConversion.ObjectConversionBody(0x509930u));
    }

    /// <summary>
    /// 선택되지 않은 경계는 요구하지 않고 실제 실패 위치까지의 타입·공간 쓰기를 유지한다.
    /// </summary>
    [Fact]
    public void ObjectConversionRejectsSelectedMissingEffectsAndKeepsWrittenPrefix()
    {
        var scene = new ConversionScene();
        scene.Prepare(112, 114, 4, 0);
        var baseOnly = new ObjectConversionHooks { Base = (sid, code) => 99u };
        var parent = new RawObjectConversion(scene.Pool, BuriedType, baseOnly);
        Assert.Equal(99u, parent.Command(ConversionSid, 0x03000015u, 0x462290u));
        Assert.Throws<InvalidOperationException>(() => parent.Command(ConversionSid, 0x030000ce, 0));
        Assert.Throws<InvalidOperationException>(() => parent.Command(ConversionSid, 0x030000ce, 0x462290u));
        scene.Types[116].ConstructorAddress = UnregisteredConstructor;
        Assert.Throws<InvalidOperationException>(() => scene.Factory.Retype(ConversionSid, 116));
        Assert.Equal((byte)116, scene.Raw()[10]);
        Assert.Equal(0x50a748u, SpGet(scene.Raw(), 0));
        scene.Prepare(112, 114, 0, 0);
        Assert.Throws<InvalidOperationException>(() => scene.Factory.Retype(ConversionSid, 114));
        Assert.Equal((byte)112, scene.Raw()[10]);
    }
}
