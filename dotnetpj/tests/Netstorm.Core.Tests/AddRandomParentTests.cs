using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 실제 seed·기본 범위·섬 검색을 실행한 720개 원문 부모의 전체 인자를 대조한다
/// (cpppj <c>tests/AddRandomCommandTests.cpp</c>의 `AddRandom_Native720ParentAndActualQueue`를 직역했다).
/// </summary>
/// <remarks>2026-11-03 F2c-10에서 추가했다. 원본 x86 fixture로 진입점 전체를 관통하므로 PORT_MAP 상태는 L1이다.</remarks>
public sealed class AddRandomParentTests
{
    /// <summary>원본 입력의 세 섬 기록을 정확한 바이트·DWORD 위치에 설치한다.</summary>
    /// <returns>원본 3204바이트 섬 정보다.</returns>
    private static byte[] AddIslands()
    {
        byte[] bytes = new byte[3204];
        Put(bytes, 0, 3);
        uint[] owners = [2, 1, 0];
        byte[] originals = [1, 1, 9];
        // 지역 바이트는 0이며 현재·원래 owner는 다르다
        for (int index = 0; index < 3; index++)
        {
            int at = 4 + index * 32;
            Put(bytes, at + 4, 1);
            Put(bytes, at + 8, owners[index]);
            bytes[at + 13] = originals[index];
        }
        return bytes;
    }

    /// <summary>원본 little endian의 네 바이트만 쓴다.</summary>
    /// <param name="bytes">쓸 버퍼다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 DWORD 값이다.</param>
    private static void Put(byte[] bytes, int at, uint value)
    {
        for (int i = 0; i < 4; i++)
        {
            bytes[at + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>같은 원시 DWORD를 넘침 없이 복원한다.</summary>
    /// <param name="value">십진수 문자열이다.</param>
    /// <returns>원본 DWORD 값이다.</returns>
    private static uint AddNumber(string value)
    {
        return uint.Parse(value, System.Globalization.CultureInfo.InvariantCulture);
    }

    /// <summary>실제 720개 원문 부모의 전체 인자를 대조한다.</summary>
    [Fact]
    public void Native720ParentUsesCurrentOwnerAfterClock()
    {
        string[][] rows = Netstorm.Tests.X86Fixture.Read("add-random-parent");
        int count = 0;
        // 기대 seed·범위·지역을 다시 계산하지 않고 호출 결과와 비교한다
        foreach (string[] row in rows)
        {
            if (row.Length > 0 && row[0] == "type")
            {
                continue;
            }
            Assert.Equal(9, row.Length);
            uint type = AddNumber(row[0]);
            uint quantity = AddNumber(row[1]);
            uint low = AddNumber(row[2]);
            uint high = AddNumber(row[3]);
            uint owner = AddNumber(row[4]);
            bool fort = AddNumber(row[5]) != 0;
            double time = BitConverter.UInt64BitsToDouble(((ulong)high << 32) | low);
            byte[] islands = AddIslands();
            System.Text.StringBuilder trace = new();
            void AddTrace(string entry)
            {
                if (trace.Length > 0)
                {
                    trace.Append(';');
                }
                trace.Append(entry);
            }
            AddRandomHooks hooks = new();
            hooks.WallSeconds = () =>
            {
                AddTrace("T:" + low + ":" + high);
                if (AddNumber(row[6]) != 0)
                {
                    owner = 2;
                    fort = true;
                }
                return time;
            };
            hooks.Local = () => owner;
            hooks.Island = (current, territory) =>
            {
                AddTrace("R:" + current + ":" + territory);
                return RawFortTerritorySave.FindFortTerritoryIsland(islands, fort, unchecked((int)current), territory);
            };
            hooks.Place = (random, area, region, amount, selected, force) =>
            {
                AddTrace("B:" + random.State() + ":" + BitConverter.SingleToUInt32Bits(area.Left) + ":" + BitConverter.SingleToUInt32Bits(area.Top) + ":" + BitConverter.SingleToUInt32Bits(area.Right) + ":" + BitConverter.SingleToUInt32Bits(area.Bottom) + ":" + unchecked((uint)region) + ":" + amount + ":" + selected + ":" + force);
                return 7u;
            };
            RawAddRandom.AddRandomObjects(type, quantity, hooks);
            Assert.Equal(row[7], trace.ToString());
            Assert.Equal("4", row[8]);
            count++;
        }
        Assert.Equal(720, count);
    }

    /// <summary>마지막 배치 결과 SID를 원문 반환 레지스터처럼 그대로 돌려준다(M6-n 반환형).</summary>
    /// <remarks>2026-11-11 0.346에서 cpppj HEAD의 반환형 추가와 함께 더했다.</remarks>
    [Fact]
    public void AddRandomObjectsReturnsLastPlacementSid()
    {
        AddRandomHooks hooks = new();
        hooks.WallSeconds = () => 1.0;
        hooks.Local = () => 2u;
        hooks.Island = (_, _) => 0;
        // 배치 몸체가 돌려준 SID를 그대로 전달한다
        hooks.Place = (_, _, _, _, _, _) => 1234u;
        Assert.Equal(1234u, RawAddRandom.AddRandomObjects(89, 1, hooks));
    }

    /// <summary>선택되지 않은 하위와 이미 실행된 벽시각 효과를 잘못 처리하지 않는다.</summary>
    [Fact]
    public void OnlyReachedDevicesAreRequired()
    {
        AddRandomHooks hooks = new();
        int clocks = 0;
        hooks.WallSeconds = () =>
        {
            clocks++;
            return 0.0;
        };
        bool missing = false;
        try
        {
            RawAddRandom.AddRandomObjects(89, 1, hooks);
        }
        catch (InvalidOperationException)
        {
            missing = true;
        }
        Assert.True(missing && clocks == 1);
    }
}
