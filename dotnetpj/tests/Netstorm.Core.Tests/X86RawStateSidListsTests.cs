using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// State SID 목록 등록·정리(RawStateSidLists)를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/StateSidListsTests.cpp 와 같다: 세 함수의 전체 출력·canary와 reset store 순서를 66개 독립 관측에서 읽는다.
/// 등록 표·목록 용량·쓰기 실패 경계도 대조한다. 기준 판본은 10.78 이다 (R0-4 재대조, 2026-10-28).
/// </summary>
public sealed class X86RawStateSidListsTests
{
    /// <summary>최초 Native 관측에 사용한 실제 등록 표 기준 주소.</summary>
    private const uint RegistryAddress = 0x592938u;

    /// <summary>최초 Native 관측에 사용한 격리 헤더 기준 주소.</summary>
    private const uint HeaderAddress = 0x12000100u;

    /// <summary>CPU와 같은 DWORD 폭의 쓰기를 실제 바이트에 적용한다.</summary>
    /// <param name="bytes">쓸 물리.</param>
    /// <param name="at">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void SidListPut(Span<byte> bytes, int at, uint value)
    {
        // 인접 바이트는 보존한다
        for (int b = 0; b < 4; b++)
        {
            bytes[at + b] = (byte)(value >> (b * 8));
        }
    }

    /// <summary>호스트 정렬이나 endian에 기대지 않고 네 바이트를 읽는다.</summary>
    /// <param name="bytes">읽을 물리.</param>
    /// <param name="at">읽기 시작 위치.</param>
    private static uint SidListRead(ReadOnlySpan<byte> bytes, int at)
    {
        uint value = 0;
        // 표와 헤더의 실제 입력이다
        for (int b = 0; b < 4; b++)
        {
            value |= (uint)bytes[at + b] << (b * 8);
        }
        return value;
    }

    /// <summary>바이트 배열을 Native의 소문자 16진 표로 바꾼다.</summary>
    /// <param name="bytes">바꿀 물리 전체.</param>
    private static string SidListHex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        var text = new System.Text.StringBuilder(bytes.Length * 2);
        // canary도 제외하지 않는다
        foreach (byte value in bytes)
        {
            text.Append(digits[value >> 4]);
            text.Append(digits[value & 15]);
        }
        return text.ToString();
    }

    /// <summary>
    /// 실제 세 함수의 전체 출력·canary와 reset store 순서를 66개 최초 독립 관측에서 읽는다.
    /// </summary>
    [Fact]
    public void StateSidListsIndependentNative66AndCurrentPointerOrder()
    {
        string[][] rows = X86Fixture.Read("state-sid-lists");
        int count = 0;
        int resetRows = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(10, row.Length);
            byte[] table = Convert.FromHexString(row[4]);
            byte[] headers = Convert.FromHexString(row[5]);
            uint[] pointers = new uint[20];
            // 실제 CPU 입력을 선택 토큰으로만 해석한다
            for (int index = 0; index < 20; index++)
            {
                pointers[index] = SidListRead(table, 16 + index * 4);
            }
            string stores = string.Empty;
            uint argument = X86Fixture.UInt(row[3]);
            if (row[0] == "register")
            {
                RawStateSidLists.Register(pointers, argument);
            }
            else if (row[0] == "unregister")
            {
                RawStateSidLists.Unregister(pointers, argument);
            }
            else
            {
                resetRows++;
                RawStateSidLists.ResetRegistered(pointers, header =>
                {
                    uint target = header + 12;
                    string evt = target + ":4:0";
                    stores += (stores.Length == 0 ? string.Empty : ";") + evt;
                    if (target >= RegistryAddress && target < RegistryAddress + 80)
                    {
                        pointers[(target - RegistryAddress) / 4] = 0;
                    }
                    else
                    {
                        Assert.True(target >= HeaderAddress && target < HeaderAddress + 640);
                        SidListPut(headers, 16 + (int)(target - HeaderAddress), 0);
                    }
                });
            }
            // 비활성 포인터와 표 이웃 32바이트까지 전체 비교한다
            for (int index = 0; index < 20; index++)
            {
                SidListPut(table, 16 + index * 4, pointers[index]);
            }
            Assert.Equal(row[6], SidListHex(table));
            Assert.Equal(row[7], SidListHex(headers));
            if (row[0] == "reset")
            {
                Assert.Equal(row[8], stores.Length == 0 ? "-" : stores);
            }
            Assert.True(X86Fixture.ULong(row[9]) > 0);
            count++;
            return true;
        });
        Assert.Equal(66, count);
        Assert.Equal(22, resetRows);
    }

    /// <summary>
    /// Native 정적 생성 순서의 실제 용량과 등록 순서 9개·빈 11칸을 직접 확인한다.
    /// </summary>
    [Fact]
    public void StateSidListsExactStaticCapacitiesAndNineActualHeaders()
    {
        var lists = new RawStateSidLists();
        (RawPostPopList List, int Capacity)[] owners =
        [
            (lists.Factories, 500), (lists.Dudes, 30), (lists.Outposts, 100), (lists.Providers, 256),
            (lists.Geysers, 100), (lists.Priests, 16), (lists.Residences, 50), (lists.Temples, 20),
        ];
        uint[] headers = [0x55a4d0u, 0x55a4ecu, 0x55a708u, 0x592f80u, 0x5950d8u, 0x5954c0u, 0x59a9d4u, 0x5caf28u, 0x5e1898u];
        // 템플 20과 아웃포스트 100을 바꾸면 실패한다
        for (int index = 0; index < owners.Length; index++)
        {
            Assert.Equal(owners[index].Capacity, owners[index].List.Entries.Length);
            Assert.Equal(0u, owners[index].List.Count);
        }
        Assert.Equal(headers, lists.RegisteredHeaders.AsSpan(0, 9).ToArray());
        Assert.All(lists.RegisteredHeaders.AsSpan(9).ToArray(), value => Assert.Equal(0u, value));
    }

    /// <summary>
    /// 선택된 쓰기 실패는 완료 앞부분을 보존하고 빈 표에는 효과 함수를 요구하지 않는다.
    /// </summary>
    [Fact]
    public void StateSidListsDemandFailureAndDuplicateRegistration()
    {
        uint[] headers = new uint[21];
        headers[20] = 0x87654321u;
        RawStateSidLists.ResetRegistered(headers, _ => { });
        Assert.Throws<ArgumentException>(() => RawStateSidLists.Register(headers.AsSpan(0, 19), 1));
        RawStateSidLists.Register(headers, 7);
        RawStateSidLists.Register(headers, 7);
        RawStateSidLists.Register(headers, 0);
        Assert.Equal(7u, headers[0]);
        Assert.Equal(7u, headers[1]);
        Assert.Equal(0u, headers[2]);
        Assert.Equal(0x87654321u, headers[20]);
        uint count = 0;
        uint calls = 0;
        Assert.Throws<InvalidOperationException>(() => RawStateSidLists.ResetRegistered(headers, header =>
        {
            Assert.Equal(7u, header);
            if (++calls == 2)
            {
                throw new InvalidOperationException("명시 실제 store 중단");
            }
            count = 0;
        }));
        Assert.Equal(2u, calls);
        Assert.Equal(0u, count);
        Assert.Equal(7u, headers[0]);
        Assert.Equal(7u, headers[1]);
        Assert.Equal(0x87654321u, headers[20]);
        RawStateSidLists.Unregister(headers, 7);
        Assert.Equal(0u, headers[0]);
        Assert.Equal(0u, headers[1]);
        Assert.Equal(0x87654321u, headers[20]);
        Assert.Throws<ArgumentException>(() => RawStateSidLists.ResetRegistered(headers.AsSpan(0, 19), _ => { }));
    }

    /// <summary>
    /// 개수 정리와 일반 설치 fresh가 데이터·용량·등록 표를 보존하는지 확인한다.
    /// </summary>
    [Fact]
    public void StateSidListsResetCountAndPrepareGeneralWorld()
    {
        var lists = new RawStateSidLists();
        lists.Factories.Entries[0] = 17;
        lists.Factories.Count = 5;
        int multiple = 9;
        lists.ResetCount(0x55a4d0u, ref multiple);
        Assert.Equal(0u, lists.Factories.Count);
        Assert.Equal(17u, lists.Factories.Entries[0]);
        Assert.Equal(9, multiple);
        lists.ResetCount(0x5caf28u, ref multiple);
        Assert.Equal(0, multiple);
        Assert.Throws<ArgumentOutOfRangeException>(() => lists.ResetCount(0x12345678u, ref multiple));
        lists.Providers.Entries[255] = 0xfedcba98u;
        lists.Providers.Count = 3;
        lists.PrepareGeneralWorld();
        foreach (RawPostPopList list in new[] { lists.Factories, lists.Dudes, lists.Outposts, lists.Providers, lists.Geysers, lists.Priests, lists.Residences, lists.Temples })
        {
            Assert.Equal(0u, list.Count);
            Assert.All(list.Entries, value => Assert.Equal(0u, value));
        }
    }
}
