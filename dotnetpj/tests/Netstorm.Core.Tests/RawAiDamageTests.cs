// 실제 AI 피해 반응 149입력의 전체 본체·네 목록·효과 순서를 대조한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// AI 피해 반응을 검사하는 클래스다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.
// 근거: cpppj tests/AiDamageTests.cpp의 2건 전부다.
public sealed class RawAiDamageTests
{
    /// <summary>native와 같은 실제 등록부의 첫 opaque32 핸들이다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    private const uint DamageBrain = 0x13000000u;

    /// <summary>네 실제 목록 offset이다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    private static readonly int[] DamageLists = [0x360, 0x36c, 0x378, 0x384];

    /// <summary>원본 fixture와 같은 소문자 16진 표현이다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    /// <param name="raw">바꿀 바이트다.</param>
    /// <returns>소문자 16진이다.</returns>
    private static string Hex(byte[] raw) => Convert.ToHexString(raw).ToLowerInvariant();

    /// <summary>원본 fixture와 같은 소문자 16진 표현이다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    /// <param name="raw">바꿀 바이트 범위다.</param>
    /// <returns>소문자 16진이다.</returns>
    private static string Hex(ReadOnlySpan<byte> raw) => Convert.ToHexString(raw).ToLowerInvariant();

    /// <summary>현재 원문 DWORD를 little endian으로 쓴다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);

    /// <summary>현재 원문 DWORD를 읽는다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint Get(byte[] raw, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));

    /// <summary>독립 입력으로 실제 memory/Pool을 준비한다. 결과 필드는 fixture에서만 읽는다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    private sealed class DamageScene
    {
        /// <summary>native가 받은 17개 독립 정수다.</summary>
        public readonly uint[] Input;

        /// <summary>Session과 같은 실제 풀 크기이며 예측 영역 23001을 포함한다.</summary>
        public readonly RawSidPool Pool = new(24000);

        /// <summary>현재 TYPE genus/group 입력이다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();

        /// <summary>실제 본체·네 backing 블록의 소유자다.</summary>
        public readonly RawAiBrainMemory Memory;

        /// <summary>같은 memory/Pool의 실제 피해 처리기다.</summary>
        public readonly RawAiDamage Damage;

        /// <summary>실제 Allocate가 반환한 네 목록 token이다.</summary>
        public readonly List<uint> Blocks = [];

        /// <summary>native가 출력한 모든 입력 슬롯 순서다.</summary>
        public readonly List<ushort> Slots = [];

        /// <summary>실제 home/log 요청 순서다.</summary>
        public string Trace = "";

        /// <summary>각 미초기화 입력 패턴의 독립 번호다.</summary>
        private int _allocation;

        /// <summary>현재 경계가 실제 호출될 때만 효과를 기록한다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
        /// <param name="text">남길 기록이다.</param>
        public void Event(string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }

            Trace += text;
        }

        /// <summary>실제 raw 슬롯의 입력 바이트만 준비한다. 할당·생성 성공을 합성하지 않는다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
        /// <param name="sid">쓸 슬롯 번호다.</param>
        /// <param name="type">쓸 타입 번호다.</param>
        /// <param name="state">쓸 상태 바이트다.</param>
        /// <param name="x">쓸 X 좌표다.</param>
        /// <param name="y">쓸 Y 좌표다.</param>
        public void Slot(ushort sid, byte type, byte state, float x, float y)
        {
            Span<byte> raw = Pool.RawSlot(sid);
            // 전체 입력 padding 패턴을 쓰는 반복문이다.
            for (int i = 0; i < raw.Length; i++)
            {
                raw[i] = unchecked((byte)(i * 3 + sid));
            }

            raw[10] = type;
            raw[11] = state;
            Put(raw, 14, BitConverter.SingleToUInt32Bits(x));
            Put(raw, 18, BitConverter.SingleToUInt32Bits(y));
            Slots.Add(sid);
        }

        /// <summary>입력 준비 뒤 같은 실제 등록부에 모듈을 연결한다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
        /// <param name="values">쉼표로 나눈 17개 독립 입력이다.</param>
        public DamageScene(string values)
        {
            Input = values.Split(',').Select(X86Fixture.UInt).ToArray();
            Assert.Equal(17, Input.Length);
            Memory = new RawAiBrainMemory(DamageBrain, bytes =>
            {
                // 실제 할당 직후의 독립 입력을 쓰는 반복문이다.
                for (int i = 0; i < bytes.Length; i++)
                {
                    bytes[i] = _allocation != 0
                        ? unchecked((byte)(i * 7 + Input[15] + (_allocation - 1) * 17))
                        : unchecked((byte)(i * 13 + Input[15]));
                }

                _allocation++;
            });
            Assert.Equal(DamageBrain, Memory.Allocate(RawAiBrainConstructor.BytesCount));
            // 네 목록의 초기 pointer/capacity/count 입력을 쓰는 반복문이다.
            for (int index = 0; index < DamageLists.Length; index++)
            {
                uint block = Memory.Allocate(32);
                Blocks.Add(block);
                int offset = DamageLists[index];
                uint cap = index == 1 ? Input[5] : index == 2 ? Input[8] : 4u;
                uint count = index == 1 ? Input[6] : index == 2 ? Input[9] : index == 0 ? Input[3] : Input[2];
                byte[] raw = Memory.Bytes(DamageBrain);
                Put(raw, offset, block);
                Put(raw, offset + 4, cap);
                Put(raw, offset + 8, count);
                // native와 같은 초기 목록 내용을 쓰는 반복문이다.
                for (uint item = 0; item < count; item++)
                {
                    Put(Memory.Bytes(block), (int)(item * 4), index == 0 || index == 3 ? 50u : item == 0 && Input[index == 1 ? 7 : 10] != 0 ? 60u : 200u + (uint)index * 10u + item);
                }
            }

            Put(Memory.Bytes(DamageBrain), 0x248, Input[4]);
            Put(Memory.Bytes(DamageBrain), 0x404, 0x11223344u);
            byte victimType = (byte)(Input[1] != 0 ? 164 : 71);
            Types[victimType].Flags2 = Input[0];
            Types[140].Flags2 = Input[11] != 0 ? 0x100000u : 0u;
            Types[73].Group = (int)Input[14];
            Slot(50, victimType, 0, 5.5f, 7.5f);
            Slot(60, 140, 0, -4.75f, 9.25f);
            if (Input[12] > 0 && Input[12] < 24000)
            {
                Slot((ushort)Input[12], 73, (byte)Input[13], 12.75f, -3.5f);
            }

            Slots.Sort();
            var hooks = new AiDamageHooks();
            hooks.Home = attacker =>
            {
                Event("H:" + attacker.Value);
                return new Sid((ushort)Input[12]);
            };
            hooks.Log = (token, log) =>
            {
                Assert.Equal(DamageBrain, token);
                Event("L:" + (log.More ? 1 : 0) + ':' + log.Attacker.Value + ':' + log.Type + ':' + log.X + ':' + log.Y);
            };
            Damage = new RawAiDamage(Memory, Pool, Types, hooks);
        }

        /// <summary>네 실제 backing 블록의 보존 바이트도 모두 직렬화한다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
        /// <returns>이어 붙인 16진이다.</returns>
        public string Lists()
        {
            string result = "";
            // 네 블록을 순서대로 읽는 반복문이다.
            foreach (uint token in Blocks)
            {
                result += Hex(Memory.Bytes(token));
            }

            return result;
        }

        /// <summary>입력 슬롯이 피해 반응으로 변하지 않았는지 전체 폭을 비교한다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
        /// <returns>이어 붙인 16진이다.</returns>
        public string SlotsHex()
        {
            string result = "";
            // 모든 입력 슬롯을 순서대로 읽는 반복문이다.
            foreach (ushort sid in Slots)
            {
                result += Hex(Pool.Slot(sid));
            }

            return result;
        }
    }

    /// <summary>모든 기대값은 두 정밀도로 일치한 실제 10.78 기계어 관찰이다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    [Fact]
    public void Native149()
    {
        string[][] rows = X86Fixture.Read("ai-damage");
        int count = 0;
        var unique = new HashSet<string>();
        // fixture의 모든 데이터 행을 순서대로 재생하는 반복문이다.
        foreach (string[] row in rows)
        {
            if (row[0] == "input")
            {
                continue;
            }

            Assert.Equal(5, row.Length);
            Assert.True(unique.Add(row[0]));
            var scene = new DamageScene(row[0]);
            scene.Damage.Notice(DamageBrain, new Sid(50), new Sid((ushort)scene.Input[16]));
            Assert.Equal(row[1], Hex(scene.Memory.Bytes(DamageBrain)));
            Assert.Equal(row[2], scene.Lists());
            Assert.Equal(row[3], scene.Trace.Length == 0 ? "-" : scene.Trace);
            Assert.Equal(row[4], scene.SlotsHex());
            Assert.Equal(5, scene.Memory.Allocations.Count);
            Assert.Empty(scene.Memory.Frees);
            count++;
        }

        Assert.Equal(149, count);
    }

    /// <summary>미연결 실제 효과를 빈 성공으로 바꾸지 않고 요청 시점에 거부한다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    [Fact]
    public void RejectsMissingSelectedEffects()
    {
        var scene = new DamageScene("512,0,0,0,2,4,0,0,4,0,0,1,100,0,3,19,60");
        var missing = new RawAiDamage(scene.Memory, scene.Pool, scene.Types, new AiDamageHooks());
        Assert.ThrowsAny<Exception>(() => missing.Notice(DamageBrain, new Sid(50), new Sid(60)));
        Assert.Equal(1u, Get(scene.Memory.Bytes(DamageBrain), 0x374));
        Assert.Equal(60u, Get(scene.Memory.Bytes(scene.Blocks[1]), 0));
        Assert.Equal(0u, Get(scene.Memory.Bytes(DamageBrain), 0x380));
        Assert.ThrowsAny<Exception>(() => missing.Notice(DamageBrain + 1, new Sid(50), new Sid(60)));
    }
}
