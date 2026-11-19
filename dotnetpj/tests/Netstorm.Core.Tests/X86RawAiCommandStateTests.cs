using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// 직접 AI 지갑·파라미터 기록기의 독립 원문 1940입력과 현재 조회·보존 경계를 대조한다. 2026-11-19 0.637에서 R4-5 AI 초기화용으로 추가했다.
// 장면은 cpppj/tests/AiCommandStateTests.cpp 와 같다. 원문 1940행 전수 재생과 지연 조회·명시 실패 검사를 수행한다.
// 근거: cpppj o/RawAiCommandState.h/.cpp 와 ai-command-state-x86.tsv(1940행)다.
public sealed class X86RawAiCommandStateTests
{
    /// <summary>독립 원문 fixture의 전체 출력 DWORD 순서다.</summary>
    /// <remarks>2026-11-19 0.637에서 추가했다. cpppj AiCommandStateTests.cpp 의 kCommandOffsets와 같다.</remarks>
    private static readonly int[] CommandOffsets = [0x220, 0x228, 0x230, 0x234, 0x24c, 0x2d8, 0x600];

    /// <summary>값을 리틀 엔디언으로 바이트 배열에 쓴다. 2026-11-19 0.637에서 추가했다.</summary>
    /// <param name="bytes">대상 바이트 배열이다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(byte[] bytes, int offset, uint value)
    {
        // 낮은 바이트부터 폭 4바이트로 쓰는 반복문이다.
        for (int i = 0; i < 4; ++i)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>바이트 배열에서 리틀 엔디언 DWORD를 읽는다. 2026-11-19 0.637에서 추가했다.</summary>
    /// <param name="bytes">원본 바이트 배열이다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    /// <returns>읽은 DWORD다.</returns>
    private static uint Get(byte[] bytes, int offset) =>
        unchecked((uint)(bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)));

    /// <summary>한 행의 아홉 실제 본체와 현재 Player 기록을 소유하는 장면. 2026-11-19 0.637에서 추가했다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 Player 9기록(1620바이트)이다.</summary>
        public readonly byte[] Players = new byte[RawAiCommandState.PlayerBytes * RawAiCommandState.PlayerSlots];

        /// <summary>소유자별 실제 본체 토큰이다.</summary>
        public readonly uint[] Tokens = new uint[RawAiCommandState.PlayerSlots];

        /// <summary>토큰별 실제 본체 배열이다.</summary>
        public readonly Dictionary<uint, byte[]> Blocks = [];

        /// <summary>아홉 실제 본체를 토큰 순서대로 확보한다.</summary>
        /// <param name="mask">연결할 소유자 비트다.</param>
        public Scene(uint mask)
        {
            // 아홉 실제 본체를 확보하고 0xa0+소유자로 채우는 반복문이다.
            for (int owner = 0; owner < RawAiCommandState.PlayerSlots; ++owner)
            {
                uint token = 0x4f000000u + (uint)owner * 0xb10u;
                Tokens[owner] = token;
                var raw = new byte[RawAiCommandState.BodyBytes];
                Array.Fill(raw, (byte)(0xa0 + owner));
                Blocks[token] = raw;
                if (owner != 0 && (mask & (1u << (owner - 1))) != 0)
                {
                    Put(Players, owner * RawAiCommandState.PlayerBytes + RawAiCommandState.BrainOffset, token);
                }
            }
        }
    }

    /// <summary>1940행을 실제 같은 소유 본체에 기록하고 나머지 모든 바이트·Player 포인터를 확인한다. 2026-11-19 0.637에서 추가했다.</summary>
    [Fact]
    public void Native1940RowsAndWholeBodyPreservation()
    {
        string[][] rows = X86Fixture.Read("ai-command-state");
        Assert.Equal(1940, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            if (row.Length != 6)
            {
                throw new ArgumentException("AI 직접 명령 입력 6칸 오류", nameof(row));
            }

            var scene = new Scene(uint.Parse(row[1], CultureInfo.InvariantCulture));
            uint packed = uint.Parse(row[2], CultureInfo.InvariantCulture);
            uint selected = packed & 7u;
            bool flipped = false;
            int bodyCalls = 0;
            var hooks = new AiCommandStateHooks
            {
                Players = () => scene.Players,
                Body = token =>
                {
                    ++bodyCalls;
                    if (row[3] == "1" && !flipped)
                    {
                        Put(scene.Players, (int)selected * RawAiCommandState.PlayerBytes + RawAiCommandState.BrainOffset, scene.Tokens[8]);
                        flipped = true;
                    }

                    return scene.Blocks[token];
                },
            };
            if (row[0] == "Money")
            {
                RawAiCommandState.CrankAiMoney(hooks);
            }
            else
            {
                RawAiCommandState.SetAiParameter(packed, hooks);
            }

            string[] owners = row[4].Split('/');
            if (owners.Length != 9)
            {
                throw new ArgumentException("AI 직접 명령 소유자 9칸 오류", nameof(row));
            }

            // 아홉 소유자의 7DWORD 이외 바이트 보존을 확인하는 반복문이다.
            for (int owner = 0; owner < 9; ++owner)
            {
                string[] fields = owners[owner].Split(',');
                if (fields.Length != 7)
                {
                    throw new ArgumentException("AI 직접 명령 필드 7칸 오류", nameof(row));
                }

                var expected = new byte[RawAiCommandState.BodyBytes];
                Array.Fill(expected, (byte)(0xa0 + owner));
                // 일곱 DWORD 기대값을 순서대로 쓰는 반복문이다.
                for (int field = 0; field < 7; ++field)
                {
                    Put(expected, CommandOffsets[field], uint.Parse(fields[field], CultureInfo.InvariantCulture));
                }

                if (!expected.SequenceEqual(scene.Blocks[scene.Tokens[owner]]))
                {
                    return false;
                }

                bool attached = owner != 0 && (uint.Parse(row[1], CultureInfo.InvariantCulture) & (1u << (owner - 1))) != 0;
                uint expectedToken = flipped && owner == (int)selected ? scene.Tokens[8] : attached ? scene.Tokens[owner] : 0u;
                if (Get(scene.Players, owner * RawAiCommandState.PlayerBytes + RawAiCommandState.BrainOffset) != expectedToken)
                {
                    return false;
                }
            }

            if (scene.Blocks.Count != 9)
            {
                return false;
            }

            if (row[3] == "1" && !(flipped && bodyCalls == 2))
            {
                return false;
            }

            return true;
        });
    }

    /// <summary>소유자 0·없는 brain·미지원 선택기는 선택하지 않은 본체 경계를 요구하지 않는다. 2026-11-19 0.637에서 추가했다.</summary>
    [Fact]
    public void LazyParentsCurrentPointersAndExplicitPhysicalFailures()
    {
        RawAiCommandState.SetAiParameter(0, new AiCommandStateHooks());
        RawAiCommandState.SetAiParameter(8, new AiCommandStateHooks());
        var players = new byte[RawAiCommandState.PlayerBytes * RawAiCommandState.PlayerSlots];
        int reads = 0;
        var hooks = new AiCommandStateHooks { Players = () => { ++reads; return players; } };
        RawAiCommandState.CrankAiMoney(hooks);
        Assert.Equal(8, reads);
        RawAiCommandState.SetAiParameter(1, hooks);
        Assert.Equal(9, reads);
        Put(players, 180 + 0xb0, 0xdeadbeef);
        RawAiCommandState.SetAiParameter(1 | (255u << 4), hooks);
        Assert.Equal(10, reads);
        Assert.Throws<InvalidOperationException>(() => RawAiCommandState.SetAiParameter(1, hooks));
        var shortBody = new byte[8];
        hooks = new AiCommandStateHooks { Players = () => players, Body = _ => shortBody };
        Assert.Throws<ArgumentException>(() => RawAiCommandState.CrankAiMoney(hooks));
        hooks = new AiCommandStateHooks { Players = Array.Empty<byte> };
        Assert.Throws<ArgumentException>(() => RawAiCommandState.SetAiParameter(1, hooks));
        Assert.Throws<InvalidOperationException>(() => RawAiCommandState.CrankAiMoney(new AiCommandStateHooks()));
    }

    /// <summary>앞선 본체 해석의 후속 Player 변경을 다음 지갑 방문이 현재 포인터로 읽는다. 2026-11-19 0.637에서 추가했다.</summary>
    [Fact]
    public void MoneyVisitsCurrentLaterBrainAndStopsAtFailure()
    {
        var players = new byte[RawAiCommandState.PlayerBytes * RawAiCommandState.PlayerSlots];
        var first = new byte[RawAiCommandState.BodyBytes];
        var second = new byte[RawAiCommandState.BodyBytes];
        Array.Fill(first, (byte)0x7b);
        Array.Fill(second, (byte)0x7b);
        const uint firstToken = 0x4f000001u;
        const uint secondToken = 0x4f000011u;
        Put(players, 180 + 0xb0, firstToken);
        var hooks = new AiCommandStateHooks
        {
            Players = () => players,
            Body = token =>
            {
                if (token == firstToken)
                {
                    Put(players, 2 * 180 + 0xb0, secondToken);
                    return first;
                }

                return second;
            },
        };
        RawAiCommandState.CrankAiMoney(hooks);
        Assert.Equal(30000u, Get(first, 0x2d8));
        Assert.Equal(30000u, Get(second, 0x2d8));
        Put(first, 0x2d8, 17);
        Put(second, 0x2d8, 19);
        hooks = new AiCommandStateHooks
        {
            Players = () => players,
            Body = token => token == secondToken
                ? throw new InvalidOperationException("명시 두 번째 현재 본체 실패")
                : first,
        };
        Assert.Throws<InvalidOperationException>(() => RawAiCommandState.CrankAiMoney(hooks));
        Assert.Equal(30000u, Get(first, 0x2d8));
        Assert.Equal(19u, Get(second, 0x2d8));
    }
}
